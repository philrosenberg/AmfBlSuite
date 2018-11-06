#include"Ceilometer.h"
#include"AmfNc.h"
#include"ProgressReporter.h"
#include<svector/splot.h>
#include"Plotting.h"


char CampbellCeilometerProfile::getProfileFlag() const
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
std::vector<char> CampbellCeilometerProfile::getGateFlags() const
{
	//Depending upon how the ceilometer is set up it can replace data
	//that it believes is below the signal to noise threshold with 0.
	//Also data below 1e-7 sr m-1 is probably noise too so flag it

	std::vector<char> result(getBetas().size(), 1);
	sci::assign(result, getBetas() < steradianPerMetre(1e-7), char(2));
	return result;
}

void CeilometerProcessor::writeToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, sci::string directory,
	const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const PlatformInfo &platformInfo, const sci::string &comment)
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
	std::vector<char> profileFlag(profiles.size());
	std::vector<std::vector<char>> gateFlags(profiles.size());
	//the vertical resolution of the profiles - we check this is the same for each profile
	metre resolution;
	//Some other housekeeping information
	std::vector<size_t> pulseQuantities(profiles.size());
	std::vector<megahertz> sampleRates(profiles.size());
	std::vector<percent> windowTransmissions(profiles.size());
	std::vector<percent> laserEnergies(profiles.size());
	std::vector<kelvin> laserTemperatures(profiles.size());
	std::vector<radian> tiltAngles(profiles.size());
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
			if(resolution != profiles[i].getResolution())
				throw("Found a change in the resolution during a day. Cannot process this day.");
			for (size_t j = 0; j < gates.size(); ++i)
				if (gates[j] != profiles[i].getGates()[j])
					throw("Found a change in the gates during a day. Cannot process this day.");
		}

		//assign the flags
		profileFlag[i] = profiles[i].getProfileFlag();
		gateFlags[i] = profiles[i].getGateFlags();

		//assign the housekeeping data
		pulseQuantities[i] = profiles[i].getPulseQuantity();
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
		projectInfo, platformInfo, comment, times, nonTimeDimensions);

	//add the data variables

	//gates/heights
	std::vector<unitless> gatesPhysical;
	sci::convert(gatesPhysical, gates);
	ceilometerFile.write(AmfNcVariable<int32_t>(sU("gate-number"), ceilometerFile, gateDimension, sU(""), sU("1")), gates);
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-lower-height"), ceilometerFile, gateDimension, sU("")), AmfNcVariable<metre>::convertValues(gatesPhysical*resolution));
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-upper-height"), ceilometerFile, gateDimension, sU("")), AmfNcVariable<metre>::convertValues((gatesPhysical + unitless(1))*resolution));
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-mid-height"), ceilometerFile, gateDimension, sU("")), AmfNcVariable<metre>::convertValues((gatesPhysical + unitless(0.5))*resolution));
	
	//backscatter
	std::vector<sci::NcDimension*> backscatterDimensions{ &ceilometerFile.getTimeDimension(), &gateDimension };
	ceilometerFile.write(AmfNcVariable<steradianPerMetre>(sU("aerosol-backscatter"), ceilometerFile, backscatterDimensions, sU("aerosol-backscatter")), AmfNcVariable<steradianPerMetre>::convertValues(backscatter));

	//cloud bases
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-1"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), AmfNcVariable<metre>::convertValues(cloudBase1));
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-2"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), AmfNcVariable<metre>::convertValues(cloudBase2));
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-3"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), AmfNcVariable<metre>::convertValues(cloudBase3));
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-4"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), AmfNcVariable<metre>::convertValues(cloudBase4));

	//flags
	ceilometerFile.write(sci::NcVariable<char>(sU("profile-flag"), ceilometerFile, ceilometerFile.getTimeDimension()),profileFlag);
	ceilometerFile.write(sci::NcVariable<char>(sU("gate-flag"), ceilometerFile, backscatterDimensions),gateFlags);

	//housekeeping
	ceilometerFile.write(sci::NcVariable<size_t>(sU("pulse-quantity"), ceilometerFile, ceilometerFile.getTimeDimension()), pulseQuantities);
	ceilometerFile.write(sci::NcVariable<megahertz>(sU("sample-rate"), ceilometerFile, ceilometerFile.getTimeDimension()), sampleRates);
	ceilometerFile.write(sci::NcVariable<percent>(sU("window-transmission"), ceilometerFile, ceilometerFile.getTimeDimension()), windowTransmissions);
	ceilometerFile.write(sci::NcVariable<percent>(sU("laser-energy"), ceilometerFile, ceilometerFile.getTimeDimension()), laserEnergies);
	ceilometerFile.write(sci::NcVariable<kelvin>(sU("laser-temperature"), ceilometerFile, ceilometerFile.getTimeDimension()), laserTemperatures);
	ceilometerFile.write(sci::NcVariable<radian>(sU("tilt-angle"), ceilometerFile, ceilometerFile.getTimeDimension()), tiltAngles);
	ceilometerFile.write(sci::NcVariable<millivolt>(sU("background"), ceilometerFile, ceilometerFile.getTimeDimension()), backgrounds);
}



