#include"Ceilometer.h"
#include"AmfNc.h"
#include"ProgressReporter.h"
#include<svector/splot.h>
#include"Plotting.h"


uint8_t CampbellCeilometerProfile::getProfileFlag() const
{
	if (m_profile.getMessageStatus() == cms_rawDataMissingOrSuspect)
		return 5;
	if (m_profile.getMessageStatus() == cms_someObscurationTransparent)
		return 4;
	if (m_profile.getMessageStatus() == cms_fullObscurationNoCloudBase)
		return 3;
	if (m_profile.getMessageStatus() == cms_noSignificantBackscatter)
		return 2;
	return 1;
}
std::vector<uint8_t> CampbellCeilometerProfile::getGateFlags() const
{
	//Depending upon how the ceilometer is set up it can replace data
	//that it believes is below the signal to noise threshold with 0.
	//Also data below 1e-7 sr m-1 is probably noise too so flag it

	std::vector<uint8_t> result(getBetas().size(), 1);
	sci::assign(result, getBetas() < steradianPerMetre(1e-7f), uint8_t(2));
	return result;
}


void CeilometerProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	sci::assertThrow(hasData(), sci::err(sci::SERR_USER, 0, sU("Attempted to write ceilometer data to netcdf when it has not been read")));

	writeToNc(m_firstHeaderHpl, m_allData, directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions);
}

