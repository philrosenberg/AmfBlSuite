#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"HplHeader.h"
#include"HplProfile.h"
#include"ProgressReporter.h"
#include<svector/sstring.h>
#include"ProgressReporter.h"

class VadPlanTransformer : public splotTransformer
{
public:
	VadPlanTransformer(radian elevation)
		:m_cosElevationRad(sci::cos(elevation)),
		m_sinElevationRad(sci::sin(elevation))
	{

	}
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		radian azimuth(oldx);

		metre range(oldy);
		metre horizDistance = range * m_cosElevationRad;

		newx = (horizDistance * sci::sin(azimuth)).value<metre>();
		newy = (horizDistance * sci::cos(azimuth)).value<metre>();
	}
private:
	const unitless m_cosElevationRad;
	const unitless m_sinElevationRad;
};

class VadSideTransformer : public splotTransformer
{
public:
	VadSideTransformer(radian elevation)
		:m_cosElevationRad(sci::cos(elevation)),
		m_sinElevationRad(sci::sin(elevation))
	{

	}
	//note that the input azimuths for dx, should be centred on the viewing direction 
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		radian enteredAzimuth = oldx;
		//clip the azimuth to the view azimuth +/-90.
		radian clippedAzimuth = std::max(std::min(enteredAzimuth, radian(M_PI / 2.0)), radian(-M_PI / 2.0));

		metre range = oldy;
		metre horizDistance = range * m_cosElevationRad;
		metre verticalDistance = range * m_sinElevationRad;


		newx = (horizDistance * sci::sin(clippedAzimuth)).value<metre>();
		newy = verticalDistance.value<metre>();
	}
private:
	const unitless m_cosElevationRad;
	const unitless m_sinElevationRad;
};

void LidarVadProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(getNFilesRead() == 1, sci::err(sci::SERR_USER, 0, "Attempted to plot a VAD with either no files or multiple files. This code can only plot one file at a time."));

	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		plotDataPlan(outputFilename + sU("_plan"), maxRanges[i], progressReporter, parent);
		plotDataPlan(outputFilename + sU("_cone"), maxRanges[i], progressReporter, parent);
		plotDataPlan(outputFilename + sU("_unwrapped"), maxRanges[i], progressReporter, parent);
	}
}

void LidarVadProcessor::plotDataPlan(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::ostringstream rangeLimitedfilename;
	rangeLimitedfilename << outputFilename;
	if (maxRange != metre(std::numeric_limits<double>::max()))
		rangeLimitedfilename << sU("_maxRange_") << maxRange;

	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, sU(""), parent);
	WindowCleaner cleaner(window);


	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetre>> sortedBetas;
	std::vector<radian> sortedElevations;
	std::vector<radian> sortedMidAzimuths;
	std::vector<radian> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);
	std::vector<metre> ranges = getGateBoundariesForPlotting(0);

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less like a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)m_nSegmentsMin / (double)sortedBetas.size());

	//Plot each segment as a separate data set
	std::vector< std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>>gridData(sortedBetas.size());
	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		std::vector<radian> segmentAzimuths(duplicationFactor + 1);
		std::vector<std::vector<perSteradianPerMetre>> segmentBetas(duplicationFactor);
		for (size_t j = 0; j < duplicationFactor; ++j)
		{
			segmentAzimuths[j] = azimuths[i] + (azimuths[i + 1] - azimuths[i]) / (unitless)(duplicationFactor*(double)j);
			segmentBetas[j] = sortedBetas[i];
		}
		segmentAzimuths.back() = azimuths[i + 1];
		gridData[i] = std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>(new PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>(segmentAzimuths, ranges, segmentBetas, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadPlanTransformer(sortedElevations[i]))));
	}
	for (size_t i = 0; i < gridData.size(); ++i)
		plot->addData(gridData[i]);

	//manually set limits as the transformer messes that up
	metre maxHorizDistance = 0.0;
	metre farthestRange = ranges.back();
	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		metre thisHorizDistance = std::min(farthestRange, maxRange)*sci::cos(sortedElevations[i]);
		if (thisHorizDistance > maxHorizDistance)
			maxHorizDistance = thisHorizDistance;
	}
	plot->setxlimits(-maxHorizDistance.value<decltype(gridData)::value_type::element_type::xUnitType>(), maxHorizDistance.value<decltype(gridData)::value_type::element_type::xUnitType>());
	plot->setylimits(-maxHorizDistance.value<decltype(gridData)::value_type::element_type::yUnitType>(), maxHorizDistance.value<decltype(gridData)::value_type::element_type::yUnitType>());

	plot->getxaxis()->settitle(sU("Horizontal Distance ") + gridData[0]->getXAxisUnits());
	plot->getyaxis()->settitle(sU("Horizontal Distance ") + gridData[0]->getYAxisUnits());


	createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
}


