#include"MicroRainRadar.h"
#include<svector/sreadwrite.h>
#include"ProgressReporter.h"

const uint8_t microRainRadarUnusedFlag = 0;
const uint8_t microRainRadarGoodDataFlag = 1;
const uint8_t microRainRadarQualityBelowOnehundredPercentFlag = 2;
const uint8_t microRainRadarQualityBelowFiftyPercentFlag = 3;
const uint8_t microRainRadarBelowNoiseFloorFlag = 4;
const uint8_t microRainRadarSpeedsOutsideExpectedLimitsFlag = 5;
const uint8_t microRainRadarDiameterNotDerivedFlag = 6;
const uint8_t microRainRadarNegativeDiametersFlag = 7;
const uint8_t microRainRadarNegativeConcentrationFlag = 8;
const uint8_t microRainRadarUnrealisticConcentrationFlag = 9;
const uint8_t microRainRadarNegativeRainfallRateFlag = 10;

const std::vector<std::pair<uint8_t, sci::string>> microRainRadarFlags
{
{ microRainRadarUnusedFlag, sU("not_used") },
{microRainRadarGoodDataFlag, sU("good_data")},
{microRainRadarQualityBelowOnehundredPercentFlag, sU("Fewer than 100% of spectra in averaging period were valid") },
{microRainRadarQualityBelowFiftyPercentFlag, sU("Fewer than 50% of spectra in averaging period were valid") },
{microRainRadarBelowNoiseFloorFlag, sU("Signal Below Noise Floor - logarithmic units invalid") },
{microRainRadarSpeedsOutsideExpectedLimitsFlag, sU("Droplet speed outside expected limits (0-20 m s-1)") },
{microRainRadarDiameterNotDerivedFlag, sU("Diameter not derived by data processing software")},
{microRainRadarNegativeDiametersFlag, sU("Negative diameters derived by retrieval")},
{microRainRadarNegativeConcentrationFlag, sU("Negative concentration derived by retrieval")},
{microRainRadarUnrealisticConcentrationFlag, sU("Unrealistic concentration derived by retrieval (>1e6 m-3 mm-1)")},
{microRainRadarNegativeRainfallRateFlag, sU("Negative rainfall rate derived by retrieval")}
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
	m_heightResolution = metreF((metreF::valueType)std::atof(headerChunks[6].c_str()));
	m_instrumentHeight = metreF((metreF::valueType)std::atof(headerChunks[8].c_str()));
	m_samplingRate = hertzF((hertzF::valueType)std::atof(headerChunks[10].c_str()));
	m_serviceVersionNumber = sci::fromCodepage(headerChunks[12]);
	m_firmwareVersionNumber = sci::fromCodepage(headerChunks[14]);
	m_serialNumber = sci::fromCodepage(headerChunks[16]);
	m_calibrationConstant = unitlessF((unitlessF::valueType)std::atof(headerChunks[18].c_str()));
	m_validFraction = percentF((percentF::valueType)std::atof(headerChunks[20].c_str()));
	if (headerChunks[22] == "AVE")
		m_profileType = MRRPT_averaged;
	else if (headerChunks[22] == "PRO")
		m_profileType = MRRPT_processed;
	else if (headerChunks[22] == "RAW")
		m_profileType = MRRPT_raw;
	else
		throw(sci::err(sci::SERR_USER, 0, "Found and invalid micro rain radar profile type."));

	//Read each chiunk of data in turn
	m_ranges = readDataLine<metreF>(stream, "H  ");
	m_transferFunction = readDataLine<unitlessF>(stream, "TF ");
	m_spectralReflectivities.reshape({ 64, 31 });
	for (size_t i = 0; i < m_spectralReflectivities.shape()[0]; ++i)
	{
		std::ostringstream prefix;
		prefix << "F" << (i < 10 ? "0" : "") << i;
		auto line = readAndUndbDataLine<perMetreF>(stream, prefix.str());
		for (size_t j = 0; j < 31; ++j)
			m_spectralReflectivities[i][j] = line[j];
	}
	m_dropDiameters.reshape({ 64, 31 });
	for (size_t i = 0; i < m_dropDiameters.shape()[0]; ++i)
	{
		std::ostringstream prefix;
		prefix << "D" << (i < 10 ? "0" : "") << i;
		auto line = readDataLine<millimetreF>(stream, prefix.str());
		for (size_t j = 0; j < 31; ++j)
			m_dropDiameters[i][j] = line[j];
	}
	m_numberDistribution.reshape({ 64, 31 });
	for (size_t i = 0; i < m_numberDistribution.shape()[0]; ++i)
	{
		std::ostringstream prefix;
		prefix << "N" << (i < 10 ? "0" : "") << i;
		auto line = readDataLine<perMetreCubedPerMillimetreF>(stream, prefix.str());
		for (size_t j = 0; j < 31; ++j)
			m_numberDistribution[i][j] = line[j];
	}
	m_pathIntegratedAttenuation = readAndUndbDataLine<unitlessF>(stream, "PIA");
	m_reflectivity = readAndUndbDataLine<reflectivityF>(stream, "z  ");
	m_reflectivityAttenuationCorrected = readAndUndbDataLine<reflectivityF>(stream, "Z  ");
	m_rainRate = readDataLine<millimetrePerHourF>(stream, "RR ");
	m_liquidWaterContent = readDataLine<gramPerMetreCubedF>(stream, "LWC");
	m_fallVelocity = readDataLine<metrePerSecondF>(stream, "W  ");

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
	sci::GridData<sci::UtcTime, 1> times(m_profiles.size());
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

	size_t maxRanges = 0;
	for (size_t i = 0; i < m_profiles.size(); ++i)
		maxRanges = std::max(maxRanges, m_profiles[i].getRanges().size());


	//pull all the 1d variables out

	//declare the arrays that will hold the data
	sci::GridData<metreF,2> altitudes({m_profiles.size(), maxRanges}, std::numeric_limits<metreF>::quiet_NaN());
	sci::GridData<millimetrePerHourF,2> rainfallRates({ m_profiles.size(), maxRanges }, std::numeric_limits<millimetrePerHourF>::quiet_NaN());
	sci::GridData<gramPerMetreCubedF, 2> rainLiquidWaterContent({ m_profiles.size(), maxRanges }, std::numeric_limits<gramPerMetreCubedF>::quiet_NaN());
	sci::GridData<metrePerSecondF, 2> rainfallVelocity({ m_profiles.size(), maxRanges }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
	sci::GridData<reflectivityF, 2> radarReflectivity({ m_profiles.size(), maxRanges }, std::numeric_limits<reflectivityF>::quiet_NaN());
	sci::GridData<reflectivityF, 2> radarReflectivityAttenuated({ m_profiles.size(), maxRanges }, std::numeric_limits<reflectivityF>::quiet_NaN());
	sci::GridData<unitlessF, 2> pathIntegratedAttenuation({ m_profiles.size(), maxRanges }, std::numeric_limits<unitlessF>::quiet_NaN());

	//assign amf version based on if we are moving or not. Note that we swap from v2 to v1 below if the altitudes
	//are not consistent
	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;

	degreeF longitude;
	degreeF latitude;
	metreF instrumentAltitude;
	sci::GridData<metreF, 1> firstAltitudes;
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		platform.getLocation(m_profiles[i].getTime() - m_profiles[i].getAveragingTime(), m_profiles[i].getTime(), latitude, longitude, instrumentAltitude);
		unitlessF sinElevation;
		unitlessF sinAzimuth;
		unitlessF sinRoll;
		unitlessF cosElevation;
		unitlessF cosAzimuth;
		unitlessF cosRoll;
		platform.getInstrumentTrigAttitudesForDirectionCorrection(m_profiles[i].getTime() - m_profiles[i].getAveragingTime(), m_profiles[i].getTime(), sinElevation, sinAzimuth, sinRoll, cosElevation,cosAzimuth, cosRoll);
		for(size_t j=0; j<m_profiles[i].getRanges().size(); ++j)
			altitudes[i][j]=m_profiles[i].getRanges()[j] * cosElevation + instrumentAltitude;
		if (i == 0)
			firstAltitudes = altitudes[0];
		else if (amfVersion == AmfVersion::v2_0_0 && !sci::allTrue(altitudes[i] == firstAltitudes))
			amfVersion = AmfVersion::v1_1_0;
	}

	for (size_t i = 0; i < m_profiles.size(); ++i)
		for(size_t j=0; j< m_profiles[i].getRainRate().size(); ++j)
			rainfallRates[i][j] = m_profiles[i].getRainRate()[j];
	for (size_t i = 0; i < m_profiles.size(); ++i)
		for (size_t j = 0; j < m_profiles[i].getLiquidWaterContent().size(); ++j)
			rainLiquidWaterContent[i][j] = m_profiles[i].getLiquidWaterContent()[j];
	for (size_t i = 0; i < m_profiles.size(); ++i)
		for (size_t j = 0; j < m_profiles[i].getFallVeocity().size(); ++j)
			rainfallVelocity[i][j] = m_profiles[i].getFallVeocity()[j];
	for (size_t i = 0; i < m_profiles.size(); ++i)
		for (size_t j = 0; j < m_profiles[i].getReflectivityAttenuationCorrected().size(); ++j)
			radarReflectivity[i][j] = m_profiles[i].getReflectivityAttenuationCorrected()[j];
	for (size_t i = 0; i < m_profiles.size(); ++i)
		for (size_t j = 0; j < m_profiles[i].getReflectivity().size(); ++j)
			radarReflectivityAttenuated[i][j] = m_profiles[i].getReflectivity()[j];
	for (size_t i = 0; i < m_profiles.size(); ++i)
		for (size_t j = 0; j < m_profiles[i].getPathIntegratedAttenuation().size(); ++j)
			pathIntegratedAttenuation[i][j] = m_profiles[i].getPathIntegratedAttenuation()[j];

	//generate the flag variable.
	sci::GridData<unsigned char, 2> flags({ m_profiles.size(), maxRanges }, microRainRadarGoodDataFlag);
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		if (m_profiles[i].getValidFraction() < percentF(50))
			for(size_t j=0; j<maxRanges; ++j)
				flags[i][j] = microRainRadarQualityBelowFiftyPercentFlag;
		else if (m_profiles[i].getValidFraction() < percentF(100))
			for (size_t j = 0; j < maxRanges; ++j)
				flags[i][j] = microRainRadarQualityBelowOnehundredPercentFlag;

		for (size_t j = 0; j < maxRanges; ++j)
		{
			bool missing = (rainfallRates[i][j] != rainfallRates[i][j]) ||
				(rainLiquidWaterContent[i][j] != rainLiquidWaterContent[i][j]) ||
				(rainfallVelocity[i][j] != rainfallVelocity[i][j]) ||
				(radarReflectivity[i][j] != radarReflectivity[i][j]) ||
				(radarReflectivityAttenuated[i][j] != radarReflectivityAttenuated[i][j]) ||
				(pathIntegratedAttenuation[i][j] != pathIntegratedAttenuation[i][j]);


			if (missing)
				flags[i][j] = microRainRadarBelowNoiseFloorFlag;
			else if (rainfallRates[i][j] < millimetrePerHourF(0.0))
				flags[i][j] = microRainRadarNegativeRainfallRateFlag;
			else if (rainfallVelocity[i][j] < metrePerSecondF(0.0) || rainfallVelocity[i][j] > metrePerSecondF(20.0))
				flags[i][j] = microRainRadarSpeedsOutsideExpectedLimitsFlag;
		}
	}

	//Output the one D parameters

	std::vector<sci::NcDimension*> nonTimeDimensions1D;
	sci::NcDimension altitudeDimension1d(sU("altitude"), altitudes[0].size());
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
	AmfNcVariable<millimetrePerHourF> rainfallRatesVariable(sU("rainfall_rate"), file1d, dimensions1d, sU("Rainfall Rate"), sU("rainfall_rate"), rainfallRates, true, coordinates1d, cellMethods1d, flags, sU(""));
	AmfNcVariable<gramPerMetreCubedF> rainLiquidWaterContentVariable(sU("rain_liquid_water_content"), file1d, dimensions1d, sU("Rain Liquid Water Content"), sU(""), rainLiquidWaterContent, true, coordinates1d, cellMethods1d, flags, sU(""));
	AmfNcVariable<metrePerSecondF> rainfallVelocityVariable(sU("rainfall_velocity"), file1d, dimensions1d, sU("Rainfall Velocity"), sU(""), rainfallVelocity, true, coordinates1d, cellMethods1d, flags, sU(""));
	AmfNcDbVariableFromLinearData<reflectivityF> radarReflectivityVariable(sU("radar_reflectivity"), file1d, dimensions1d, sU("Radar Reflectivity (Z)"), sU(""), radarReflectivity, true, coordinates1d, cellMethods1d, true, false, flags, sU(""));
	AmfNcDbVariableFromLinearData<reflectivityF> radarReflectivityAttenuatedVariable(sU("attenuated_radar_reflectivity"), file1d, dimensions1d, sU("Attenuated Radar Reflectivity (z)"), sU(""), radarReflectivityAttenuated, true, coordinates1d, cellMethods1d, true, false, flags, sU(""));
	AmfNcDbVariableFromLinearData<unitlessF> pathIntegratedAttenuationVariable(sU("path_integrated_attenuation"), file1d, dimensions1d, sU("Path Integrated Attenuation"), sU(""), pathIntegratedAttenuation, true, coordinates1d, cellMethods1d, false, false, flags, sU(""));
	AmfNcFlagVariable flagVariable(sU("qc_flag"), microRainRadarFlags, file1d, dimensions1d);
	
	if (amfVersion == AmfVersion::v1_1_0)
	{
		AmfNcVariable<metreF> altitudeVariable(sU("altitude"), file1d, dimensions1d, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates1d, std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1), sU(""));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file1d);
		file1d.writeTimeAndLocationData(platform);
		file1d.write(altitudeVariable, altitudes);
	}
	else
	{
		AmfNcVariable<metreF> altitudeVariable(sU("altitude"), file1d, *nonTimeDimensions1D[0], sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes[0], true, coordinates1d, std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1), sU(""));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file1d);
		file1d.writeTimeAndLocationData(platform);
		file1d.write(altitudeVariable, altitudes[0]);
	}

	file1d.write(rainfallRatesVariable, rainfallRates);
	file1d.write(rainLiquidWaterContentVariable, rainLiquidWaterContent);
	file1d.write(rainfallVelocityVariable, rainfallVelocity);
	file1d.write(radarReflectivityVariable, radarReflectivity);
	file1d.write(radarReflectivityAttenuatedVariable, radarReflectivityAttenuated);
	file1d.write(pathIntegratedAttenuationVariable, pathIntegratedAttenuation);
	file1d.write(flagVariable, flags);

	//pull out the 2 d variables. We must transpose them so that the range dimension is first
	size_t maxSizes = 0;
	for (size_t i = 0; i < m_profiles.size(); ++i)
		maxSizes = std::max(maxSizes, m_profiles[i].getSpectralReflectivities().shape()[0]);
	sci::GridData<perMetreF, 3> spectralReflectivity({ m_profiles.size(), maxRanges, maxSizes }, std::numeric_limits<perMetreF>::quiet_NaN());
	sci::GridData<millimetreF, 3> dropDiameters({ m_profiles.size(), maxRanges, maxSizes }, std::numeric_limits<millimetreF>::quiet_NaN());
	sci::GridData<perMetreCubedPerMillimetreF, 3> sizeDistributions({ m_profiles.size(), maxRanges, maxSizes }, std::numeric_limits<perMetreCubedPerMillimetreF>::quiet_NaN());

	for (size_t i = 0; i < m_profiles.size(); ++i)
		for(size_t j=0; j<m_profiles[i].getSpectralReflectivities().shape()[0]; ++j)
			for(size_t k=0; k< m_profiles[i].getSpectralReflectivities().shape()[1]; ++k)
				spectralReflectivity[i][k][j] = m_profiles[i].getSpectralReflectivities()[j][k]; //note the transpose
	for (size_t i = 0; i < m_profiles.size(); ++i)
		for (size_t j = 0; j < m_profiles[i].getDropDiameters().shape()[0]; ++j)
			for (size_t k = 0; k < m_profiles[i].getDropDiameters().shape()[1]; ++k)
				dropDiameters[i][k][j] = m_profiles[i].getDropDiameters()[j][k]; //note the transpose
	for (size_t i = 0; i < m_profiles.size(); ++i)
		for (size_t j = 0; j < m_profiles[i].getNumberDistribution().shape()[0]; ++j)
			for (size_t k = 0; k < m_profiles[i].getNumberDistribution().shape()[1]; ++k)
				sizeDistributions[i][k][j] = m_profiles[i].getNumberDistribution()[j][k]; //note the transpose


	//generate the flag variables.
	sci::GridData<unsigned char, 3> spectralFlags({ m_profiles.size(), maxRanges, maxSizes });
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		for (size_t j = 0; j < maxRanges; ++j)
		{
			for (size_t k = 0; k < maxSizes; ++k)
			{
				if (dropDiameters[i][j][k] < millimetreF(0.0))
					spectralFlags[i][j][k] = microRainRadarNegativeDiametersFlag;
				if (dropDiameters[i][j][k] != dropDiameters[i][j][k])
					spectralFlags[i][j][k] = microRainRadarDiameterNotDerivedFlag;
				if (sizeDistributions[i][j][k] < perMetreCubedPerMillimetreF(0.0f))
					spectralFlags[i][j][k] = microRainRadarNegativeConcentrationFlag;
				if (sizeDistributions[i][j][k] > perMetreCubedPerMillimetreF(1.0e6f))
					spectralFlags[i][j][k] = microRainRadarUnrealisticConcentrationFlag;
				else
					spectralFlags[i][j][k] = flags[i][j];
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

	AmfNcDbVariableFromLinearData<perMetreF> spectralReflectivityVariable(sU("spectral_reflectivity"), file2d, dimensions2d, sU("Spectral Reflectivity"), sU(""), spectralReflectivity, true, coordinates2d, cellMethods2d, false, false, spectralFlags, sU(""));
	AmfNcVariable<millimetreF> dropDiameterVariable(sU("rain_drop_diameter"), file2d, dimensions2d, sU("Rain Drop Diameter"), sU(""), dropDiameters, true, coordinates2d, cellMethods2d, spectralFlags);
	AmfNcVariable<perMetreCubedPerMillimetreF> numberDistributionVariable(sU("drop_size_distribution"), file2d, dimensions2d, sU("Rain Size Distribution"), sU(""), sizeDistributions, true, coordinates2d, cellMethods2d, spectralFlags, sU(""));
	AmfNcFlagVariable spectralFlagVariable(sU("qc_flag"), microRainRadarFlags, file2d, dimensions2d);
	
	if (amfVersion == AmfVersion::v1_1_0)
	{
		std::vector<sci::NcDimension*> altitudeDimensions2d{ &file1d.getTimeDimension(), &altitudeIndexDimension2d };
		AmfNcVariable<metreF> altitudeVariable(sU("altitude"), file2d, altitudeDimensions2d, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates2d, std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1), sU(""));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file2d);
		file2d.writeTimeAndLocationData(platform);
		file2d.write(altitudeVariable, altitudes);
	}
	else
	{
		AmfNcVariable<metreF> altitudeVariable(sU("altitude"), file2d, altitudeDimension2d, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes[0], true, coordinates2d, std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1), sU(""));
		altitudeVariable.addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), file2d);
		file2d.writeTimeAndLocationData(platform);
		file2d.write(altitudeVariable, altitudes[0]);
	}

	file2d.write(spectralReflectivityVariable, spectralReflectivity);
	file2d.write(dropDiameterVariable, dropDiameters);
	file2d.write(numberDistributionVariable, sizeDistributions);
	file2d.write(spectralFlagVariable, spectralFlags);
}

std::vector<std::vector<sci::string>> MicroRainRadarProcessor::groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
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