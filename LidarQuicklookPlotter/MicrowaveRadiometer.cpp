#include"MicrowaveRadiometer.h"
#include"ProgressReporter.h"

MicrowaveRadiometerProcessor::MicrowaveRadiometerProcessor(const InstrumentInfo& instrumentInfo, const CalibrationInfo& calibrationInfo)
	: InstrumentProcessor(sU("[/\\\\]Y....[/\\\\]M..[/\\\\]D..[/\\\\]......\\.LWP"))
{
	m_hasData = false;
	m_instrumentInfo = instrumentInfo;
	m_calibrationInfo = calibrationInfo;
}

void MicrowaveRadiometerProcessor::readData(const std::vector<sci::string>& inputFilenames, const Platform& platform, ProgressReporter& progressReporter)
{

	size_t elementsInFirstFile;
	size_t elementsInSecondFile;
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		//after reading the first two files reserve space for the remaining files. This
		//can speed things up a lot
		if (i == 1)
			elementsInFirstFile = m_lwp.size();
		if (i == 2)
		{
			elementsInSecondFile = m_lwp.size() - elementsInFirstFile;
			size_t reserveSize = inputFilenames.size() * std::max(elementsInFirstFile, elementsInSecondFile);
			m_lwpTime.reserve(reserveSize);
			m_lwp.reserve(reserveSize);
			m_lwpElevation.reserve(reserveSize);
			m_lwpAzimuth.reserve(reserveSize);
			m_lwpRainFlag.reserve(reserveSize);
			m_lwpQuality.reserve(reserveSize);
			m_lwpQualityExplanation.reserve(reserveSize);
		}

		try
		{
			readData(inputFilenames[i], platform, progressReporter, i == 0);
		}
		catch (sci::err err)
		{
			//catch any sci::errors - these probably mean a corrupt or incomplete file, but we can continue with other files.
			progressReporter << err.getErrorMessage() << "\n";
		}
		if (progressReporter.shouldStop())
			break;
	}
}

void MicrowaveRadiometerProcessor::readData(const sci::string& inputFilename, const Platform& platform, ProgressReporter& progressReporter, bool clear)
{
	if (clear)
	{
		m_hasData = false;
		m_lwpTime.resize(0);
		m_lwp.resize(0);
		m_lwpElevation.resize(0);
		m_lwpAzimuth.resize(0);
		m_lwpRainFlag.resize(0);
		m_lwpQuality.resize(0);
		m_lwpQualityExplanation.resize(0);

		readHatproLwpData(inputFilename, m_lwpTime, m_lwp, m_lwpElevation, m_lwpAzimuth, m_lwpRainFlag, m_lwpQuality, m_lwpQualityExplanation);
	}
	else
	{
		std::vector<sci::UtcTime> lwpTime;
		std::vector<gramPerMetreSquaredF> lwp;
		std::vector<degreeF> lwpElevation;
		std::vector<degreeF> lwpAzimuth;
		std::vector<bool> lwpRainFlag;
		std::vector<unsigned char> lwpQuality;
		std::vector<unsigned char>lwpQualityExplanation;

		readHatproLwpData(inputFilename, lwpTime, lwp, lwpElevation, lwpAzimuth, lwpRainFlag, lwpQuality, lwpQualityExplanation);

		m_lwpTime.insert(m_lwpTime.end(), lwpTime.begin(), lwpTime.end());
		m_lwp.insert(m_lwp.end(), lwp.begin(), lwp.end());
		m_lwpElevation.insert(m_lwpElevation.end(), lwpElevation.begin(), lwpElevation.end());
		m_lwpAzimuth.insert(m_lwpAzimuth.end(), lwpAzimuth.begin(), lwpAzimuth.end());
		m_lwpRainFlag.insert(m_lwpRainFlag.end(), lwpRainFlag.begin(), lwpRainFlag.end());
		m_lwpQuality.insert(m_lwpQuality.end(), lwpQuality.begin(), lwpQuality.end());
		m_lwpQualityExplanation.insert(m_lwpQualityExplanation.end(), lwpQualityExplanation.begin(), lwpQualityExplanation.end());
	}


}

