#include"Ceilometer.h"
#include"AmfNc.h"
#include"ProgressReporter.h"
#include<svector/splot.h>
#include"Plotting.h"
#include<svector/svector.h>


uint8_t CampbellCeilometerProfile::getProfileFlag() const
{
	if (m_profile.getMessageStatus() == ceilometerMessageStatus::rawDataMissingOrSuspect)
		return ceilometerRawDataMissingOrSuspectFlag;
	if (m_profile.getMessageStatus() == ceilometerMessageStatus::someObscurationTransparent)
		return ceilometerSomeObscurationTransparentFlag;
	if (m_profile.getMessageStatus() == ceilometerMessageStatus::fullObscurationNoCloudBase)
		return ceilometerFullObscurationNoCloudBaseFlag;
	if (m_profile.getMessageStatus() == ceilometerMessageStatus::noSignificantBackscatter)
		return ceilometerNoSignificantBackscatterFlag;
	return ceilometerGoodFlag;
}
std::vector<uint8_t> CampbellCeilometerProfile::getGateFlags() const
{
	//Depending upon how the ceilometer is set up it can replace data
	//that it believes is below the signal to noise threshold with 0.
	//Also data below 1e-7 sr m-1 is probably noise too so flag it

	std::vector<uint8_t> result(getBetas().size(), ceilometerGoodFlag);
	sci::assign(result, getBetas() < perSteradianPerMetreF(1e-12f), ceilometerFilteredNoiseFlag);
	sci::assign(result, getBetas() < perSteradianPerMetreF(1e-7f), ceilometerLowSignalFlag);
	return result;
}


void CeilometerProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	sci::assertThrow(hasData(), sci::err(sci::SERR_USER, 0, sU("Attempted to write ceilometer data to netcdf when it has not been read")));

	writeToNc(m_firstHeaderHpl, m_allData, directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions);
}

