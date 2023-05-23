#include"MicrowaveRadiometer.h"
#include"ProgressReporter.h"

MicrowaveRadiometerProcessor::MicrowaveRadiometerProcessor(const InstrumentInfo& instrumentInfo, const CalibrationInfo& calibrationInfo)
	//: InstrumentProcessor(sU("[/\\\\]Y....[/\\\\]M..[/\\\\]D..[/\\\\]........\\.(LWP|IWV|HKD|STA|BRT|MET|ATN|TPC|TPB|HPC)$"))
	: InstrumentProcessor(sU("[/\\\\].*.(LWP|IWV|HKD|STA|BRT|MET|ATN|TPC|TPB|HPC)$"))
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

template<class T>
void appendData2dInFirstDimension(sci::GridData<T, 2>& destination, const sci::GridData<T, 2>& source)
{
	if (destination.shape()[0] == 0 || destination.shape()[1] == 0)
	{
		destination = source;
		return;
	}

	sci::assertThrow(destination.shape()[1] == source.shape()[1], sci::err(sci::SERR_USER, 0, sU("Trying to append 2d data where the size in the second dimension does not match.")));

	auto destShape = destination.shape();
	auto sourceShape = source.shape();
	std::array<size_t, 2> resultShape{ destShape[0] + sourceShape[0], destShape[1] };

	destination.reshape(resultShape);

	for (size_t i = 0; i < sourceShape[0]; ++i)
	{
		for (size_t j = 0; j < sourceShape[1]; ++j)
		{
			destination[i+destShape[0]][j] = source[i][j];
		}
	}
}