void CeilometerProcessor::writeToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, sci::string directory,
	const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions)
{
	InstrumentInfo ceilometerInfo;
	ceilometerInfo.name = sU("ncas-ceilometer-3");
	ceilometerInfo.manufacturer = sU("Campbell");
	ceilometerInfo.model = sU("CS135");
	ceilometerInfo.description = sU("");
	ceilometerInfo.operatingSoftware = sU("");
	ceilometerInfo.operatingSoftwareVersion = sU("");
	ceilometerInfo.serial = sU("");

	CalibrationInfo ceilometerCalibrationInfo;

	DataInfo dataInfo;

	//------create the data arrays-----

	//time array, holds time/date
	std::vector<sci::UtcTime> times(profiles.size());
	//backscatter vs time and height
	std::vector<std::vector<steradianPerKilometre>> backscatter(profiles.size());
	//cloud bases
	std::vector<metre> cloudBase1(profiles.size());
	std::vector<metre> cloudBase2(profiles.size());
	std::vector<metre> cloudBase3(profiles.size());
	std::vector<metre> cloudBase4(profiles.size());
	//the gates - just an index from 0 to n_gates-1
	std::vector<int32_t> gates;
	//flags - one flag for each profile and one flage for each
	//each gate in each profile
	std::vector<uint8_t> profileFlag(profiles.size());
	std::vector<std::vector<uint8_t>> gateFlags(profiles.size());
	//the vertical resolution of the profiles - we check this is the same for each profile
	metre resolution;
	//Some other housekeeping information
	sci::assertThrow(profiles.size() < std::numeric_limits<long>::max(), sci::err(sci::SERR_USER, 0, "Profile is too long to fit in a netcdf"));
	std::vector<int32_t> pulseQuantities(profiles.size());
	std::vector<megahertz> sampleRates(profiles.size());
	std::vector<percent> windowTransmissions(profiles.size());
	std::vector<percent> laserEnergies(profiles.size());
	std::vector<kelvin> laserTemperatures(profiles.size());
	std::vector<degree> tiltAngles(profiles.size());
	std::vector<millivolt> backgrounds(profiles.size());
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		times[i] = profiles[i].getTime();
		backscatter[i] = profiles[i].getBetas();
		cloudBase1[i] = profiles[i].getCloudBase1();
		cloudBase2[i] = profiles[i].getCloudBase2();
		cloudBase3[i] = profiles[i].getCloudBase3();
		cloudBase4[i] = profiles[i].getCloudBase4();

		//assign gates for the first profile then check it is identical for the remaining profiles
		if (i == 0)
		{
			for (size_t j = 0; j < profiles[i].getGates().size(); ++j)
				gates[j] = (int32_t)profiles[i].getGates()[j];
			resolution = profiles[i].getResolution();
		}
		else
		{
			sci::assertThrow(resolution == profiles[i].getResolution(), sci::err(sci::SERR_USER, 0, sU("Found a change in the resolution during a day. Cannot process this day.")));

			for (size_t j = 0; j < gates.size(); ++i)
				sci::assertThrow(gates[j] == profiles[i].getGates()[j], sci::err(sci::SERR_USER, 0, sU("Found a change in the gates during a day. Cannot process this day.")));
		}

		//assign the flags
		profileFlag[i] = profiles[i].getProfileFlag();
		gateFlags[i] = profiles[i].getGateFlags();

		//assign the housekeeping data
		pulseQuantities[i] = (int32_t)profiles[i].getPulseQuantity();
		sampleRates[i] = profiles[i].getSampleRate();
		windowTransmissions[i] = profiles[i].getWindowTransmission();
		laserEnergies[i] = profiles[i].getLaserPulseEnergy();
		laserTemperatures[i] = profiles[i].getLaserTemperature();
		tiltAngles[i] = profiles[i].getTiltAngle();
		backgrounds[i] = profiles[i].getBackground();
	}


	//according to the data procucts spreadsheet the instrument needs
	//to produce aerosol-backscatter and cloud-base products

	//The time dimension will be created automatically when we create our
	//output file, but we must specify that we need a height dimension too
	sci::NcDimension gateDimension(sU("gate"), gates.size());
	std::vector<sci::NcDimension*> nonTimeDimensions;
	nonTimeDimensions.push_back(&gateDimension);
	//create the file and dimensions. The time variable is added automatically
	OutputAmfNcFile ceilometerFile(directory, ceilometerInfo, author, processingSoftwareInfo, ceilometerCalibrationInfo, dataInfo,
		projectInfo, platform, sU("ceilometer profile"), times, nonTimeDimensions);

	//add the data variables

	//gates/heights
	std::vector<unitless> gatesPhysical;
	sci::convert(gatesPhysical, gates);
	/*ceilometerFile.write(AmfNcVariable<int32_t>(sU("gate-number"), ceilometerFile, gateDimension, sU(""), sU("1")), gates);
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-lower-height"), ceilometerFile, gateDimension, sU("")), gatesPhysical*resolution);
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-upper-height"), ceilometerFile, gateDimension, sU("")), (gatesPhysical + unitless(1))*resolution);
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-mid-height"), ceilometerFile, gateDimension, sU("")), (gatesPhysical + unitless(0.5))*resolution);

	//backscatter
	std::vector<sci::NcDimension*> backscatterDimensions{ &ceilometerFile.getTimeDimension(), &gateDimension };
	ceilometerFile.write(AmfNcVariable<steradianPerMetre>(sU("aerosol-backscatter"), ceilometerFile, backscatterDimensions, sU("aerosol-backscatter")), backscatter);

	//cloud bases
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-1"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), cloudBase1);
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-2"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), cloudBase2);
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-3"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), cloudBase3);
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-4"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), cloudBase4);

	//flags
	ceilometerFile.write(sci::NcVariable<uint8_t>(sU("profile-flag"), ceilometerFile, ceilometerFile.getTimeDimension()),profileFlag);
	ceilometerFile.write(sci::NcVariable<uint8_t>(sU("gate-flag"), ceilometerFile, backscatterDimensions),gateFlags);

	//housekeeping
	ceilometerFile.write(sci::NcVariable<int32_t>(sU("pulse-quantity"), ceilometerFile, ceilometerFile.getTimeDimension()), pulseQuantities);
	ceilometerFile.write(AmfNcVariable<megahertz>(sU("sample-rate"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), sampleRates);
	ceilometerFile.write(AmfNcVariable<percent>(sU("window-transmission"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), windowTransmissions);
	ceilometerFile.write(AmfNcVariable<percent>(sU("laser-energy"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), laserEnergies);
	ceilometerFile.write(AmfNcVariable<kelvin>(sU("laser-temperature"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), laserTemperatures);
	ceilometerFile.write(AmfNcVariable<radian>(sU("tilt-angle"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), tiltAngles);
	ceilometerFile.write(AmfNcVariable<millivolt>(sU("background"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), backgrounds);*/
}



sci::UtcTime getCeilometerTime(const sci::string &timeDateString)
{
	sci::UtcTime result(
		std::atoi(sci::utf16ToUtf8(timeDateString.substr(0, 4)).c_str()), //year
		std::atoi(sci::utf16ToUtf8(timeDateString.substr(5, 2)).c_str()), //month
		std::atoi(sci::utf16ToUtf8(timeDateString.substr(8, 2)).c_str()), //day
		std::atoi(sci::utf16ToUtf8(timeDateString.substr(11, 2)).c_str()), //hour
		std::atoi(sci::utf16ToUtf8(timeDateString.substr(14, 2)).c_str()), //minute
		std::atof(sci::utf16ToUtf8(timeDateString.substr(17, std::string::npos)).c_str()) //second
	);
	return result;
}

