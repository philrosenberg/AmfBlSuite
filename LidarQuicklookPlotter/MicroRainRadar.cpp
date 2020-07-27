#include"MicroRainRadar.h"
#include<svector/sreadwrite.h>
#include"ProgressReporter.h"

const uint8_t microRainRadarUnusedFlag = 0;
const uint8_t microRainRadarGoodDataFlag = 1;
const uint8_t microRainRadarBelowNoiseFloorFlag = 2;
const uint8_t microRainRadarQualityBelowOnehundredPercentFlag = 3;
const uint8_t microRainRadarQualityBelowFiftyPercentFlag = 4;
const uint8_t microRainRadarDiameterNotDerivedFlag = 5;
const uint8_t microRainRadarBelowNoiseFloorAndDiameterNotDerivedFlag = 6;

const std::vector<std::pair<uint8_t, sci::string>> microRainRadarFlags
{
{ microRainRadarUnusedFlag, sU("not_used") },
{microRainRadarGoodDataFlag, sU("good_data")},
{microRainRadarBelowNoiseFloorFlag, sU("Signal Below Noise Floor - logarithmic units invalid") },
{microRainRadarDiameterNotDerivedFlag, sU("Diameter not derived by data processing software")},
{microRainRadarQualityBelowOnehundredPercentFlag, sU("Fewer than 100% of spectra in averaging period were valid") },
{microRainRadarQualityBelowFiftyPercentFlag, sU("Fewer than 50% of spectra in averaging period were valid") },
{microRainRadarBelowNoiseFloorAndDiameterNotDerivedFlag, sU("Signal Below Noise Floor - logarithmic units invalid and diameter not derived by data processing software") }
};

MicroRainRadarProcessor::MicroRainRadarProcessor(const InstrumentInfo &instrumentInfo, const CalibrationInfo &calibrationInfo)
	: InstrumentProcessor(sU("[/\\\\]ProcessedData[/\\\\]......[/\\\\]....\\.pro"))
{
	m_hasData = false;
	m_instrumentInfo = instrumentInfo;
	m_calibrationInfo = calibrationInfo;
}