void MicrowaveRadiometerProcessor::readData(const sci::string& inputFilename, const Platform& platform, ProgressReporter& progressReporter, bool clear)
{
	progressReporter << "Reading file " << inputFilename << "\n";

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

		m_staTime.resize(0);
		m_staRainFlag.resize(0);
		m_liftedIndex.resize(0);
		m_kModifiedIndex.resize(0);
		m_totalTotalsIndex.resize(0);
		m_kIndex.resize(0);
		m_showalterIndex.resize(0);
		m_cape.resize(0);

		sci::GridData<sci::UtcTime, 1> m_bldTime;
		sci::GridData<metreF, 1> m_boundaryLayerDepth;

		m_brtTime.resize(0);
		m_brtFrequencies.resize(0);
		m_brightnessTemperature.reshape({ 0,0 });
		m_brtElevation.resize(0);
		m_brtAzimuth.resize(0);
		m_brtRainFlag.resize(0);

		m_atnTime.resize(0);
		m_atnFrequencies.resize(0);
		m_attenuation.reshape({ 0,0 });
		m_atnElevation.resize(0);
		m_atnAzimuth.resize(0);
		m_atnRainFlag.resize(0);

		m_tpbTime.resize(0);
		m_tpbAltitudes.resize(0);
		m_tpbTemperatures.reshape({ 0,0 });
		m_tpbRainFlag.resize(0);

		m_tpcTime.resize(0);
		m_tpcAltitudes.resize(0);
		m_tpcTemperatures.reshape({ 0,0 });
		m_tpcRainFlag.resize(0);

		m_hpcTime.resize(0);
		m_hpcAltitudes.resize(0);
		m_hpcAbsoluteHumidity.reshape({ 0,0 });
		m_hpcRelativeHumidity.reshape({ 0,0 });
		m_hpcRainFlag.resize(0);
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
	sci::GridData<float, 2>environmentOtherSensors;
	sci::GridData<kelvinF, 1> liftedIndex;
	sci::GridData<kelvinF, 1> kModifiedIndex;
	sci::GridData<kelvinF, 1> totalTotalsIndex;
	sci::GridData<kelvinF, 1> kIndex;
	sci::GridData<kelvinF, 1> showalterIndex;
	sci::GridData<joulePerKilogramF, 1> cape;
	sci::GridData<metreF, 1> boundaryLayerDepth;
	sci::GridData<perSecondF, 1> frequencies;
	sci::GridData<kelvinF, 2> brightnessTemperature;
	sci::GridData<unitlessF, 2> attenuation;
	sci::GridData<kelvinF, 2> temperatures;
	sci::GridData<gramPerMetreCubedF, 2> absoluteHumidity;
	sci::GridData<percentF, 2> relativeHumidity;
	sci::GridData<metreF, 1> altitudes;

	sci::string extension = inputFilename.length() < 3 ? inputFilename : inputFilename.substr(inputFilename.length() - 3);
	

	if (extension == sU("HKD"))
		readHatproHkdFile(inputFilename, time, latitude, longitude, ambientTarget1Temperature, ambientTarget2Temperature,
			humidityProfilerTemperature, temperatureProfilerTemperature, temperatureStabilityReceiver1, temperatureStabilityReceiver2,
			status, remainingMemory, fileTypeId);
	else if (extension == sU("IWV"))
		readHatproIwvData(inputFilename, time, water, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
	else if (extension == sU("LWP"))
		readHatproLwpData(inputFilename, time, water, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
	else if (extension == sU("MET"))
		readHatproMetData(inputFilename, time, enviromentTemperature, enviromentRelativeHumidity, enviromentPressure, environmentOtherSensors, rainFlag, fileTypeId);
	else if (extension == sU("STA"))
		readHatproStaData(inputFilename, time, liftedIndex, kModifiedIndex, totalTotalsIndex, kIndex, showalterIndex, cape, rainFlag, fileTypeId);
	else if (extension == sU("BRT"))
		readHatproBrtData(inputFilename, time, frequencies, brightnessTemperature, elevation, azimuth, rainFlag, fileTypeId);
	else if (extension == sU("ATN"))
		readHatproAtnData(inputFilename, time, frequencies, attenuation, elevation, azimuth, rainFlag, fileTypeId);
	else if (extension == sU("TPB"))
		readHatproTpbData(inputFilename, time, altitudes, temperatures, rainFlag, quality, qualityExplanation, fileTypeId);
	else if (extension == sU("TPC"))
		readHatproTpcData(inputFilename, time, altitudes, temperatures, rainFlag, quality, qualityExplanation, fileTypeId);
	else if (extension == sU("HPC"))
		readHatproHpcData(inputFilename, time, altitudes, absoluteHumidity, relativeHumidity, rainFlag, quality, qualityExplanation, fileTypeId);
	else
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, sU("Unexpected extension. Read aborted.")));

	if (fileTypeId == hatproLwpV1Id || fileTypeId == hatproLwpV2Id)
	{
		m_lwpTime.insert(m_lwpTime.size(), time);
		m_lwp.insert(m_lwp.size(), water);
		m_lwpElevation.insert(m_lwpElevation.size(), elevation);
		m_lwpAzimuth.insert(m_lwpAzimuth.size(), azimuth);
		m_lwpRainFlag.insert(m_lwpRainFlag.size(), rainFlag);
		m_lwpQuality.insert(m_lwpQuality.size(), quality);
		m_lwpQualityExplanation.insert(m_lwpQualityExplanation.size(), qualityExplanation);
		m_hasData = true;
	}
	else if (fileTypeId == hatproIwvV1Id || fileTypeId == hatproIwvV2Id)
	{
		m_iwvTime.insert(m_iwvTime.size(), time);
		m_iwv.insert(m_iwv.size(), water);
		m_iwvElevation.insert(m_iwvElevation.size(), elevation);
		m_iwvAzimuth.insert(m_iwvAzimuth.size(), azimuth);
		m_iwvRainFlag.insert(m_iwvRainFlag.size(), rainFlag);
		m_iwvQuality.insert(m_iwvQuality.size(), quality);
		m_iwvQualityExplanation.insert(m_iwvQualityExplanation.size(), qualityExplanation);
		m_hasData = true;
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
		m_remainingMemory.insert(m_remainingMemory.size(), remainingMemory);
		m_hasData = true;
	}
	else if (fileTypeId == hatproMetId || fileTypeId == hatproMetNewId)
	{
		//note, we don't currently do anything with the "additional sensor" data
		m_metTime.insert(m_metTime.size(), time);
		m_enviromentTemperature.insert(m_enviromentTemperature.size(), enviromentTemperature);
		m_enviromentPressure.insert(m_enviromentPressure.size(), enviromentPressure);
		m_enviromentRelativeHumidity.insert(m_enviromentRelativeHumidity.size(), enviromentRelativeHumidity);
		m_hasData = true;
	}
	else if (fileTypeId == hatproStaId)
	{
		m_staTime.insert(m_staTime.size(), time);
		m_staRainFlag.insert(m_staRainFlag.size(), rainFlag);
		m_liftedIndex.insert(m_liftedIndex.size(), liftedIndex);
		m_kModifiedIndex.insert(m_kModifiedIndex.size(), kModifiedIndex);
		m_totalTotalsIndex.insert(m_totalTotalsIndex.size(), totalTotalsIndex);
		m_kIndex.insert(m_kIndex.size(), kIndex);
		m_showalterIndex.insert(m_showalterIndex.size(), showalterIndex);
		m_cape.insert(m_cape.size(), cape);
		m_hasData = true;
	}
	else if (fileTypeId == hatproBrtV1Id || fileTypeId == hatproBrtV2Id)
	{
		m_brtTime.insert(m_brtTime.size(), time);
		appendData2dInFirstDimension(m_brightnessTemperature, brightnessTemperature);
		if(m_brtFrequencies.size()==0)
			m_brtFrequencies = frequencies;
		else
		{
			sci::assertThrow(frequencies.size() == m_brtFrequencies.size(), sci::err(sci::SERR_USER, 0, sU("Found a file with a different number of frequencies.")));
			for (size_t i = 0; i < m_brtFrequencies.size(); ++i)
				sci::assertThrow(frequencies[i] == m_brtFrequencies[i], sci::err(sci::SERR_USER, 0, sU("Found a file with different frequencies.")));
		}
		m_brtElevation.insert(m_brtElevation.size(), elevation);
		m_brtAzimuth.insert(m_brtAzimuth.size(), azimuth);
		m_brtRainFlag.insert(m_brtRainFlag.size(), rainFlag);
		m_hasData = true;
	}
	else if (fileTypeId == hatproAtnV1Id || fileTypeId == hatproAtnV2Id)
	{
		m_atnTime.insert(m_atnTime.size(), time);
		appendData2dInFirstDimension(m_attenuation, attenuation);
		if (m_atnFrequencies.size() == 0)
			m_atnFrequencies = frequencies;
		else
		{
			sci::assertThrow(frequencies.size() == m_atnFrequencies.size(), sci::err(sci::SERR_USER, 0, sU("Found a file with a different number of frequencies.")));
			for (size_t i = 0; i < m_atnFrequencies.size(); ++i)
				sci::assertThrow(frequencies[i] == m_atnFrequencies[i], sci::err(sci::SERR_USER, 0, sU("Found a file with different frequencies.")));
		}
		m_atnElevation.insert(m_atnElevation.size(), elevation);
		m_atnAzimuth.insert(m_atnAzimuth.size(), azimuth);
		m_atnRainFlag.insert(m_atnRainFlag.size(), rainFlag);
		m_hasData = true;
	}
	else if (fileTypeId == hatproTpbId)
	{
		m_tpbTime.insert(m_tpbTime.size(), time);
		appendData2dInFirstDimension(m_tpbTemperatures, temperatures);
		if (m_tpbAltitudes.size() == 0)
			m_tpbAltitudes = altitudes;
		else
		{
			sci::assertThrow(altitudes.size() == m_tpbAltitudes.size(), sci::err(sci::SERR_USER, 0, sU("Found a file with a different number of altitudes.")));
			for (size_t i = 0; i < m_tpbAltitudes.size(); ++i)
				sci::assertThrow(altitudes[i] == m_tpbAltitudes[i], sci::err(sci::SERR_USER, 0, sU("Found a file with different altitudes.")));
		}
		m_tpbRainFlag.insert(m_tpbRainFlag.size(), rainFlag);
	}
	else if (fileTypeId == hatproTpcId)
	{
		m_tpcTime.insert(m_tpcTime.size(), time);
		appendData2dInFirstDimension(m_tpcTemperatures, temperatures);
		if (m_tpcAltitudes.size() == 0)
			m_tpcAltitudes = altitudes;
		else
		{
			sci::assertThrow(altitudes.size() == m_tpcAltitudes.size(), sci::err(sci::SERR_USER, 0, sU("Found a file with a different number of altitudes.")));
			for (size_t i = 0; i < m_tpcAltitudes.size(); ++i)
				sci::assertThrow(altitudes[i] == m_tpcAltitudes[i], sci::err(sci::SERR_USER, 0, sU("Found a file with different altitudes.")));
		}
		m_tpcRainFlag.insert(m_tpcRainFlag.size(), rainFlag);
	}
	else if (fileTypeId == hatproHpcWithRhId || fileTypeId == hatproHpcNoRhId)
	{
		m_hpcTime.insert(m_hpcTime.size(), time);
		appendData2dInFirstDimension(m_hpcAbsoluteHumidity, absoluteHumidity);
		if (relativeHumidity.size() > 0)
		{
			appendData2dInFirstDimension(m_hpcRelativeHumidity, relativeHumidity);
			sci::assertThrow(m_hpcRelativeHumidity.shape() == m_hpcAbsoluteHumidity.shape(), sci::err(sci::SERR_USER, 0, sU("Some files contain RH and some do not.")));
		}

		if (m_hpcAltitudes.size() == 0)
			m_hpcAltitudes = altitudes;
		else
		{
			sci::assertThrow(altitudes.size() == m_hpcAltitudes.size(), sci::err(sci::SERR_USER, 0, sU("Found a file with a different number of altitudes.")));
			for (size_t i = 0; i < m_hpcAltitudes.size(); ++i)
				sci::assertThrow(altitudes[i] == m_hpcAltitudes[i], sci::err(sci::SERR_USER, 0, sU("Found a file with different altitudes.")));
		}
		m_hpcRainFlag.insert(m_hpcRainFlag.size(), rainFlag);
	}
}

