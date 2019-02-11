#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"Plotting.h"
#include<svector\splot.h>
#include"ProgressReporter.h"

void LidarStareProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(getNFilesRead() == 1, sci::err(sci::SERR_USER, 0, "Attempted to plot a set of stares with either no files or multiple files. This code can only plot one file at a time."));
	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i] != metre(std::numeric_limits<double>::max()))
			rangeLimitedfilename << sU("_maxRange_") << maxRanges[i];

		splotframe *window;
		splot2d *plot;
		setupCanvas(&window, &plot, sU(""), parent);
		WindowCleaner cleaner(window);

		//create time boundaries between plots;
		std::vector<second> times = getTimesSeconds();
		sci::assertThrow(times.size() > 0, sci::err(sci::SERR_USER, 0, "Attempted to plot a set of stares with no profiles."));
		if (times.size() == 1)
			times.push_back(times.back() + second(2.5));
		else
			times.push_back(times.back() + times.back() - times[times.size() - 2]);

		std::shared_ptr<PhysicalGridData<second::unit, metre::unit, perSteradianPerMetre::unit>> gridData(new PhysicalGridData<second::unit, metre::unit, perSteradianPerMetre::unit>(times, getGateBoundariesForPlotting(0), getBetas(), g_lidarColourscale, true, true));
		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Time"));
		plot->getxaxis()->settimeformat(sU("%H:%M:%S"));
		plot->getyaxis()->settitle(sU("Height ") + gridData->getYAxisUnits());

		if (getGateBoundariesForPlotting(0).back() > maxRanges[i])
			plot->setmaxy(sci::physicalsToValues<decltype(gridData)::element_type::yUnitType>(maxRanges[i]));

		createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
	}
}

std::vector<std::vector<sci::string>> LidarStareProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	return InstrumentProcessor::groupFilesPerDayForReprocessing(newFiles, allFiles, 9);
}

void LidarStareProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter)
{
	DataInfo dataInfo;
	dataInfo.continuous = true;
	dataInfo.samplingInterval = second(OutputAmfNcFile::getFillValue());//set to fill value initially - calculate it later
	dataInfo.averagingPeriod = second(OutputAmfNcFile::getFillValue());//set to fill value initially - calculate it later
	dataInfo.startTime = getTimesUtcTime()[0];
	dataInfo.endTime = getTimesUtcTime().back();
	dataInfo.featureType = ft_timeSeriesPoint;
	dataInfo.maxLatDecimalDegrees = sci::max<radian>(platformInfo.latitudes).value<radian>()*180.0 / M_PI;
	dataInfo.minLatDecimalDegrees = sci::min<radian>(platformInfo.latitudes).value<radian>()*180.0 / M_PI;
	dataInfo.maxLonDecimalDegrees = sci::max<radian>(platformInfo.longitudes).value<radian>()*180.0 / M_PI;
	dataInfo.minLonDecimalDegrees = sci::min<radian>(platformInfo.longitudes).value<radian>()*180.0 / M_PI;

	//build up our data arrays.
	std::vector<sci::UtcTime> times = getTimesUtcTime();
	std::vector<radian> azimuthAngles = getAzimuths();
	std::vector<radian> elevationAngles = getElevations();
	std::vector<std::vector<metrePerSecond>> dopplerVelocities = getDopplerVelocities();
	std::vector<std::vector<perSteradianPerMetre>> backscatters = getBetas();
	std::vector<std::vector<metre>> ranges(backscatters.size());
	for (size_t i = 0; i < ranges.size(); ++i)
	{
		ranges[i] = getGateCentres(i);
	}

	//If the user changed the number of range gates during the day we need to expand the data arrays
	//padding with fill value
	size_t maxNGates = 0;
	for (size_t i = 0; i < times.size(); ++i)
	{
		maxNGates = std::max(maxNGates, ranges[i].size());
	}
	for (size_t i = 0; i < times.size(); ++i)
	{
		dopplerVelocities[i].resize(maxNGates, OutputAmfNcFile::getFillValue());
		backscatters[i].resize(maxNGates, OutputAmfNcFile::getFillValue());
		ranges[i].resize(maxNGates, OutputAmfNcFile::getFillValue());
	}

	//work out the averaging time - this is the difference between the scan start and end times.
	//use the median as the value for the file
	if (times.size() > 0)
	{
		std::vector <size_t> pulsesPerPoint(times.size());
		for (size_t i = 0; i < times.size(); ++i)
			pulsesPerPoint[i] = getHeaderForProfile(i).pulsesPerRay * getHeaderForProfile(i).nRays;
		std::vector <size_t> sortedPulsesPerRay = pulsesPerPoint;
		std::sort(sortedPulsesPerRay.begin(), sortedPulsesPerRay.end());
		dataInfo.averagingPeriod = unitless(sortedPulsesPerRay[sortedPulsesPerRay.size() / 2]) / sci::Physical<sci::Hertz<1, 3>>(15.0);
	}

	//work out the sampling interval.
	//use the median as the value for the file
	if (times.size() > 1)
	{
		std::vector<sci::TimeInterval> samplingIntervals = std::vector<sci::UtcTime>(times.begin() + 1, times.end()) - std::vector<sci::UtcTime>(times.begin(), times.end() - 1);
		std::vector < sci::TimeInterval> sortedSamplingIntervals = samplingIntervals;
		std::sort(sortedSamplingIntervals.begin(), sortedSamplingIntervals.end());
		dataInfo.samplingInterval = second(sortedSamplingIntervals[sortedSamplingIntervals.size() / 2]);
	}

	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension rangeIndexDimension(sU("index_of_range"), maxNGates);
	nonTimeDimensions.push_back(&rangeIndexDimension);


	OutputAmfNcFile file(directory, getInstrumentInfo(), author, processingSoftwareInfo, getCalibrationInfo(), dataInfo,
		projectInfo, platformInfo, comment, times, platformInfo.longitudes[0], platformInfo.latitudes[0], nonTimeDimensions);

	AmfNcVariable<metre> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), metre(0), metre(10000.0));
	AmfNcVariable<radian> azimuthVariable(sU("sensor_azimuth_angle"), file, file.getTimeDimension(), sU("Scanning head azimuth angle"), radian(0), radian(2.0*M_PI));
	AmfNcVariable<radian> elevationVariable(sU("sensor_view_angle"), file, file.getTimeDimension(), sU("Scanning head elevation angle"), radian(-M_PI / 2.0), radian(M_PI / 2.0));
	AmfNcVariable<metrePerSecond> dopplerVariable(sU("los_radial_wind_speed"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Line of sight radial wind speed"), metrePerSecond(-19.0), metrePerSecond(19.0));
	AmfNcVariable<perSteradianPerMetre> backscatterVariable(sU("attenuated_aerosol_backscatter_coefficient"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Attenuated Aerosol Backscatter Coefficient"), perSteradianPerMetre(0.0), perSteradianPerMetre(1e-3));

	file.write(rangeVariable, ranges);
	file.write(azimuthVariable, azimuthAngles);
	file.write(elevationVariable, elevationAngles);
	file.write(dopplerVariable, dopplerVelocities);
	file.write(backscatterVariable, backscatters);
}