bool MicroRainRadarProfile::readProfile(std::istream &stream)
{
	std::string line;

	//read the header line
	std::getline(stream, line);
	if (line.length() == 0 && stream.eof())
		return false;
	sci::assertThrow(!stream.eof(), sci::err(sci::SERR_USER, 0, sU("Found an unexpected end of data.")));
	sci::assertThrow(!stream.bad(), sci::err(sci::SERR_USER, 0, sU("Bad read.")));

	//parse the header line
	std::vector<std::string> headerChunks = sci::splitstring(line, " ", true);
	sci::assertThrow(headerChunks.size() == 23 &&
		headerChunks[0] == "MRR" &&
		headerChunks[2] == "UTC" &&
		headerChunks[3] == "AVE" &&
		headerChunks[5] == "STP" &&
		headerChunks[7] == "ASL" &&
		headerChunks[9] == "SMP" &&
		headerChunks[11] == "SVS" &&
		headerChunks[13] == "DVS" &&
		headerChunks[15] == "DSN" &&
		headerChunks[17] == "CC" &&
		headerChunks[19] == "MDQ" &&
		headerChunks[21] == "TYP",
		sci::err(sci::SERR_USER, 0, sU("Incorrectly formatted header line.")));
	sci::assertThrow(headerChunks[1].length() == 12, sci::err(sci::SERR_USER, 0, sU("Incorrectly formatted time in the header")));
	m_time = sci::UtcTime(std::atoi(headerChunks[1].substr(0, 2).c_str()) + 2000,
		std::atoi(headerChunks[1].substr(2, 2).c_str()),
		std::atoi(headerChunks[1].substr(4, 2).c_str()),
		std::atoi(headerChunks[1].substr(6, 2).c_str()),
		std::atoi(headerChunks[1].substr(8, 2).c_str()),
		std::atoi(headerChunks[1].substr(10, 2).c_str()));
	m_averagingTime = second(std::atof(headerChunks[4].c_str()));
	m_heightResolution = metre((metre::valueType)std::atof(headerChunks[6].c_str()));
	m_instrumentHeight = metre((metre::valueType)std::atof(headerChunks[8].c_str()));
	m_samplingRate = hertz((hertz::valueType)std::atof(headerChunks[10].c_str()));
	m_serviceVersionNumber = sci::fromCodepage(headerChunks[12]);
	m_firmwareVersionNumber = sci::fromCodepage(headerChunks[14]);
	m_serialNumber = sci::fromCodepage(headerChunks[16]);
	m_calibrationConstant = unitless((unitless::valueType)std::atof(headerChunks[18].c_str()));
	m_validFraction = percent((percent::valueType)std::atof(headerChunks[20].c_str()));
	if (headerChunks[22] == "AVE")
		m_profileType = MRRPT_averaged;
	else if (headerChunks[22] == "PRO")
		m_profileType = MRRPT_processed;
	else if (headerChunks[22] == "RAW")
		m_profileType = MRRPT_raw;
	else
		throw(sci::err(sci::SERR_USER, 0, "Found and invalid micro rain radar profile type."));

	//Read each chiunk of data in turn
	m_ranges = readDataLine<metre>(stream, "H  ");
	m_transferFunction = readDataLine<unitless>(stream, "TF ");
	m_spectralReflectivities.resize(64);
	for (size_t i = 0; i < m_spectralReflectivities.size(); ++i)
	{
		std::ostringstream prefix;
		prefix << "F" << (i < 10 ? "0" : "") << i;
		m_spectralReflectivities[i] = readAndUndbDataLine<perMetre>(stream, prefix.str());
	}
	m_dropDiameters.resize(64);
	for (size_t i = 0; i < m_dropDiameters.size(); ++i)
	{
		std::ostringstream prefix;
		prefix << "D" << (i < 10 ? "0" : "") << i;
		m_dropDiameters[i] = readDataLine<millimetre>(stream, prefix.str());
	}
	m_numberDistribution.resize(64);
	for (size_t i = 0; i < m_numberDistribution.size(); ++i)
	{
		std::ostringstream prefix;
		prefix << "N" << (i < 10 ? "0" : "") << i;
		m_numberDistribution[i] = readDataLine<perMetreCubedPerMillimetre>(stream, prefix.str());
	}
	m_pathIntegratedAttenuation = readAndUndbDataLine<unitless>(stream, "PIA");
	m_reflectivity = readAndUndbDataLine<reflectivity>(stream, "z  ");
	m_reflectivityAttenuationCorrected = readAndUndbDataLine<reflectivity>(stream, "Z  ");
	m_rainRate = readDataLine<millimetrePerHour>(stream, "RR ");
	m_liquidWaterContent = readDataLine<gramPerMetreCubed>(stream, "LWC");
	m_fallVelocity = readDataLine<metrePerSecond>(stream, "W  ");

	return true;
}