void MicrowaveRadiometerProcessor::writeToNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	//ensure the data is sorted by time and that the flags a
	progressReporter << "Checking data order of LWP data\n";
	sci::sortBy(m_lwpTime, std::make_tuple(&m_lwp, &m_lwpElevation, &m_lwpAzimuth, &m_lwpRainFlag, &m_lwpQuality, &m_lwpQualityExplanation));
	progressReporter << "Checking data order of IWV data\n";
	sci::sortBy(m_iwvTime, std::make_tuple(&m_iwv, &m_iwvElevation, &m_iwvAzimuth, &m_iwvRainFlag, &m_iwvQuality, &m_iwvQualityExplanation));
	progressReporter << "Checking data order of HKD data\n";
	sci::sortBy(m_hkdTime, std::make_tuple(&m_latitude, &m_longitude, &m_ambientTarget1Temperature, &m_ambientTarget2Temperature,
		&m_humidityProfilerTemperature, &m_temperatureProfilerTemperature, &m_temperatureStabilityReceiver1, &m_temperatureStabilityReceiver2,
		&m_status, &m_remainingMemory));
	progressReporter << "Checking data order of MET data\n";
	sci::sortBy(m_metTime, std::make_tuple(&m_enviromentTemperature, &m_enviromentPressure, &m_enviromentRelativeHumidity));
	progressReporter << "Checking data order of STA data\n";
	sci::sortBy(m_staTime, std::make_tuple(&m_staRainFlag, &m_liftedIndex, &m_kModifiedIndex, &m_totalTotalsIndex, &m_kIndex, &m_showalterIndex, &m_cape));
	progressReporter << "Checking data order of BRT data\n";
	sci::sortBy(m_brtTime, std::make_tuple(&m_brightnessTemperature, &m_brtElevation, &m_brtAzimuth, &m_brtRainFlag));
	progressReporter << "Checking data order of ATN data\n";
	sci::sortBy(m_atnTime, std::make_tuple(&m_attenuation, &m_atnElevation, &m_atnAzimuth, &m_atnRainFlag));
	progressReporter << "Checking data order of TPB data\n";
	sci::sortBy(m_tpbTime, std::make_tuple(&m_tpbTemperatures, &m_tpbRainFlag));
	progressReporter << "Checking data order of TPC data\n";
	sci::sortBy(m_tpcTime, std::make_tuple(&m_tpcTemperatures, &m_tpcRainFlag));
	progressReporter << "Checking data order of HPC data\n";
	sci::sortBy(m_hpcTime, std::make_tuple(&m_hpcAbsoluteHumidity, &m_hpcRelativeHumidity, &m_hpcRainFlag));

	writeIwvLwpNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter);
	writeSurfaceMetNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter);
	writeStabilityNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter);
	writeBrightnessTemperatureNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter);
	writeBoundaryLayerTemperatureProfileNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter);
	writeFullTroposphereTemperatureProfileNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter);
	writeMoistureProfileNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter);
}

