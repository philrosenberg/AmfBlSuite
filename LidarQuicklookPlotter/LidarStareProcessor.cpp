#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"Plotting.h"
#include<svector\splot.h>
#include"ProgressReporter.h"
#include<wx/filename.h>

void LidarStareProcessor::plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(getNFilesRead() == 1, sci::err(sci::SERR_USER, 0, "Attempted to plot a set of stares with either no files or multiple files. This code can only plot one file at a time."));
	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i] != std::numeric_limits<metreF>::max())
			rangeLimitedfilename << sU("_maxRange_") << maxRanges[i];

		splotframe *window;
		splot2d *plot;
		setupCanvas(&window, &plot, sU(""), parent);
		WindowCleaner cleaner(window);

		//create time boundaries between plots;
		sci::GridData<second, 1> times = getTimesSeconds();
		sci::assertThrow(times.size() > 0, sci::err(sci::SERR_USER, 0, "Attempted to plot a set of stares with no profiles."));
		if (times.size() == 1)
			times.push_back(times.back() + second(2.5));
		else
			times.push_back(times.back() + times.back() - times[times.size() - 2]);

		/*std::shared_ptr<PhysicalGridData<second::unit, metreF::unit, perSteradianPerMetreF::unit>> gridData(new PhysicalGridData<second::unit, metreF::unit, perSteradianPerMetreF::unit>(times, getGateBoundariesForPlotting(0), getBetas(), g_lidarColourscale, true, true));
		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Time"));
		plot->getxaxis()->settimeformat(sU("%H:%M:%S"));
		plot->getyaxis()->settitle(sU("Height ") + gridData->getYAxisUnits());

		if (getGateBoundariesForPlotting(0).back() > maxRanges[i])
			plot->setmaxy(sci::physicalsToValues<decltype(gridData)::element_type::yUnitType>(maxRanges[i]));

		createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);*/
	}
}