void MicroRainRadarProcessor::readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter)
{

	size_t profilesInFirstFile;
	size_t profilesInSecondFile;
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		//after reading the first two files reserve space for the remaining files. This
		//can speed things up a lot
		if (i == 1)
			profilesInFirstFile = m_profiles.size();
		if (i == 2)
		{
			profilesInSecondFile = m_profiles.size() - profilesInFirstFile;
			m_profiles.reserve(inputFilenames.size()*std::max(profilesInFirstFile, profilesInSecondFile));
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

void MicroRainRadarProcessor::readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter, bool clear)
{
	if (clear)
	{
		m_hasData = false;
		m_profiles.clear();
	}
	std::ifstream fin;
	fin.open(sci::nativeUnicode(inputFilename), std::ios::in);
	sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open file ") + inputFilename + sU(".")));
	size_t interval = 100;
	try
	{
		while (true)
		{
			m_profiles.resize(m_profiles.size() + 1);
			if (!m_profiles.back().readProfile(fin))
			{
				//if readProfile returns false, rather than throwing an error then it just means we got to the end of the
				//file at the end of the previous point
				m_profiles.pop_back();
				return;
			}

			if (m_profiles.size() == interval)
			{
				progressReporter << sU("Read profile 1-") << interval;
				if (progressReporter.shouldStop())
					return;
			}
			else if (m_profiles.size() % interval == 0)
			{
				progressReporter << sU(", ") << m_profiles.size() - interval + 1 << sU("-") << m_profiles.size();
				if (progressReporter.shouldStop())
					return;
			}
			m_hasData = true;
			sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Read of file failed - it may be locked or inaccessible.")));
		}
		progressReporter << sU("\n\n");
	}
	catch (...)
	{
		//this means we had some error - maybe the last profile was corrupt or was incomplete
		//discard it and rethrow
		m_profiles.pop_back();
		throw;
	}
}

void MicroRainRadarProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	DataInfo dataInfo1d;
	dataInfo1d.averagingPeriod = std::numeric_limits<second>::quiet_NaN();
	dataInfo1d.samplingInterval = std::numeric_limits<second>::quiet_NaN();
	dataInfo1d.continuous = true;
	dataInfo1d.startTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	dataInfo1d.endTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	if (m_profiles.size() > 0)
	{
		dataInfo1d.startTime = m_profiles[0].getTime();
		dataInfo1d.endTime = m_profiles.back().getTime();
	}
	dataInfo1d.featureType = FeatureType::timeSeriesPoint;
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
		dataInfo1d.averagingPeriod = sci::median(averagingTimes);
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
		dataInfo1d.samplingInterval = sci::median(samplingIntervals);
	}

	DataInfo dataInfo2d = dataInfo1d;
	dataInfo2d.productName = sU("size concentration spectra");

	//pull all the 1d variables out
	std::vector<std::vector<metre>> altitudes(m_profiles.size());
	std::vector<std::vector<millimetrePerHour>> rainfallRates(m_profiles.size());
	std::vector<std::vector<gramPerMetreCubed>> rainLiquidWaterContent(m_profiles.size());
	std::vector<std::vector<metrePerSecond>> rainfallVelocity(m_profiles.size());
	std::vector<std::vector<reflectivity>> radarReflectivity(m_profiles.size());
	std::vector<std::vector<reflectivity>> radarReflectivityAttenuated(m_profiles.size());
	std::vector<std::vector<unitless>> pathIntegratedAttenuation(m_profiles.size());

	//assign amf version based on if we are moving or not. Note that we swap from v2 to v1 below if the altitudes
	//are not consistent
	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;

	degree longitude;
	degree latitude;
	metre instrumentAltitude;
	std::vector<metre> firstAltitudes;
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		platform.getLocation(m_profiles[i].getTime() - m_profiles[i].getAveragingTime(), m_profiles[i].getTime(), latitude, longitude, instrumentAltitude);
		unitless sinElevation;
		unitless sinAzimuth;
		unitless sinRoll;
		unitless cosElevation;
		unitless cosAzimuth;
		unitless cosRoll;
		platform.getInstrumentTrigAttitudesForDirectionCorrection(m_profiles[i].getTime() - m_profiles[i].getAveragingTime(), m_profiles[i].getTime(), sinElevation, sinAzimuth, sinRoll, cosElevation,cosAzimuth, cosRoll);
		sci::convert(altitudes[i], m_profiles[i].getRanges() *cosElevation + instrumentAltitude);
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
	std::vector<std::vector<unsigned char>> flags1d(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		unsigned char baseValue = microRainRadarGoodDataFlag;
		if (m_profiles[i].getValidFraction() < percent(50))
			baseValue = microRainRadarQualityBelowFiftyPercentFlag;
		else if (m_profiles[i].getValidFraction() < percent(100))
			baseValue = microRainRadarQualityBelowOnehundredPercentFlag;

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

		flags1d[i].resize(missing.size(), baseValue);
		sci::assign(flags1d[i], missing, microRainRadarBelowNoiseFloorFlag);
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
	AmfNcVariable<millimetrePerHour, std::vector<std::vector<millimetrePerHour>>> rainfallRatesVariable(sU("rainfall_rate"), file1d, dimensions1d, sU("Rainfall Rate"), sU("rainfall_rate"), rainfallRates, true, coordinates1d, cellMethods1d);
	AmfNcVariable<gramPerMetreCubed, std::vector<std::vector<gramPerMetreCubed>>> rainLiquidWaterContentVariable(sU("rain_liquid_water_content"), file1d, dimensions1d, sU("Rain Liquid Water Content"), sU(""), rainLiquidWaterContent, true, coordinates1d, cellMethods1d);
	AmfNcVariable<metrePerSecond, std::vector<std::vector<metrePerSecond>>> rainfallVelocityVariable(sU("rainfall_velocity"), file1d, dimensions1d, sU("Rainfall Velocity"), sU(""), rainfallVelocity, true, coordinates1d, cellMethods1d);
	AmfNcVariable<Decibel<reflectivity>, std::vector<std::vector<reflectivity>>> radarReflectivityVariable(sU("radar_reflectivity"), file1d, dimensions1d, sU("Radar Reflectivity (Z)"), sU(""), radarReflectivity, true, coordinates1d, cellMethods1d, true);
	AmfNcVariable<Decibel<reflectivity>, std::vector<std::vector<reflectivity>>> radarReflectivityAttenuatedVariable(sU("attenuated_radar_reflectivity"), file1d, dimensions1d, sU("Attenuated Radar Reflectivity (z)"), sU(""), radarReflectivityAttenuated, true, coordinates1d, cellMethods1d, true);
	AmfNcVariable<Decibel<unitless>, std::vector<std::vector<unitless>>> pathIntegratedAttenuationVariable(sU("path_integrated_attenuation"), file1d, dimensions1d, sU("Path Integrated Attenuation"), sU(""), pathIntegratedAttenuation, true, coordinates1d, cellMethods1d, false);
	AmfNcFlagVariable flagVariable1d(sU("qc_flag"), microRainRadarFlags, file1d, dimensions1d);
	
	if (amfVersion == AmfVersion::v1_1_0)
	{
		AmfNcVariable<metre, decltype(altitudes)> altitudeVariable(sU("altitude"), file1d, dimensions1d, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates1d, std::vector<std::pair<sci::string, CellMethod>>(0));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file1d);
		file1d.writeTimeAndLocationData(platform);
		file1d.write(altitudeVariable);
	}
	else
	{
		AmfNcVariable<metre, std::vector<metre>> altitudeVariable(sU("altitude"), file1d, file1d.getTimeDimension(), sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes[0], true, coordinates1d, std::vector<std::pair<sci::string, CellMethod>>(0));
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
	file1d.write(flagVariable1d, flags1d);

	//pull out the 2 d variables. We must transpose them so that the range dimension is first

	std::vector<std::vector<std::vector<perMetre>>> spectralReflectivity(m_profiles.size());
	std::vector<std::vector<std::vector<millimetre>>> dropDiameters(m_profiles.size());
	std::vector<std::vector<std::vector<perMetreCubedPerMillimetre>>> sizeDistributions(m_profiles.size());

	for (size_t i = 0; i < m_profiles.size(); ++i)
		spectralReflectivity[i] = sci::transpose(m_profiles[i].getSpectralReflectivities());
	for (size_t i = 0; i < m_profiles.size(); ++i)
		dropDiameters[i] = sci::transpose(m_profiles[i].getDropDiameters());
	for (size_t i = 0; i < m_profiles.size(); ++i)
		sizeDistributions[i] = sci::transpose(m_profiles[i].getNumberDistribution());


	//generate the flag variables.
	std::vector < std::vector<std::vector<unsigned char>>> spectralFlags(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		unsigned char baseValue = microRainRadarGoodDataFlag;
		if (m_profiles[i].getValidFraction() < percent(50))
			baseValue = microRainRadarQualityBelowFiftyPercentFlag;
		else if (m_profiles[i].getValidFraction() < percent(100))
			baseValue = microRainRadarQualityBelowOnehundredPercentFlag;

		spectralFlags[i].resize(spectralReflectivity[i].size());
		for (size_t j = 0; j < spectralReflectivity[i].size(); ++j)
		{
			std::vector<SBOOL> missingReflectivity(spectralReflectivity[i][j].size());
			std::vector<SBOOL> missingDiameters(spectralReflectivity[i][j].size());
			for (size_t k = 0; k < spectralReflectivity[i][j].size(); ++k)
			{
				missingReflectivity[k] = spectralReflectivity[i][j][k] != spectralReflectivity[i][j][k];
				missingDiameters[k] = dropDiameters[i][j][k] != dropDiameters[i][j][k];
			}
			spectralFlags[i][j].resize(missingReflectivity.size(), baseValue);
			sci::assign(spectralFlags[i][j], missingReflectivity, microRainRadarBelowNoiseFloorFlag);
			sci::assign(spectralFlags[i][j], missingDiameters, microRainRadarDiameterNotDerivedFlag);
			sci::assign(spectralFlags[i][j], missingReflectivity && missingDiameters, microRainRadarBelowNoiseFloorAndDiameterNotDerivedFlag);
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

	if (dataInfo2d.processingOptions.comment.length() == 0)
		dataInfo2d.processingOptions.comment = sU("In case of low signal to noise ratio negative values can occur. Although they have no physical meaning they are retained in order to avoid statistical biases.");
	else
		dataInfo2d.processingOptions.comment = dataInfo2d.processingOptions.comment + sU("\nIn case of low signal to noise ratio negative values can occur. Although they have no physical meaning they are retained in order to avoid statistical biases.");
	
	OutputAmfNcFile file2d(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo2d,
		projectInfo, platform, sU("micro rain radar size dependent profile"), times, nonTimeDimensions2d);

	std::vector<std::pair<sci::string, CellMethod>>cellMethods2d{ {sU("time"), CellMethod::mean} };
	std::vector<sci::string> coordinates2d{ sU("latitude"), sU("longitude") };
	std::vector<sci::NcDimension*> dimensions2d{ &file1d.getTimeDimension(), nonTimeDimensions2d[0], nonTimeDimensions2d[1] };

	AmfNcVariable<Decibel<perMetre>, std::vector<std::vector<std::vector<perMetre>>>> spectralReflectivityVariable(sU("spectral_reflectivity"), file2d, dimensions2d, sU("Spectral Reflectivity"), sU(""), spectralReflectivity, true, coordinates2d, cellMethods2d, false);
	AmfNcVariable<millimetre, std::vector<std::vector<std::vector<millimetre>>>> dropDiameterVariable(sU("rain_drop_diameter"), file2d, dimensions2d, sU("Rain Drop Diameter"), sU(""), dropDiameters, true, coordinates2d, cellMethods2d);
	AmfNcVariable<perMetreCubedPerMillimetre, std::vector<std::vector<std::vector<perMetreCubedPerMillimetre>>>> numberDistributionVariable(sU("drop_size_distribution"), file2d, dimensions2d, sU("Rain Size Distribution"), sU(""), sizeDistributions, true, coordinates2d, cellMethods2d);
	AmfNcFlagVariable spectralFlagVariable2d(sU("qc_flag"), microRainRadarFlags, file2d, dimensions2d);

	if (amfVersion == AmfVersion::v1_1_0)
	{
		std::vector<sci::NcDimension*> altitudeDimensions2d{ &file1d.getTimeDimension(), nonTimeDimensions2d[0] };
		AmfNcVariable<metre, decltype(altitudes)> altitudeVariable(sU("altitude"), file2d, altitudeDimensions2d, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates2d, std::vector<std::pair<sci::string, CellMethod>>(0));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file2d);
		file2d.writeTimeAndLocationData(platform);
		file2d.write(altitudeVariable);
	}
	else
	{
		AmfNcVariable<metre, std::vector<metre>> altitudeVariable(sU("altitude"), file2d, file2d.getTimeDimension(), sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes[0], true, coordinates2d, std::vector<std::pair<sci::string, CellMethod>>(0));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file2d);
		file2d.writeTimeAndLocationData(platform);
		file2d.write(altitudeVariable);
	}

	file2d.write(spectralReflectivityVariable, spectralReflectivity);
	file2d.write(dropDiameterVariable, dropDiameters);
	file2d.write(numberDistributionVariable, sizeDistributions);
	file2d.write(spectralFlagVariable2d, spectralFlags);
}

std::vector<std::vector<sci::string>> MicroRainRadarProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	//The MRR has daily files, so just return all the new files
	std::vector<std::vector<sci::string>> result(newFiles.size());
	for (size_t i = 0; i < newFiles.size(); ++i)
	{
		result[i] = { newFiles[i] };
	}
	return result;
}

bool MicroRainRadarProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	sci::UtcTime fileStart(std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 15, 4)).c_str()),
		std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 11, 2)).c_str()),
		std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 6, 2)).c_str()),
		0, 0, 0.0);
	sci::UtcTime fileEnd = fileStart + second(60.0 * 60.0 * 24.0 - 0.0000001);
	return (fileStart<endTime && fileEnd > startTime);
}