void CeilometerProcessor::readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter)
{
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		readData(inputFilenames[i], progressReporter, i == 0);
	}
}
void CeilometerProcessor::readData(const sci::string &inputFilename, ProgressReporter &progressReporter, bool clearPrevious)
{
	if (clearPrevious)
	{
		m_hasData = false;
		m_allData.clear();
		m_inputFilenames.clear();
	}

	if (inputFilename.find(sU("_ceilometer.csv")) != std::string::npos)
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(inputFilename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Could not ope file ") + inputFilename + sU(".")));

		progressReporter << sU("Reading file ") << inputFilename << sU("\n");


		const size_t displayInterval = 120;
		sci::string firstBatchDate;
		sci::string timeDate;
		size_t nRead = 0;
		while (!fin.eof())
		{
			bool badTimeDate = false;
			timeDate = sU("");
			char character;
			fin.read(&character, 1);
			//size_t counter = 1;
			while (character != ',' && !fin.eof())
			{
				timeDate = timeDate + sci::utf8ToUtf16(std::string(&character, (&character) + 1));
				fin.read(&character, 1);
			}
			if (fin.eof())
				break;
			size_t firstDash = timeDate.find_first_of('-');
			if (firstDash == std::string::npos || firstDash < 4)
				badTimeDate = true;
			else
			{
				size_t timeDateStart = firstDash - 4;
				timeDate = timeDate.substr(timeDateStart);
				if (timeDate.length() < 19)
					badTimeDate = true;
				else if (timeDate[4] != '-')
					badTimeDate = true;
				else if (timeDate[7] != '-')
					badTimeDate = true;
				else if (timeDate[10] != 'T')
					badTimeDate = true;
				else if (timeDate[13] != ':')
					badTimeDate = true;
				else if (timeDate[16] != ':')
					badTimeDate = true;
			}

			//unless we have a bad time/date we can read in the data
			if (!badTimeDate)
			{
				//I have found one instance where time went backwards in the second entry in
				//a file. I'm not really sure why, but the best thing to do in this case seems
				//to be to discard the first entry.

				//If we have more than one entry already then I'm not sure what to do. For now abort processing this file..
				if (nRead == 1 && m_allData.back().getTime() > getCeilometerTime(timeDate))
				{
					m_allData.pop_back();
					progressReporter << sU("The second entry in the file has a timestamp earlier than the first. This may be due to a logging glitch. The first entry will be deleted.\n");
				}
				else if (nRead > 1 && m_allData.back().getTime() > getCeilometerTime(timeDate))
				{
					sci::assertThrow(false, sci::err(sci::SERR_USER, 0, sU("Time jumps backwards in this file, it cannot be processed.")));
				}
				CampbellHeader header;
				header.readHeader(fin);
				if (m_allData.size() == 0)
				{
					m_firstHeaderCampbell = header;
					m_firstHeaderHpl = createCeilometerHeader(inputFilename, header, m_allData[0]);
				}
				if (header.getMessageType() == cmt_cs && header.getMessageNumber() == 2)
				{
					if (m_allData.size() == 0)
						progressReporter << sU("Found CS 002 messages: ");
					if (m_allData.size() % displayInterval == 0)
						firstBatchDate = timeDate;
					if (m_allData.size() % displayInterval == displayInterval - 1)
						progressReporter << firstBatchDate << sU("-") << timeDate << sU("(") << displayInterval << sU(" profiles) ");
					CampbellMessage2 profile;
					profile.read(fin, header);
					if (profile.getPassedChecksum())
						m_allData.push_back(CampbellCeilometerProfile(getCeilometerTime(timeDate), profile));
					else
						progressReporter << sU("Found a message at time ") << timeDate << sU(" which does not pass the checksum and is hence corrupt. This profile will be ignored.\n");
				}
				else
				{
					progressReporter << sU("Found ") << header.getMessageNumber() << sU(" message ") << timeDate << sU(". Halting Read\n");
					break;
				}
			}
			else
				progressReporter << sU("Found a profile with an incorrectly formatted time/date. This profile will be ignored.\n");

			++nRead;
		}
		if (m_allData.size() % displayInterval != displayInterval - 1)
			progressReporter << firstBatchDate << sU("-") << timeDate << sU("(") << (m_allData.size() % displayInterval) + 1 << sU(" profiles)\n");
		progressReporter << sU("Completed reading file. ") << m_allData.size() << sU(" profiles found\n");
		fin.close();

		if (m_allData.size() > 0)
		{
			m_hasData = true;
			m_inputFilenames.push_back(inputFilename);
		}

		return;
	}
}

void CeilometerProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(hasData(), sci::err(sci::SERR_USER, 0, sU("Attempted to plot Ceilometer data when it has not been read")));

	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i].value<metre>() != std::numeric_limits<double>::max())
		{
			rangeLimitedfilename << sU("_maxRange_");
			rangeLimitedfilename << maxRanges[i];
		}
		plotCeilometerProfiles(m_firstHeaderHpl, m_allData, rangeLimitedfilename.str(), maxRanges[i], progressReporter, parent);
	}
}

void CeilometerProcessor::plotCeilometerProfiles(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, sci::string filename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, sU(""), parent, header);
	WindowCleaner cleaner(window);

	//We will do some averaging with the data - there is no point plotting thousands of profiles
	//on a plot that is ~800 pixels across.
	size_t timeAveragePeriod = 1;
	while (profiles.size() / timeAveragePeriod > 800)
		timeAveragePeriod *= 2;

	std::vector<std::vector<steradianPerMetre>> data(profiles.size() / timeAveragePeriod);
	for (size_t i = 0; i < data.size(); ++i)
	{
		sci::convert(data[i], profiles[i*timeAveragePeriod].getBetas());
		for (size_t j = 1; j < timeAveragePeriod; ++j)
			data[i] += profiles[i*timeAveragePeriod + j].getBetas();
	}
	data /= unitless((double)timeAveragePeriod);

	if (profiles[0].getResolution()*unitless(data[0].size() - 1) > maxRange)
	{
		size_t pointsNeeded = std::min((size_t)std::ceil((maxRange / profiles[0].getResolution()).value<unitless>()), data[0].size());
		for (size_t i = 0; i < data.size(); ++i)
			data[i].resize(pointsNeeded);
	}

	size_t heightAveragePeriod = 1;
	while (data[0].size() / heightAveragePeriod > 800)
		heightAveragePeriod *= 2;
	if (heightAveragePeriod > 1)
	{
		for (size_t i = 0; i < data.size(); ++i)
			data[i] = sci::boxcaraverage(data[i], heightAveragePeriod, sci::PhysicalDivide<steradianPerMetre::unit,steradianPerMetre::valueType>);
	}

	std::vector<second> xs(data.size() + 1);
	std::vector<metre> ys(data[0].size() + 1);

	//calculating our heights assumes that the profiles have range gates of 0, 1, 2, 3, ... so check this;
	bool gatesGood = true;
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<size_t> gates = profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			sci::assertThrow(gates[j] == j, sci::err(sci::SERR_USER, 0, sU("The plotting code currently assumes gates go 0, 1, 2, 3, ... but it found a profile where this was not the case.")));
	}

	xs[0] = second(profiles[0].getTime().getUnixTime());
	for (size_t i = 1; i < xs.size() - 1; ++i)
		xs[i] = second((profiles[i*timeAveragePeriod].getTime().getUnixTime() + profiles[i*timeAveragePeriod - 1].getTime().getUnixTime()) / unitless(2.0));
	xs.back() = second(profiles.back().getTime().getUnixTime());

	for (size_t i = 0; i < ys.size(); ++i)
		ys[i] = unitless(i * heightAveragePeriod)*header.rangeGateLength;

	std::vector<double> xsTemp;
	std::vector<double> ysTemp;
	std::vector<std::vector<double>> zsTemp;
	sci::convert(xsTemp, sci::physicalsToValues<second>(xs));
	sci::convert(ysTemp, sci::physicalsToValues<metre>(ys));
	sci::convert(zsTemp, sci::physicalsToValues<steradianPerMetre>(data));

	std::shared_ptr<GridData> gridData(new GridData(xsTemp, ysTemp, zsTemp, g_lidarColourscale, true, true));

	plot->addData(gridData);

	plot->getxaxis()->settitle(sU("Time"));
	plot->getxaxis()->settimeformat(sU("%H:%M:%S"));
	plot->getyaxis()->settitle(sU("Height (m)"));

	if (ys.back() > maxRange)
		plot->setmaxy(sci::physicalsToValues<metre>(maxRange));

	createDirectoryAndWritePlot(window, filename, 1000, 1000, progressReporter);
}

std::vector<std::vector<sci::string>> CeilometerProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	return InstrumentProcessor::groupFilesPerDayForReprocessing(newFiles, allFiles, 0, 8);
}

bool CeilometerProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	return InstrumentProcessor::fileCoversTimePeriod(fileName, startTime, endTime, 0, -1, -1, -1, second(86399));
}