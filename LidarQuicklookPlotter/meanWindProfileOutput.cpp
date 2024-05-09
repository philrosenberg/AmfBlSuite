#define NO_MIN_MAX
#include"AmfNc.h"
#include"Lidar.h"

void WindProfileProcessor::writeToNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter,
	const InstrumentInfo &instrumentInfo, const CalibrationInfo &calibrationInfo) const
{
	DataInfo dataInfo;
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo.continuous = true;
	dataInfo.startTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	dataInfo.endTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	if (m_profiles.size() > 0)
	{
		dataInfo.startTime = m_profiles[0].m_VadProcessor.getTimesUtcTime()[0];
		dataInfo.endTime = m_profiles.back().m_VadProcessor.getTimesUtcTime()[0];
	}
	dataInfo.featureType = FeatureType::timeSeriesProfile;
	dataInfo.options = std::vector<sci::string>(0);
	dataInfo.processingLevel = 2;
	dataInfo.productName = sU("mean winds profile");
	dataInfo.processingOptions = processingOptions;

	//get the start and end time for each wind profile retrieval
	sci::GridData<sci::UtcTime, 1> scanStartTimes(m_profiles.size());
	sci::GridData<sci::UtcTime, 1> scanEndTimes(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		scanStartTimes[i] = m_profiles[i].m_VadProcessor.getTimesUtcTime()[0];
		scanEndTimes[i] = m_profiles[i].m_VadProcessor.getTimesUtcTime().back() + (unitlessF((unitlessF::valueType)m_profiles[i].m_VadProcessor.getHeaderForProfile(0).pulsesPerRay * (unitlessF::valueType)m_profiles[i].m_VadProcessor.getHeaderForProfile(0).nRays) / sci::Physical<sci::Hertz<1, 3>, typename unitlessF::valueType>(15.0));//the second bit of this adds the duration of each ray
	}

	//work out the averaging time - this is the difference between the scan start and end times.
	//use the median as the value for the file
	if (scanStartTimes.size() > 0)
	{
		sci::GridData<sci::TimeInterval, 1> averagingTimes = scanEndTimes - scanStartTimes;
		sci::GridData< sci::TimeInterval, 1> sortedAveragingTimes = averagingTimes;
		std::sort(sortedAveragingTimes.begin(), sortedAveragingTimes.end());
		dataInfo.averagingPeriod = secondF(sortedAveragingTimes[sortedAveragingTimes.size() / 2]);
	}

	//work out the sampling interval - this is the difference between each start time.
	//use the median as the value for the file
	if (scanStartTimes.size() > 1)
	{
		std::vector<sci::TimeInterval> samplingIntervals = std::vector<sci::UtcTime>(scanStartTimes.begin() + 1, scanStartTimes.end()) - std::vector<sci::UtcTime>(scanStartTimes.begin(), scanStartTimes.end() - 1);
		std::vector < sci::TimeInterval> sortedSamplingIntervals = samplingIntervals;
		std::sort(sortedSamplingIntervals.begin(), sortedSamplingIntervals.end());
		dataInfo.samplingInterval = secondF(sortedSamplingIntervals[sortedSamplingIntervals.size() / 2]);
	}

	std::vector<sci::NcDimension*> nonTimeDimensions;

	sci::NcDimension indexDimension(sU("index"), m_profiles[0].m_heights.size());
	nonTimeDimensions.push_back(&indexDimension);
	std::vector<sci::NcAttribute*> lidarGlobalAttributes{ };
	OutputAmfNcFile file(AmfVersion::v1_1_0, directory, instrumentInfo, author, processingSoftwareInfo, calibrationInfo, dataInfo,
		projectInfo, platform, sU("doppler lidar wind profile"), scanStartTimes, nonTimeDimensions);

	size_t maxLevels = 0;
	for (size_t i = 0; i < m_profiles.size(); ++i)
		maxLevels = std::max(maxLevels, m_profiles[i].m_heights.size());

	if (m_profiles.size() > 0 && m_profiles[0].m_heights.size() > 0)
	{
		sci::GridData<metreF, 2> altitudes({ m_profiles.size(), maxLevels }, std::numeric_limits<metreF>::quiet_NaN());
		sci::GridData<metrePerSecondF, 2> windSpeeds({ m_profiles.size(), maxLevels }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		sci::GridData<degreeF, 2> windDirections({ m_profiles.size(), maxLevels }, std::numeric_limits<degreeF>::quiet_NaN());
		sci::GridData<uint8_t, 2> windFlags({ m_profiles.size(), maxLevels }, lidarMissingDataFlag);

		//copy data from profiles to the grid
		for (size_t i = 0; i < m_profiles.size(); ++i)
		{
			sci::assertThrow(m_profiles[i].m_heights.size() == altitudes.shape()[1], sci::err(sci::SERR_USER, 0, "Lidar wind profiles do not have the same number of altitude points. Aborting processing that day's data."));
			for (size_t j = 0; j < m_profiles[i].m_heights.size(); ++j)
			{
				altitudes[i][j] = m_profiles[i].m_heights[j];
				windSpeeds[i][j] = m_profiles[i].m_motionCorrectedWindSpeeds[j];
				windDirections[i][j] = m_profiles[i].m_motionCorrectedWindDirections[j];
				windFlags[i][j] = m_profiles[i].m_windFlags[j];
			}
		}
		//replace clipped data with nans
		for (size_t i = 0; i < m_profiles.size(); ++i)
		{
			if (*(windFlags.begin() + i) == lidarClippedWindProfileFlag)
			{
				*(windSpeeds.begin() + i) = std::numeric_limits<metrePerSecondF>::quiet_NaN();
				*(windDirections.begin() + i) = std::numeric_limits<degreeF>::quiet_NaN();
			}
		}
		//ensure we are on 0-360 range, not -180-180 range for wind directions
		for (auto& dir : windDirections)
			if (dir < degreeF(0))
				dir += degreeF(360);

		//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
		//std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), cm_mean}, { sU("altitude"), cm_mean } };
		std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
		std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
		AmfNcAltitudeVariable altitudeVariable(file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & indexDimension }, altitudes, FeatureType::timeSeriesProfile);
		AmfNcVariable<metrePerSecondF> windSpeedVariable(sU("wind_speed"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & indexDimension }, sU("Mean Wind Speed"), sU("wind_speed"), windSpeeds, true, coordinates, cellMethods, windFlags);
		AmfNcVariable<degreeF> windDirectionVariable(sU("wind_from_direction"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & indexDimension }, sU("Wind From Direction"), sU("wind_from_direction"), windDirections, true, coordinates, cellMethods, windFlags);
		AmfNcFlagVariable windFlagVariable(sU("qc_flag"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & indexDimension });

		file.writeTimeAndLocationData(platform);

		file.write(altitudeVariable, altitudes);
		file.write(windSpeedVariable, windSpeeds);
		file.write(windDirectionVariable, windDirections);
		file.write(windFlagVariable, windFlags);
	}
}