sci::UtcTime getCeilometerTime(const std::string &timeDateString)
{
	sci::UtcTime result(
		std::atoi(timeDateString.substr(0, 4).c_str()), //year
		std::atoi(timeDateString.substr(5, 2).c_str()), //month
		std::atoi(timeDateString.substr(8, 2).c_str()), //day
		std::atoi(timeDateString.substr(11, 2).c_str()), //hour
		std::atoi(timeDateString.substr(14, 2).c_str()), //minute
		std::atof(timeDateString.substr(17, std::string::npos).c_str()) //second
	);
	return result;
}


void CeilometerProcessor::readDataAndPlot(const std::string &inputFilename, const std::string &outputFilename, const std::vector<double> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	if (inputFilename.find("_ceilometer.csv") != std::string::npos)
	{
		std::fstream fin;
		fin.open(inputFilename.c_str(), std::ios::in | std::ios::binary);
		if (!fin.is_open())
			throw("Could not open file");

		progressReporter << "Reading file " << inputFilename << "\n";

		std::vector<CampbellCeilometerProfile> data;
		CampbellHeader firstHeader;
		const size_t displayInterval = 120;
		std::string firstBatchDate;
		std::string timeDate;
		while (!fin.eof())
		{
			bool badTimeDate = false;
			timeDate = "";
			char character;
			fin.read(&character, 1);
			//size_t counter = 1;
			while (character != ',' && !fin.eof())
			{
				timeDate = timeDate + character;
				fin.read(&character, 1);
			}
			//if (counter == 50)
			//{
			//	fin.close();
			//	throw("Failed to find the comma after the timestamp in a ceilometer file. Reading aborted.");
			//}
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
				if (data.size() == 1 && data[0].getTime() > getCeilometerTime(timeDate))
				{
					data.pop_back();
					progressReporter << "The second entry in the file has a timestamp earlier than the first. This may be due to a logging glitch. The first entry will be deleted.\n";
				}
				else if (data.size() > 1 && data[0].getTime() > getCeilometerTime(timeDate))
				{
					throw("Time jumps backwards in this file, it cannot be processed.");
				}
				CampbellHeader header;
				header.readHeader(fin);
				if (data.size() == 0)
					firstHeader = header;
				if (header.getMessageType() == cmt_cs && header.getMessageNumber() == 2)
				{
					if (data.size() == 0)
						progressReporter << "Found CS 002 messages: ";
					if (data.size() % displayInterval == 0)
						firstBatchDate = timeDate;
					if (data.size() % displayInterval == displayInterval - 1)
						progressReporter << firstBatchDate << "-" << timeDate << "(" << displayInterval << " profiles) ";
					CampbellMessage2 profile;
					profile.read(fin, header);
					if (profile.getPassedChecksum())
						data.push_back(CampbellCeilometerProfile(getCeilometerTime(timeDate), profile));
					else
						progressReporter << "Found a message at time " << timeDate << " which does not pass the checksum and is hence corrupt. This profile will be ignored.\n";
				}
				else
				{
					progressReporter << "Found " << header.getMessageNumber() << " message " << timeDate << ". Halting Read\n";
					break;
				}
			}
			else
				progressReporter << "Found a profile with an incorrectly formatted time/date. This profile will be ignored.\n";
		}
		if (data.size() % displayInterval != displayInterval - 1)
			progressReporter << firstBatchDate << "-" << timeDate << "(" << (data.size() % displayInterval) + 1 << " profiles)\n";
		progressReporter << "Completed reading file. " << data.size() << " profiles found\n";
		fin.close();

		if (data.size() > 0)
		{
			for (size_t i = 0; i < maxRanges.size(); ++i)
			{
				std::ostringstream rangeLimitedfilename;
				rangeLimitedfilename << outputFilename;
				if (maxRanges[i] != std::numeric_limits<double>::max())
					rangeLimitedfilename << "_maxRange_" << maxRanges[i];
				plotCeilometerProfiles(getCeilometerHeader(inputFilename, firstHeader, data[0]), data, rangeLimitedfilename.str(), maxRanges[i], progressReporter, parent);
			}
		}

		return;
	}
}

