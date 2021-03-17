#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"Plotting.h"
#include<svector\splot.h>
#include"ProgressReporter.h"
#include<wx/filename.h>
#include"AmfNc.h"

void LidarWindProfileProcessor::readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter)
{
	m_hasData = false;
	m_profiles.clear();

	//split the filenames into processed and hpl and match the two
	std::vector<sci::string> processedFilenames;
	std::vector<sci::string> hplFilenames;
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		//get the next string on the list and generate a filename pair for the processed and hpl files
		sci::string processedFilename;
		sci::string hplFilename;
		wxFileName filename = sci::nativeUnicode(inputFilenames[i]);
		if (filename.GetName().substr(0, 10) == "Processed_")
		{
			processedFilename = inputFilenames[i];
			wxFileName hplFilenameWx(filename.GetPath(), filename.GetName().substr(10, wxString::npos), filename.GetExt());
			hplFilename = sci::fromWxString(hplFilenameWx.GetFullPath());
		}
		else
		{
			hplFilename = inputFilenames[i];
			wxFileName processedFilenameWx(filename.GetPath(), "Processed_" + filename.GetName(), filename.GetExt());
			processedFilename = sci::fromWxString(processedFilenameWx.GetFullPath());
		}

		//if we had both processed and hpl files passed then we may have already generated this pair so check
		bool isNew = true;
		for (size_t j = 0; j < processedFilenames.size(); ++j)
		{
			if (processedFilenames[j] == processedFilename)
				isNew = false;
		}
		if (isNew)
		{
			processedFilenames.push_back(processedFilename);
			hplFilenames.push_back(hplFilename);
		}
	}

	m_profiles.reserve(processedFilenames.size());

	for (size_t i = 0; i < processedFilenames.size(); ++i)
	{
		Profile thisProfile(getInstrumentInfo(), getCalibrationInfo());
		bool readProfileWithoutErrors = true;
		try
		{
			progressReporter << sU("Reading file ") << processedFilenames[i] << sU("\n");
			std::fstream fin;
			fin.open(sci::nativeUnicode(processedFilenames[i]), std::ios::in);
			sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Could not open file ") + processedFilenames[i]));

			size_t nPoints;
			fin >> nPoints;
			sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Read of file failed - it may be locked or inaccessible.")));

			thisProfile.m_heights.resize(nPoints);
			thisProfile.m_instrumentRelativeWindDirections.resize(nPoints);
			thisProfile.m_instrumentRelativeWindSpeeds.resize(nPoints);

			for (size_t j = 0; j < nPoints; ++j)
			{
				fin >> thisProfile.m_heights[j] >> thisProfile.m_instrumentRelativeWindDirections[j] >> thisProfile.m_instrumentRelativeWindSpeeds[j];
				sci::assertThrow(!fin.eof(), sci::err(sci::SERR_USER, 0, sU("When reading file ") + processedFilenames[i] + sU(" some lines were missing.")));
				sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Failed to read file ") + processedFilenames[i] + sU(" - it may be locked or inaccessible.")));

				//get the time from the filename.
				/*int year;
				unsigned int month;
				unsigned int day;
				unsigned int hour;
				unsigned int minute;
				double second;
				size_t dateStart = processedFilenames[i].length() - 19;
				sci::istringstream(processedFilenames[i].substr(dateStart, 4)) >> year;
				sci::istringstream(processedFilenames[i].substr(dateStart + 4, 2)) >> month;
				sci::istringstream(processedFilenames[i].substr(dateStart + 6, 2)) >> day;
				sci::istringstream(processedFilenames[i].substr(dateStart + 9, 2)) >> hour;
				sci::istringstream(processedFilenames[i].substr(dateStart + 11, 2)) >> minute;
				sci::istringstream(processedFilenames[i].substr(dateStart + 13, 2)) >> second;
				m_profiles[i].m_time = sci::UtcTime(year, month, day, hour, minute, second);*/
			}
			sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Read of file failed - it may be locked or inaccessible.")));
			fin.close();
			if (progressReporter.shouldStop())
			{
				progressReporter << sU("Stopped reading at the request of the user.\n");
				break;
			}

			//now read the hpl file
			progressReporter << sU("Reading matching ray data file\n");
			thisProfile.m_VadProcessor.readData({ hplFilenames[i] }, platform, progressReporter);

			//check that we actually found some VAD data
			sci::assertThrow(thisProfile.m_VadProcessor.getTimesUtcTime().size() > 0, sci::err(sci::SERR_USER, 0, "Could not read VAD data to accompany wind profile data. Aborting read."));


			//correct for instrument misalignment
			thisProfile.m_motionCorrectedWindDirections = thisProfile.m_instrumentRelativeWindDirections;
			degreeF windElevation;
			for (size_t j = 0; j < thisProfile.m_motionCorrectedWindDirections.size(); ++j)
			{
				platform.correctDirection(thisProfile.m_VadProcessor.getTimesUtcTime()[0], thisProfile.m_VadProcessor.getTimesUtcTime().back(), thisProfile.m_motionCorrectedWindDirections[j], degreeF(0.0), thisProfile.m_motionCorrectedWindDirections[j], windElevation);
				if (thisProfile.m_motionCorrectedWindDirections[j] < degreeF(-180.0))
					thisProfile.m_motionCorrectedWindDirections[j] += degreeF(360.0);
			}
			//correct for instrument height
			degreeF latitude;
			degreeF longitude;
			metreF altitude;
			platform.getLocation(thisProfile.m_VadProcessor.getTimesUtcTime()[0], thisProfile.m_VadProcessor.getTimesUtcTime().back(), latitude, longitude, altitude);
			thisProfile.m_heights += altitude;
			//correct for instrument speed
			metrePerSecondF platformU;
			metrePerSecondF platformV;
			metrePerSecondF platformW;
			thisProfile.m_motionCorrectedWindSpeeds.resize(thisProfile.m_instrumentRelativeWindSpeeds.size());
			platform.getInstrumentVelocity(thisProfile.m_VadProcessor.getTimesUtcTime()[0], thisProfile.m_VadProcessor.getTimesUtcTime().back(), platformU, platformV, platformW);
			for (size_t j = 0; j < thisProfile.m_motionCorrectedWindDirections.size(); ++j)
			{
				metrePerSecondF u = thisProfile.m_instrumentRelativeWindSpeeds[j] * sci::sin(thisProfile.m_motionCorrectedWindDirections[j]) + platformU;
				metrePerSecondF v = thisProfile.m_instrumentRelativeWindSpeeds[j] * sci::cos(thisProfile.m_motionCorrectedWindDirections[j]) + platformV;
				thisProfile.m_motionCorrectedWindSpeeds[j] = sci::root<2>(sci::pow<2>(u) + sci::pow<2>(v));
				thisProfile.m_motionCorrectedWindDirections[j] = sci::atan2(u, v);
			}



			//set up the flags;
			//note that we only get a max of 1000 wind profiles it seems, the rest are zero, so flag them out
			thisProfile.m_windFlags.resize(std::min(thisProfile.m_heights.size(), size_t(1000)), lidarGoodDataFlag);
			thisProfile.m_windFlags.resize(thisProfile.m_heights.size(), lidarClippedWindProfileFlag);
			std::vector<std::vector<uint8_t>> vadFlags = thisProfile.m_VadProcessor.getDopplerFlags();
			for (size_t j = 0; j < vadFlags.size(); ++j)
			{
				for (size_t k = 0; k < vadFlags[j].size(); ++k)
					thisProfile.m_windFlags[k] = std::max(thisProfile.m_windFlags[k], vadFlags[j][k]);
			}
		}
		catch (sci::err err)
		{
			WarningSetter setter(&progressReporter);
			progressReporter << sU("File not included in processing\n") << err.getErrorMessage() << "\n";
			readProfileWithoutErrors = false;
		}
		catch (std::exception err)
		{
			WarningSetter setter(&progressReporter);
			progressReporter << sU("File not included in processing\n") << sci::fromCodepage(err.what()) << "\n";
			readProfileWithoutErrors = false;
		}
		if (readProfileWithoutErrors)
			m_profiles.push_back(thisProfile);
		else
		{
			WarningSetter setter(&progressReporter);
			progressReporter << sU("Profile omitted from processing.\n");
		}
	}

	m_hasData = true;
}