void CeilometerProcessor::formatDataForOutput(const HplHeader& header,
	const std::vector<CampbellCeilometerProfile>& profiles,
	InstrumentInfo& ceilometerInfo,
	CalibrationInfo& ceilometerCalibrationInfo,
	DataInfo& dataInfo, sci::GridData<sci::UtcTime, 1>& times,
	sci::GridData<metreF, 2>& altitudesAboveInstrument,
	sci::GridData<perSteradianPerMetreF, 2>& backscatter,
	sci::GridData<metreF, 1>& cloudBase1, sci::GridData<metreF, 1>& cloudBase2, sci::GridData<metreF, 1>& cloudBase3,
	sci::GridData<metreF, 1>& cloudBase4, sci::GridData<percentF, 1> &laserEnergies, sci::GridData<kelvinF, 1> &laserTemperatures,
	sci::GridData<unitlessF, 1>& pulseQuantities, sci::GridData<degreeF, 1>& tiltAngles, sci::GridData<percentF, 1>& scales,
	sci::GridData<percentF, 1>& windowTransmissions, sci::GridData<millivoltF, 1>& windowContaminations, sci::GridData<millivoltF, 1>& backgrounds, sci::GridData<perSteradianF, 1>& sums,
	sci::GridData<uint8_t, 1> &profileFlags, sci::GridData<uint8_t, 2> &gateFlags, sci::GridData<uint8_t, 1>& cloudBaseFlags)
{
	ceilometerInfo = m_instrumentInfo;
	ceilometerInfo.operatingSoftwareVersion = m_ceilometerOsVersion;

	ceilometerCalibrationInfo = m_calibrationInfo;

	dataInfo.continuous = true;
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later;
	dataInfo.startTime = profiles.size() > 0 ? profiles[0].getTime() : sci::UtcTime();
	dataInfo.endTime = profiles.size() > 0 ? profiles.back().getTime() : sci::UtcTime();
	dataInfo.featureType = FeatureType::timeSeriesProfile;
	dataInfo.processingLevel = 1;
	dataInfo.productName = sU(""); //we are producing three product - detail them below

	//------create the data arrays-----

	std::array<size_t, 2> shape{ profiles.size(),0 };
	for (size_t i = 0; i < profiles.size(); ++i)
		shape[1] = std::max(profiles[i].getBetas().size(), shape[1]);

	//time array, holds time/date
	times.resize(shape[0]);
	//altitudes of each gate
	altitudesAboveInstrument.reshape(shape, std::numeric_limits<metreF>::quiet_NaN());
	//backscatter vs time and height
	backscatter.reshape(shape, std::numeric_limits<perSteradianPerMetreF>::quiet_NaN());
	//cloud bases
	cloudBase1.resize(shape[0]);
	cloudBase2.resize(shape[0]);
	cloudBase3.resize(shape[0]);
	cloudBase4.resize(shape[0]);
	//flags - one flag for each profile and one flage for each
	//each gate in each profile
	profileFlags.resize(shape[0]);
	cloudBaseFlags.resize(shape[0]);
	gateFlags.reshape(shape, ceilometerPaddingFlag);
	//Some other housekeeping information
	sci::assertThrow(shape[0] < std::numeric_limits<long>::max(), sci::err(sci::SERR_USER, 0, "Profile is too long to fit in a netcdf"));
	pulseQuantities.resize(shape[0]);
	std::vector<megahertzF> sampleRates(shape[0]);
	windowTransmissions.resize(shape[0]);
	windowContaminations.resize(shape[0]);
	laserEnergies.resize(shape[0]);
	scales.resize(shape[0]);
	laserTemperatures.resize(shape[0]);
	tiltAngles.resize(shape[0]);
	backgrounds.resize(shape[0]);
	sums.resize(shape[0]);
	sci::GridData<metreF,2> ranges(shape);
	for (size_t i = 0; i < shape[0]; ++i)
	{
		size_t profileSize = profiles[i].getBetas().size();
		times[i] = profiles[i].getTime();
		for (size_t j = 0; j < profileSize; ++j)
			backscatter[i][j] = profiles[i].getBetas()[j];
		cloudBase1[i] = profiles[i].getCloudBase1();
		cloudBase2[i] = profiles[i].getCloudBase2();
		cloudBase3[i] = profiles[i].getCloudBase3();
		cloudBase4[i] = profiles[i].getCloudBase4();

		//assign the flags
		cloudBaseFlags[i] = profiles[i].getProfileFlag();
		if (cloudBaseFlags[i] == ceilometerRawDataMissingOrSuspectFlag)
			profileFlags[i] = cloudBaseFlags[i];
		else
			profileFlags[i] = cloudBaseFlags[i] = ceilometerGoodFlag;
		std::vector<uint8_t> vectorGateFlags = profiles[i].getGateFlags();// we actually build the flags when we call this function, so ensure we only call it once per profile 
		for(size_t j=0; j< profileSize; ++j)
			gateFlags[i][j] = vectorGateFlags[j];
		if (profileFlags[i] != ceilometerGoodFlag)
			for (size_t j = 0; j < profileSize; ++j)
				if (gateFlags[i][j] != ceilometerGoodFlag)
					gateFlags[i][j] = profileFlags[i];

		//assign the housekeeping data
		pulseQuantities[i] = unitlessF((unitlessF::valueType)profiles[i].getPulseQuantity());
		sampleRates[i] = profiles[i].getSampleRate();
		windowTransmissions[i] = profiles[i].getWindowTransmission();
		windowContaminations[i] = (percentF(100) - windowTransmissions[i]) * millivoltF(2500);
		laserEnergies[i] = profiles[i].getLaserPulseEnergy();
		scales[i] = profiles[i].getScale();
		laserTemperatures[i] = profiles[i].getLaserTemperature();
		tiltAngles[i] = profiles[i].getTiltAngle();
		backgrounds[i] = profiles[i].getBackground();
		sums[i] = profiles[i].getSum();

		//determine the altitudes/ranges
		auto gates = profiles[i].getGates();
		unitlessF cosTilt = sci::cos(tiltAngles[i]);
		for (size_t j = 0; j < profileSize; ++j)
		{
			ranges[i][j] = unitlessF(float(gates[j]) + 0.5f) * profiles[i].getResolution();
			altitudesAboveInstrument[i][j] = ranges[i][j] * cosTilt;
		}

		//do the range squares correction
		//backscatterRangeSquaredCorrected[i].resize(backscatter[i].size());
		//for (size_t j = 0; j < backscatter[i].size(); ++j)
		//	backscatterRangeSquaredCorrected[i][j] = sci::ln(backscatter[i][j] * ranges[i][j] * ranges[i][j] / steradianPerKilometre(1) / metre(1) / metre(1));
	}

	//work out time intervals
	if (times.size() > 1)
	{
		sci::GridData<secondF, 1> intervals(times.size() - 1);
		for (size_t i = 0; i < intervals.size(); ++i)
			intervals[i] = times[i + 1] - times[i];
		dataInfo.samplingInterval = sci::median( intervals );
	}

	//if (times.size() > 0)
	//{
	//	std::vector<decltype(dataInfo.averagingPeriod)> averagePeriods(sampleRates.size());
	//	for (size_t i = 0; i < averagePeriods.size(); ++i)
	//		averagePeriods[i] = pulseQuantities[i] / hertz(10000.0);
	//	std::sort(averagePeriods.begin(), averagePeriods.end());
	//	if (averagePeriods.size() % 2 == 1)
	//		dataInfo.averagingPeriod = averagePeriods[averagePeriods.size() / 2];
	//	else
	//		dataInfo.averagingPeriod = (averagePeriods[averagePeriods.size() / 2 - 1] + averagePeriods[averagePeriods.size() / 2]) / unitless(2.0);
	//}
	//the above code is apparently incorrect. The averaging period is the output period
	dataInfo.averagingPeriod = dataInfo.samplingInterval;

}

