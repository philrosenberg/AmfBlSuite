#include"MicroRainRadar.h"
#include<svector/sreadwrite.h>
#include"ProgressReporter.h"

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
	DataInfo dataInfo;
	dataInfo.averagingPeriod = second(OutputAmfNcFile::getFillValue());
	dataInfo.samplingInterval = second(OutputAmfNcFile::getFillValue());
	dataInfo.continuous = true;
	dataInfo.startTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	dataInfo.endTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	if (m_profiles.size() > 0)
	{
		dataInfo.startTime = m_profiles[0].getTime();
		dataInfo.endTime = m_profiles.back().getTime();
	}
	dataInfo.featureType = ft_timeSeriesPoint;
	dataInfo.options = std::vector<sci::string>(0);
	dataInfo.processingLevel = 2;
	dataInfo.productName = sU("rain lwc velocity reflectivity");
	dataInfo.processingOptions = processingOptions;

	if (m_profiles.size() > 0)
	{
		std::vector<second> averagingTimes(m_profiles.size());
		for (size_t i = 0; i < averagingTimes.size(); ++i)
		{
			averagingTimes[i] = m_profiles[i].getAveragingTime();
		}
		std::vector <second> sortedAveragingTimes = averagingTimes;
		std::sort(sortedAveragingTimes.begin(), sortedAveragingTimes.end());
		dataInfo.averagingPeriod = sortedAveragingTimes[sortedAveragingTimes.size() / 2];
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
		dataInfo.samplingInterval = second(sortedSamplingIntervals[sortedSamplingIntervals.size() / 2]);
	}

	//pull all the variables out
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



	std::vector<sci::NcDimension*> nonTimeDimensions;

	sci::NcDimension indexDimension(sU("index"), m_profiles[0].getRanges().size());
	nonTimeDimensions.push_back(&indexDimension);

	OutputAmfNcFile file(directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, times, nonTimeDimensions);

	AmfNcVariable<metre> altitudesVariable(sU("altitude"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Geometric height above geoid (WGS84)"), sU("altitude"), sci::min<metre>(altitudes), sci::max<metre>(altitudes));
	AmfNcVariable<millimetrePerHour> rainfallRatesVariable(sU("rainfall_rate"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Rainfall Rate"), sU(""), sci::min<millimetrePerHour>(rainfallRates), sci::max<millimetrePerHour>(rainfallRates));
	AmfNcVariable<gramPerMetreCubed> rainLiquidWaterContentVariable(sU("rain_liquid_water_content"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Rain Liquid Water Content"), sU(""), sci::min<gramPerMetreCubed>(rainLiquidWaterContent), sci::max<gramPerMetreCubed>(rainLiquidWaterContent));
	AmfNcVariable<metrePerSecond> rainfallVelocityVariable(sU("rainfall_velocity"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Rainfall Velocity"), sU(""), sci::min<metrePerSecond>(rainfallVelocity), sci::max<metrePerSecond>(rainfallVelocity));
	AmfNcVariable<reflectivity> radarReflectivityVariable(sU("radar_reflectivity"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Radar Reflectivity (Z)"), sU(""), sci::min<reflectivity>(radarReflectivity), sci::max<reflectivity>(radarReflectivity));
	AmfNcVariable<reflectivity> radarReflectivityAttenuatedVariable(sU("attenuated_radar_reflectivity"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Attenuated Radar Reflectivity (z)"), sU(""), sci::min<reflectivity>(radarReflectivityAttenuated), sci::max<reflectivity>(radarReflectivity));
	AmfNcVariable<unitless> pathIntegratedAttenuationVariable(sU("path_integrated_attenuation"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &indexDimension }, sU("Path Integrated Attenuation"), sU(""), sci::min<unitless>(pathIntegratedAttenuation), sci::max<unitless>(pathIntegratedAttenuation));

	file.writeTimeAndLocationData(platform);

	file.write(altitudesVariable, altitudes);
	file.write(rainfallRatesVariable, rainfallRates);
	file.write(rainLiquidWaterContentVariable, rainLiquidWaterContent);
	file.write(rainfallVelocityVariable, rainfallVelocity);
	file.write(radarReflectivityVariable, radarReflectivity);
	file.write(radarReflectivityAttenuatedVariable, radarReflectivityAttenuated);
	file.write(pathIntegratedAttenuationVariable, pathIntegratedAttenuation);
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