void MicrowaveRadiometerProcessor::writeToNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	/*DataInfo dataInfo1d;
	dataInfo1d.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo1d.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo1d.continuous = true;
	dataInfo1d.startTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	dataInfo1d.endTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	if (m_profiles.size() > 0)
	{
		dataInfo1d.startTime = m_profiles[0].getTime();
		dataInfo1d.endTime = m_profiles.back().getTime();
	}
	dataInfo1d.featureType = FeatureType::timeSeriesProfile;
	dataInfo1d.options = std::vector<sci::string>(0);
	dataInfo1d.processingLevel = 2;
	dataInfo1d.productName = sU("rain lwc velocity reflectivity");
	dataInfo1d.processingOptions = processingOptions;

	if (m_profiles.size() > 0)
	{
		std::vector<second> averagingTimes(m_profiles.size());
		for (size_t i = 0; i < averagingTimes.size(); ++i)
		{
			averagingTimes[i] = m_profiles[i].getAveragingTime();
		}
		dataInfo1d.averagingPeriod = secondF(sci::median(averagingTimes));
	}

	//grab the times;
	std::vector<sci::UtcTime> times(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		times[i] = m_profiles[i].getTime();
	}

	//work out the sampling interval - this is the difference between each start time.
	//use the median as the value for the file
	if (m_profiles.size() > 1)
	{
		std::vector<second> samplingIntervals(m_profiles.size());
		for (size_t i = 1; i < samplingIntervals.size(); ++i)
		{
			samplingIntervals[i] = times[i] - times[i - 1];
		}
		dataInfo1d.samplingInterval = secondF(sci::median(samplingIntervals));
	}

	DataInfo dataInfo2d = dataInfo1d;
	dataInfo2d.productName = sU("size concentration spectra");

	//pull all the 1d variables out
	std::vector<std::vector<metreF>> altitudes(m_profiles.size());
	std::vector<std::vector<millimetrePerHourF>> rainfallRates(m_profiles.size());
	std::vector<std::vector<gramPerMetreCubedF>> rainLiquidWaterContent(m_profiles.size());
	std::vector<std::vector<metrePerSecondF>> rainfallVelocity(m_profiles.size());
	std::vector<std::vector<reflectivityF>> radarReflectivity(m_profiles.size());
	std::vector<std::vector<reflectivityF>> radarReflectivityAttenuated(m_profiles.size());
	std::vector<std::vector<unitlessF>> pathIntegratedAttenuation(m_profiles.size());

	//assign amf version based on if we are moving or not. Note that we swap from v2 to v1 below if the altitudes
	//are not consistent
	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;

	degreeF longitude;
	degreeF latitude;
	metreF instrumentAltitude;
	std::vector<metreF> firstAltitudes;
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		platform.getLocation(m_profiles[i].getTime() - m_profiles[i].getAveragingTime(), m_profiles[i].getTime(), latitude, longitude, instrumentAltitude);
		unitlessF sinElevation;
		unitlessF sinAzimuth;
		unitlessF sinRoll;
		unitlessF cosElevation;
		unitlessF cosAzimuth;
		unitlessF cosRoll;
		platform.getInstrumentTrigAttitudesForDirectionCorrection(m_profiles[i].getTime() - m_profiles[i].getAveragingTime(), m_profiles[i].getTime(), sinElevation, sinAzimuth, sinRoll, cosElevation, cosAzimuth, cosRoll);
		sci::convert(altitudes[i], m_profiles[i].getRanges() * cosElevation + instrumentAltitude);
		if (i == 0)
			firstAltitudes = altitudes[0];
		else if (amfVersion == AmfVersion::v2_0_0 && sci::anyFalse(sci::isEq(altitudes[i], firstAltitudes)))
			amfVersion = AmfVersion::v1_1_0;
	}

	for (size_t i = 0; i < m_profiles.size(); ++i)
		rainfallRates[i] = m_profiles[i].getRainRate();
	for (size_t i = 0; i < m_profiles.size(); ++i)
		rainLiquidWaterContent[i] = m_profiles[i].getLiquidWaterContent();
	for (size_t i = 0; i < m_profiles.size(); ++i)
		rainfallVelocity[i] = m_profiles[i].getFallVeocity();
	for (size_t i = 0; i < m_profiles.size(); ++i)
		radarReflectivity[i] = m_profiles[i].getReflectivityAttenuationCorrected();
	for (size_t i = 0; i < m_profiles.size(); ++i)
		radarReflectivityAttenuated[i] = m_profiles[i].getReflectivity();
	for (size_t i = 0; i < m_profiles.size(); ++i)
		pathIntegratedAttenuation[i] = m_profiles[i].getPathIntegratedAttenuation();

	//generate the flag variable. Basically the only option here is that data could be missing
	//due to negative reflectivities from subtraction of the noise floor. This is marked by NaNs
	std::vector<std::vector<unsigned char>> flags(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		flags[i].resize(radarReflectivity[i].size(), microRainRadarGoodDataFlag);
		if (m_profiles[i].getValidFraction() < percentF(50))
			flags[i].assign(flags[i].size(), microRainRadarQualityBelowFiftyPercentFlag);
		else if (m_profiles[i].getValidFraction() < percentF(100))
			flags[i].assign(flags[i].size(), microRainRadarQualityBelowOnehundredPercentFlag);

		std::vector<SBOOL> missing(rainfallRates[i].size());
		for (size_t j = 0; j < missing.size(); ++j)
		{
			missing[j] = (rainfallRates[i][j] != rainfallRates[i][j]) ||
				(rainLiquidWaterContent[i][j] != rainLiquidWaterContent[i][j]) ||
				(rainfallVelocity[i][j] != rainfallVelocity[i][j]) ||
				(radarReflectivity[i][j] != radarReflectivity[i][j]) ||
				(radarReflectivityAttenuated[i][j] != radarReflectivityAttenuated[i][j]) ||
				(pathIntegratedAttenuation[i][j] != pathIntegratedAttenuation[i][j]);


		}
		sci::assign(flags[i], rainfallVelocity[i] < metrePerSecondF(0.0) || rainfallVelocity[i] > metrePerSecondF(20.0), microRainRadarSpeedsOutsideExpectedLimitsFlag);
		sci::assign(flags[i], missing, microRainRadarBelowNoiseFloorFlag);
	}

	//Output the one D parameters

	std::vector<sci::NcDimension*> nonTimeDimensions1D;
	sci::NcDimension altitudeDimension1d(sU("altitudes"), altitudes[0].size());
	sci::NcDimension indexDimension(sU("index"), altitudes[0].size());
	if (amfVersion == AmfVersion::v1_1_0)
		nonTimeDimensions1D.push_back(&indexDimension);
	else
		nonTimeDimensions1D.push_back(&altitudeDimension1d);

	OutputAmfNcFile file1d(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo1d,
		projectInfo, platform, sU("micro rain radar size integrated profile"), times, nonTimeDimensions1D);

	std::vector<std::pair<sci::string, CellMethod>>cellMethods1d{ {sU("time"), CellMethod::mean} };
	std::vector<sci::string> coordinates1d{ sU("latitude"), sU("longitude") };
	std::vector<sci::NcDimension*> dimensions1d{ &file1d.getTimeDimension(), nonTimeDimensions1D[0] };

	//AmfNcVariable<metre, std::vector<std::vector<metre>>> altitudesVariable(sU("altitude"), file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), & altitudeDimension1d }, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates1d, std::vector<std::pair<sci::string, CellMethod>>(0));
	//altitudesVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file1d);
	AmfNcVariable<millimetrePerHourF, std::vector<std::vector<millimetrePerHourF>>> rainfallRatesVariable(sU("rainfall_rate"), file1d, dimensions1d, sU("Rainfall Rate"), sU("rainfall_rate"), rainfallRates, true, coordinates1d, cellMethods1d);
	AmfNcVariable<gramPerMetreCubedF, std::vector<std::vector<gramPerMetreCubedF>>> rainLiquidWaterContentVariable(sU("rain_liquid_water_content"), file1d, dimensions1d, sU("Rain Liquid Water Content"), sU(""), rainLiquidWaterContent, true, coordinates1d, cellMethods1d);
	AmfNcVariable<metrePerSecondF, std::vector<std::vector<metrePerSecondF>>> rainfallVelocityVariable(sU("rainfall_velocity"), file1d, dimensions1d, sU("Rainfall Velocity"), sU(""), rainfallVelocity, true, coordinates1d, cellMethods1d);
	AmfNcVariable<Decibel<reflectivityF>, std::vector<std::vector<reflectivityF>>> radarReflectivityVariable(sU("radar_reflectivity"), file1d, dimensions1d, sU("Radar Reflectivity (Z)"), sU(""), radarReflectivity, true, coordinates1d, cellMethods1d, true, false);
	AmfNcVariable<Decibel<reflectivityF>, std::vector<std::vector<reflectivityF>>> radarReflectivityAttenuatedVariable(sU("attenuated_radar_reflectivity"), file1d, dimensions1d, sU("Attenuated Radar Reflectivity (z)"), sU(""), radarReflectivityAttenuated, true, coordinates1d, cellMethods1d, true, false);
	AmfNcVariable<Decibel<unitlessF>, std::vector<std::vector<unitlessF>>> pathIntegratedAttenuationVariable(sU("path_integrated_attenuation"), file1d, dimensions1d, sU("Path Integrated Attenuation"), sU(""), pathIntegratedAttenuation, true, coordinates1d, cellMethods1d, false, false);
	AmfNcFlagVariable flagVariable(sU("qc_flag"), microRainRadarFlags, file1d, dimensions1d);

	if (amfVersion == AmfVersion::v1_1_0)
	{
		AmfNcVariable<metreF, decltype(altitudes)> altitudeVariable(sU("altitude"), file1d, dimensions1d, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates1d, std::vector<std::pair<sci::string, CellMethod>>(0));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file1d);
		file1d.writeTimeAndLocationData(platform);
		file1d.write(altitudeVariable);
	}
	else
	{
		AmfNcVariable<metreF, std::vector<metreF>> altitudeVariable(sU("altitude"), file1d, file1d.getTimeDimension(), sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes[0], true, coordinates1d, std::vector<std::pair<sci::string, CellMethod>>(0));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file1d);
		file1d.writeTimeAndLocationData(platform);
		file1d.write(altitudeVariable);
	}

	file1d.write(rainfallRatesVariable);
	file1d.write(rainLiquidWaterContentVariable);
	file1d.write(rainfallVelocityVariable);
	file1d.write(radarReflectivityVariable);
	file1d.write(radarReflectivityAttenuatedVariable);
	file1d.write(pathIntegratedAttenuationVariable);
	file1d.write(flagVariable, flags);

	//pull out the 2 d variables. We must transpose them so that the range dimension is first

	std::vector<std::vector<std::vector<perMetreF>>> spectralReflectivity(m_profiles.size());
	std::vector<std::vector<std::vector<millimetreF>>> dropDiameters(m_profiles.size());
	std::vector<std::vector<std::vector<perMetreCubedPerMillimetreF>>> sizeDistributions(m_profiles.size());

	for (size_t i = 0; i < m_profiles.size(); ++i)
		spectralReflectivity[i] = sci::transpose(m_profiles[i].getSpectralReflectivities());
	for (size_t i = 0; i < m_profiles.size(); ++i)
		dropDiameters[i] = sci::transpose(m_profiles[i].getDropDiameters());
	for (size_t i = 0; i < m_profiles.size(); ++i)
		sizeDistributions[i] = sci::transpose(m_profiles[i].getNumberDistribution());


	//generate the flag variables.
	std::vector<std::vector<std::vector<unsigned char>>> spectralFlags(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		spectralFlags[i].resize(dropDiameters[i].size());
		for (size_t j = 0; j < dropDiameters[i].size(); ++j)
		{
			spectralFlags[i][j].resize(dropDiameters[i][j].size(), flags[i][j]);
			for (size_t k = 0; k < dropDiameters[i][j].size(); ++k)
			{
				if (dropDiameters[i][j][k] < millimetreF(0.0))
					spectralFlags[i][j][k] = microRainRadarNegativeDiametersFlag;
				if (dropDiameters[i][j][k] != dropDiameters[i][j][k])
					spectralFlags[i][j][k] = microRainRadarDiameterNotDerivedFlag;
			}
		}
	}


	//create and write the file
	std::vector<sci::NcDimension*> nonTimeDimensions2d;
	sci::NcDimension altitudeDimension2d(sU("altitude"), altitudes[0].size());
	sci::NcDimension altitudeIndexDimension2d(sU("index_range"), altitudes[0].size());
	if (amfVersion == AmfVersion::v1_1_0)
		nonTimeDimensions2d.push_back(&altitudeIndexDimension2d);
	else
		nonTimeDimensions2d.push_back(&altitudeDimension2d);
	sci::NcDimension indexBinDimension(sU("index_bin"), spectralReflectivity[0][0].size());
	nonTimeDimensions2d.push_back(&indexBinDimension);


	sci::string extraComment = sci::string(sU("In case of low signal to noise ratio negative values can occur. Although they have no physical meaning they are retained in order to avoid statistical biases.\nThe reference unit for dBZ is m-3 mm-6"));
	dataInfo2d.processingOptions.comment = dataInfo2d.processingOptions.comment + extraComment;

	OutputAmfNcFile file2d(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo2d,
		projectInfo, platform, sU("micro rain radar size dependent profile"), times, nonTimeDimensions2d);

	std::vector<std::pair<sci::string, CellMethod>>cellMethods2d{ {sU("time"), CellMethod::mean} };
	std::vector<sci::string> coordinates2d{ sU("latitude"), sU("longitude") };
	std::vector<sci::NcDimension*> dimensions2d{ &file1d.getTimeDimension(), nonTimeDimensions2d[0], nonTimeDimensions2d[1] };

	AmfNcVariable<Decibel<perMetreF>, std::vector<std::vector<std::vector<perMetreF>>>> spectralReflectivityVariable(sU("spectral_reflectivity"), file2d, dimensions2d, sU("Spectral Reflectivity"), sU(""), spectralReflectivity, true, coordinates2d, cellMethods2d, false, false);
	AmfNcVariable<millimetreF, std::vector<std::vector<std::vector<millimetreF>>>> dropDiameterVariable(sU("rain_drop_diameter"), file2d, dimensions2d, sU("Rain Drop Diameter"), sU(""), dropDiameters, true, coordinates2d, cellMethods2d);
	AmfNcVariable<perMetreCubedPerMillimetreF, std::vector<std::vector<std::vector<perMetreCubedPerMillimetreF>>>> numberDistributionVariable(sU("drop_size_distribution"), file2d, dimensions2d, sU("Rain Size Distribution"), sU(""), sizeDistributions, true, coordinates2d, cellMethods2d);
	AmfNcFlagVariable spectralFlagVariable(sU("qc_flag"), microRainRadarFlags, file2d, dimensions2d);

	if (amfVersion == AmfVersion::v1_1_0)
	{
		std::vector<sci::NcDimension*> altitudeDimensions2d{ &file1d.getTimeDimension(), nonTimeDimensions2d[0] };
		AmfNcVariable<metreF, decltype(altitudes)> altitudeVariable(sU("altitude"), file2d, altitudeDimensions2d, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates2d, std::vector<std::pair<sci::string, CellMethod>>(0));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file2d);
		file2d.writeTimeAndLocationData(platform);
		file2d.write(altitudeVariable);
	}
	else
	{
		AmfNcVariable<metreF, std::vector<metreF>> altitudeVariable(sU("altitude"), file2d, file2d.getTimeDimension(), sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes[0], true, coordinates2d, std::vector<std::pair<sci::string, CellMethod>>(0));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file2d);
		file2d.writeTimeAndLocationData(platform);
		file2d.write(altitudeVariable);
	}

	file2d.write(spectralReflectivityVariable, spectralReflectivity);
	file2d.write(dropDiameterVariable, dropDiameters);
	file2d.write(numberDistributionVariable, sizeDistributions);
	file2d.write(spectralFlagVariable, spectralFlags);*/
}

std::vector<std::vector<sci::string>> MicrowaveRadiometerProcessor::groupInputFilesbyOutputFiles(const std::vector<sci::string>& newFiles, const std::vector<sci::string>& allFiles) const
{
	//The MRR has daily files, so just return all the new files
	std::vector<std::vector<sci::string>> result(newFiles.size());
	for (size_t i = 0; i < newFiles.size(); ++i)
	{
		result[i] = { newFiles[i] };
	}
	return result;
}

bool MicrowaveRadiometerProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	sci::UtcTime fileStart(std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 15, 4)).c_str()),
		std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 11, 2)).c_str()),
		std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 6, 2)).c_str()),
		0, 0, 0.0);
	sci::UtcTime fileEnd = fileStart + second(60.0 * 60.0 * 24.0 - 0.0000001);
	return (fileStart<endTime && fileEnd > startTime);
}