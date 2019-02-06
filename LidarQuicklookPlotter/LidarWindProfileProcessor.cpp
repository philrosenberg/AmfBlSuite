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

void LidarWindProfileProcessor::readData(const std::vector<sci::string> &inputFilenames, ProgressReporter &progressReporter, wxWindow *parent)
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
			wxFileName processedFilenameWx(filename.GetPath(), "Processed_"+filename.GetName(), filename.GetExt());
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

	m_profiles.resize(processedFilenames.size(), Profile(getCalibrationInfo()));

	for (size_t i=0; i<processedFilenames.size(); ++i)
	{
		progressReporter << sU("Reading file ") << processedFilenames[i] << sU("\n");
		std::fstream fin;
		fin.open(sci::nativeUnicode(processedFilenames[i]), std::ios::in);
		if (!fin.is_open())
			throw(sU("Could not open file ") + processedFilenames[i]);

		size_t nPoints;
		fin >> nPoints;

		m_profiles[i].m_heights.resize(nPoints);
		m_profiles[i].m_windDirections.resize(nPoints);
		m_profiles[i].m_windSpeeds.resize(nPoints);

		for (size_t j = 0; j < nPoints; ++j)
		{
			double windDirectionDegrees;
			fin >> m_profiles[i].m_heights[j] >> windDirectionDegrees >> m_profiles[i].m_windSpeeds[j];
			m_profiles[i].m_windDirections[j] = radian(windDirectionDegrees*(M_PI / 180.0));
			if (fin.fail())
				throw(sU("When opening file ") + processedFilenames[i] + sU(" some lines were missing."));

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
		fin.close();
		progressReporter << sU("Done.\n");
		if (progressReporter.shouldStop())
		{
			progressReporter << sU("Stopped reading at the request of the user.");
		}

		//now read the hpl file, we can simply reuse the VAD code as they are the same.
		m_profiles[i].m_VadProcessor.readData({ hplFilenames[i] }, progressReporter, parent);

		//check that we actually found some VAD data
		sci::assertThrow(m_profiles[i].m_VadProcessor.getTimesUtcTime().size() > 0, sci::err(sci::SERR_USER, 0, "Could not read VAD data to accompany wind profile data. Aborting read."));
	}

	m_hasData = true;
}

void LidarWindProfileProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		for (size_t j = 0; j < maxRanges.size(); ++j)
		{
			//split the suffix from the filename and insert additional bits
			wxFileName outputFilenameSplit(sci::nativeUnicode(outputFilename));
			sci::ostringstream finalOutputfilename;
			finalOutputfilename << sci::fromWxString(outputFilenameSplit.GetPathWithSep()) << sci::fromWxString(outputFilenameSplit.GetName());
			if (maxRanges[j] != decltype(maxRanges[j])(std::numeric_limits<double>::max()))
				finalOutputfilename << sU("_maxRange_") << maxRanges[j];
			finalOutputfilename << sU(".") << sci::fromWxString(outputFilenameSplit.GetExt());

			splotframe *window = new splotframe(parent, true);
			WindowCleaner cleaner(window);
			splot2d *speedPlot = window->addplot(0.1, 0.1, 0.4, 0.7, false, false);
			splot2d *directionPlot = window->addplot(0.5, 0.1, 0.4, 0.7, false, false);
			std::shared_ptr<PhysicalPointData<decltype(m_profiles[i].m_windSpeeds)::value_type::unit, decltype(m_profiles[i].m_heights)::value_type::unit>> speedData(new PhysicalPointData<decltype(m_profiles[i].m_windSpeeds)::value_type::unit, decltype(m_profiles[i].m_heights)::value_type::unit>(m_profiles[i].m_windSpeeds, m_profiles[i].m_heights, Symbol(sym::filledCircle, 1.0)));
			std::shared_ptr<PhysicalPointData<decltype(m_profiles[i].m_windDirections)::value_type::unit, decltype(m_profiles[i].m_heights)::value_type::unit>> directionData(new PhysicalPointData<decltype(m_profiles[i].m_windDirections)::value_type::unit, decltype(m_profiles[i].m_heights)::value_type::unit>(m_profiles[i].m_windDirections, m_profiles[i].m_heights, Symbol(sym::filledCircle, 1.0)));

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
			directionPlot->setxlimits(0.0, 2.0*M_PI);


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
	const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter)
{
	DataInfo dataInfo;
	dataInfo.averagingPeriod = 0.0;
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		dataInfo.averagingPeriod += (m_profiles[i].m_VadProcessor.getTimesSeconds().back() - m_profiles[i].m_VadProcessor.getTimesSeconds()[0]).value<second>();
	}
	dataInfo.averagingPeriod /= double(m_profiles.size());
	dataInfo.averagingPeriodUnit = sU("s");
	dataInfo.continuous = true;
	dataInfo.startTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	dataInfo.endTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	if (m_profiles.size() > 0)
	{
		dataInfo.startTime = m_profiles[0].m_VadProcessor.getTimesUtcTime()[0];
		dataInfo.endTime = m_profiles.back().m_VadProcessor.getTimesUtcTime()[0];
	}
	dataInfo.featureType = ft_timeSeriesPoint;
	dataInfo.maxLatDecimalDegrees = sci::max<radian>(platformInfo.latitudes).value<radian>()/M_PI*180.0;
	dataInfo.minLatDecimalDegrees = sci::min<radian>(platformInfo.latitudes).value<radian>() / M_PI * 180.0;
	dataInfo.maxLonDecimalDegrees = sci::max<radian>(platformInfo.longitudes).value<radian>() / M_PI * 180.0;
	dataInfo.minLonDecimalDegrees = sci::min<radian>(platformInfo.longitudes).value<radian>() / M_PI * 180.0;

	//get the time for each wind profile retrieval
	//Note that each retrieval is multiple measurements so we use the
	//time of the first measurement
	std::vector<sci::UtcTime> times(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		times[i] = m_profiles[i].m_VadProcessor.getTimesUtcTime()[0];
	}
	
	std::vector<sci::NcDimension*> nonTimeDimensions;

	sci::NcDimension indexDimension(sU("index"), m_profiles[0].m_heights.size());
	nonTimeDimensions.push_back(&indexDimension);

	OutputAmfNcFile file(directory, getInstrumentInfo(), author, processingSoftwareInfo, getCalibrationInfo(), dataInfo,
		projectInfo, platformInfo, comment, times, platformInfo.longitudes[0], platformInfo.latitudes[0], nonTimeDimensions);

	AmfNcVariable<metre> altitudeVariable(sU("altitude"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU(""), metre(0), metre(20000.0));
	altitudeVariable.addAttribute(sci::NcAttribute(sU("Note:"), sU("Altitude is above instrument, not referenced to sea level or any geoid and not compensated for instrument platform height.")));
	AmfNcVariable<metrePerSecond> windSpeedVariable(sU("wind_speed"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU(""), metrePerSecond(0), metrePerSecond(20.0));
	AmfNcVariable<radian> windDirectionVariable(sU("wind_from_direction"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU(""), radian(0), radian(2.0*M_PI));
	
	file.writeTimeAndLocationData();

	if (m_profiles.size() > 0 && m_profiles[0].m_heights.size() > 0)
	{
		std::vector<std::vector<metre>> altitudes = sci::makevector<metre>(0.0, m_profiles.size(), m_profiles[0].m_heights.size());
		std::vector<std::vector<metrePerSecond>> windSpeeds = sci::makevector<metrePerSecond>(0.0, m_profiles.size(), m_profiles[0].m_heights.size());
		std::vector<std::vector<radian>> windDirections = sci::makevector<radian>(0.0, m_profiles.size(), m_profiles[0].m_heights.size());
		
		altitudes[0] = m_profiles[0].m_heights;
		windSpeeds[0] = m_profiles[0].m_windSpeeds;
		windDirections[0] = m_profiles[0].m_windDirections;

		for (size_t i = 1; i < m_profiles.size(); ++i)
		{
			sci::assertThrow(m_profiles[i].m_heights.size() == altitudes[0].size(), sci::err(sci::SERR_USER, 0, "Lidar wind profiles do not have the same number of altitude points. Aborting processing that day's data."));

			altitudes[i] = m_profiles[i].m_heights;
			windSpeeds[i] = m_profiles[i].m_windSpeeds;
			windDirections[i] = m_profiles[i].m_windDirections;
		}
		file.write(altitudeVariable, altitudes);
		file.write(windSpeedVariable, windSpeeds);
		file.write(windDirectionVariable, windDirections);
	}
}

std::vector<std::vector<sci::string>> LidarWindProfileProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	return InstrumentProcessor::groupFilesPerDayForReprocessing(newFiles, allFiles, 26);
}