/*void CeilometerProcessor::writeToNc(const HplHeader& header, const std::vector<CampbellCeilometerProfile>& profiles, sci::string directory,
	const PersonInfo& author, const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions)
{
	if (platform.getPlatformInfo().platformType == PlatformType::moving)
		writeToNc_1_1_0(header, profiles, directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions);
	else
		writeToNc_2_0_0(header, profiles, directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions);
}

void CeilometerProcessor::writeToNc_1_1_0(const HplHeader& header, const std::vector<CampbellCeilometerProfile>& profiles,
	sci::string directory, const PersonInfo& author, const ProcessingSoftwareInfo& processingSoftwareInfo,
	const ProjectInfo& projectInfo, const Platform& platform, const ProcessingOptions& processingOptions)
{

}*/
void CeilometerProcessor::writeToNc(const HplHeader& header, const std::vector<CampbellCeilometerProfile>& profiles,
	sci::string directory, const PersonInfo& author, const ProcessingSoftwareInfo& processingSoftwareInfo,
	const ProjectInfo& projectInfo, const Platform& platform, const ProcessingOptions& processingOptions)
{

	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;

	//according to the data procucts spreadsheet the instrument needs
	//to produce aerosol-backscatter, cloud-base and cloud-coverage products

	InstrumentInfo ceilometerInfo;
	CalibrationInfo ceilometerCalibrationInfo;
	DataInfo dataInfo;
	sci::GridData<sci::UtcTime, 1> times;
	sci::GridData<metreF, 2> altitudes;
	sci::GridData<perSteradianPerMetreF, 2> backscatter;
	sci::GridData<metreF, 1> cloudBase1;
	sci::GridData<metreF, 1> cloudBase2;
	sci::GridData<metreF, 1> cloudBase3;
	sci::GridData<metreF, 1> cloudBase4;
	sci::GridData<percentF, 1> laserEnergies;
	sci::GridData<kelvinF, 1> laserTemperatures;
	sci::GridData<unitlessF, 1> pulseQuantities;
	sci::GridData<degreeF, 1> tiltAngles;
	sci::GridData<percentF, 1> scales;
	sci::GridData<percentF, 1> windowTransmissions;
	sci::GridData<millivoltF, 1> windowContaminations;
	sci::GridData<millivoltF, 1> backgrounds;
	sci::GridData<perSteradianF, 1> sums;
	sci::GridData<uint8_t, 1> profileFlags;
	sci::GridData<uint8_t, 1> cloudBaseFlags;
	sci::GridData<uint8_t, 2> gateFlags;
	sci::GridData<uint8_t, 2> cloudCoverage;

	formatDataForOutput(header, profiles, ceilometerInfo, ceilometerCalibrationInfo, dataInfo, times, altitudes,
		backscatter, cloudBase1, cloudBase2, cloudBase3, cloudBase4, laserEnergies,
		laserTemperatures, pulseQuantities, tiltAngles, scales, windowTransmissions, windowContaminations, backgrounds, sums, profileFlags,
		gateFlags, cloudBaseFlags);
	dataInfo.processingOptions = processingOptions;
	//build the cloud base data as a 2d vector, time dimension first
	sci::GridData<metreF, 2> cloudBases({ cloudBase1.size(), 4 });
	for (size_t i = 0; i < cloudBases.shape()[0]; ++i)
	{
		cloudBases[i][0] = cloudBase1[i];
		cloudBases[i][1] = cloudBase2[i];
		cloudBases[i][2] = cloudBase3[i];
		cloudBases[i][3] = cloudBase4[i];
	}
	if (platform.getFixedAltitude())
	{
		if (times.size() > 0)
		{
			degreeF longitude;
			degreeF latitude;
			metreF altitude;

			platform.getLocation(times.front(), times.back(), latitude, longitude, altitude);

			altitudes += altitude;
			cloudBases += altitude;
		}
	}
	else
	{
		auto altitudesShape = altitudes.shape();
		auto cloudBasesShape = cloudBases.shape();
		for (size_t i = 0; i < altitudesShape[0]; ++i)
		{
			degreeF longitude;
			degreeF latitude;
			metreF altitude;

			platform.getLocation(times[i], times[i] - dataInfo.averagingPeriod, latitude, longitude, altitude);

			for(size_t j=0; j<altitudesShape[1]; ++j)
				altitudes[i][j] += altitude;
			for(size_t j=0; j<cloudBasesShape[1]; ++j)
				cloudBases[i][j] += altitude;
		}
	}

	//The time dimension will be created automatically when we create our
	//output file, but we must specify that we need a height dimension too
	sci::NcAttribute wavelengthAttribute(sU("laser_wavelength"), sU("910 nm"));
	sci::NcAttribute energyAttribute(sU("nominal_laser_pulse_energy"), sU("3.0e-06 J"));
	sci::NcAttribute pulseFrequencyAttribute(sU("pulse_repetition_frequency"), sU("6500 Hz"));
	sci::NcAttribute diameterAttribute(sU("lens_diameter"), sU("0.148 m"));
	sci::NcAttribute divergenceAttribute(sU("beam_divergence"), sU("0.14 degrees"));
	sci::NcAttribute pulseLengthAttribute(sU("pulse_length"), sU("1.1e-07 s"));
	sci::NcAttribute samplingFrequencyAttribute(sU("sampling_frequency"), sU("1.5e+07 Hz"));
	std::vector<sci::NcAttribute*> ceilometerGlobalAttributes{ &wavelengthAttribute, &energyAttribute,
		&pulseFrequencyAttribute, &diameterAttribute, &divergenceAttribute, &pulseLengthAttribute, &samplingFrequencyAttribute };

	std::vector<std::pair<sci::string, CellMethod>>cellMethodsTimeMean{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsTimeSum{ {sU("time"), CellMethod::sum} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsTimePoint{ {sU("time"), CellMethod::point} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsNone{  };
	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };

	{
		// put this stuff in a scope of its own to avoid name clashes with other files

		//////////////////////////////////////////////////////////
		//Backscatter file first
		//////////////////////////////////////////////////////////

		DataInfo backscatterDataInfo = dataInfo;
		backscatterDataInfo.productName = sU("aerosol-backscatter");
		sci::NcDimension altitudeDimension(sU("altitude"), altitudes[0].size());
		sci::NcDimension indexDimension(sU("index"), altitudes[0].size());
		std::vector<sci::NcDimension*> nonTimeDimensions;
		if (amfVersion == AmfVersion::v1_1_0)
			nonTimeDimensions.push_back(&indexDimension);
		else
			nonTimeDimensions.push_back(&altitudeDimension);

		//create the file and dimensions. The time variable is added automatically
		OutputAmfNcFile ceilometerBackscatterFile(amfVersion, directory, ceilometerInfo, author, processingSoftwareInfo, ceilometerCalibrationInfo, backscatterDataInfo,
			projectInfo, platform, sU("ceilometer profile"), times, nonTimeDimensions, sU(""), ceilometerGlobalAttributes);

		std::vector<sci::NcDimension*> backscatterDimensions{ &ceilometerBackscatterFile.getTimeDimension(), nonTimeDimensions[0] };

		AmfNcVariable<perSteradianPerMetreF> backscatterVariable(sU("attenuated_aerosol_backscatter_coefficient"), ceilometerBackscatterFile, std::vector<sci::NcDimension*>{ &ceilometerBackscatterFile.getTimeDimension(), nonTimeDimensions[0] }, sU("Attenuated Aerosol Backscatter Coefficient"), sU(""), backscatter, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<percentF> laserEnergiesVariable(sU("laser_pulse_energy"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Laser Pulse Energy (% of maximum)"), sU(""), laserEnergies, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<kelvinF> laserTemperaturesVariable(sU("laser_temperature"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Laser Temperature"), sU(""), laserTemperatures, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<degreeF> tiltAngleVariable(sU("sensor_zenith_angle"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Sensor Zenith Angle (from vertical)"), sU("sensor_zenith_angle"), tiltAngles, true, coordinates, cellMethodsTimeMean, profileFlags);
		//AmfNcVariable<degreeF, std::vector<degreeF>> azimuthAngleVariable(sU("sensor_azimuth_angle"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Sensor Azimuth Angle (clockwise from true North)"), sU("sensor_azimuth_angle"), std::vector<degreeF>(tiltAngles.size(), std::numeric_limits<degreeF>::quiet_NaN()), true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<unitlessF> pulseQuantitiesVariable(sU("profile_pulses"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Number of pulses in each profile"), sU(""), pulseQuantities, true, coordinates, cellMethodsTimeSum, profileFlags);
		AmfNcVariable<percentF> scalesVariable(sU("profile_scaling"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Scaling of range profile (default=100%)"), sU(""), scales, true, coordinates, cellMethodsTimePoint, profileFlags);
		AmfNcVariable<percentF> windowTransmissionsVariable(sU("window_transmittance"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Window Transmittance, % of nominal value"), sU(""), windowTransmissions, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<millivoltF> windowContaminationsVariable(sU("window_contamination"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Window Contamination (mV as measured by ADC: 0 - 2500)"), sU(""), windowContaminations, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<millivoltF> backgroundsVariable(sU("background_light"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Background Light (mV as measured by ADC: 0 - 2500)"), sU(""), backgrounds, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<perSteradianF> sumsVariable(sU("backscatter_sum"), ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension(), sU("Sum of detected and normalized backscatter"), sU(""), sums, true, coordinates, cellMethodsTimeMean, profileFlags);
		//AmfNcFlagVariable profileFlagsVariable(sU("qc_flag_profiles"), ceilometerFlags, ceilometerBackscatterFile, ceilometerBackscatterFile.getTimeDimension() );
		AmfNcFlagVariable gateFlagsVariable(sU("qc_flag"), ceilometerFlags, ceilometerBackscatterFile, backscatterDimensions);

		if (amfVersion == AmfVersion::v1_1_0)
		{
			AmfNcAltitudeVariable altitudeVariable(ceilometerBackscatterFile, backscatterDimensions, altitudes, dataInfo.featureType);
			//AmfNcVariable<metreF> altitudeVariable(sU("altitude"), ceilometerBackscatterFile, backscatterDimensions, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates, cellMethodsNone, 1);
			//altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), ceilometerBackscatterFile);
			ceilometerBackscatterFile.writeTimeAndLocationData(platform);
			ceilometerBackscatterFile.write(altitudeVariable, altitudes);
		}
		else
		{
			AmfNcAltitudeVariable altitudeVariable(ceilometerBackscatterFile, backscatterDimensions, altitudes[0], dataInfo.featureType);
			//AmfNcVariable<metreF> altitudeVariable(sU("altitude"), ceilometerBackscatterFile, altitudeDimension, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes[0], true, coordinates, cellMethodsNone, 1);
			//altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), ceilometerBackscatterFile);
			ceilometerBackscatterFile.writeTimeAndLocationData(platform);
			ceilometerBackscatterFile.write(altitudeVariable, altitudes);
		}


		ceilometerBackscatterFile.write(backscatterVariable, backscatter);
		ceilometerBackscatterFile.write(laserEnergiesVariable, laserEnergies);
		ceilometerBackscatterFile.write(laserTemperaturesVariable, laserTemperatures);
		ceilometerBackscatterFile.write(tiltAngleVariable, tiltAngles);
		//ceilometerBackscatterFile.write(azimuthAngleVariable);
		ceilometerBackscatterFile.write(pulseQuantitiesVariable, pulseQuantities);
		ceilometerBackscatterFile.write(scalesVariable, scales);
		ceilometerBackscatterFile.write(windowTransmissionsVariable, windowTransmissions);
		ceilometerBackscatterFile.write(windowContaminationsVariable, windowContaminations);
		ceilometerBackscatterFile.write(backgroundsVariable, backgrounds);
		ceilometerBackscatterFile.write(sumsVariable, sums);

		//flags
		//ceilometerBackscatterFile.write(profileFlagsVariable, profileFlags);
		ceilometerBackscatterFile.write(gateFlagsVariable, gateFlags);
	}
	{
		// put this stuff in a scope of its own to avoid name clashes with other files

		//////////////////////////////////////////////////////////
		//Cloud base file
		//////////////////////////////////////////////////////////

		DataInfo cloudBaseDataInfo = dataInfo;
		cloudBaseDataInfo.productName = sU("cloud-base");
		cloudBaseDataInfo.featureType = FeatureType::timeSeries;
		sci::NcDimension layerIndexDimension(sU("layer_index"), 4);
		std::vector<sci::NcDimension*> nonTimeDimensions;
		nonTimeDimensions.push_back(&layerIndexDimension);


		//create the file and dimensions. The time variable is added automatically
		OutputAmfNcFile ceilometerCloudBaseFile(amfVersion, directory, ceilometerInfo, author, processingSoftwareInfo, ceilometerCalibrationInfo, cloudBaseDataInfo,
			projectInfo, platform, sU("ceilometer profile"), times, nonTimeDimensions, sU(""), ceilometerGlobalAttributes);

		//add the data variables
		AmfNcVariable<metreF> cloudBaseVariable(sU("cloud_base_altitude"), ceilometerCloudBaseFile, std::vector<sci::NcDimension*>{ &ceilometerCloudBaseFile.getTimeDimension(), & layerIndexDimension }, sU("Cloud Base Altitude (Geometric height above geoid (WGS84))"), sU("cloud_base_altitude"), cloudBases, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<percentF> laserEnergiesVariable(sU("laser_pulse_energy"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Laser Pulse Energy (% of maximum)"), sU(""), laserEnergies, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<kelvinF> laserTemperaturesVariable(sU("laser_temperature"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Laser Temperature"), sU(""), laserTemperatures, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<degreeF> tiltAngleVariable(sU("sensor_zenith_angle"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Sensor Zenith Angle (from vertical)"), sU("sensor_zenith_angle"), tiltAngles, true, coordinates, cellMethodsTimeMean, profileFlags);
		//AmfNcVariable<degreeF, std::vector<degreeF>> azimuthAngleVariable(sU("sensor_azimuth_angle"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Sensor Azimuth Angle (clockwise from true North)"), sU("sensor_azimuth_angle"), std::vector<degreeF>(tiltAngles.size(), std::numeric_limits<degreeF>::quiet_NaN()), true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<unitlessF> pulseQuantitiesVariable(sU("profile_pulses"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Number of pulses in each profile"), sU(""), pulseQuantities, true, coordinates, cellMethodsTimeSum, profileFlags);
		AmfNcVariable<percentF> scalesVariable(sU("profile_scaling"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Scaling of range profile (default=100%)"), sU(""), scales, true, coordinates, cellMethodsTimePoint, profileFlags);
		AmfNcVariable<percentF> windowTransmissionsVariable(sU("window_transmittance"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Window Transmittance, % of nominal value"), sU(""), windowTransmissions, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<millivoltF> backgroundsVariable(sU("background_light"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Background Light (mV as measured by ADC: 0 - 2500)"), sU(""), backgrounds, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<perSteradianF> sumsVariable(sU("backscatter_sum"), ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension(), sU("Sum of detected and normalized backscatter"), sU(""), sums, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcFlagVariable profileFlagsVariable(sU("qc_flag"), ceilometerFlags, ceilometerCloudBaseFile, ceilometerCloudBaseFile.getTimeDimension());

		ceilometerCloudBaseFile.writeTimeAndLocationData(platform);

		ceilometerCloudBaseFile.write(cloudBaseVariable, cloudBases);
		ceilometerCloudBaseFile.write(laserEnergiesVariable, laserEnergies);
		ceilometerCloudBaseFile.write(laserTemperaturesVariable, laserTemperatures);
		ceilometerCloudBaseFile.write(tiltAngleVariable, tiltAngles);
		//ceilometerCloudBaseFile.write(azimuthAngleVariable);
		ceilometerCloudBaseFile.write(pulseQuantitiesVariable, pulseQuantities);
		ceilometerCloudBaseFile.write(scalesVariable, scales);
		ceilometerCloudBaseFile.write(windowTransmissionsVariable, windowTransmissions);
		ceilometerCloudBaseFile.write(backgroundsVariable, backgrounds);
		ceilometerCloudBaseFile.write(sumsVariable, sums);

		ceilometerCloudBaseFile.write(profileFlagsVariable, profileFlags);
	}

	if(cloudCoverage.size() > 0)
	{
		// put this stuff in a scope of its own to avoid name clashes with other files

		//////////////////////////////////////////////////////////
		//Cloud coverage file
		//NOTE, at the time of writing we have been using message002,
		//Which does not report cloud boverage (known as sky
		//condition in the manual), so this is just placeholder
		//code which is not used;
		//////////////////////////////////////////////////////////

		DataInfo cloudCoverageDataInfo = dataInfo;
		cloudCoverageDataInfo.productName = sU("cloud-coverage");
		sci::NcDimension layerIndexDimension(sU("layer_index"), 4);
		std::vector<sci::NcDimension*> nonTimeDimensions;
		nonTimeDimensions.push_back(&layerIndexDimension);

		//create the file and dimensions. The time variable is added automatically
		OutputAmfNcFile ceilometerCloudCoverageFile(amfVersion, directory, ceilometerInfo, author, processingSoftwareInfo, ceilometerCalibrationInfo, cloudCoverageDataInfo,
			projectInfo, platform, sU("ceilometer profile"), times, nonTimeDimensions, sU(""), ceilometerGlobalAttributes);

		//add the data variables
		AmfNcVariable<metreF> cloudBaseVariable(sU("cloud_base_altitude"), ceilometerCloudCoverageFile, std::vector<sci::NcDimension*>{ &ceilometerCloudCoverageFile.getTimeDimension(), & layerIndexDimension }, sU("Cloud Base Altitude (Geometric height above geoid (WGS84))"), sU("cloud_base_altitude"), cloudBases, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<uint8_t> cloudCoverVariable(sU("cloud_base_altitude"), ceilometerCloudCoverageFile, std::vector<sci::NcDimension*>{ &ceilometerCloudCoverageFile.getTimeDimension(), & layerIndexDimension }, sU("Cloud Coverage in oktas (0 = clear, 8 = full coverage)"), sU(""), sU("okta"), cloudCoverage, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<percentF> laserEnergiesVariable(sU("laser_pulse_energy"), ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension(), sU("Laser Pulse Energy (% of maximum)"), sU(""), laserEnergies, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<kelvinF> laserTemperaturesVariable(sU("laser_temperature"), ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension(), sU("Laser Temperature"), sU(""), laserTemperatures, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<degreeF> tiltAngleVariable(sU("sensor_zenith_angle"), ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension(), sU("Sensor Zenith Angle (from vertical)"), sU("sensor_zenith_angle"), tiltAngles, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<unitlessF> pulseQuantitiesVariable(sU("profile_pulses"), ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension(), sU("Number of pulses in each profile"), sU(""), pulseQuantities, true, coordinates, cellMethodsTimeSum, profileFlags);
		AmfNcVariable<percentF> scalesVariable(sU("profile_scaling"), ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension(), sU("Scaling of range profile (default=100%)"), sU(""), scales, true, coordinates, cellMethodsTimePoint, profileFlags);
		AmfNcVariable<percentF> windowTransmissionsVariable(sU("window_transmittance"), ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension(), sU("Window Transmittance, % of nominal value"), sU(""), windowTransmissions, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<millivoltF> backgroundsVariable(sU("background_light"), ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension(), sU("Background Light (mV as measured by ADC: 0 - 2500)"), sU(""), backgrounds, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcVariable<perSteradianF> sumsVariable(sU("backscatter_sum"), ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension(), sU("Sum of detected and normalized backscatter"), sU(""), sums, true, coordinates, cellMethodsTimeMean, profileFlags);
		AmfNcFlagVariable profileFlagsVariable(sU("profile-flag"), ceilometerFlags, ceilometerCloudCoverageFile, ceilometerCloudCoverageFile.getTimeDimension());

		ceilometerCloudCoverageFile.writeTimeAndLocationData(platform);

		ceilometerCloudCoverageFile.write(cloudBaseVariable, cloudBases);
		ceilometerCloudCoverageFile.write(laserEnergiesVariable, laserEnergies);
		ceilometerCloudCoverageFile.write(laserTemperaturesVariable, laserTemperatures);
		ceilometerCloudCoverageFile.write(tiltAngleVariable, tiltAngles);
		ceilometerCloudCoverageFile.write(pulseQuantitiesVariable, pulseQuantities);
		ceilometerCloudCoverageFile.write(scalesVariable, scales);
		ceilometerCloudCoverageFile.write(windowTransmissionsVariable, windowTransmissions);
		ceilometerCloudCoverageFile.write(backgroundsVariable, backgrounds);
		ceilometerCloudCoverageFile.write(sumsVariable, sums);

		ceilometerCloudCoverageFile.write(profileFlagsVariable, profileFlags);
	}


	//gates/heights
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
				if (m_allData.size() == 1 && m_allData.back().getTime() > getCeilometerTime(timeDate))
				{
					m_allData.pop_back();
					progressReporter << sU("The second entry in the file has a timestamp earlier than the first. This may be due to a logging glitch. The first entry will be deleted.\n");
				}
				else if (m_allData.size() > 1 && m_allData.back().getTime() > getCeilometerTime(timeDate))
				{
					sci::assertThrow(false, sci::err(sci::SERR_USER, 0, sU("Time jumps backwards in this file, it cannot be processed.")));
				}
				CampbellHeader header;
				header.readHeader(fin);
				if (m_ceilometerOsVersion.length() == 0)
					m_ceilometerOsVersion = sci::fromCodepage(header.getOs());
				if (m_allData.size() == 1)
				{
					m_firstHeaderCampbell = header;
					m_firstHeaderHpl = createCeilometerHeader(inputFilename, header, m_allData[0]);
				}
				if (header.getMessageType() == campbellMessageType::cs && header.getMessageNumber() == 2)
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

void CeilometerProcessor::plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(hasData(), sci::err(sci::SERR_USER, 0, sU("Attempted to plot Ceilometer data when it has not been read")));

	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i].value<metreF>() != std::numeric_limits<double>::max())
		{
			rangeLimitedfilename << sU("_maxRange_");
			rangeLimitedfilename << maxRanges[i];
		}
		plotCeilometerProfiles(m_firstHeaderHpl, m_allData, rangeLimitedfilename.str(), maxRanges[i], progressReporter, parent);
	}
}

void CeilometerProcessor::plotCeilometerProfiles(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, sci::string filename, metreF maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	/*
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, sU(""), parent, header);
	WindowCleaner cleaner(window);

	//We will do some averaging with the data - there is no point plotting thousands of profiles
	//on a plot that is ~800 pixels across.
	size_t timeAveragePeriod = 1;
	while (profiles.size() / timeAveragePeriod > 800)
		timeAveragePeriod *= 2;

	std::vector<std::vector<perSteradianPerMetreF>> data(profiles.size() / timeAveragePeriod);
	for (size_t i = 0; i < data.size(); ++i)
	{
		sci::convert(data[i], profiles[i*timeAveragePeriod].getBetas());
		for (size_t j = 1; j < timeAveragePeriod; ++j)
			data[i] += profiles[i*timeAveragePeriod + j].getBetas();
	}
	data /= unitlessF((unitlessF::valueType)timeAveragePeriod);

	if (profiles[0].getResolution()*unitlessF(unitlessF::valueType(data[0].size() - 1)) > maxRange)
	{
		size_t pointsNeeded = std::min((size_t)std::ceil((maxRange / profiles[0].getResolution()).value<unitlessF>()), data[0].size());
		for (size_t i = 0; i < data.size(); ++i)
			data[i].resize(pointsNeeded);
	}

	size_t heightAveragePeriod = 1;
	while (data[0].size() / heightAveragePeriod > 800)
		heightAveragePeriod *= 2;
	if (heightAveragePeriod > 1)
	{
		for (size_t i = 0; i < data.size(); ++i)
			data[i] = sci::boxcaraverage(data[i], heightAveragePeriod, sci::PhysicalDivide<perSteradianPerMetreF::unit,steradianPerMetreF::valueType>);
	}

	std::vector<second> xs(data.size() + 1);
	std::vector<metreF> ys(data[0].size() + 1);

	//calculating our heights assumes that the profiles have range gates of 0, 1, 2, 3, ... so check this;
	bool gatesGood = true;
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<size_t> gates = profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			sci::assertThrow(gates[j] == j, sci::err(sci::SERR_USER, 0, sU("The plotting code currently assumes gates go 0, 1, 2, 3, ... but it found a profile where this was not the case.")));
	}

	xs[0] = second(profiles[0].getTime() - sci::UtcTime(1970, 1, 1, 0, 0, 0));
	for (size_t i = 1; i < xs.size() - 1; ++i)
		xs[i] = second((profiles[i*timeAveragePeriod].getTime() - sci::UtcTime(1970, 1, 1, 0, 0, 0) + profiles[i*timeAveragePeriod - 1].getTime() - sci::UtcTime(1970, 1, 1, 0, 0, 0)) / unitlessF(2.0));
	xs.back() = second(profiles.back().getTime() - sci::UtcTime(1970, 1, 1, 0, 0, 0));

	for (size_t i = 0; i < ys.size(); ++i)
		ys[i] = unitlessF(unitlessF::valueType(i * heightAveragePeriod)) * header.rangeGateLength;

	std::vector<double> xsTemp;
	std::vector<double> ysTemp;
	std::vector<std::vector<double>> zsTemp;
	sci::convert(xsTemp, sci::physicalsToValues<second>(xs));
	sci::convert(ysTemp, sci::physicalsToValues<metreF>(ys));
	sci::convert(zsTemp, sci::physicalsToValues<perSteradianPerMetreF>(data));

	std::shared_ptr<GridData> gridData(new GridData(xsTemp, ysTemp, zsTemp, g_lidarColourscale, true, true));

	plot->addData(gridData);

	plot->getxaxis()->settitle(sU("Time"));
	plot->getxaxis()->settimeformat(sU("%H:%M:%S"));
	plot->getyaxis()->settitle(sU("Height (m)"));

	if (ys.back() > maxRange)
		plot->setmaxy(sci::physicalsToValues<metreF>(maxRange));

	createDirectoryAndWritePlot(window, filename, 1000, 1000, progressReporter);
	*/
}

std::vector<std::vector<sci::string>> CeilometerProcessor::groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	return InstrumentProcessor::groupInputFilesbyOutputFiles(newFiles, allFiles, 0, 8);
}

bool CeilometerProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	return InstrumentProcessor::fileCoversTimePeriod(fileName, startTime, endTime, 0, -1, -1, -1, second(86399));
}