void LidarWindProfileProcessor::plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		std::vector<sci::Physical<sci::Metre<>, double>> heightsDouble;
		sci::convert(heightsDouble, m_profiles[i].m_heights);
		std::vector<sci::Physical<sci::DividedUnit<sci::Metre<>,sci::Second<>>, double>> speedsDouble;
		sci::convert(speedsDouble, m_profiles[i].m_instrumentRelativeWindSpeeds);
		std::vector<sci::Physical<sci::Degree<>, double>> directionsDouble;
		sci::convert(directionsDouble, m_profiles[i].m_instrumentRelativeWindDirections);
		for (size_t j = 0; j < maxRanges.size(); ++j)
		{
			//split the suffix from the filename and insert additional bits
			wxFileName outputFilenameSplit(sci::nativeUnicode(outputFilename));
			sci::ostringstream finalOutputfilename;
			finalOutputfilename << sci::fromWxString(outputFilenameSplit.GetPathWithSep()) << sci::fromWxString(outputFilenameSplit.GetName());
			if (maxRanges[j] != std::numeric_limits<metreF>::max() && maxRanges[j] != std::numeric_limits<metreF>::infinity())
				finalOutputfilename << sU("_maxRange_") << maxRanges[j];
			finalOutputfilename << sU(".") << sci::fromWxString(outputFilenameSplit.GetExt());

			splotframe *window = new splotframe(parent, true);
			WindowCleaner cleaner(window);
			splot2d *speedPlot = window->addplot(0.1, 0.1, 0.4, 0.7, false, false);
			splot2d *directionPlot = window->addplot(0.5, 0.1, 0.4, 0.7, false, false);
			std::shared_ptr<PhysicalPointData<decltype(speedsDouble)::value_type::unit, decltype(heightsDouble)::value_type::unit>> speedData(new PhysicalPointData<decltype(speedsDouble)::value_type::unit, decltype(heightsDouble)::value_type::unit>(speedsDouble, heightsDouble, Symbol(sym::filledCircle, 1.0)));
			std::shared_ptr<PhysicalPointData<decltype(directionsDouble)::value_type::unit, decltype(heightsDouble)::value_type::unit>> directionData(new PhysicalPointData<decltype(directionsDouble)::value_type::unit, decltype(heightsDouble)::value_type::unit>(directionsDouble, heightsDouble, Symbol(sym::filledCircle, 1.0)));

			speedPlot->addData(speedData);
			directionPlot->addData(directionData);
			if (maxRanges[j] < m_profiles[i].m_heights.back())
			{
				speedPlot->setmaxy(maxRanges[j].value<decltype(speedData)::element_type::yUnitType>());
				directionPlot->setmaxy(maxRanges[j].value<decltype(directionData)::element_type::yUnitType>());
			}

			speedPlot->setminy(0.0);
			directionPlot->setminy(0.0);

			speedPlot->setminx(0.0);
			directionPlot->setxlimits(0.0, 360.0);


			speedPlot->getyaxis()->settitle(sU("Height ") + speedData->getYAxisUnits());
			speedPlot->getyaxis()->settitlesize(15);
			speedPlot->getyaxis()->settitledistance(4.5);
			speedPlot->getyaxis()->setlabelsize(15);
			speedPlot->getxaxis()->settitle(sU("Wind Speed ") + speedData->getXAxisUnits());
			speedPlot->getxaxis()->settitlesize(15);
			speedPlot->getxaxis()->settitledistance(4.5);
			speedPlot->getxaxis()->setlabelsize(15);

			directionPlot->getyaxis()->settitle(sU(""));
			directionPlot->getyaxis()->setlabelsize(0);
			directionPlot->getxaxis()->settitle(sU("Wind Direction ") + directionData->getXAxisUnits());
			directionPlot->getxaxis()->settitlesize(15);
			directionPlot->getxaxis()->settitledistance(4.5);
			directionPlot->getxaxis()->setlabelsize(15);
			directionPlot->getxaxis()->setmajorinterval(45);

			splotlegend* legend = window->addlegend(0.1, 0.8, 0.8, 0.15, wxColour(0, 0, 0), 0);
			legend->settitlesize(20);
			sci::ostringstream plotTitle;
			plotTitle << sU("Lidar Wind Retrieval\n");
			plotTitle << sci::fromWxString(outputFilenameSplit.GetName());

			legend->settitle(plotTitle.str());

			createDirectoryAndWritePlot(window, finalOutputfilename.str(), 1000, 1000, progressReporter);
		}
	}
	return;
}

void LidarWindProfileProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	DataInfo dataInfo;
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo.continuous = true;
	dataInfo.startTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	dataInfo.endTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	if (m_profiles.size() > 0)
	{
		dataInfo.startTime = m_profiles[0].m_VadProcessor.getTimesUtcTime()[0];
		dataInfo.endTime = m_profiles.back().m_VadProcessor.getTimesUtcTime()[0];
	}
	dataInfo.featureType = FeatureType::timeSeriesProfile;
	dataInfo.options = std::vector<sci::string>(0);
	dataInfo.processingLevel = 2;
	dataInfo.productName = sU("mean winds profile");
	dataInfo.processingOptions = processingOptions;

	//get the start and end time for each wind profile retrieval
	std::vector<sci::UtcTime> scanStartTimes(m_profiles.size());
	std::vector<sci::UtcTime> scanEndTimes(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		scanStartTimes[i] = m_profiles[i].m_VadProcessor.getTimesUtcTime()[0];
		scanEndTimes[i] = m_profiles[i].m_VadProcessor.getTimesUtcTime().back() + (unitlessF((unitlessF::valueType)m_profiles[i].m_VadProcessor.getHeaderForProfile(0).pulsesPerRay * (unitlessF::valueType)m_profiles[i].m_VadProcessor.getHeaderForProfile(0).nRays) / sci::Physical<sci::Hertz<1, 3>, typename unitlessF::valueType>(15.0));//the second bit of this adds the duration of each ray
	}

	//work out the averaging time - this is the difference between the scan start and end times.
	//use the median as the value for the file
	if (scanStartTimes.size() > 0)
	{
		std::vector<sci::TimeInterval> averagingTimes = scanEndTimes - scanStartTimes;
		std::vector < sci::TimeInterval> sortedAveragingTimes = averagingTimes;
		std::sort(sortedAveragingTimes.begin(), sortedAveragingTimes.end());
		dataInfo.averagingPeriod = secondF(sortedAveragingTimes[sortedAveragingTimes.size() / 2]);
	}

	//work out the sampling interval - this is the difference between each start time.
	//use the median as the value for the file
	if (scanStartTimes.size() > 1)
	{
		std::vector<sci::TimeInterval> samplingIntervals = std::vector<sci::UtcTime>(scanStartTimes.begin() + 1, scanStartTimes.end()) - std::vector<sci::UtcTime>(scanStartTimes.begin(), scanStartTimes.end() - 1);
		std::vector < sci::TimeInterval> sortedSamplingIntervals = samplingIntervals;
		std::sort(sortedSamplingIntervals.begin(), sortedSamplingIntervals.end());
		dataInfo.samplingInterval = secondF(sortedSamplingIntervals[sortedSamplingIntervals.size() / 2]);
	}

	std::vector<sci::NcDimension*> nonTimeDimensions;

	sci::NcDimension indexDimension(sU("index"), m_profiles[0].m_heights.size());
	nonTimeDimensions.push_back(&indexDimension);

	OutputAmfNcFile file(AmfVersion::v1_1_0, directory, getInstrumentInfo(), author, processingSoftwareInfo, getCalibrationInfo(), dataInfo,
		projectInfo, platform, sU("doppler lidar wind profile"), scanStartTimes, nonTimeDimensions);



	if (m_profiles.size() > 0 && m_profiles[0].m_heights.size() > 0)
	{
		std::vector<std::vector<metreF>> altitudes = sci::makevector<metreF>(metreF(0.0), m_profiles.size(), m_profiles[0].m_heights.size());
		std::vector<std::vector<metrePerSecondF>> windSpeeds = sci::makevector<metrePerSecondF>(metrePerSecondF(0.0), m_profiles.size(), m_profiles[0].m_heights.size());
		std::vector<std::vector<degreeF>> windDirections = sci::makevector<degreeF>(degreeF(0.0), m_profiles.size(), m_profiles[0].m_heights.size());
		std::vector<std::vector<uint8_t>> windFlags = sci::makevector<uint8_t>(lidarGoodDataFlag, m_profiles.size(), m_profiles[0].m_heights.size());

		size_t maxLevels = 0;
		for (size_t i = 0; i < m_profiles.size(); ++i)
		{
			sci::assertThrow(m_profiles[i].m_heights.size() == altitudes[0].size(), sci::err(sci::SERR_USER, 0, "Lidar wind profiles do not have the same number of altitude points. Aborting processing that day's data."));

			altitudes[i] = m_profiles[i].m_heights;
			windSpeeds[i] = m_profiles[i].m_motionCorrectedWindSpeeds;
			windDirections[i] = m_profiles[i].m_motionCorrectedWindDirections;
			windFlags[i] = m_profiles[i].m_windFlags;
			maxLevels = std::max(maxLevels, m_profiles[i].m_heights.size());
		}
		for (size_t i = 0; i < m_profiles.size(); ++i)
		{
			//We may have some missing data recorded as zeros instead of fill value as some data is set to
			//zero in the data file. Change it to fill value
			sci::assign(windSpeeds[i], sci::isEq(windFlags[i], lidarClippedWindProfileFlag), std::numeric_limits<metrePerSecondF>::quiet_NaN());
			sci::assign(windDirections[i], sci::isEq(windFlags[i], lidarClippedWindProfileFlag), std::numeric_limits<degreeF>::quiet_NaN());

			altitudes[i].resize(maxLevels, std::numeric_limits<metreF>::quiet_NaN());
			windSpeeds[i].resize(maxLevels, std::numeric_limits<metrePerSecondF>::quiet_NaN());
			windDirections[i].resize(maxLevels, std::numeric_limits<degreeF>::quiet_NaN());
			windFlags[i].resize(maxLevels, lidarUserChangedGatesFlag);
		}

		//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
		//std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), cm_mean}, { sU("altitude"), cm_mean } };
		std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
		std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
		AmfNcVariable<metreF, decltype(altitudes)> altitudeVariable(sU("altitude"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates, std::vector<std::pair<sci::string, CellMethod>>(0));
		AmfNcVariable<metrePerSecondF, decltype(windSpeeds)> windSpeedVariable(sU("wind_speed"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Wind Speed"), sU("wind_speed"), windSpeeds, true, coordinates, cellMethods, sU("Derived by data logging software, assuming lidar is perfectly level. In Earth frame for moving platforms."));
		AmfNcVariable<degreeF, decltype(windDirections)> windDirectionVariable(sU("wind_from_direction"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Wind Direction"), sU("wind_from_direction"), windDirections, true, coordinates, cellMethods, sU("Derived by data logging software, assuming lidar is perfectly level. In Earth frame for moving platforms."));
		AmfNcFlagVariable windFlagVariable(sU("qc_flag_wind"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension });

		file.writeTimeAndLocationData(platform);

		file.write(altitudeVariable);
		file.write(windSpeedVariable);
		file.write(windDirectionVariable);
		file.write(windFlagVariable, windFlags);
	}
}