#include"Lidar.h"
#include<wx/regex.h>

void LidarDepolProcessor::readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter)
{
	std::vector<sci::string> crossFileNames;
	std::vector<sci::string> coFileNames;
	crossFileNames.reserve(inputFilenames.size());
	coFileNames.reserve(inputFilenames.size());
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		if (isCrossFile(inputFilenames[i]))
			crossFileNames.push_back(inputFilenames[i]);
		else
			coFileNames.push_back(inputFilenames[i]);
	}
	m_crosspolarisedProcessor.readData(crossFileNames, platform, progressReporter);
	m_copolarisedProcessor.readData(coFileNames, platform, progressReporter);
}

bool LidarDepolProcessor::isCrossFile(const sci::string &fileName) const
{
	sci::string regEx = m_crosspolarisedProcessor.getFileSearchRegex();
	wxRegEx regularExpression = sci::nativeUnicode( regEx );
	sci::assertThrow(regularExpression.IsValid(), sci::err(sci::SERR_USER, 0, sci::string(sU("Invalid regex:")) + regEx));
	return (regularExpression.Matches(sci::nativeUnicode(fileName)));
}

std::vector<std::vector<sci::string>> LidarDepolProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	//just pass these all to the copolarised processor - the process to check the day is the same
	//for both files, but it is way easier to just send all the files to one processor
	return m_copolarisedProcessor.groupFilesPerDayForReprocessing(newFiles, allFiles);
}

void LidarDepolProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	throw(sci::err(sci::SERR_USER, 0, sU("Plotting lidar depolarisation is not currently supported")));
}

void LidarDepolProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	DataInfo dataInfo;
	dataInfo.continuous = true;
	dataInfo.samplingInterval = std::numeric_limits<second>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.averagingPeriod = std::numeric_limits<second>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.startTime = m_copolarisedProcessor.getTimesUtcTime()[0];
	dataInfo.endTime = m_copolarisedProcessor.getTimesUtcTime().back();
	dataInfo.featureType = ft_timeSeriesProfile;
	dataInfo.processingLevel = 1;
	dataInfo.options = {};
	dataInfo.productName = sU("depolarisation ratio");
	dataInfo.processingOptions = processingOptions;

	std::vector<sci::UtcTime> timesCo;
	std::vector<degree> instrumentRelativeAzimuthAnglesCo;
	std::vector<degree> instrumentRelativeElevationAnglesCo;
	std::vector<std::vector<metrePerSecond>> instrumentRelativeDopplerVelocitiesCo;
	std::vector<degree> attitudeCorrectedAzimuthAnglesCo;
	std::vector<degree> attitudeCorrectedElevationAnglesCo;
	std::vector<std::vector<metrePerSecond>> motionCorrectedDopplerVelocitiesCo;
	std::vector<std::vector<perSteradianPerMetre>> backscattersCo;
	std::vector<std::vector<unitless>> snrPlusOneCo;
	std::vector<std::vector<uint8_t>> dopplerVelocityFlagsCo;
	std::vector<std::vector<uint8_t>> backscatterFlagsCo;
	std::vector<std::vector<metre>> rangesCo;
	second averagingPeriodCo;
	second samplingIntervalCo;

	std::vector<sci::UtcTime> timesCross;
	std::vector<degree> instrumentRelativeAzimuthAnglesCross;
	std::vector<degree> instrumentRelativeElevationAnglesCross;
	std::vector<std::vector<metrePerSecond>> instrumentRelativeDopplerVelocitiesCross;
	std::vector<degree> attitudeCorrectedAzimuthAnglesCross;
	std::vector<degree> attitudeCorrectedElevationAnglesCross;
	std::vector<std::vector<metrePerSecond>> motionCorrectedDopplerVelocitiesCross;
	std::vector<std::vector<perSteradianPerMetre>> backscattersCross;
	std::vector<std::vector<unitless>> snrPlusOneCross;
	std::vector<std::vector<uint8_t>> dopplerVelocityFlagsCross;
	std::vector<std::vector<uint8_t>> backscatterFlagsCross;
	std::vector<std::vector<metre>> rangesCross;
	second averagingPeriodCross;
	second samplingIntervalCross;

	m_copolarisedProcessor.getFormattedData(timesCo, instrumentRelativeAzimuthAnglesCo, instrumentRelativeElevationAnglesCo, instrumentRelativeDopplerVelocitiesCo,
		attitudeCorrectedAzimuthAnglesCo, attitudeCorrectedElevationAnglesCo, motionCorrectedDopplerVelocitiesCo, backscattersCo,
		snrPlusOneCo, dopplerVelocityFlagsCo, backscatterFlagsCo, rangesCo, averagingPeriodCo, samplingIntervalCo);

	m_crosspolarisedProcessor.getFormattedData(timesCross, instrumentRelativeAzimuthAnglesCross, instrumentRelativeElevationAnglesCross, instrumentRelativeDopplerVelocitiesCross,
		attitudeCorrectedAzimuthAnglesCross, attitudeCorrectedElevationAnglesCross, motionCorrectedDopplerVelocitiesCross, backscattersCross,
		snrPlusOneCross, dopplerVelocityFlagsCross, backscatterFlagsCross, rangesCross, averagingPeriodCross, samplingIntervalCross);

	//We are going to fix this at 10.0 s for now
	dataInfo.averagingPeriod = second(10.0);
	dataInfo.samplingInterval = (samplingIntervalCo + samplingIntervalCross) / unitless(2);

	//We should have one co for each cross and the co should be jsut before the cross.
	//go through the data and check this is the case. removing any unpaired data.
	//Unpaired data would be the result of e.g. powering down the lidar.
	for(size_t i=0; i<std::max(timesCo.size(), timesCross.size()); ++i)
	{
		
		//This code checks for missing co data. If it finds it
		//then it inserts data to match.
		if (i==timesCo.size() || timesCo[i] > timesCross[i])
		{
			//this is a really slow way to insert data if there are many missing points,
			//but actually I can't think of any case where we would end up with missing
			//the copolarised data, unless we totally messed up the files
			timesCo.insert(timesCross.begin() + i, timesCross[i]);
			instrumentRelativeAzimuthAnglesCo.insert(instrumentRelativeAzimuthAnglesCo.begin() + i, instrumentRelativeAzimuthAnglesCross[i]);
			instrumentRelativeElevationAnglesCo.insert(instrumentRelativeElevationAnglesCo.begin() + i, instrumentRelativeElevationAnglesCross[i]);
			attitudeCorrectedAzimuthAnglesCo.insert(attitudeCorrectedAzimuthAnglesCo.begin() + i, attitudeCorrectedAzimuthAnglesCross[i]);
			attitudeCorrectedElevationAnglesCo.insert(attitudeCorrectedElevationAnglesCo.begin() + i, attitudeCorrectedElevationAnglesCross[i]);
			backscattersCo.insert(backscattersCo.begin() + i, std::vector<perSteradianPerMetre>(backscattersCross.size(), std::numeric_limits<perSteradianPerMetre>::quiet_NaN()));
			snrPlusOneCo.insert(snrPlusOneCo.begin() + i, std::vector<unitless>(backscattersCross.size(), std::numeric_limits<unitless>::quiet_NaN()));
			backscatterFlagsCo.insert(backscatterFlagsCo.begin() + i, std::vector<uint8_t>(backscatterFlagsCross.size(), lidarPaddedBackscatter));
			rangesCo.insert(rangesCo.begin() + i, rangesCross[i]);
		}
		
		if (i<timesCo.size()-1)
		{
			//This code checks for missing cross data and if it finds any
			//then it inserts padding data. Note that here we check for multiple
			//contiguous missing profiles as it might be that cross polarised data
			//was turned off for a time and it is very slow to add lots of data one
			//element at a time, part way through a data set
			size_t nToInsert = 0;
			if (i == timesCross.size())
				nToInsert = timesCo.size() - timesCross.size();
			else
			{
				while (timesCross[i] > timesCo[i + nToInsert + 1])
				{
					++nToInsert;
					if (i + nToInsert + 1 == timesCo.size())
						break;
				}
			}
			if(nToInsert > 0)
			{
				timesCross.insert(timesCross.begin() + i, timesCo.begin() + i, timesCo.begin() + i + nToInsert);
				instrumentRelativeAzimuthAnglesCross.insert(instrumentRelativeAzimuthAnglesCross.begin() + i, instrumentRelativeAzimuthAnglesCo.begin() + i, instrumentRelativeAzimuthAnglesCo.begin() + i + nToInsert);
				instrumentRelativeElevationAnglesCross.insert(instrumentRelativeElevationAnglesCross.begin() + i, instrumentRelativeElevationAnglesCo.begin() + i, instrumentRelativeElevationAnglesCo.begin() + i + nToInsert);
				attitudeCorrectedAzimuthAnglesCross.insert(attitudeCorrectedAzimuthAnglesCross.begin() + i, attitudeCorrectedAzimuthAnglesCo.begin() + i, attitudeCorrectedAzimuthAnglesCo.begin() + i + nToInsert);
				attitudeCorrectedElevationAnglesCross.insert(attitudeCorrectedElevationAnglesCross.begin() + i, attitudeCorrectedElevationAnglesCo.begin() + i, attitudeCorrectedElevationAnglesCo.begin() + i + nToInsert);
				rangesCross.insert(rangesCross.begin() + i, rangesCo.begin() + i, rangesCo.begin() + i + nToInsert);

				backscattersCross.insert(backscattersCross.begin() + i, nToInsert, std::vector<perSteradianPerMetre>(0));
				snrPlusOneCross.insert(snrPlusOneCross.begin() + i, nToInsert, std::vector<unitless>(0));
				backscatterFlagsCross.insert(backscatterFlagsCross.begin() + i, nToInsert, std::vector<uint8_t> (0));
				for (size_t j = i; j < i + nToInsert; ++j)
				{
					backscattersCross[j].resize(backscattersCo[j].size(), std::numeric_limits<perSteradianPerMetre>::quiet_NaN());
					snrPlusOneCross[j].resize(snrPlusOneCo[j].size(), std::numeric_limits<unitless>::quiet_NaN());
					backscatterFlagsCross[j].resize(backscatterFlagsCo[j].size(), lidarPaddedBackscatter);
				}
			}
			//no need to check all the data we just added
			if(nToInsert > 0)
				i += nToInsert-1;
		}
	}

	//check that the number of range gates in the co and cross data are the same
	if (rangesCo.size() > 0 && rangesCo[0].size() != rangesCross[0].size())
	{
		size_t maxGates = std::max(rangesCo[0].size(), rangesCross[0].size());
		for (size_t i = 0; i < timesCo.size(); ++i)
		{
			backscattersCross[i].resize(maxGates, std::numeric_limits<perSteradianPerMetre>::quiet_NaN());
			snrPlusOneCross[i].resize(maxGates, std::numeric_limits<unitless>::quiet_NaN());
			backscatterFlagsCross[i].resize(maxGates, lidarPaddedBackscatter);
			backscattersCo[i].resize(maxGates, std::numeric_limits<perSteradianPerMetre>::quiet_NaN());
			snrPlusOneCo[i].resize(maxGates, std::numeric_limits<unitless>::quiet_NaN());
			backscatterFlagsCo[i].resize(maxGates, lidarPaddedBackscatter);
		}
	}
	//Check that the values of the ranges and the angles are the same for cross and co data
	for (size_t i = 0; i < rangesCo.size(); ++i)
	{
		bool goodProfile = true;
		goodProfile = goodProfile && sci::abs(instrumentRelativeAzimuthAnglesCo[i] - instrumentRelativeAzimuthAnglesCross[i]) < degree(0.01f);
		goodProfile = goodProfile && sci::abs(instrumentRelativeElevationAnglesCo[i] - instrumentRelativeElevationAnglesCross[i]) < degree(0.01f);
		for (size_t j = 0; j < rangesCo[i].size(); ++j)
		{
			goodProfile = goodProfile && sci::abs(rangesCo[i][j] - rangesCross[i][j]) < metre(0.001f);
		}
		if (!goodProfile)
		{
			size_t nGates = rangesCo[i].size();
			backscattersCross[i].assign(nGates, std::numeric_limits<perSteradianPerMetre>::quiet_NaN());
			snrPlusOneCross[i].assign(nGates, std::numeric_limits<unitless>::quiet_NaN());
			backscatterFlagsCross[i].assign(nGates, lidarNonMatchingRanges);
			backscattersCo[i].assign(nGates, std::numeric_limits<perSteradianPerMetre>::quiet_NaN());
			snrPlusOneCo[i].assign(nGates, std::numeric_limits<unitless>::quiet_NaN());
			backscatterFlagsCo[i].assign(nGates, lidarNonMatchingRanges);
		}
	}

	//average over an appropriate time period. For now we're going with 10s
	std::vector<sci::UtcTime> averagedTime;
	std::vector<std::vector<metre>> averagedRanges;
	std::vector< std::vector<perSteradianPerMetre>> averagedBackscatterCross;
	std::vector< std::vector<perSteradianPerMetre>> averagedBackscatterCo;
	std::vector<std::vector<unitless>> depolarisation;
	std::vector<std::vector<uint8_t>> averagedFlagsCross;
	std::vector<std::vector<uint8_t>> averagedFlagsCo;
	std::vector<std::vector<uint8_t>> depolarisationFlags;
	std::vector<degree> instrumentRelativeAzimuthAngles;
	std::vector<degree> instrumentRelativeElevationAngles;
	std::vector<degree> attitudeCorrectedAzimuthAngles;
	std::vector<degree> attitudeCorrectedElevationAngles;
	std::vector<second> integrationTimes;

	averagedTime.reserve(backscattersCross.size());
	averagedRanges.reserve(backscattersCross.size());
	averagedBackscatterCross.reserve(backscattersCross.size());
	averagedBackscatterCo.reserve(backscattersCross.size());
	depolarisation.reserve(backscattersCross.size());
	averagedFlagsCross.reserve(backscattersCross.size());
	averagedFlagsCo.reserve(backscattersCross.size());
	depolarisationFlags.reserve(backscattersCross.size());
	instrumentRelativeAzimuthAngles.reserve(attitudeCorrectedAzimuthAnglesCo.size());
	instrumentRelativeElevationAngles.reserve(attitudeCorrectedAzimuthAnglesCo.size());
	attitudeCorrectedAzimuthAngles.reserve(attitudeCorrectedAzimuthAnglesCo.size());
	attitudeCorrectedElevationAngles.reserve(attitudeCorrectedElevationAnglesCo.size());
	integrationTimes.reserve(backscattersCross.size());

	{
		//put this inside its own scope to hide the i variable from the rest of the code
		size_t i = 0;
		while(i<backscattersCross.size()-1)
		{
			sci::UtcTime startTime = timesCo[i];
			std::vector<metre> startRanges = rangesCo[i];
			degree startInstrumentRelativeAzimuthAngles = instrumentRelativeAzimuthAnglesCo[i];
			degree startInstrumentRelativeElevationAngles = instrumentRelativeElevationAnglesCo[i];
			std::vector<perSteradianPerMetre> crossSum(backscattersCross[i].size(), perSteradianPerMetre(0.0));
			std::vector<perSteradianPerMetre> coSum(backscattersCross[i].size(), perSteradianPerMetre(0.0));
			std::vector<uint8_t> worstFlagCo(backscattersCross[i].size(), 1);
			std::vector<uint8_t> worstFlagCr(backscattersCross[i].size(), 1);
			std::vector<uint8_t> worstFlagDepol(backscattersCross[i].size(), 1);
			degree instrumentRelativeAverageAzimuth(0.0);
			degree instrumentRelativeAverageElevation(0.0);
			degree attitudeCorrectedAverageAzimuth(0.0);
			degree attitudeCorrectedAverageElevation(0.0);
			bool goodAverage = true;
			size_t count = 0;
			second integrationTime(0.0);
			while (i < backscattersCross.size() - 1 && integrationTime < dataInfo.averagingPeriod )
			{
				if (count > 0)
				{
					if (backscattersCross[i].size() != backscattersCross[i - count].size() //check for a change in number of range gates
						|| sci::abs(rangesCo[i][0] - rangesCo[i - count][0]) > metre(0.1) //check for change in range resolution
						|| sci::abs(rangesCross[i][0]-rangesCross[i-count][0]) > metre(0.1) //check for change in range resolution
						|| sci::abs(instrumentRelativeAzimuthAnglesCo[i]-startInstrumentRelativeAzimuthAngles) > degree(0.01f) //check for change in azimuth
						|| sci::abs(instrumentRelativeElevationAnglesCo[i] - startInstrumentRelativeElevationAngles) > degree(0.01f) //check for change in elevation
						|| (count >0 && timesCo[i+1]- timesCo[i] > dataInfo.samplingInterval*unitless(3))) //check for large gaps
					{
						goodAverage = false;
						break;
					}
				}
				integrationTime += (timesCross[i] -timesCo[i]) * unitless(2.0); //calculate the integration time this way because we know co and cross match, but there could be a gap co to co
				crossSum += backscattersCross[i];
				coSum += backscattersCross[i];
				for (size_t j = 0; j < worstFlagCo.size(); ++j)
				{
					worstFlagCo[j] = std::max(backscatterFlagsCo[i][j], worstFlagCo[j]);
					worstFlagCr[j] = std::max(backscatterFlagsCross[i][j], worstFlagCr[j]);
					worstFlagDepol[j] = std::max(worstFlagCr[j], worstFlagCo[j]);
				}
				attitudeCorrectedAverageAzimuth += (attitudeCorrectedAzimuthAnglesCo[i] + attitudeCorrectedAzimuthAnglesCross[i]) / unitless(2);
				attitudeCorrectedAverageElevation += (attitudeCorrectedElevationAnglesCo[i] + attitudeCorrectedElevationAnglesCross[i]) / unitless(2);
				++count;
				++i;
			}
			if (goodAverage)
			{
				averagedTime.push_back(startTime);
				averagedRanges.push_back(startRanges);
				depolarisation.push_back(std::vector<unitless>(crossSum.size()));
				averagedBackscatterCo.push_back(std::vector<perSteradianPerMetre>(crossSum.size()));
				averagedBackscatterCross.push_back(std::vector<perSteradianPerMetre>(crossSum.size()));
				for (size_t j = 0; j < crossSum.size(); ++j)
				{
					depolarisation.back()[j]=crossSum[j] / (crossSum[j] + coSum[j]);
					averagedBackscatterCo.back()[j] = coSum[j] / unitless(count);
					averagedBackscatterCross.back()[j] = crossSum[j] / unitless(count);
				}
				depolarisationFlags.push_back(worstFlagDepol);
				averagedFlagsCo.push_back(worstFlagCo);
				averagedFlagsCross.push_back(worstFlagCr);
				instrumentRelativeAzimuthAngles.push_back(startInstrumentRelativeAzimuthAngles);
				instrumentRelativeElevationAngles.push_back(startInstrumentRelativeElevationAngles);
				attitudeCorrectedAzimuthAngles.push_back(attitudeCorrectedAverageAzimuth / unitless(count));
				attitudeCorrectedElevationAngles.push_back(attitudeCorrectedAverageElevation / unitless(count));
				integrationTimes.push_back(integrationTime);
			}
		}
	}

	//check the median integration time
	std::vector<second> sortedIntegrationTimes = sci::sort(integrationTimes);
	dataInfo.averagingPeriod = sortedIntegrationTimes[sortedIntegrationTimes.size() / 2];

	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension rangeIndexDimension(sU("index_of_range"), rangesCo.size() > 0 ? rangesCo[0].size() : 0);
	nonTimeDimensions.push_back(&rangeIndexDimension);

	sci::stringstream comment;
	comment << sU("Doppler lidar depolorisation profiles, averaged for at least ") << dataInfo.averagingPeriod << sU(" if the ray time is shorter than this.");

	OutputAmfNcFile file(directory, m_copolarisedProcessor.getInstrumentInfo(), author, processingSoftwareInfo, m_copolarisedProcessor.getCalibrationInfo(), dataInfo,
		projectInfo, platform, comment.str(), averagedTime, nonTimeDimensions);

	//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), cm_mean}, { sU("range"), cm_mean } };
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ {sU("time"), cm_point} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), cm_mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAnglesEarthFrame{ {sU("time"), cm_mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsRange{ };
	std::vector<sci::string> coordinatesData{ sU("latitude"), sU("longitude") };
	std::vector<sci::string> coordinatesAngles{ sU("latitude"), sU("longitude") };
	std::vector<sci::string> coordinatesAnglesEarthFrame{ sU("latitude"), sU("longitude") };
	std::vector<sci::string> coordinatesRange{ sU("latitude"), sU("longitude") };

	AmfNcVariable<metre, decltype(rangesCo)> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), sU("range"), averagedRanges, true, coordinatesRange, cellMethodsRange);
	AmfNcVariable<degree, decltype(instrumentRelativeAzimuthAnglesCo)> azimuthVariable(sU("sensor_azimuth_angle"), file, file.getTimeDimension(), sU("Scanning Head Azimuth Angle"), sU("sensor_azimuth_angle"), instrumentRelativeAzimuthAngles, true, coordinatesAngles, cellMethodsAngles, sU("Relative to the instrument with 0 degrees being to the \"front\" of the instruments."));
	AmfNcVariable<degree, decltype(instrumentRelativeElevationAnglesCo)> elevationVariable(sU("sensor_view_angle"), file, file.getTimeDimension(), sU("Scanning Head Elevation Angle"), sU("sensor_view_angle"), instrumentRelativeElevationAngles, true, coordinatesAngles, cellMethodsAngles, sU("Relative to the instruments with 0 degrees being \"horizontal\", positive being upwards, negative being downwards."));
	AmfNcVariable<degree, decltype(attitudeCorrectedAzimuthAnglesCo)> azimuthVariableEarthFrame(sU("sensor_azimuth_angle_earth_frame"), file, file.getTimeDimension(), sU("Scanning Head Azimuth Angle Earth Frame"), sU(""), attitudeCorrectedAzimuthAngles, true, coordinatesAnglesEarthFrame, cellMethodsAnglesEarthFrame, sU("Relative to the geoid with 0 degrees being north."));
	AmfNcVariable<degree, decltype(attitudeCorrectedElevationAnglesCo)> elevationVariableEarthFrame(sU("sensor_view_angle_earth_frame"), file, file.getTimeDimension(), sU("Scanning Head Elevation Angle Earth Frame"), sU(""), attitudeCorrectedElevationAngles, true, coordinatesAnglesEarthFrame, cellMethodsAnglesEarthFrame, sU("Relative to the geoid with 0 degrees being horizontal, positive being upwards, negative being downwards."));
	AmfNcVariable<perSteradianPerMetre, decltype(backscattersCo)> backscatterCoVariable(sU("attenuated_aerosol_backscatter_coefficient_co"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Attenuated Aerosol Backscatter Coefficient (Planar Polarised)"), sU(""), averagedBackscatterCo, true, coordinatesData, cellMethodsData);
	//AmfNcVariable<unitless, decltype(snrPlusOneCo)> snrPlusOneCoVariable(sU("signal_to_noise_ratio_plus_1_co"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Signal to Noise Ratio: SNR+1 (Planar Polarised)"), sU(""), snrPlusOneCo, true, coordinatesData, cellMethodsData);
	AmfNcVariable<perSteradianPerMetre, decltype(backscattersCross)> backscatterCrossVariable(sU("attenuated_aerosol_backscatter_coefficient_cr"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Attenuated Aerosol Backscatter Coefficient (Cross Polarised)"), sU(""), averagedBackscatterCross, true, coordinatesData, cellMethodsData);
	//AmfNcVariable<unitless, decltype(snrPlusOneCross)> snrPlusOneCrossVariable(sU("signal_to_noise_ratio_plus_1_cr"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Signal to Noise Ratio: SNR+1 (Cross Polarised)"), sU(""), snrPlusOneCross, true, coordinatesData, cellMethodsData);
	AmfNcVariable<unitless, decltype(depolarisation)> depolarisationVariable(sU("depolarisation_ratio"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Depolarisation Ratio"), sU(""), depolarisation, true, coordinatesData, cellMethodsData,sU("attenuated_aerosol_backscatter_coefficient_cr / ( attenuated_aerosol_backscatter_coefficient_cr + attenuated_aerosol_backscatter_coefficient_co)"));
	AmfNcFlagVariable backscatterFlagCoVariable(sU("qc_flag_attenuated_aerosol_backscatter_coefficient_co"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension});
	AmfNcFlagVariable backscatterFlagCrossVariable(sU("qc_flag_attenuated_aerosol_backscatter_coefficient_cr"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension});
	AmfNcFlagVariable depolarisationFlagVariable(sU("qc_flag_depolarisation_ratio"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension});

	file.writeTimeAndLocationData(platform);

	file.write(rangeVariable);
	file.write(azimuthVariable);
	file.write(azimuthVariableEarthFrame);
	file.write(elevationVariable);
	file.write(elevationVariableEarthFrame);
	file.write(backscatterCoVariable);
	//file.write(snrPlusOneCoVariable);
	file.write(backscatterCrossVariable);
	//file.write(snrPlusOneCrossVariable);
	file.write(depolarisationVariable);
	file.write(backscatterFlagCoVariable, averagedFlagsCo);
	file.write(backscatterFlagCrossVariable, averagedFlagsCross);
	file.write(depolarisationFlagVariable, depolarisationFlags);
}