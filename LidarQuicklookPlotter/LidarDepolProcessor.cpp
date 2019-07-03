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

	dataInfo.averagingPeriod = (averagingPeriodCo + averagingPeriodCross) / unitless(2);
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
		goodProfile = goodProfile && sci::abs(instrumentRelativeAzimuthAnglesCo[i] - instrumentRelativeAzimuthAnglesCross[i]) < degree(0.01);
		goodProfile = goodProfile && sci::abs(instrumentRelativeElevationAnglesCo[i] - instrumentRelativeElevationAnglesCross[i]) < degree(0.01);
		for (size_t j = 0; j < rangesCo[i].size(); ++j)
		{
			goodProfile = goodProfile && sci::abs(rangesCo[i][j] - rangesCross[i][j]) < metre(0.001);
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

	std::vector<std::vector<unitless>> depolarisation(backscattersCross.size());
	std::vector<std::vector<uint8_t>> depolarisationFlags(backscatterFlagsCo.size());
	std::vector<degree> attitudeCorrectedAzimuthAngles(attitudeCorrectedAzimuthAnglesCo.size());
	std::vector<degree> attitudeCorrectedElevationAngles(attitudeCorrectedElevationAnglesCo.size());

	for (size_t i = 0; i < depolarisation.size(); ++i)
	{
		attitudeCorrectedAzimuthAngles[i] = (attitudeCorrectedAzimuthAnglesCo[i] + attitudeCorrectedAzimuthAnglesCross[i]) / unitless(2);
		attitudeCorrectedElevationAngles[i] = (attitudeCorrectedElevationAnglesCo[i] + attitudeCorrectedElevationAnglesCross[i]) / unitless(2);

		depolarisation[i].resize(backscattersCo[i].size());
		depolarisationFlags[i] = backscatterFlagsCo[i];
		for (size_t j = 0; j < depolarisation[i].size(); ++j)
		{
			depolarisation[i][j]=backscattersCross[i][j] / backscattersCo[i][j];
			depolarisationFlags[i][j] = std::max(depolarisationFlags[i][j], backscatterFlagsCross[i][j]);
		}
	}

	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension rangeIndexDimension(sU("index_of_range"), rangesCo.size() > 0 ? rangesCo[0].size() : 0);
	nonTimeDimensions.push_back(&rangeIndexDimension);


	OutputAmfNcFile file(directory, m_copolarisedProcessor.getInstrumentInfo(), author, processingSoftwareInfo, m_copolarisedProcessor.getCalibrationInfo(), dataInfo,
		projectInfo, platform, sU("doppler lidar depolorisation profiles"), timesCo, nonTimeDimensions);

	//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), cm_mean}, { sU("range"), cm_mean } };
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ {sU("time"), cm_point} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), cm_mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAnglesEarthFrame{ {sU("time"), cm_mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsRange{ };
	std::vector<sci::string> coordinatesData{ sU("latitude"), sU("longitude"), sU("range") };
	std::vector<sci::string> coordinatesAngles{ sU("latitude"), sU("longitude") };
	std::vector<sci::string> coordinatesAnglesEarthFrame{ sU("latitude"), sU("longitude") };
	std::vector<sci::string> coordinatesRange{ sU("latitude"), sU("longitude") };

	AmfNcVariable<metre, decltype(rangesCo)> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), sU("range"), rangesCo, true, coordinatesRange, cellMethodsRange);
	AmfNcVariable<degree, decltype(instrumentRelativeAzimuthAnglesCo)> azimuthVariable(sU("sensor_azimuth_angle"), file, file.getTimeDimension(), sU("Scanning Head Azimuth Angle"), sU("sensor_azimuth_angle"), instrumentRelativeAzimuthAnglesCo, true, coordinatesAngles, cellMethodsAngles, sU("Relative to the instrument with 0 degrees being to the \"front\" of the instruments."));
	AmfNcVariable<degree, decltype(instrumentRelativeElevationAnglesCo)> elevationVariable(sU("sensor_view_angle"), file, file.getTimeDimension(), sU("Scanning Head Elevation Angle"), sU("sensor_view_angle"), instrumentRelativeElevationAnglesCo, true, coordinatesAngles, cellMethodsAngles, sU("Relative to the instruments with 0 degrees being \"horizontal\", positive being upwards, negative being downwards."));
	AmfNcVariable<degree, decltype(attitudeCorrectedAzimuthAnglesCo)> azimuthVariableEarthFrame(sU("sensor_azimuth_angle_earth_frame"), file, file.getTimeDimension(), sU("Scanning Head Azimuth Angle Earth Frame"), sU(""), attitudeCorrectedAzimuthAngles, true, coordinatesAnglesEarthFrame, cellMethodsAnglesEarthFrame, sU("Relative to the geoid with 0 degrees being north."));
	AmfNcVariable<degree, decltype(attitudeCorrectedElevationAnglesCo)> elevationVariableEarthFrame(sU("sensor_view_angle_earth_frame"), file, file.getTimeDimension(), sU("Scanning Head Elevation Angle Earth Frame"), sU(""), attitudeCorrectedElevationAngles, true, coordinatesAnglesEarthFrame, cellMethodsAnglesEarthFrame, sU("Relative to the geoid with 0 degrees being horizontal, positive being upwards, negative being downwards."));
	AmfNcVariable<perSteradianPerMetre, decltype(backscattersCo)> backscatterCoVariable(sU("attenuated_aerosol_backscatter_coefficient_co"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Attenuated Aerosol Backscatter Coefficient (Planar Polarised)"), sU(""), backscattersCo, true, coordinatesData, cellMethodsData);
	AmfNcVariable<unitless, decltype(snrPlusOneCo)> snrPlusOneCoVariable(sU("signal_to_noise_ratio_plus_1_co"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Signal to Noise Ratio: SNR+1 (Planar Polarised)"), sU(""), snrPlusOneCo, true, coordinatesData, cellMethodsData);
	AmfNcVariable<perSteradianPerMetre, decltype(backscattersCross)> backscatterCrossVariable(sU("attenuated_aerosol_backscatter_coefficient_cr"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Attenuated Aerosol Backscatter Coefficient (Cross Polarised)"), sU(""), backscattersCross, true, coordinatesData, cellMethodsData);
	AmfNcVariable<unitless, decltype(snrPlusOneCross)> snrPlusOneCrossVariable(sU("signal_to_noise_ratio_plus_1_cr"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Signal to Noise Ratio: SNR+1 (Cross Polarised)"), sU(""), snrPlusOneCross, true, coordinatesData, cellMethodsData);
	AmfNcVariable<unitless, decltype(depolarisation)> depolarisationVariable(sU("depolarisation_ratio"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Depolarisation Ratio"), sU(""), depolarisation, true, coordinatesData, cellMethodsData);
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
	file.write(snrPlusOneCoVariable);
	file.write(backscatterCrossVariable);
	file.write(snrPlusOneCrossVariable);
	file.write(depolarisationVariable);
	file.write(backscatterFlagCoVariable, backscatterFlagsCo);
	file.write(backscatterFlagCrossVariable, backscatterFlagsCross);
	file.write(depolarisationFlagVariable, depolarisationFlags);
}