void MicrowaveRadiometerProcessor::writeIwvLwpNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	if (m_lwpTime.size() == 0 && m_iwvTime.size() == 0)
	{
		progressReporter << "No IWV/LWP data to write.\n";
		return;
	}
	progressReporter << "Writing IWV/LWP data file\n";

	DataInfo dataInfo = buildDataInfo(sU("iwv lwp"), m_lwpTime, FeatureType::timeSeries, processingOptions);

	//assign amf version based on if we are moving or not.
	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;
	


	//check that the iwv and lwp data are the same size or that one is missing entirely so we can omit it
	//sci::assertThrow(m_iwv.size() == 0 || m_lwp.size() == 0 || m_iwv.size() == m_lwp.size(), sci::err(sci::SERR_USER, 0, sU("IWV and LWP files have missmatch in data length.")));


	//convert the hatpro flags into amof flags
	sci::GridData<uint8_t, 1> lwpFlag = buildAmfFlagFromHatproQualityAndNanBadData(m_lwpQuality, m_lwpQualityExplanation, m_lwp);
	sci::GridData<uint8_t, 1> iwvFlag = buildAmfFlagFromHatproQualityAndNanBadData(m_iwvQuality, m_iwvQualityExplanation, m_iwv);

	//pad the data so it matches the housekeeping data
	if(m_iwvTime.size() > 0)
		padDataToMatchHkp(m_hkdTime, m_iwvTime, m_iwv, m_iwvElevation, m_iwvAzimuth, m_iwvRainFlag, m_iwvQuality, m_iwvQualityExplanation, iwvFlag);
	if(m_lwpTime.size() > 0)
		padDataToMatchHkp(m_hkdTime, m_lwpTime, m_lwp, m_lwpElevation, m_lwpAzimuth, m_lwpRainFlag, m_lwpQuality, m_lwpQualityExplanation, lwpFlag);

	if(m_iwvTime.size() > 0 && m_lwpTime.size() > 0)
		for(size_t i=0; i< m_iwv.size(); ++i)
			sci::assertThrow(m_iwvTime[i] == m_lwpTime[i], sci::err(sci::SERR_USER, 0, sU("IWV and LWP files have missmatched times.")));

	sci::GridData<sci::UtcTime, 1>& time = m_iwvTime.size() > 0 ? m_iwvTime : m_lwpTime;

	//get attitudes corrected for platform attitude
	sci::GridData<degreeF, 1> correctedElevations(m_lwpTime.size());
	sci::GridData<degreeF, 1> correctedAzimuths(m_lwpTime.size());
	for (size_t i = 0; i < time.size(); ++i)
	{
		sci::UtcTime startTime = i > 0 ? time[i - 1] : time[0] - (time[1] - time[0]);
		platform.correctDirection(startTime, time[i], m_lwpAzimuth[i], m_lwpElevation[i], correctedAzimuths[i], correctedElevations[i]);

	}
	
	MetAndStabilityFlags metAndStabilityflags(m_lwpRainFlag, m_status, m_temperatureStabilityReceiver1,
		m_temperatureStabilityReceiver2);
	sci::GridData<uint8_t, 1> minMaxFilter = metAndStabilityflags.getFilter();

	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
	OutputAmfNcFile file(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("hatpro microwave radiometer intergrated water vapour and liquid water path retrieval"),
		time, std::vector<sci::NcDimension*>(), sU("Retrievals: IWV_") + platform.getHatproRetrieval() + sU("LWP_") + platform.getHatproRetrieval());

	std::unique_ptr<AmfNcVariable<kilogramPerMetreSquaredF>> iwvVariable(nullptr);
	std::unique_ptr<AmfNcVariable<gramPerMetreSquaredF>> lwpVariable(nullptr);
	
	if(m_iwv.size() > 0 && sci::anyTrue(m_iwv==m_iwv))
		iwvVariable.reset(new AmfNcVariable<kilogramPerMetreSquaredF> (sU("integrated_water_vapor"), file, file.getTimeDimension(), sU("Integrated Water Vapour"), sU(""), m_iwv, true, coordinates, cellMethods, minMaxFilter, sU("")));
	if (m_lwp.size() > 0 && sci::anyTrue(m_lwp == m_lwp))
		lwpVariable.reset(new AmfNcVariable<gramPerMetreSquaredF>(sU("liquid_water_path"), file, file.getTimeDimension(), sU("Liquid Water Path"), sU(""), m_lwp, true, coordinates, cellMethods, minMaxFilter, sU("")));
	
	if (!iwvVariable && !lwpVariable)
	{
		file.close();
		wxRemoveFile(sci::nativeUnicode(file.getFileName()));
		progressReporter << "Removed file " << file.getFileName() << " as it contained no useful data.\n";
		return;
	}

	metAndStabilityflags.createNcFlags(file, false);


	file.writeTimeAndLocationData(platform);

	if (iwvVariable)
		file.write(*iwvVariable, m_iwv);
	if (lwpVariable)
		file.write(*lwpVariable, m_lwp);
	metAndStabilityflags.writeToNetCdf(file);
}

