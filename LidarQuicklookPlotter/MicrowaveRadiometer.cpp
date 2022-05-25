#include"MicrowaveRadiometer.h"
#include"ProgressReporter.h"

MicrowaveRadiometerProcessor::MicrowaveRadiometerProcessor(const InstrumentInfo& instrumentInfo, const CalibrationInfo& calibrationInfo)
	: InstrumentProcessor(sU("[/\\\\]Y....[/\\\\]M..[/\\\\]D..[/\\\\]........\\.(LWP|IWV|HKD)$"))
	//: InstrumentProcessor(sU("[/\\\\]Y....[/\\\\]M..[/\\\\]D..[/\\\\]........\\.LWP$"))
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
		//We use the second file to work out the space needed as the first one might be a partial hour.
		bool reservedSpace = false;
		if (i == 1)
			elementsInFirstFile = std::max(m_lwp.size(), m_iwv.size());
		if (!reservedSpace && i > 1 && (m_lwp.size() > elementsInFirstFile || m_iwv.size() > elementsInFirstFile))
		{
			elementsInSecondFile = std::max(m_lwp.size(), m_iwv.size()) - elementsInFirstFile;
			size_t reserveSize = inputFilenames.size() * std::max(elementsInFirstFile, elementsInSecondFile);

			m_lwpTime.reserve(reserveSize);
			m_lwp.reserve(reserveSize);
			m_lwpElevation.reserve(reserveSize);
			m_lwpAzimuth.reserve(reserveSize);
			m_lwpRainFlag.reserve(reserveSize);
			m_lwpQuality.reserve(reserveSize);
			m_lwpQualityExplanation.reserve(reserveSize);

			m_iwvTime.reserve(reserveSize);
			m_iwv.reserve(reserveSize);
			m_iwvElevation.reserve(reserveSize);
			m_iwvAzimuth.reserve(reserveSize);
			m_iwvRainFlag.reserve(reserveSize);
			m_iwvQuality.reserve(reserveSize);
			m_iwvQualityExplanation.reserve(reserveSize);

			reservedSpace = true;
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

		m_iwvTime.resize(0);
		m_iwv.resize(0);
		m_iwvElevation.resize(0);
		m_iwvAzimuth.resize(0);
		m_iwvRainFlag.resize(0);
		m_iwvQuality.resize(0);
		m_iwvQualityExplanation.resize(0);
		
		m_hkdTime.resize(0);
		m_latitude.resize(0);
		m_longitude.resize(0);
		m_ambientTarget1Temperature.resize(0);
		m_ambientTarget2Temperature.resize(0);
		m_humidityProfilerTemperature.resize(0);
		m_temperatureProfilerTemperature.resize(0);
		m_temperatureStabilityReceiver1.resize(0);
		m_temperatureStabilityReceiver2.resize(0);
		m_status.resize(0);
		m_remainingMemory.resize(0);

		m_metTime.resize(0);
		m_enviromentTemperature.resize(0);
		m_enviromentPressure.resize(0);
		m_enviromentRelativeHumidity.resize(0);
	}

	uint32_t fileTypeId;
	sci::GridData<sci::UtcTime, 1> time;
	sci::GridData<gramPerMetreSquaredF, 1> water;
	sci::GridData<degreeF, 1> elevation;
	sci::GridData<degreeF, 1> azimuth;
	sci::GridData<bool, 1> rainFlag;
	sci::GridData<unsigned char, 1> quality;
	sci::GridData<unsigned char, 1> qualityExplanation;
	sci::GridData<sci::UtcTime, 1> hkdTime;
	sci::GridData<degreeF, 1> latitude;
	sci::GridData<degreeF, 1> longitude;
	sci::GridData<kelvinF, 1> ambientTarget1Temperature;
	sci::GridData<kelvinF, 1> ambientTarget2Temperature;
	sci::GridData<kelvinF, 1> humidityProfilerTemperature;
	sci::GridData<kelvinF, 1> temperatureProfilerTemperature;
	sci::GridData<kelvinF, 1> temperatureStabilityReceiver1;
	sci::GridData<kelvinF, 1> temperatureStabilityReceiver2;
	sci::GridData<uint32_t, 1> status;
	sci::GridData<uint32_t, 1> remainingMemory;
	sci::GridData<kelvinF, 1> enviromentTemperature;
	sci::GridData<hectoPascalF, 1> enviromentPressure;
	sci::GridData<unitlessF, 1> enviromentRelativeHumidity;

	sci::string extension = inputFilename.length() < 3 ? inputFilename : inputFilename.substr(inputFilename.length() - 3);
	

	if (extension == sU("HKD"))
		readHatproHkdFile(inputFilename, time, latitude, longitude, ambientTarget1Temperature, ambientTarget2Temperature,
			humidityProfilerTemperature, temperatureProfilerTemperature, temperatureStabilityReceiver1, temperatureStabilityReceiver2,
			status, remainingMemory, fileTypeId);
	else if (extension == sU("LWP") || extension == sU("IWV"))
		readHatproLwpOrIwvData(inputFilename, time, water, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
	else if (extension == sU("MET"))
		readHatproMetData(inputFilename, time, enviromentTemperature, enviromentRelativeHumidity, enviromentPressure, rainFlag, fileTypeId);

	if (fileTypeId == hatproLwpId)
	{
		m_lwpTime.insert(m_lwpTime.size(), time);
		m_lwp.insert(m_lwp.size(), water);
		m_lwpElevation.insert(m_lwpElevation.size(), elevation);
		m_lwpAzimuth.insert(m_lwpAzimuth.size(), azimuth);
		m_lwpRainFlag.insert(m_lwpRainFlag.size(), rainFlag);
		m_lwpQuality.insert(m_lwpQuality.size(), quality);
		m_lwpQualityExplanation.insert(m_lwpQualityExplanation.size(), qualityExplanation);
	}
	else if (fileTypeId == hatproIwvId)
	{
		m_iwvTime.insert(m_iwvTime.size(), time);
		m_iwv.insert(m_iwv.size(), water);
		m_iwvElevation.insert(m_iwvElevation.size(), elevation);
		m_iwvAzimuth.insert(m_iwvAzimuth.size(), azimuth);
		m_iwvRainFlag.insert(m_iwvRainFlag.size(), rainFlag);
		m_iwvQuality.insert(m_iwvQuality.size(), quality);
		m_iwvQualityExplanation.insert(m_iwvQualityExplanation.size(), qualityExplanation);
	}
	else if (fileTypeId == hatproHkdId)
	{
		m_hkdTime.insert(m_hkdTime.size(), time);
		m_latitude.insert(m_latitude.size(), latitude);
		m_longitude.insert(m_longitude.size(), longitude);
		m_ambientTarget1Temperature.insert(m_ambientTarget1Temperature.size(), ambientTarget1Temperature);
		m_ambientTarget2Temperature.insert(m_ambientTarget2Temperature.size(), ambientTarget2Temperature);
		m_humidityProfilerTemperature.insert(m_humidityProfilerTemperature.size(), humidityProfilerTemperature);
		m_temperatureProfilerTemperature.insert(m_temperatureProfilerTemperature.size(), temperatureProfilerTemperature);
		m_temperatureStabilityReceiver1.insert(m_temperatureStabilityReceiver1.size(), temperatureStabilityReceiver1);
		m_temperatureStabilityReceiver2.insert(m_temperatureStabilityReceiver2.size(), temperatureStabilityReceiver2);
		m_status.insert(m_status.size(), status);
	}
	else if (fileTypeId == hatproMetId)
	{
		m_metTime.insert(m_metTime.size(), time);
		m_enviromentTemperature.insert(m_enviromentTemperature.size(), enviromentTemperature);
		m_enviromentPressure.insert(m_enviromentPressure.size(), enviromentPressure);
		m_enviromentRelativeHumidity.insert(m_enviromentRelativeHumidity.size(), enviromentRelativeHumidity);
	}

	m_hasData = m_lwpTime.size() > 0 || m_iwvTime.size() > 0 || m_hkdTime.size() > 0;
}