void CeilometerProcessor::plotCeilometerProfiles(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, std::string filename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, "", parent, header);
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
		size_t pointsNeeded = std::min((size_t)std::ceil((maxRange / profiles[0].getResolution()).m_v), data[0].size());
		for (size_t i = 0; i < data.size(); ++i)
			data[i].resize(pointsNeeded);
	}

	size_t heightAveragePeriod = 1;
	while (data[0].size() / heightAveragePeriod > 800)
		heightAveragePeriod *= 2;
	if (heightAveragePeriod > 1)
	{
		for (size_t i = 0; i < data.size(); ++i)
			data[i] = sci::boxcaraverage(data[i], heightAveragePeriod, sci::PhysicalDivide<steradianPerMetre::unit>);
	}

	std::vector<second> xs(data.size() + 1);
	std::vector<metre> ys(data[0].size() + 1);

	//calculating our heights assumes that the profiles have range gates of 0, 1, 2, 3, ... so check this;
	bool gatesGood = true;
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<size_t> gates = profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			if (gates[j] != j)
				throw("The plotting code currently assumes gates go 0, 1, 2, 3, ... but it found a profile where this was not the case.");
	}

	xs[0] = second(profiles[0].getTime().getUnixTime());
	for (size_t i = 1; i < xs.size() - 1; ++i)
		xs[i] = second((profiles[i*timeAveragePeriod].getTime().getUnixTime() + profiles[i*timeAveragePeriod - 1].getTime().getUnixTime()) / 2.0);
	xs.back() = second(profiles.back().getTime().getUnixTime());

	for (size_t i = 0; i < ys.size(); ++i)
		ys[i] = unitless(i * heightAveragePeriod)*header.rangeGateLength;

	std::shared_ptr<GridData> gridData(new GridData(sci::physicalsToValues<second>(xs), sci::physicalsToValues<metre>(ys), sci::physicalsToValues<steradianPerMetre>(data), g_lidarColourscale, true, true));

	plot->addData(gridData);

	plot->getxaxis()->settitle("Time");
	plot->getxaxis()->settimeformat("%H:%M:%S");
	plot->getyaxis()->settitle("Height (m)");

	if (ys.back() > maxRange)
		plot->setmaxy(sci::physicalsToValues<metre>(maxRange));

	createDirectoryAndWritePlot(window, filename, 1000, 1000, progressReporter);
}