void MicrowaveRadiometerProcessor::writeSurfaceMetNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	if (m_metTime.size() == 0)
	{
		progressReporter << "No meteorology data to write.\n";
		return;
	}
	progressReporter << "Writing meteorology data file\n";

	DataInfo dataInfo = buildDataInfo(sU("surface met"), m_metTime, FeatureType::timeSeries, processingOptions);

	//assign amf version based on if we are moving or not.
	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;

	MetFlags metFlags(m_enviromentTemperature.size());
	sci::GridData<uint8_t, 1> minMaxFilter = metFlags.getFilter();

	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
	OutputAmfNcFile file(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("hatpro surface met data"), m_metTime);

	std::unique_ptr<AmfNcVariable<hectoPascalF>> pressureVariable(nullptr);
	std::unique_ptr<AmfNcVariable<kelvinF>> temperatureVariable(nullptr);
	std::unique_ptr<AmfNcVariable<percentF>> relativeHumidityVariable(nullptr);

	pressureVariable.reset(new AmfNcVariable<hectoPascalF>(sU("air_pressure"), file, file.getTimeDimension(), sU("Air Pressure"), sU("air_pressure"), m_enviromentPressure, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	temperatureVariable.reset(new AmfNcVariable<kelvinF>(sU("air_temperature"), file, file.getTimeDimension(), sU("Air Temperture"), sU("air_temperature"), m_enviromentTemperature, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	relativeHumidityVariable.reset(new AmfNcVariable<percentF>(sU("relative_humidity"), file, file.getTimeDimension(), sU("Relative Humidity"), sU("relative_humidity"), m_enviromentRelativeHumidity, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));

	metFlags.createNcFlags(file, true);


	file.writeTimeAndLocationData(platform);

	file.write(*pressureVariable, m_enviromentPressure);
	file.write(*temperatureVariable, m_enviromentTemperature);
	file.write(*relativeHumidityVariable, m_enviromentRelativeHumidity);
	metFlags.writeToNetCdf(file);
}

void MicrowaveRadiometerProcessor::writeStabilityNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	if (m_staTime.size() == 0)
	{
		progressReporter << "No stability data to write.\n";
		return;
	}
	progressReporter << "Writing stability data file\n";

	if (m_liftedIndex.size() == 0 && m_kModifiedIndex.size() == 0 && m_totalTotalsIndex.size() == 0 && m_kIndex.size() == 0
		&& m_showalterIndex.size() == 0 && m_cape.size() == 0)
		return;

	DataInfo dataInfo = buildDataInfo(sU("stability indices"), m_staTime, FeatureType::timeSeries, processingOptions);

	//assign amf version based on if we are moving or not.
	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;

	MetAndStabilityFlags metAndStabilityflags(m_staTime, m_hkdTime, m_staRainFlag, m_status, m_temperatureStabilityReceiver1,
		m_temperatureStabilityReceiver2);
	sci::GridData<uint8_t, 1> minMaxFilter = metAndStabilityflags.getFilter();

	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
	OutputAmfNcFile file(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("hatpro microwave radiometer intergrated water vapour and liquid water path retrieval"), m_staTime,
		std::vector<sci::NcDimension*>(), sU("Retrieval: STA_") + platform.getHatproRetrieval());

	std::unique_ptr<AmfNcVariable<kelvinF>> liftedIndexVariable(nullptr);
	std::unique_ptr<AmfNcVariable<kelvinF>> kModifiedIndexVariable(nullptr);
	std::unique_ptr<AmfNcVariable<kelvinF>> totalTotalsIndexVariable(nullptr);
	std::unique_ptr<AmfNcVariable<kelvinF>> kIndexVariable(nullptr);
	std::unique_ptr<AmfNcVariable<kelvinF>> showalterIndexVariable(nullptr);
	std::unique_ptr<AmfNcVariable<joulePerKilogramF>> capeVariable(nullptr);

	if (m_liftedIndex.size() > 0)
		liftedIndexVariable.reset(new AmfNcVariable<kelvinF>(sU("atmosphere_stability_lifted_index"), file, file.getTimeDimension(), sU("Atmosphere Stability Lifted Index (LI)"), sU(""), m_liftedIndex, true, coordinates, cellMethods, minMaxFilter, sU("")));
	if (m_kModifiedIndex.size() > 0)
		kModifiedIndexVariable.reset(new AmfNcVariable<kelvinF>(sU("modified_atmosphere_stability_k_index"), file, file.getTimeDimension(), sU("Modified Atmosphere Stability K Index (KOI)"), sU(""), m_kModifiedIndex, true, coordinates, cellMethods, minMaxFilter, sU("")));
	if (m_totalTotalsIndex.size() > 0)
		totalTotalsIndexVariable.reset(new AmfNcVariable<kelvinF>(sU("atmosphere_stability_total_totals_index"), file, file.getTimeDimension(), sU("Atmosphere Stability Total Totals Index (TTI)"), sU("atmosphere_stability_total_totals_index"), m_totalTotalsIndex, true, coordinates, cellMethods, minMaxFilter, sU("")));
	if (m_kIndex.size() > 0)
		kIndexVariable.reset(new AmfNcVariable<kelvinF>(sU("atmosphere_stability_k_index"), file, file.getTimeDimension(), sU("Atmosphere Stability K Index (KI)"), sU("atmosphere_stability_k_index"), m_kIndex, true, coordinates, cellMethods, minMaxFilter, sU("")));
	if (m_showalterIndex.size() > 0)
		showalterIndexVariable.reset(new AmfNcVariable<kelvinF>(sU("atmosphere_stability_showalter_index"), file, file.getTimeDimension(), sU("Atmosphere Stability Showalter Index (SI)"), sU("atmosphere_stability_showalter_index"), m_showalterIndex, true, coordinates, cellMethods, minMaxFilter, sU("")));
	if (m_cape.size() > 0)
		capeVariable.reset(new AmfNcVariable<joulePerKilogramF>(sU("atmosphere_convective_available_potential_energy"), file, file.getTimeDimension(), sU("Atmosphere Convective Available Potential Energy (CAPE)"), sU("atmosphere_convective_available_potential_energy"), m_cape, true, coordinates, cellMethods, minMaxFilter, sU("")));
	

	metAndStabilityflags.createNcFlags(file, false);


	file.writeTimeAndLocationData(platform);

	if (liftedIndexVariable)
		file.write(*liftedIndexVariable, m_liftedIndex);
	if (kModifiedIndexVariable)
		file.write(*kModifiedIndexVariable, m_kModifiedIndex);
	if (totalTotalsIndexVariable)
		file.write(*totalTotalsIndexVariable, m_totalTotalsIndex);
	if (kIndexVariable)
		file.write(*kIndexVariable, m_kIndex);
	if (showalterIndexVariable)
		file.write(*showalterIndexVariable, m_showalterIndex);
	if (capeVariable)
		file.write(*capeVariable, m_cape);

	metAndStabilityflags.writeToNetCdf(file);
}

void MicrowaveRadiometerProcessor::writeBrightnessTemperatureNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	if (m_brtTime.size() == 0)
	{
		progressReporter << "No brightness temperature data to write.\n";
		return;
	}
	progressReporter << "Writing brightness temperature data file\n";

	sci::assertThrow(m_brtFrequencies.size() == m_atnFrequencies.size(), sci::err(sci::SERR_USER, 0, sU("BRT and ATN data have a different number of frequencies")));

	DataInfo dataInfo = buildDataInfo(sU("brightness temperature"), m_brtTime, FeatureType::timeSeries, processingOptions);

	//assign amf version based on if we are moving or not.
	AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;

	MetAndStabilityFlags metAndStabilityflags(m_brtTime, m_hkdTime, m_brtRainFlag, m_status, m_temperatureStabilityReceiver1,
		m_temperatureStabilityReceiver2);
	sci::GridData<uint8_t, 1> minMaxFilter = metAndStabilityflags.getFilter();

	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension frequencyDimension(sU("radiation_frequency"), m_brtFrequencies.size());
	nonTimeDimensions.push_back(&frequencyDimension);
	OutputAmfNcFile file(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("hatpro brightness temperature data"), m_brtTime, nonTimeDimensions, sU("Retrieval: ATN_") + platform.getHatproRetrieval());

	std::unique_ptr<AmfNcVariable<perSecondF>> frequencyVariable(nullptr);
	std::unique_ptr<AmfNcVariable<kelvinF>> brightnessTemperatureVariable(nullptr);
	std::unique_ptr<AmfNcDbVariableFromLogarithmicData<unitlessF>> attenuationVariable(nullptr);

	frequencyVariable.reset(new AmfNcVariable<perSecondF>(sU("radiation_frequency"), file, frequencyDimension, sU("Receiver Channel Centre Frequency"), sU("radiation_frequency"), m_brtFrequencies, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	brightnessTemperatureVariable.reset(new AmfNcVariable<kelvinF>(sU("brightness_temperature"), file, { &file.getTimeDimension(), &frequencyDimension }, sU("Brightness Temperature"), sU("brightness_temperatue"), m_brightnessTemperature, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	//                                                         (const sci::string & name,     const sci::OutputNcFile & ncFile, const std::vector<sci::NcDimension*> &dimensions, const sci::string & longName,  const sci::string & standardName, sci::grid_view< REFERENCE_UNIT, NDATADIMS> dataLinear, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, bool isDbZ, bool outputReferenceUnit, sci::grid_view< uint8_t, NFLAGDIMS> flags, const sci::string & comment = sU(""))
	attenuationVariable.reset(new AmfNcDbVariableFromLogarithmicData<unitlessF>(sU("atmospheric_attenuation"), file, { &file.getTimeDimension(), &frequencyDimension }, sU("Atmospheric Attenuation"), sU(""), m_attenuation, true, coordinates, cellMethods, false, false, sci::GridData<uint8_t, 0>(1), sU("")));
	//attenuationVariable.reset(new AmfNcDbVariable<unitlessF>(sU("atmospheric_attenuation"), file, {&file.getTimeDimension(), &frequencyDimension }, sU("Atmospheric Attenuation"), sU(""), m_attenuation);
	
	metAndStabilityflags.createNcFlags(file, false);


	file.writeTimeAndLocationData(platform);

	file.write(*frequencyVariable, m_brtFrequencies);
	file.write(*brightnessTemperatureVariable, m_brightnessTemperature);
	file.write(*attenuationVariable, m_attenuation);
	metAndStabilityflags.writeToNetCdf(file);
}

void MicrowaveRadiometerProcessor::writeBoundaryLayerTemperatureProfileNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	if (m_tpbTime.size() == 0)
	{
		progressReporter << "No BL temperature profile data to write.\n";
		return;
	}
	progressReporter << "Writing BL temperature profile data file\n";

	sci::assertThrow(m_brtFrequencies.size() == m_atnFrequencies.size(), sci::err(sci::SERR_USER, 0, sU("BRT and ATN data have a different number of frequencies")));

	DataInfo dataInfo = buildDataInfo(sU("boundary layer temperature profiles"), m_tpbTime, FeatureType::timeSeriesProfile, processingOptions);

	//assign amf version based on if we are moving or not.
	//AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;
	AmfVersion amfVersion = AmfVersion::v1_1_0;

	MetAndStabilityFlags metAndStabilityflags(m_tpbTime, m_hkdTime, m_tpbRainFlag, m_status, m_temperatureStabilityReceiver1,
		m_temperatureStabilityReceiver2);
	sci::GridData<uint8_t, 1> minMaxFilter = metAndStabilityflags.getFilter(); //this is always fine as I never set this to anything other than good

	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension indexDimension(sU("index"), m_tpbAltitudes.size());
	nonTimeDimensions.push_back(&indexDimension);
	sci::GridData<metreF, 2> altitudes({m_tpbTime.size(), m_tpbAltitudes.size()}, metreF(0));
	for (size_t i = 0; i < altitudes.shape()[0]; ++i)
	{
		metreF instrumentAltitude;
		degreeF latitude;
		degreeF longitude;
		sci::UtcTime previousTime;
		if (i == 0)
			previousTime = m_tpbTime[i] - (m_tpbTime[i + 1] - m_tpbTime[i]);
		else
			previousTime = m_tpbTime[i - 1];
		platform.getLocation(previousTime, m_tpbTime[i], latitude, longitude, instrumentAltitude);
		for (size_t j = 0; j < altitudes.shape()[1]; ++j)
		{
			altitudes[i][j] = m_tpbAltitudes[j] + instrumentAltitude;
		}
	}

	OutputAmfNcFile file(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("hatpro microwave radiometer boundary layer temperature profile data"), m_tpbTime, nonTimeDimensions,
		sU("Retrieval: TPB_") + platform.getHatproRetrieval());

	std::unique_ptr<AmfNcVariable<metreF>> altitudeVariable(nullptr);
	std::unique_ptr<AmfNcVariable<kelvinF>> temperatureVariable(nullptr);

	altitudeVariable.reset(new AmfNcVariable<metreF>(sU("altitude"), file, { &file.getTimeDimension(), &indexDimension }, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	temperatureVariable.reset(new AmfNcVariable<kelvinF>(sU("air_temperature"), file, { &file.getTimeDimension(), &indexDimension }, sU("Air Temperature"), sU("air_temperature"), m_tpbTemperatures, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	
	metAndStabilityflags.createNcFlags(file, false);


	file.writeTimeAndLocationData(platform);

	file.write(*altitudeVariable, altitudes);
	file.write(*temperatureVariable, m_tpbTemperatures);
	metAndStabilityflags.writeToNetCdf(file);
}

void MicrowaveRadiometerProcessor::writeFullTroposphereTemperatureProfileNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	if (m_tpcTime.size() == 0)
	{
		progressReporter << "No troposphere temperature profile data to write.\n";
		return;
	}
	progressReporter << "Writing tropospher temperature profile data file\n";

	DataInfo dataInfo = buildDataInfo(sU("full troposphere temperature profiles"), m_tpcTime, FeatureType::timeSeriesProfile, processingOptions);

	//assign amf version based on if we are moving or not.
	//AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;
	AmfVersion amfVersion = AmfVersion::v1_1_0;

	MetAndStabilityFlags metAndStabilityflags(m_tpcTime, m_hkdTime, m_tpcRainFlag, m_status, m_temperatureStabilityReceiver1,
		m_temperatureStabilityReceiver2);
	sci::GridData<uint8_t, 1> minMaxFilter = metAndStabilityflags.getFilter(); //this is always fine as I never set this to anything other than good

	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension indexDimension(sU("index"), m_tpcAltitudes.size());
	nonTimeDimensions.push_back(&indexDimension);
	sci::GridData<metreF, 2> altitudes({ m_tpcTime.size(), m_tpcAltitudes.size() }, metreF(0));
	for (size_t i = 0; i < altitudes.shape()[0]; ++i)
	{
		metreF instrumentAltitude;
		degreeF latitude;
		degreeF longitude;
		sci::UtcTime previousTime;
		if (i == 0)
			previousTime = m_tpcTime[i] - (m_tpcTime[i + 1] - m_tpcTime[i]);
		else
			previousTime = m_tpcTime[i - 1];
		platform.getLocation(previousTime, m_tpcTime[i], latitude, longitude, instrumentAltitude);
		for (size_t j = 0; j < altitudes.shape()[1]; ++j)
		{
			altitudes[i][j] = m_tpcAltitudes[j] + instrumentAltitude;
		}
	}

	OutputAmfNcFile file(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("hatpro microwave radiometer full troposphere temperature profile data"), m_tpcTime, nonTimeDimensions,
		sU("Retrieval: TPT_") + platform.getHatproRetrieval());

	std::unique_ptr<AmfNcVariable<metreF>> altitudeVariable(nullptr);
	std::unique_ptr<AmfNcVariable<kelvinF>> temperatureVariable(nullptr);

	altitudeVariable.reset(new AmfNcVariable<metreF>(sU("altitude"), file, { &file.getTimeDimension(), &indexDimension }, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	temperatureVariable.reset(new AmfNcVariable<kelvinF>(sU("air_temperature"), file, { &file.getTimeDimension(), &indexDimension }, sU("Air Temperature"), sU("air_temperature"), m_tpcTemperatures, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));

	metAndStabilityflags.createNcFlags(file, false);


	file.writeTimeAndLocationData(platform);

	file.write(*altitudeVariable, altitudes);
	file.write(*temperatureVariable, m_tpcTemperatures);
	metAndStabilityflags.writeToNetCdf(file);
}

void MicrowaveRadiometerProcessor::writeMoistureProfileNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	if (m_hpcTime.size() == 0)
	{
		progressReporter << "No moisture profile data to write.\n";
		return;
	}
	progressReporter << "Writing moisture profile data file\n";

	DataInfo dataInfo = buildDataInfo(sU("moisture profiles"), m_hpcTime, FeatureType::timeSeriesProfile, processingOptions);

	//assign amf version based on if we are moving or not.
	//AmfVersion amfVersion = platform.getPlatformInfo().platformType == PlatformType::moving ? AmfVersion::v1_1_0 : AmfVersion::v2_0_0;
	AmfVersion amfVersion = AmfVersion::v1_1_0;

	MetAndStabilityFlags metAndStabilityflags(m_hpcTime, m_hkdTime, m_hpcRainFlag, m_status, m_temperatureStabilityReceiver1,
		m_temperatureStabilityReceiver2);
	sci::GridData<uint8_t, 1> minMaxFilter = metAndStabilityflags.getFilter(); //this is always fine as I never set this to anything other than good

	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension indexDimension(sU("index"), m_hpcAltitudes.size());
	nonTimeDimensions.push_back(&indexDimension);
	sci::GridData<metreF, 2> altitudes({ m_hpcTime.size(), m_hpcAltitudes.size() }, metreF(0));
	for (size_t i = 0; i < altitudes.shape()[0]; ++i)
	{
		metreF instrumentAltitude;
		degreeF latitude;
		degreeF longitude;
		sci::UtcTime previousTime;
		if (i == 0)
			previousTime = m_hpcTime[i] - (m_hpcTime[i + 1] - m_hpcTime[i]);
		else
			previousTime = m_hpcTime[i - 1];
		platform.getLocation(previousTime, m_hpcTime[i], latitude, longitude, instrumentAltitude);
		for (size_t j = 0; j < altitudes.shape()[1]; ++j)
		{
			altitudes[i][j] = m_hpcAltitudes[j] + instrumentAltitude;
		}
	}

	OutputAmfNcFile file(amfVersion, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("hatpro microwave radiometer moisture profile data"), m_hpcTime, nonTimeDimensions,
		sU("Retrievals: HPT_") + platform.getHatproRetrieval() + sU("RHP_") + platform.getHatproRetrieval());

	std::unique_ptr<AmfNcVariable<metreF>> altitudeVariable(nullptr);
	std::unique_ptr<AmfNcVariable<gramPerMetreCubedF>> absoluteHumidityVariable(nullptr);
	std::unique_ptr<AmfNcVariable<percentF>> relativeHumidityVariable(nullptr);

	altitudeVariable.reset(new AmfNcVariable<metreF>(sU("altitude"), file, { &file.getTimeDimension(), &indexDimension }, sU("Geometric height above geoid (WGS84)"), sU("altitude"), altitudes, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	if(sci::anyTrue(m_hpcAbsoluteHumidity == m_hpcAbsoluteHumidity))
		absoluteHumidityVariable.reset(new AmfNcVariable<gramPerMetreCubedF>(sU("absolute_humidity"), file, { &file.getTimeDimension(), &indexDimension }, sU("Absolute Humidity"), sU(""), m_hpcAbsoluteHumidity, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));
	if (sci::anyTrue(m_hpcRelativeHumidity == m_hpcRelativeHumidity))
		relativeHumidityVariable.reset(new AmfNcVariable<percentF>(sU("relative_humidity"), file, { &file.getTimeDimension(), &indexDimension }, sU("Relative Humidity"), sU("relative_humidity"), m_hpcRelativeHumidity, true, coordinates, cellMethods, sci::GridData<uint8_t, 0>(1), sU("")));

	metAndStabilityflags.createNcFlags(file, false);


	file.writeTimeAndLocationData(platform);

	file.write(*altitudeVariable, altitudes);
	if(absoluteHumidityVariable)
		file.write(*absoluteHumidityVariable, m_hpcAbsoluteHumidity);
	if(relativeHumidityVariable)
		file.write(*relativeHumidityVariable, m_hpcRelativeHumidity);
	metAndStabilityflags.writeToNetCdf(file);
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