/*void LidarStareProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	DataInfo dataInfo;
	dataInfo.continuous = true;
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.startTime = getTimesUtcTime()[0];
	dataInfo.endTime = getTimesUtcTime().back();
	dataInfo.featureType = FeatureType::timeSeriesProfile;
	dataInfo.processingLevel = 1;
	dataInfo.options = getProcessingOptions();
	dataInfo.productName = sU("aerosol backscatter radial winds");
	dataInfo.processingOptions = processingOptions;
	std::vector<sci::UtcTime> times;
	std::vector<degreeF> instrumentRelativeAzimuthAngles;
	std::vector<degreeF> instrumentRelativeElevationAngles;
	std::vector<std::vector<metrePerSecondF>> instrumentRelativeDopplerVelocities;
	std::vector<degreeF> attitudeCorrectedAzimuthAngles;
	std::vector<degreeF> attitudeCorrectedElevationAngles;
	std::vector<std::vector<metrePerSecondF>> motionCorrectedDopplerVelocities;
	std::vector<std::vector<perSteradianPerMetreF>> backscatters;
	std::vector<std::vector<unitlessF>> snrPlusOne;
	std::vector<std::vector<uint8_t>> dopplerVelocityFlags;
	std::vector<std::vector<uint8_t>> backscatterFlags;
	std::vector<std::vector<metreF>> ranges;

	getFormattedData(times, instrumentRelativeAzimuthAngles, instrumentRelativeElevationAngles, instrumentRelativeDopplerVelocities,
		attitudeCorrectedAzimuthAngles, attitudeCorrectedElevationAngles, motionCorrectedDopplerVelocities, backscatters,
		snrPlusOne, dopplerVelocityFlags, backscatterFlags, ranges, dataInfo.averagingPeriod, dataInfo.samplingInterval);

	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension rangeIndexDimension(sU("index_of_range"), ranges.size()>0 ? ranges[0].size() : 0);
	nonTimeDimensions.push_back(&rangeIndexDimension);


	OutputAmfNcFile file(AmfVersion::v1_1_0, directory, getInstrumentInfo(), author, processingSoftwareInfo, getCalibrationInfo(), dataInfo,
		projectInfo, platform, sU("doppler lidar stare"), times, nonTimeDimensions);

	//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), cm_mean}, { sU("range"), cm_mean } };
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ {sU("time"), cm_point} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAnglesEarthFrame{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsRange{ };
	std::vector<sci::string> coordinatesData{ sU("latitude"), sU("longitude"), sU("range") };
	std::vector<sci::string> coordinatesAngles{ sU("latitude"), sU("longitude") };
	std::vector<sci::string> coordinatesAnglesEarthFrame{ sU("latitude"), sU("longitude") };
	std::vector<sci::string> coordinatesRange{ sU("latitude"), sU("longitude") };

	AmfNcVariable<metreF, decltype(ranges)> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), sU("range"), ranges, true, coordinatesRange, cellMethodsRange);
	AmfNcVariable<degreeF, decltype(instrumentRelativeAzimuthAngles)> azimuthVariable(sU("sensor_azimuth_angle"), file, file.getTimeDimension(), sU("Scanning Head Azimuth Angle"), sU("sensor_azimuth_angle"), instrumentRelativeAzimuthAngles, true, coordinatesAngles, cellMethodsAngles, sU("Relative to the instrument with 0 degrees being to the \"front\" of the instruments."));
	AmfNcVariable<degreeF, decltype(instrumentRelativeElevationAngles)> elevationVariable(sU("sensor_view_angle"), file, file.getTimeDimension(), sU("Scanning Head Elevation Angle"), sU("sensor_view_angle"), instrumentRelativeElevationAngles, true, coordinatesAngles, cellMethodsAngles, sU("Relative to the instruments with 0 degrees being \"horizontal\", positive being upwards, negative being downwards."));
	AmfNcVariable<metrePerSecondF, decltype(instrumentRelativeDopplerVelocities)> dopplerVariable(sU("radial_velocity_of_scatterers_away_from_instrument"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Radial Velocity of Scatterers Away From Instrument"), sU("radial_velocity_of_scatterers_away_from_instrument"), instrumentRelativeDopplerVelocities, true, coordinatesData, cellMethodsData, sU("Instrument relative. Positive is away, negative is towards."));
	AmfNcVariable<degreeF, decltype(attitudeCorrectedAzimuthAngles)> azimuthVariableEarthFrame(sU("sensor_azimuth_angle_earth_frame"), file, file.getTimeDimension(), sU("Scanning Head Azimuth Angle Earth Frame"), sU(""), attitudeCorrectedAzimuthAngles, true, coordinatesAnglesEarthFrame, cellMethodsAnglesEarthFrame, sU("Relative to the geoid with 0 degrees being north."));
	AmfNcVariable<degreeF, decltype(attitudeCorrectedElevationAngles)> elevationVariableEarthFrame(sU("sensor_view_angle_earth_frame"), file, file.getTimeDimension(), sU("Scanning Head Elevation Angle Earth Frame"), sU(""), attitudeCorrectedElevationAngles, true, coordinatesAnglesEarthFrame, cellMethodsAnglesEarthFrame, sU("Relative to the geoid with 0 degrees being horizontal, positive being upwards, negative being downwards."));
	AmfNcVariable<metrePerSecondF, decltype(motionCorrectedDopplerVelocities)> dopplerVariableEarthFrame(sU("radial_velocity_of_scatterers_away_from_instrument_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Radial Velocity of Scatterers Away From Instrument - Earth Frame"), sU(""), motionCorrectedDopplerVelocities, true, coordinatesData, cellMethodsData, sU("Instrument relative. Positive is away, negative is towards."));
	AmfNcVariable<perSteradianPerMetreF, decltype(backscatters)> backscatterVariable(sU("attenuated_aerosol_backscatter_coefficient"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Attenuated Aerosol Backscatter Coefficient"), sU(""), backscatters, true, coordinatesData, cellMethodsData);
	AmfNcVariable<unitlessF, decltype(snrPlusOne)> snrPlusOneVariable(sU("signal_to_noise_ratio_plus_1"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Signal to Noise Ratio: SNR+1"), sU(""), snrPlusOne, true, coordinatesData, cellMethodsData);
	AmfNcFlagVariable dopplerFlagVariable(sU("qc_flag_radial_velocity_of_scatterers_away_from_instrument"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension});
	AmfNcFlagVariable backscatterFlagVariable(sU("qc_flag_attenuated_aerosol_backscatter_coefficient"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension});

	file.writeTimeAndLocationData(platform);

	file.write(rangeVariable);
	file.write(azimuthVariable);
	file.write(azimuthVariableEarthFrame);
	file.write(elevationVariable);
	file.write(elevationVariableEarthFrame);
	file.write(dopplerVariable);
	file.write(dopplerVariableEarthFrame);
	file.write(azimuthVariable);
	file.write(elevationVariable);
	file.write(dopplerVariable);
	file.write(backscatterVariable);
	file.write(snrPlusOneVariable);
	file.write(dopplerFlagVariable, dopplerVelocityFlags);
	file.write(backscatterFlagVariable, backscatterFlags);
}

void LidarStareProcessor::getFormattedData(std::vector<sci::UtcTime> &times,
	std::vector<degreeF> &instrumentRelativeAzimuthAngles,
	std::vector<degreeF> &instrumentRelativeElevationAngles,
	std::vector<std::vector<metrePerSecondF>> &instrumentRelativeDopplerVelocities,
	std::vector<degreeF> &attitudeCorrectedAzimuthAngles,
	std::vector<degreeF> &attitudeCorrectedElevationAngles,
	std::vector<std::vector<metrePerSecondF>> &motionCorrectedDopplerVelocities,
	std::vector<std::vector<perSteradianPerMetreF>> &backscatters,
	std::vector<std::vector<unitlessF>> &snrPlusOne,
	std::vector<std::vector<uint8_t>> &dopplerVelocityFlags,
	std::vector<std::vector<uint8_t>> &backscatterFlags,
	std::vector<std::vector<metreF>> &ranges,
	secondF &averagingPeriod,
	secondF &samplingInterval)
{
	//build up our data arrays.
	times = getTimesUtcTime();
	instrumentRelativeAzimuthAngles = getInstrumentRelativeAzimuths();
	instrumentRelativeElevationAngles = getInstrumentRelativeElevations();
	instrumentRelativeDopplerVelocities = getInstrumentRelativeDopplerVelocities();
	attitudeCorrectedAzimuthAngles = getAttitudeCorrectedAzimuths();
	attitudeCorrectedElevationAngles = getAttitudeCorrectedElevations();
	motionCorrectedDopplerVelocities = getMotionCorrectedDopplerVelocities();
	backscatters = getBetas();
	snrPlusOne = getSignalToNoiseRatiosPlusOne();
	dopplerVelocityFlags = getDopplerFlags();
	backscatterFlags = getBetaFlags();
	ranges.resize(backscatters.size());
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
		instrumentRelativeDopplerVelocities[i].resize(maxNGates, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		motionCorrectedDopplerVelocities[i].resize(maxNGates, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		backscatters[i].resize(maxNGates, std::numeric_limits<perSteradianPerMetreF>::quiet_NaN());
		snrPlusOne[i].resize(maxNGates, std::numeric_limits<unitlessF>::quiet_NaN());
		ranges[i].resize(maxNGates, std::numeric_limits<metreF>::quiet_NaN());
		dopplerVelocityFlags[i].resize(maxNGates, lidarUserChangedGatesFlag);
		backscatterFlags[i].resize(maxNGates, lidarUserChangedGatesFlag);
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
		averagingPeriod = unitlessF((unitlessF::valueType)sortedPulsesPerRay[sortedPulsesPerRay.size() / 2]) / sci::Physical<sci::Hertz<1, 3>, typename unitlessF::valueType>(15.0);
	}

	//work out the sampling interval.
	//use the median as the value for the file
	if (times.size() > 1)
	{
		std::vector<sci::TimeInterval> samplingIntervals = std::vector<sci::UtcTime>(times.begin() + 1, times.end()) - std::vector<sci::UtcTime>(times.begin(), times.end() - 1);
		std::vector < sci::TimeInterval> sortedSamplingIntervals = samplingIntervals;
		std::sort(sortedSamplingIntervals.begin(), sortedSamplingIntervals.end());
		samplingInterval = secondF(sortedSamplingIntervals[sortedSamplingIntervals.size() / 2]);
	}


}*/