void MicrowaveRadiometerProcessor::writeToNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	writeIwvLwpNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter);
}

void MicrowaveRadiometerProcessor::writeIwvLwpNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	//ensure the data is sorted by time
	sci::sortBy(m_lwpTime, std::make_tuple(&m_lwp, &m_lwpElevation, &m_lwpAzimuth, &m_lwpRainFlag, &m_lwpQuality, &m_lwpQualityExplanation));
	sci::sortBy(m_iwvTime, std::make_tuple(&m_iwv, &m_iwvElevation, &m_iwvAzimuth, &m_iwvRainFlag, &m_iwvQuality, &m_lwpQualityExplanation));
	sci::sortBy(m_hkdTime, std::make_tuple(&m_iwv, &m_latitude, &m_longitude, &m_ambientTarget1Temperature, &m_ambientTarget2Temperature,
		&m_humidityProfilerTemperature, &m_temperatureProfilerTemperature, &m_temperatureStabilityReceiver1, &m_temperatureStabilityReceiver2,
		&m_remainingMemory));


	DataInfo dataInfo;
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo.continuous = true;
	dataInfo.startTime = m_lwpTime[0];
	dataInfo.endTime = m_lwpTime.back();
	
	dataInfo.featureType = FeatureType::timeSeriesPoint;
	dataInfo.options = std::vector<sci::string>(0);
	dataInfo.processingLevel = 1;
	dataInfo.productName = sU("iwv lwp");
	dataInfo.processingOptions = processingOptions;
	
	sci::GridData<secondF, 1> intervals(m_lwpTime.size() - 1);
	for (size_t i = 0; i < intervals.size(); ++i)
		intervals[i] = secondF(m_lwpTime[i + 1] - m_lwpTime[i]);
	dataInfo.averagingPeriod = sci::median(intervals);
	dataInfo.samplingInterval = dataInfo.averagingPeriod;

	//assign amf version based on if we are moving or not.
	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;
	

	//get attitudes corrected for platform attitude
	sci::GridData<degreeF, 1> correctedElevations(m_lwpTime.size());
	sci::GridData<degreeF, 1> correctedAzimuths(m_lwpTime.size());
	for (size_t i = 0; i < m_lwpTime.size(); ++i)
	{
		sci::UtcTime startTime = i > 0 ? m_lwpTime[i-1] : m_lwpTime[0] - (m_lwpTime[1]-m_lwpTime[0]);
		platform.correctDirection(startTime, m_lwpTime[i], m_lwpAzimuth[i], m_lwpElevation[i], correctedAzimuths[i], correctedElevations[i]);
		
	}

	//check that the iwv and lwp data are the same size or that one is missing entirely so we can omit it
	sci::assertThrow(m_iwv.size() == 0 || m_lwp.size() == 0 || m_iwv.size() == m_lwp.size(), sci::err(sci::SERR_USER, 0, sU("IWV and LWP files have missmatch in data length.")));

	for(size_t i=0; i< m_iwv.size(); ++i)
		sci::assertThrow(m_iwvTime[i] == m_lwpTime[i], sci::err(sci::SERR_USER, 0, sU("IWV and LWP files have missmatched times.")));

	//convert the hatpro flags into amof flags
	sci::GridData<uint8_t, 1> lwpFlag(m_lwpTime.size());
	for (size_t i = 0; i < lwpFlag.size(); ++i)
		lwpFlag[i] = hatproToAmofFlag(m_lwpQuality[i], m_lwpQualityExplanation[i]);
	sci::GridData<uint8_t, 1> iwvFlag(m_iwvTime.size());
	for (size_t i = 0; i < iwvFlag.size(); ++i)
		iwvFlag[i] = hatproToAmofFlag(m_iwvQuality[i], m_iwvQualityExplanation[i]);

	//nan out any data that is flagged not good or missing
	for (size_t i = 0; i < lwpFlag.size(); ++i)
		if (lwpFlag[i] == mwrLowQualityUnknownReasonFlag
			|| lwpFlag[i] == mwrLowQualityInterferenceFailureFlag
			|| lwpFlag[i] == mwrLowQualityLwpTooHighFlag
			|| lwpFlag[i] == mwrMissingDataFlag)
			m_lwp[i] = std::numeric_limits<gramPerMetreSquaredF>::quiet_NaN();
	for (size_t i = 0; i < iwvFlag.size(); ++i)
		if (iwvFlag[i] == mwrLowQualityUnknownReasonFlag
			|| iwvFlag[i] == mwrLowQualityInterferenceFailureFlag
			|| iwvFlag[i] == mwrLowQualityLwpTooHighFlag
			|| iwvFlag[i] == mwrMissingDataFlag)
			m_iwv[i] = std::numeric_limits<kilogramPerMetreSquaredF>::quiet_NaN();

	for (size_t i = 0; i < std::min(m_iwvTime.size(), m_hkdTime.size()); ++i)
	{
		if (m_iwvTime[i] > m_hkdTime[i])
		{
			size_t j;
			for (j = i; j < m_hkdTime.size(); ++j)
				if (m_iwvTime[i] <= m_hkdTime[j])
					break;
			if (j == m_hkdTime.size())
			{
				break;
			}
			else
			{
				//if we found an exact match for our time, then pad with nans
				//if we went past our time, then also nan out the curent point - there is no matching hkd time
				size_t nToPad = j - i;
				if (m_iwvTime[i] < m_hkdTime[j])
				{
					--nToPad;
					m_iwvTime[i] = m_hkdTime[j];
					m_iwv[i] = std::numeric_limits<kilogramPerMetreSquaredF>::quiet_NaN();
					m_iwvElevation[i] = std::numeric_limits<degreeF>::quiet_NaN();
					m_iwvAzimuth[i] = std::numeric_limits<degreeF>::quiet_NaN();
					m_iwvRainFlag[i] = mwrRainMissingDataFlag;
					m_iwvQuality[i] = mwrMissingDataFlag;
					m_iwvQualityExplanation[i] = 0;
					iwvFlag[i] = mwrMissingDataFlag;

					m_lwpTime[i] = m_hkdTime[j];
					m_lwp[i] = std::numeric_limits<kilogramPerMetreSquaredF>::quiet_NaN();
					m_lwpElevation[i] = std::numeric_limits<degreeF>::quiet_NaN();
					m_lwpAzimuth[i] = std::numeric_limits<degreeF>::quiet_NaN();
					m_lwpRainFlag[i] = mwrRainMissingDataFlag;
					m_lwpQuality[i] = mwrMissingDataFlag;
					m_lwpQualityExplanation[i] = 0;
					lwpFlag[i] = mwrMissingDataFlag;
				}
				//now pad
				if (nToPad > 0) //this can actually be zero if there was just one bad time and it is decremented above
				{
					m_iwvTime.insert(i, m_hkdTime.subGrid(i, nToPad));
					m_iwv.insert(i, nToPad, std::numeric_limits<kilogramPerMetreSquaredF>::quiet_NaN());
					m_iwvElevation.insert(i, nToPad, std::numeric_limits<degreeF>::quiet_NaN());
					m_iwvAzimuth.insert(i, nToPad, std::numeric_limits<degreeF>::quiet_NaN());
					m_iwvRainFlag.insert(i, nToPad, mwrRainMissingDataFlag);
					m_iwvQuality.insert(i, nToPad, mwrMissingDataFlag);
					m_iwvQualityExplanation.insert(i, nToPad, 0);
					iwvFlag.insert(i, nToPad, mwrMissingDataFlag);

					m_lwpTime.insert(i, m_hkdTime.subGrid(i, nToPad));
					m_lwp.insert(i, nToPad, std::numeric_limits<kilogramPerMetreSquaredF>::quiet_NaN());
					m_lwpElevation.insert(i, nToPad, std::numeric_limits<degreeF>::quiet_NaN());
					m_lwpAzimuth.insert(i, nToPad, std::numeric_limits<degreeF>::quiet_NaN());
					m_lwpRainFlag.insert(i, nToPad, mwrRainMissingDataFlag);
					m_lwpQuality.insert(i, nToPad, mwrMissingDataFlag);
					m_lwpQualityExplanation.insert(i, nToPad, 0);
					lwpFlag.insert(i, nToPad, mwrMissingDataFlag);
				}
			}
		}
	}
	if (m_iwvTime.size() < m_hkdTime.size())
	{
		m_iwvTime.insert(m_iwvTime.size(), m_hkdTime.subGrid(m_iwvTime.size(), m_hkdTime.size() - m_iwvTime.size()));
		m_iwv.resize(m_hkdTime.size(), std::numeric_limits<kilogramPerMetreSquaredF>::quiet_NaN());
		m_iwvElevation.resize(m_hkdTime.size(), std::numeric_limits<degreeF>::quiet_NaN());
		m_iwvAzimuth.resize(m_hkdTime.size(), std::numeric_limits<degreeF>::quiet_NaN());
		m_iwvRainFlag.resize(m_hkdTime.size(), mwrRainMissingDataFlag);
		m_iwvQuality.resize(m_hkdTime.size(), mwrMissingDataFlag);
		m_iwvQualityExplanation.resize(m_hkdTime.size(), 0);
		iwvFlag.resize(m_hkdTime.size(), mwrMissingDataFlag);


		m_lwpTime.insert(m_hkdTime.size(), m_hkdTime.subGrid(m_lwpTime.size(), m_hkdTime.size() - m_lwpTime.size()));
		m_lwp.resize(m_hkdTime.size(), std::numeric_limits<kilogramPerMetreSquaredF>::quiet_NaN());
		m_lwpElevation.resize(m_hkdTime.size(), std::numeric_limits<degreeF>::quiet_NaN());
		m_lwpAzimuth.resize(m_iwvTime.size(), std::numeric_limits<degreeF>::quiet_NaN());
		m_lwpRainFlag.resize(m_iwvTime.size(), mwrRainMissingDataFlag);
		m_lwpQuality.resize(m_iwvTime.size(), mwrMissingDataFlag);
		m_lwpQualityExplanation.resize(m_iwvTime.size(), 0);
		lwpFlag.resize(m_hkdTime.size(), mwrMissingDataFlag);
	}

	
	
	//set the met flags all to good - we don't even include the met data in the files
	sci::GridData<uint8_t, 1> surfaceTemperatureFlag(m_iwv.size(), mwrGoodDataFlag);
	sci::GridData<uint8_t, 1> surfacePressureFlag(m_iwv.size(), mwrGoodDataFlag);
	sci::GridData<uint8_t, 1> surfaceRelativeHumidityFlag(m_iwv.size(), mwrGoodDataFlag);
	//set the rest of the flags
	sci::GridData<uint8_t, 1> rainFlag(m_iwvRainFlag.size(), mwrRainGoodDataFlag);
	for (size_t i = 0; i < m_iwvRainFlag.size(); ++i)
		if (m_iwvRainFlag[i])
			rainFlag[i] = mwrRainRainingFlag;
	sci::GridData<sci::GridData<uint8_t, 1>,1> channelFailureFlags(14, sci::GridData<uint8_t, 1>(m_status.size()));
	sci::GridData<uint32_t, 1> statusFilters{ 0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000100, 0x00000200,
		0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000 }; //note 0x0080 and 0x8000 are missing deliberately
	for (size_t i = 0; i < 14; ++i)
		for (size_t j = 0; j < m_status.size(); ++j)
			channelFailureFlags[i][j] = (m_status[j] & statusFilters[i]) == 0 ? mwrChannelFailFlag : mwrChannelGoodDataFlag;
	sci::GridData<uint8_t, 1> temperatureReceiverStabilityFlag(m_temperatureStabilityReceiver1.size(), mwrStabilityGoodDataFlag);
	for (size_t i = 0; i < temperatureReceiverStabilityFlag.size(); ++i)
	{
		if (m_temperatureStabilityReceiver1[i] > millikelvinF(50))
			temperatureReceiverStabilityFlag[i] = mwrStabilityVeryVeryPoorFlag;
		else if (m_temperatureStabilityReceiver1[i] > millikelvinF(10))
			temperatureReceiverStabilityFlag[i] = mwrStabilityVeryPoorFlag;
		else if (m_temperatureStabilityReceiver1[i] > millikelvinF(5))
			temperatureReceiverStabilityFlag[i] = mwrStabilityPoorFlag;
	}
	sci::GridData<uint8_t, 1> relativeHumidityReceiverStabilityFlag(m_temperatureStabilityReceiver2.size(), mwrStabilityGoodDataFlag);
	for (size_t i = 0; i < relativeHumidityReceiverStabilityFlag.size(); ++i)
	{
		if (m_temperatureStabilityReceiver2[i] > millikelvinF(50))
			relativeHumidityReceiverStabilityFlag[i] = mwrStabilityVeryVeryPoorFlag;
		else if (m_temperatureStabilityReceiver2[i] > millikelvinF(10))
			relativeHumidityReceiverStabilityFlag[i] = mwrStabilityVeryPoorFlag;
		else if (m_temperatureStabilityReceiver2[i] > millikelvinF(5))
			relativeHumidityReceiverStabilityFlag[i] = mwrStabilityPoorFlag;
	}







	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
	OutputAmfNcFile file(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("hatpro microwave radiometer intergrated water vapour and liquid water path retrieval"), m_iwv.size() > 0 ? m_iwvTime : m_lwpTime);

	std::unique_ptr<AmfNcVariable<kilogramPerMetreSquaredF>> iwvVariable(nullptr);
	std::unique_ptr<AmfNcVariable<gramPerMetreSquaredF>> lwpVariable(nullptr);
	
	if(m_iwv.size() > 0)
		iwvVariable.reset(new AmfNcVariable<kilogramPerMetreSquaredF> (sU("integrated_water_vapor"), file, file.getTimeDimension(), sU("Integrated Water Vapour"), sU(""), m_iwv, true, coordinates, cellMethods, iwvFlag, sU("")));
	if (m_lwp.size() > 0)
		lwpVariable.reset(new AmfNcVariable<gramPerMetreSquaredF>(sU("liquid_water_path"), file, file.getTimeDimension(), sU("Liquid Water Path"), sU(""), m_lwp, true, coordinates, cellMethods, lwpFlag, sU("")));
	
	if (m_iwv.size() == 0 && m_lwp.size() == 0)
		return;

	AmfNcFlagVariable temperatureFlagVariable(sU("qc_flag_surface_temperature"), mwrMetFlags, file, file.getTimeDimension());
	AmfNcFlagVariable relativeHumidityFlagVariable(sU("qc_flag_surface_relative_humidity"), mwrMetFlags, file, file.getTimeDimension());
	AmfNcFlagVariable pressureFlagVariable(sU("qc_flag_surface_pressure"), mwrMetFlags, file, file.getTimeDimension());
	AmfNcFlagVariable rainFlagVariable(sU("qc_flag_surface_precipitation"), mwrRainFlags , file, file.getTimeDimension());
	AmfNcFlagVariable ch1FlagVariable(sU("qc_flag_channel_1_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch2FlagVariable(sU("qc_flag_channel_2_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch3FlagVariable(sU("qc_flag_channel_3_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch4FlagVariable(sU("qc_flag_channel_4_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch5FlagVariable(sU("qc_flag_channel_5_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch6FlagVariable(sU("qc_flag_channel_6_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch7FlagVariable(sU("qc_flag_channel_7_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch8FlagVariable(sU("qc_flag_channel_8_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch9FlagVariable(sU("qc_flag_channel_9_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch10FlagVariable(sU("qc_flag_channel_10_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch11FlagVariable(sU("qc_flag_channel_11_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch12FlagVariable(sU("qc_flag_channel_12_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch13FlagVariable(sU("qc_flag_channel_13_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable ch14FlagVariable(sU("qc_flag_channel_14_failure"), mwrChannelFlags, file, file.getTimeDimension());
	AmfNcFlagVariable temperatureStabilityFlagVariable(sU("qc_flag_t_receiver_temperature_stability"), mwrStabilityFlags, file, file.getTimeDimension());
	AmfNcFlagVariable relativeHumidityStabilityFlagVariable(sU("qc_flag_rh_receiver_temperature_stability"), mwrStabilityFlags, file, file.getTimeDimension());


	file.writeTimeAndLocationData(platform);

	if (iwvVariable)
		file.write(*iwvVariable, m_iwv);
	if (lwpVariable)
		file.write(*lwpVariable, m_lwp);
	file.write(temperatureFlagVariable, surfaceTemperatureFlag);
	file.write(relativeHumidityFlagVariable, surfaceRelativeHumidityFlag);
	file.write(pressureFlagVariable, surfacePressureFlag);
	file.write(rainFlagVariable, rainFlag);
	file.write(ch1FlagVariable, channelFailureFlags[0]);
	file.write(ch2FlagVariable, channelFailureFlags[1]);
	file.write(ch3FlagVariable, channelFailureFlags[2]);
	file.write(ch4FlagVariable, channelFailureFlags[3]);
	file.write(ch5FlagVariable, channelFailureFlags[4]);
	file.write(ch6FlagVariable, channelFailureFlags[5]);
	file.write(ch7FlagVariable, channelFailureFlags[6]);
	file.write(ch8FlagVariable, channelFailureFlags[7]);
	file.write(ch9FlagVariable, channelFailureFlags[8]);
	file.write(ch10FlagVariable, channelFailureFlags[9]);
	file.write(ch11FlagVariable, channelFailureFlags[10]);
	file.write(ch12FlagVariable, channelFailureFlags[11]);
	file.write(ch13FlagVariable, channelFailureFlags[12]);
	file.write(ch14FlagVariable, channelFailureFlags[13]);
	file.write(temperatureStabilityFlagVariable, temperatureReceiverStabilityFlag);
	file.write(relativeHumidityStabilityFlagVariable, relativeHumidityReceiverStabilityFlag);
}

std::vector<std::vector<sci::string>> MicrowaveRadiometerProcessor::groupInputFilesbyOutputFiles(const std::vector<sci::string>& newFiles, const std::vector<sci::string>& allFiles) const
{
	std::vector<std::vector<sci::string>> result;
	for (auto& a : newFiles)
	{
		bool include = false;
		//check if a new file exists on the for the day of the file, if so we want this file
		for (auto& n : newFiles)
			if (n.substr(n.length() - 12, 6) == a.substr(a.length() - 12, 6))
				include = true;

		//if include is false, go back up to the begining of the for loop and move to the next file
		if (!include)
			continue;

		bool added = false;
		//check if there is already a group for the day of this file and add the new files there if there is
		for (auto& r : result)
		{
			if (a.substr(a.length() - 12, 6) == r[0].substr(r[0].length() - 12, 6))
			{
				r.push_back(a);
				added = true;
			}
		}
		//if there wasn't then add the new file as the first of new group
		if (!added)
			result.push_back(std::vector<sci::string>(1, a));
	}

	//sort each group in alphabetical (chronological) order
	for (auto& r : result)
		std::sort(r.begin(), r.end());

	return result;
}

bool MicrowaveRadiometerProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	sci::UtcTime fileStart(std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 12, 2)).c_str()) + 2000,
		std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 10, 2)).c_str()),
		std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 8, 2)).c_str()),
		std::atoi(sci::nativeCodepage(fileName.substr(fileName.length() - 6, 2)).c_str()),
		0, 0.0);
	sci::UtcTime fileEnd = fileStart + second(60.0 * 60.0 - 0.0000001);
	return (fileStart<endTime && fileEnd > startTime);
}