void LidarVadProcessor::plotDataCone(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	WindowCleaner cleaner(window);
	window->SetClientSize(1000, 2000);
	splot2d *plot;

	plot = window->addplot(0.1, 0.8, 0.8, 0.15, false, false);
	plotDataCone(radian(0.0), maxRange, plot);

	plot = window->addplot(0.1, 0.55, 0.8, 0.15, false, false);
	plotDataCone(radian(M_PI / 2.0), maxRange, plot);

	plot = window->addplot(0.1, 0.3, 0.8, 0.15, false, false);
	plotDataCone(radian(M_PI), maxRange, plot);

	plot = window->addplot(0.1, 0.05, 0.8, 0.15, false, false);
	plotDataCone(radian(M_PI*3.0 / 2.0), maxRange, plot);

	createDirectoryAndWritePlot(window, outputFilename, 1000, 2000, progressReporter);
}

void LidarVadProcessor::plotDataCone(radian viewAzimuth, metre maxRange, splot2d * plot)
{
	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetre>> sortedBetas;
	std::vector<radian> sortedElevations;
	std::vector<radian> sortedMidAzimuths;
	std::vector<radian> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);

	std::vector<metre> ranges = getGateBoundariesForPlotting(0);

	//Plot each segment as a separate data set

	std::vector<std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>> plotData;
	std::vector<radian> absRelativeAzimuths;

	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		std::vector<std::vector<perSteradianPerMetre>> segmentData(1);
		segmentData[0] = sortedBetas[i];
		radian lowerReleativeAzimuth = azimuths[i] - viewAzimuth;
		radian upperReleativeAzimuth = azimuths[i + 1] - viewAzimuth;
		if (upperReleativeAzimuth < lowerReleativeAzimuth)
			upperReleativeAzimuth += radian(2.0*M_PI);
		if (lowerReleativeAzimuth >= radian(M_PI))
		{
			lowerReleativeAzimuth -= radian(2.0*M_PI);
			upperReleativeAzimuth -= radian(2.0*M_PI);
		}
		if (upperReleativeAzimuth <= -radian(M_PI))
		{
			lowerReleativeAzimuth += radian(2.0*M_PI);
			upperReleativeAzimuth += radian(2.0*M_PI);
		}

		//don't plot if limits are outside view range
		if (upperReleativeAzimuth <= -radian(M_PI / 2.0) || lowerReleativeAzimuth >= radian(M_PI / 2.0))
			continue;


		std::vector<radian> segmentAzimuths = std::vector<radian>{ std::max(lowerReleativeAzimuth, radian(-M_PI / 2.)), std::min(upperReleativeAzimuth, radian(M_PI / 2.0)) };

		plotData.push_back(std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>(new PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>(segmentAzimuths, ranges, segmentData, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadSideTransformer(sortedElevations[i])))));
		absRelativeAzimuths.push_back(sci::abs(lowerReleativeAzimuth + upperReleativeAzimuth) / unitless(2.0));
	}

	//plot the segments farthest first.
	std::vector<size_t> plotDataNewLocations;
	std::vector<radian> sortedAbsRelativeAzimuths;
	sci::sort(absRelativeAzimuths, sortedAbsRelativeAzimuths, plotDataNewLocations);
	plotData = sci::reorder(plotData, plotDataNewLocations);
	for (size_t i = 0; i < plotData.size(); ++i)
		plot->addData(plotData[i]);

	plot->setxlimits(-ranges.back().value<decltype(plotData)::value_type::element_type::xUnitType>(), ranges.back().value<decltype(plotData)::value_type::element_type::xUnitType>());
	plot->setylimits(0, ranges.back().value<decltype(plotData)::value_type::element_type::yUnitType>());


	plot->getxaxis()->settitle(sU("Horizontal Distance ") + plotData[0]->getXAxisUnits());
	plot->getyaxis()->settitle(sU("Height ") + plotData[0]->getYAxisUnits());
	plot->getxaxis()->settitlesize(15);
	plot->getyaxis()->settitlesize(15);
	plot->getxaxis()->setlabelsize(15);
	plot->getyaxis()->setlabelsize(15);
	plot->getyaxis()->settitledistance(4.5);
}

