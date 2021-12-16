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
	}
	sci::GridData<sci::UtcTime, 1> time;
	sci::GridData<gramPerMetreSquaredF, 1> water;
	sci::GridData<degreeF, 1> elevation;
	sci::GridData<degreeF, 1> azimuth;
	sci::GridData<bool, 1> rainFlag;
	sci::GridData<unsigned char, 1> quality;
	sci::GridData<unsigned char, 1> qualityExplanation;
	uint32_t fileTypeId;

	if (inputFilename.substr(inputFilename.length() - 3) == sU("HKD"))
		readHatproHkdFile(inputFilename, m_hkdTime, m_latitude, m_longitude, m_ambientTarget1Temperature, m_ambientTarget2Temperature,
			m_humidityProfilerTemperature, m_temperatureProfilerTemperature, m_temperatureStabilityReceiver1, m_temperatureStabilityReceiver2,
			m_remainingMemory);
	else
		readHatproLwpOrIwvData(inputFilename, time, water, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);

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
		m_iwvTime.insert(m_lwpTime.size(), time);
		m_iwv.insert(m_iwv.size(), water);
		m_iwvElevation.insert(m_iwvElevation.size(), elevation);
		m_iwvAzimuth.insert(m_iwvAzimuth.size(), azimuth);
		m_iwvRainFlag.insert(m_iwvRainFlag.size(), rainFlag);
		m_iwvQuality.insert(m_iwvQuality.size(), quality);
		m_iwvQualityExplanation.insert(m_iwvQualityExplanation.size(), qualityExplanation);
	}

	m_hasData = m_lwpTime.size() > 0 || m_iwvTime.size() > 0;
}

void MicrowaveRadiometerProcessor::writeToNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
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
		intervals[i] = m_lwpTime[i + 1] - m_lwpTime[i];
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

	//convert the hatpro flags into amof flags
	sci::GridData<uint8_t, 1> lwpFlag(m_lwpTime.size());
	for (size_t i = 0; i < lwpFlag.size(); ++i)
		lwpFlag[i] = hatproToAmofFlag(m_lwpQuality[i], m_lwpQualityExplanation[i]);
	sci::GridData<uint8_t, 1> iwvFlag(m_iwvTime.size());
	for (size_t i = 0; i < iwvFlag.size(); ++i)
		iwvFlag[i] = hatproToAmofFlag(m_iwvQuality[i], m_iwvQualityExplanation[i]);

	//nan out any data theat is flagged not good or missing
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

	//check that the iwv and lwp data are the same size or that one is missing entirely so we can omit it
	sci::assertThrow(m_iwv.size() == 0 || m_lwp.size() == 0 || m_iwv.size() == m_lwp.size(), sci::err(sci::SERR_USER, 0, sU("IWV and LWP files have missmatch in data length.")));

	for(size_t i=0; i< m_iwv.size(); ++i)
		sci::assertThrow(m_iwvTime[i] == m_lwpTime[i], sci::err(sci::SERR_USER, 0, sU("IWV and LWP files have missmatched times.")));

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

	file.writeTimeAndLocationData(platform);

	if (iwvVariable)
		file.write(*iwvVariable, m_iwv);
	if (lwpVariable)
		file.write(*lwpVariable, m_lwp);
	
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