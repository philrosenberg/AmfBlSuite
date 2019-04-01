#include"MicroRainRadar.h"
#include<svector/sreadwrite.h>
#include"ProgressReporter.h"

const uint8_t microRainRadarUnusedFlag = 0;
const uint8_t microRainRadarGoodDataFlag = 1;
const uint8_t microRainRadarBelowNoiseFloorFlag = 2;
const uint8_t microRainRadarQualityBelowOnehundredPercentFlag = 3;
const uint8_t microRainRadarQualityBelowFiftyPercentFlag = 4;
const uint8_t microRainRadarDiameterNotDerivedFlag = 5;

const std::vector<std::pair<uint8_t, sci::string>> microRainRadarFlags
{
{ microRainRadarUnusedFlag, sU("not_used") },
{microRainRadarGoodDataFlag, sU("good_data")},
{microRainRadarBelowNoiseFloorFlag, sU("Signal Below Noise Floor - logarithmic units invalid") },
{microRainRadarDiameterNotDerivedFlag, sU("Diameter not derived by data processing software")},
{microRainRadarQualityBelowOnehundredPercentFlag, sU("Fewer than 100% of spectra in averaging period were valid") },
{microRainRadarQualityBelowFiftyPercentFlag, sU("Fewer than 50% of spectra in averaging period were valid") },
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
	sci::assertThrow(headerChunks.size()==23 &&
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
	m_heightResolution = metre(std::atof(headerChunks[6].c_str()));
	m_instrumentHeight = metre(std::atof(headerChunks[8].c_str()));
	m_samplingRate = hertz(std::atof(headerChunks[10].c_str()));
	m_serviceVersionNumber = sci::fromCodepage(headerChunks[12]);
	m_firmwareVersionNumber = sci::fromCodepage(headerChunks[14]);
	m_serialNumber = sci::fromCodepage(headerChunks[16]);
	m_calibrationConstant = unitless(std::atof(headerChunks[18].c_str()));
	m_validFraction = percent(std::atof(headerChunks[20].c_str()));
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
			m_profiles.reserve(inputFilenames.size()*std::max(profilesInFirstFile,profilesInSecondFile));
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
			m_hasData = true;
			sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Read of file failed - it may be locked or inaccessible.")));
		}
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
	dataInfo1d.averagingPeriod = second(OutputAmfNcFile::getFillValue());
	dataInfo1d.samplingInterval = second(OutputAmfNcFile::getFillValue());
	dataInfo1d.continuous = true;
	dataInfo1d.startTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	dataInfo1d.endTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	if (m_profiles.size() > 0)
	{
		dataInfo1d.startTime = m_profiles[0].getTime();
		dataInfo1d.endTime = m_profiles.back().getTime();
	}
	dataInfo1d.featureType = ft_timeSeriesPoint;
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
		std::vector <second> sortedAveragingTimes = averagingTimes;
		std::sort(sortedAveragingTimes.begin(), sortedAveragingTimes.end());
		dataInfo1d.averagingPeriod = sortedAveragingTimes[sortedAveragingTimes.size() / 2];
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
			samplingIntervals[i] = times[i] - times[i-1];
		}
		std::vector < sci::TimeInterval> sortedSamplingIntervals = samplingIntervals;
		std::sort(sortedSamplingIntervals.begin(), sortedSamplingIntervals.end());
		dataInfo1d.samplingInterval = second(sortedSamplingIntervals[sortedSamplingIntervals.size() / 2]);
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

	degree longitude;
	degree latitude;
	metre instrumentAltitude;
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		platform.getLocation(m_profiles[i].getTime(), m_profiles[i].getTime() + m_profiles[i].getAveragingTime(), latitude, longitude, instrumentAltitude);
		altitudes[i] = m_profiles[i].getRanges() + instrumentAltitude;
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

	//replace nans with fill value
	sci::replacenans(rainfallRates, millimetrePerHour(OutputAmfNcFile::getFillValue()));
	sci::replacenans(rainLiquidWaterContent, gramPerMetreCubed(OutputAmfNcFile::getFillValue()));
	sci::replacenans(rainfallVelocity, metrePerSecond(OutputAmfNcFile::getFillValue()));
	sci::replacenans(radarReflectivity, reflectivity(OutputAmfNcFile::getFillValue()));
	sci::replacenans(radarReflectivityAttenuated, reflectivity(OutputAmfNcFile::getFillValue()));
	sci::replacenans(pathIntegratedAttenuation, unitless(OutputAmfNcFile::getFillValue()));

	//Output the one D parameters

	std::vector<sci::NcDimension*> nonTimeDimensions1D;
	sci::NcDimension indexDimension1d(sU("index"), altitudes[0].size());
	nonTimeDimensions1D.push_back(&indexDimension1d);

	OutputAmfNcFile file1d(directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo1d,
		projectInfo, platform, times, nonTimeDimensions1D);

	AmfNcVariable<metre> altitudesVariable(sU("altitude"), file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), &indexDimension1d }, sU("Geometric height above geoid (WGS84)"), sU("altitude"), sci::min<metre>(altitudes), sci::max<metre>(altitudes));
	AmfNcVariable<millimetrePerHour> rainfallRatesVariable(sU("rainfall_rate"), file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), &indexDimension1d }, sU("Rainfall Rate"), sU(""), sci::min<millimetrePerHour>(rainfallRates), sci::max<millimetrePerHour>(rainfallRates));
	AmfNcVariable<gramPerMetreCubed> rainLiquidWaterContentVariable(sU("rain_liquid_water_content"), file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), &indexDimension1d }, sU("Rain Liquid Water Content"), sU(""), sci::min<gramPerMetreCubed>(rainLiquidWaterContent), sci::max<gramPerMetreCubed>(rainLiquidWaterContent));
	AmfNcVariable<metrePerSecond> rainfallVelocityVariable(sU("rainfall_velocity"), file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), &indexDimension1d }, sU("Rainfall Velocity"), sU(""), sci::min<metrePerSecond>(rainfallVelocity), sci::max<metrePerSecond>(rainfallVelocity));
	AmfNcVariable<Decibel<reflectivity>> radarReflectivityVariable(sU("radar_reflectivity"), file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), &indexDimension1d }, sU("Radar Reflectivity (Z)"), sU(""), sci::min<reflectivity>(radarReflectivity), sci::max<reflectivity>(radarReflectivity));
	AmfNcVariable<Decibel<reflectivity>> radarReflectivityAttenuatedVariable(sU("attenuated_radar_reflectivity"), file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), &indexDimension1d }, sU("Attenuated Radar Reflectivity (z)"), sU(""), sci::min<reflectivity>(radarReflectivityAttenuated), sci::max<reflectivity>(radarReflectivity));
	AmfNcVariable<Decibel<unitless>> pathIntegratedAttenuationVariable(sU("path_integrated_attenuation"), file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), &indexDimension1d }, sU("Path Integrated Attenuation"), sU(""), sci::min<unitless>(pathIntegratedAttenuation), sci::max<unitless>(pathIntegratedAttenuation));
	AmfNcFlagVariable flagVariable1d(sU("qc_flag"), microRainRadarFlags, file1d, std::vector<sci::NcDimension*>{ &file1d.getTimeDimension(), &indexDimension1d });

	file1d.writeTimeAndLocationData(platform);

	file1d.write(altitudesVariable, altitudes);
	file1d.write(rainfallRatesVariable, rainfallRates);
	file1d.write(rainLiquidWaterContentVariable, rainLiquidWaterContent);
	file1d.write(rainfallVelocityVariable, rainfallVelocity);
	file1d.write(radarReflectivityVariable, radarReflectivity);
	file1d.write(radarReflectivityAttenuatedVariable, radarReflectivityAttenuated);
	file1d.write(pathIntegratedAttenuationVariable, pathIntegratedAttenuation);
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
	std::vector < std::vector<std::vector<unsigned char>>> spectralReflectivityFlags(m_profiles.size());
	std::vector < std::vector<std::vector<unsigned char>>> dropDiameterFlags(m_profiles.size());
	std::vector < std::vector<std::vector<unsigned char>>> sizeDistributionFlags(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		unsigned char baseValue = microRainRadarGoodDataFlag;
		if (m_profiles[i].getValidFraction() < percent(50))
			baseValue = microRainRadarQualityBelowFiftyPercentFlag;
		else if (m_profiles[i].getValidFraction() < percent(100))
			baseValue = microRainRadarQualityBelowOnehundredPercentFlag;

		for (size_t j = 0; j < spectralReflectivity[i].size(); ++j)
		{
			std::vector<SBOOL> missing(spectralReflectivity[i][j].size());
			for (size_t k = 0; k < spectralReflectivity[i][j].size(); ++k)
			{
				missing[k] = spectralReflectivity[i][j][k] != spectralReflectivity[i][j][k];
			}
			spectralReflectivityFlags[i][j].resize(missing.size(), baseValue);
			sci::assign(spectralReflectivityFlags[i][j], missing, microRainRadarBelowNoiseFloorFlag);
		}


		for (size_t j = 0; j < dropDiameters[i].size(); ++j)
		{
			std::vector<SBOOL> missing(dropDiameters[i][j].size());
			for (size_t k = 0; k < dropDiameters[i][j].size(); ++k)
			{
				missing[k] = dropDiameters[i][j][k] != dropDiameters[i][j][k];
			}
			dropDiameterFlags[i][j].resize(missing.size(), baseValue);
			sci::assign(dropDiameterFlags[i][j], missing, microRainRadarDiameterNotDerivedFlag);
		}

		for (size_t j = 0; j < sizeDistributions[i].size(); ++j)
		{
			std::vector<SBOOL> missing(sizeDistributions[i][j].size());
			for (size_t k = 0; k < sizeDistributions[i][j].size(); ++k)
			{
				missing[k] = sizeDistributions[i][j][k] != sizeDistributions[i][j][k];
			}
			sizeDistributionFlags[i][j].resize(missing.size(), baseValue);
			sci::assign(sizeDistributionFlags[i][j], missing, microRainRadarDiameterNotDerivedFlag);
		}
	}

	sci::replacenans(spectralReflectivity, perMetre(OutputAmfNcFile::getFillValue()));
	sci::replacenans(dropDiameters, millimetre(OutputAmfNcFile::getFillValue()));
	sci::replacenans(sizeDistributions, perMetreCubedPerMillimetre(OutputAmfNcFile::getFillValue()));


	//create and write the file
	std::vector<sci::NcDimension*> nonTimeDimensions2d;
	sci::NcDimension indexRangeDimension(sU("index_range"), altitudes[0].size());
	sci::NcDimension indexBinDimension(sU("index_bin"), m_profiles[0].getRanges().size());
	nonTimeDimensions2d.push_back(&indexRangeDimension);
	nonTimeDimensions2d.push_back(&indexBinDimension);

	OutputAmfNcFile file2d(directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo2d,
		projectInfo, platform, times, nonTimeDimensions2d);

	AmfNcVariable<metre> altitudesVariable2d(sU("altitude"), file2d, std::vector<sci::NcDimension*>{ &file2d.getTimeDimension(), &indexRangeDimension }, sU("Geometric height above geoid (WGS84)"), sU("altitude"), sci::min<metre>(altitudes), sci::max<metre>(altitudes));
	AmfNcVariable<Decibel<perMetre>> spectralReflectivityVariable(sU("spectral_reflectivity"), file2d, std::vector<sci::NcDimension*>{ &file2d.getTimeDimension(), &indexRangeDimension, &indexBinDimension }, sU("Spectral Reflectivity"), sU(""), sci::min<perMetre>(spectralReflectivity), sci::max<perMetre>(spectralReflectivity));
	AmfNcVariable<millimetre> dropDiameterVariable(sU("rain_drop_diameter"), file2d, std::vector<sci::NcDimension*>{ &file2d.getTimeDimension(), &indexRangeDimension, &indexBinDimension }, sU("Rain Drop Diameter"), sU(""), sci::min<millimetre>(dropDiameters), sci::max<millimetre>(dropDiameters));
	AmfNcVariable<perMetreCubedPerMillimetre> numberDistributionVariable(sU("drop_size_distribution"), file2d, std::vector<sci::NcDimension*>{ &file2d.getTimeDimension(), &indexRangeDimension, &indexBinDimension }, sU("Rain Size Distribution"), sU(""), sci::min<perMetreCubedPerMillimetre>(sizeDistributions), sci::max<perMetreCubedPerMillimetre>(sizeDistributions), sU("In case of low signal to noise ratio negative values can occur. Although they have no physical meaning they are retained in order to avoid statistical biases."));
	AmfNcFlagVariable spectralReflectivityFlagVariable2d(sU("spectral_reflectivity_flag"), microRainRadarFlags, file2d, std::vector<sci::NcDimension*>{ &file2d.getTimeDimension(), &indexRangeDimension, &indexBinDimension });
	AmfNcFlagVariable dropDiameterFlagVariable2d(sU("rain_drop_diameter_flag"), microRainRadarFlags, file2d, std::vector<sci::NcDimension*>{ &file2d.getTimeDimension(), &indexRangeDimension, &indexBinDimension });
	AmfNcFlagVariable sizeDistributionFlagVariable2d(sU("drop_size_distribution_flag"), microRainRadarFlags, file2d, std::vector<sci::NcDimension*>{ &file2d.getTimeDimension(), &indexRangeDimension, &indexBinDimension });

	file2d.writeTimeAndLocationData(platform);

	file2d.write(altitudesVariable2d, altitudes);
	file2d.write(spectralReflectivityVariable, spectralReflectivity);
	file2d.write(dropDiameterVariable, dropDiameters);
	file2d.write(numberDistributionVariable, sizeDistributions);
	file2d.write(spectralReflectivityFlagVariable2d, spectralReflectivityFlags);
	file2d.write(dropDiameterFlagVariable2d, dropDiameterFlags);
	file2d.write(sizeDistributionFlagVariable2d, sizeDistributionFlags);
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
	sci::UtcTime fileEnd = fileStart + second(60 * 60 * 24 - 0.0000001);
	return (fileStart<endTime && fileEnd > endTime);
}