void LidarVadProcessor::plotDataUnwrapped(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	splot2d *plot;
	setupCanvas(&window, &plot, sU(""), parent);
	WindowCleaner cleaner(window);
	window->SetClientSize(1000, 2000);

	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetre>> sortedBetas;
	std::vector<radian> sortedElevations;
	std::vector<radian> sortedMidAzimuths;
	std::vector<radian> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);

	std::vector<metre> ranges = getGateBoundariesForPlotting(0);

	std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit>> gridData(new PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit>(azimuths, ranges, sortedBetas, g_lidarColourscale, true, true));

	plot->addData(gridData);

	plot->getxaxis()->settitle(sU("Azimuth ") + gridData->getXAxisUnits());
	plot->getyaxis()->settitle(sU("Range ") + gridData->getYAxisUnits());
	if (ranges.back() > maxRange)
		plot->setmaxy(maxRange.value<decltype(gridData)::element_type::yUnitType>());
	plot->setxlimits(0, M_PI*2.0);

	createDirectoryAndWritePlot(window, outputFilename, 1000, 1000, progressReporter);
}

void LidarVadProcessor::getDataSortedByAzimuth(std::vector<std::vector<perSteradianPerMetre>> &sortedBetas, std::vector<radian> &sortedElevations, std::vector<radian> &sortedMidAzimuths, std::vector<radian> &azimuthBoundaries)
{

	std::vector<radian> midAzimuths = getAzimuths();
	std::vector<size_t> newLocations;
	sci::sort(midAzimuths, sortedMidAzimuths, newLocations);
	std::vector<std::vector<perSteradianPerMetre>> betas = getBetas();
	sortedBetas = sci::reorder(betas, newLocations);
	std::vector<radian> elevations = getElevations();
	sortedElevations = sci::reorder(elevations, newLocations);

	azimuthBoundaries.resize(sortedBetas.size() + 1);
	if (sortedMidAzimuths.back() > sortedMidAzimuths[0])
		azimuthBoundaries[0] = (sortedMidAzimuths[0] + sortedMidAzimuths.back() - radian(M_PI*2.0)) / unitless(2.0);
	else
		azimuthBoundaries[0] = (sortedMidAzimuths[0] + sortedMidAzimuths.back()) / unitless(2.0);

	for (size_t i = 1; i < azimuthBoundaries.size() - 1; ++i)
	{
		if (sortedMidAzimuths[i - 1] > sortedMidAzimuths[i])
			azimuthBoundaries[i] = (sortedMidAzimuths[i] + sortedMidAzimuths[i - 1] - radian(M_PI*2.0)) / unitless(2.0);
		else
			azimuthBoundaries[i] = (sortedMidAzimuths[i] + sortedMidAzimuths[i - 1]) / unitless(2.0);
	}

	azimuthBoundaries.back() = azimuthBoundaries[0] + radian(M_PI * 2);
}

void LidarVadProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter)
{
	DataInfo dataInfo;
	dataInfo.averagingPeriod = second(0); //To Do, this should be the time for one VAD
	dataInfo.continuous = true;
	dataInfo.startTime = getTimesUtcTime()[0];
	dataInfo.endTime = getTimesUtcTime().back();
	dataInfo.featureType = ft_timeSeriesPoint;
	dataInfo.maxLatDecimalDegrees = sci::max<radian>(platformInfo.latitudes).value<radian>()*180.0/M_PI;
	dataInfo.minLatDecimalDegrees = sci::min<radian>(platformInfo.latitudes).value<radian>()*180.0 / M_PI;
	dataInfo.maxLonDecimalDegrees = sci::max<radian>(platformInfo.longitudes).value<radian>()*180.0 / M_PI;
	dataInfo.minLonDecimalDegrees = sci::min<radian>(platformInfo.longitudes).value<radian>()*180.0 / M_PI;

	//build up our data arrays. We must account for the fact that the user could change
	//the number of profiles in a scan pattern or the range of the instruemnt during a day
	//so we work through the profiles checking when the filename changes. At this point things could
	//change. The number of gates in each profile can change within a file, but the distance per
	//gate cannot, so we find the longest range per hile and use this
	//work out how many lidar profiles there are per VAD and find the start time of each VAD and the ranges array for each vad
	size_t maxProfilesPerScan = 1;
	size_t thisProfilesPerScan = 1;
	std::vector<sci::UtcTime> allTimes = getTimesUtcTime();
	std::vector<radian> allAzimuths = getAzimuths();
	std::vector<radian> allElevations = getElevations();
	std::vector<std::vector<metrePerSecond>> allDopplerVelocities = getDopplerVelocities();
	std::vector<std::vector<perSteradianPerMetre>> allBackscatters = getBetas();
	std::vector<sci::UtcTime> scanStartTimes;
	scanStartTimes.reserve(allTimes.size()/6);
	//a 2d array for ranges (time,range)
	std::vector<std::vector<metre>> ranges;
	ranges.reserve(allTimes.size()/6);
	//a 2d array for azimuth (time, profile within scan)
	std::vector<std::vector<radian>> azimuthAngles;
	azimuthAngles.reserve(allTimes.size() / 6);
	//a 2d array for elevation angle (time, profile within scan)
	std::vector<std::vector<radian>> elevationAngles;
	elevationAngles.reserve(allTimes.size() / 6);
	//a 3d array for doppler speeds (time, profile within a scan, elevation) - note this is not the order we need to output
	//so immediately after this code we transpose the second two dimensions (time, elevation, profile within a scan)
	std::vector<std::vector<std::vector<metrePerSecond>>> dopplerVelocities;
	dopplerVelocities.reserve(allTimes.size() / 6);
	//a 3d array for backscatters (time, profile within a scan, elevation) - note this is not the order we need to output
	//so immediately after this code we transpose the second two dimensions (time, elevation, profile within a scan)
	std::vector<std::vector<std::vector<perSteradianPerMetre>>> backscatters;
	backscatters.reserve(allTimes.size() / 6);

	HplHeader previousHeader;
	size_t maxNGatesThisScan;
	size_t maxNGates;

	if (allTimes.size() > 0)
	{
		//keep track of the previous header as we go throught the data. We compare current to previous
		//header to check if we have moved to the next scan.
		//Set all the variables using the first profile.
		previousHeader = getHeaderForProfile(0);
		scanStartTimes.push_back(allTimes[0]);
		maxNGatesThisScan = getNGates(0);
		maxNGates = maxNGatesThisScan;
		ranges.push_back(getGateCentres(0));
		azimuthAngles.resize(1);
		azimuthAngles.back().reserve(6);
		azimuthAngles.back().push_back(allAzimuths[0]);
		elevationAngles.resize(1);
		elevationAngles.back().reserve(6);
		elevationAngles.back().push_back(allElevations[0]);
		dopplerVelocities.resize(1);
		dopplerVelocities.back().reserve(6);
		dopplerVelocities.back().push_back(allDopplerVelocities[0]);
		backscatters.resize(1);
		backscatters.back().reserve(6);
		backscatters.back().push_back(allBackscatters[0]);
	}

	//Work through all the profiles finding the needed data
	for (size_t i = 1; i < allTimes.size(); ++i)
	{
		if (getHeaderForProfile(i).filename == previousHeader.filename)
		{
			++thisProfilesPerScan;
			//we want to find the maximum number of gates in the whole set
			//For a particular scan we want to set the ranges using the profile with the most gates, but
			//we can't use the longest profile in the whole dataset as the gate length may change between
			//scans.
			if (getNGates(i) > maxNGatesThisScan)
			{
				maxNGatesThisScan = getNGates(i);
				maxNGates = std::max(maxNGates, maxNGatesThisScan);
				ranges.back() = getGateCentres(i);
			}
		}
		else
		{
			previousHeader = getHeaderForProfile(i);
			maxProfilesPerScan = std::max(maxProfilesPerScan, thisProfilesPerScan);
			thisProfilesPerScan = 1;
			scanStartTimes.push_back(allTimes[i]);
			maxNGatesThisScan = getNGates(i);
			ranges.push_back(getGateCentres(i));
			azimuthAngles.resize(azimuthAngles.size() + 1);
			azimuthAngles.back().reserve(maxProfilesPerScan);
			elevationAngles.resize(elevationAngles.size() + 1);
			elevationAngles.back().reserve(maxProfilesPerScan);
			dopplerVelocities.resize(dopplerVelocities.size() + 1);
			dopplerVelocities.back().reserve(maxProfilesPerScan);
			backscatters.resize(backscatters.size() + 1);
			backscatters.back().reserve(maxProfilesPerScan);
		}
		azimuthAngles.back().push_back(allAzimuths[i]);
		elevationAngles.back().push_back(allElevations[i]);
		dopplerVelocities.back().push_back(allDopplerVelocities[i]);
		backscatters.back().push_back(allBackscatters[i]);
	}
	//check if the last scan happened to be the one with the most profiles.
	maxProfilesPerScan = std::max(maxProfilesPerScan, thisProfilesPerScan);

	//Now we must transpose each element of the doppler and backscatter data
	//so they are indexed by the range before the profile within the scan.
	for (size_t i = 0; i < dopplerVelocities.size(); ++i)
	{
		dopplerVelocities[i] = sci::transpose(dopplerVelocities[i]);
		backscatters[i] = sci::transpose(backscatters[i]);
	}


	//expand the arrays, padding with fill value as needed
	for (size_t i = 0; i < ranges.size(); ++i)
	{
		ranges[i].resize(maxNGates, OutputAmfNcFile::getFillValue());
		azimuthAngles[i].resize(maxProfilesPerScan, OutputAmfNcFile::getFillValue());
		elevationAngles[i].resize(maxProfilesPerScan, OutputAmfNcFile::getFillValue());
		dopplerVelocities[i].resize(maxNGates);
		backscatters[i].resize(maxNGates);
		for (size_t j = 0; j < maxNGates; ++j)
		{
			dopplerVelocities[i][j].resize(maxProfilesPerScan, OutputAmfNcFile::getFillValue());
			backscatters[i][j].resize(maxProfilesPerScan, OutputAmfNcFile::getFillValue());
		}
	}

	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension rangeIndexDimension(sU("index_of_range"), maxNGates);
	sci::NcDimension angleIndexDimension(sU("index_of_angle"), maxProfilesPerScan);



	OutputAmfNcFile file(directory, getInstrumentInfo(), author, processingSoftwareInfo, getCalibrationInfo(), dataInfo,
		projectInfo, platformInfo, comment, scanStartTimes, platformInfo.longitudes[0], platformInfo.latitudes[0], nonTimeDimensions);

	AmfNcVariable<metre> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), metre(0), metre(10000.0));
	AmfNcVariable<radian> azimuthVariable(sU("sensor_azimuth_angle"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning head azimuth angle"), radian(0), radian(2.0*M_PI));
	AmfNcVariable<radian> elevationVariable(sU("sensor_view_angle"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning head elevation angle"), radian(-M_PI / 2.0), radian(M_PI / 2.0));
	AmfNcVariable<metrePerSecond> dopplerVariable(sU("los_radial_wind_speed"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Line of sight radial wind speed"), metrePerSecond(-19.0), metrePerSecond(19.0));
	AmfNcVariable<perSteradianPerMetre> backscatterVariable(sU("attenuated_aerosol_backscatter_coefficient"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Attenuated Aerosol Backscatter Coefficient"), perSteradianPerMetre(0.0), perSteradianPerMetre(1e-3));

	file.write(rangeVariable, ranges);
	file.write(azimuthVariable, azimuthAngles);
	file.write(elevationVariable, elevationAngles);
	file.write(dopplerVariable, dopplerVelocities);
	file.write(backscatterVariable, backscatters);

	//get the time for each wind profile retrieval
	//Note that each retrieval is multiple measurements so we use the
	//time of the first measurement
	/*std::vector<sci::UtcTime> times(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		times[i] = m_profiles[i].m_VadProcessor.getTimesUtcTime()[0];
	}


	OutputAmfNcFile file(directory, getInstrumentInfo(), author, processingSoftwareInfo, calibrationInfo, dataInfo,
		projectInfo, platformInfo, comment, times);*/
}

std::vector<std::vector<sci::string>> LidarVadProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	return InstrumentProcessor::groupFilesPerDayForReprocessing(newFiles, allFiles, 7);
}