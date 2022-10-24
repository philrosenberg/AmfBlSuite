#include"Lidar.h"
#include<wx/regex.h>
#include"ProgressReporter.h"
#include<svector/operators.h>
#include<svector/ArrayManipulation.h>

const std::vector<double>first(5);
const std::vector<double>sec(5);
const std::vector<double>third = first + sec;





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
	wxRegEx regularExpression = wxString(sci::nativeUnicode( regEx ));
	sci::assertThrow(regularExpression.IsValid(), sci::err(sci::SERR_USER, 0, sci::string(sU("Invalid regex:")) + regEx));
	return (regularExpression.Matches(sci::nativeUnicode(fileName)));
}

std::vector<std::vector<sci::string>> LidarDepolProcessor::groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	//just pass these all to the copolarised processor - the process to check the day is the same
	//for both files, but it is way easier to just send all the files to one processor
	return m_copolarisedProcessor.groupInputFilesbyOutputFiles(newFiles, allFiles);
}

void LidarDepolProcessor::plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	throw(sci::err(sci::SERR_USER, 0, sU("Plotting lidar depolarisation is not currently supported")));
}


//this is an awful hack but life is too short
//If newShape goes out of scope then the new grid_view becomes invalid
template<class T, size_t NDIMSOLD, size_t NDIMSNEW>
sci::grid_view<T, NDIMSNEW> reshape(sci::grid_view<T, NDIMSOLD> old, const std::array<size_t, NDIMSNEW> &newShape)
{
	auto oldShape = old.getShape();
	size_t oldProduct = 1;
	for (size_t i = 0; i < oldShape.size(); ++i)
		oldProduct *= oldShape;
	size_t newProduct = 1;
	for (size_t i = 0; i < newShape.size(); ++i)
		newProduct *= newShape;

	sci::assertThrow(newProduct == oldProduct, sci::err(sci::SERR_USER, sU("Tries to reshape a grid, but the overall size does not match.")));

	return sci::grid_view<T, NDIMSNEW>(old.getSpan(), &newShape[0]);
}

template<class T, size_t NDIMSOLD, size_t NDIMSNEW>
sci::grid_view<T, NDIMSNEW> reshape(sci::GridData<T, NDIMSOLD> &old, const std::array<size_t, NDIMSNEW>& newShape)
{
	return reshape(old.getgrid_view(), newShape);
}

void LidarDepolProcessor::writeToNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	DataInfo dataInfo;
	dataInfo.continuous = true;
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.startTime = m_copolarisedProcessor.getTimesUtcTime()[0];
	dataInfo.endTime = m_copolarisedProcessor.getTimesUtcTime().back();
	dataInfo.featureType = FeatureType::timeSeriesProfile;
	dataInfo.processingLevel = 3;
	dataInfo.options = {};
	dataInfo.productName = sU("depolarisation ratio");
	dataInfo.processingOptions = processingOptions;

	sci::GridData<sci::UtcTime, 1> startTimesCo;
	sci::GridData<sci::UtcTime, 1> endTimesCo;
	sci::GridData<degreeF, 2> instrumentRelativeAzimuthAnglesCo;
	sci::GridData<degreeF, 2> instrumentRelativeElevationAnglesCo;
	sci::GridData<metrePerSecondF, 3> instrumentRelativeDopplerVelocitiesCo;
	sci::GridData<degreeF, 2> attitudeCorrectedAzimuthAnglesCo;
	sci::GridData<degreeF, 2> attitudeCorrectedElevationAnglesCo;
	sci::GridData<metrePerSecondF, 3> motionCorrectedDopplerVelocitiesCo;
	sci::GridData<perSteradianPerMetreF, 3> backscattersCo;
	sci::GridData<unitlessF, 3> snrPlusOneCo;
	sci::GridData<uint8_t, 3> dopplerVelocityFlagsCo;
	sci::GridData<uint8_t, 3> backscatterFlagsCo;
	sci::GridData<metreF, 3> rangesCo;
	secondF averagingPeriodCo;
	secondF samplingIntervalCo;
	size_t maxProfilesPerScanCo;
	size_t maxNGatesCo;
	size_t pulsesPerRayCo;
	size_t raysPerPointCo;
	metreF focusCo;
	metrePerSecondF dopplerResolutionCo;
	metreF gateLengthCo;
	size_t pointsPerGateCo;

	sci::GridData<sci::UtcTime, 1> startTimesCross;
	sci::GridData<sci::UtcTime, 1> endTimesCross;
	sci::GridData<degreeF, 2> instrumentRelativeAzimuthAnglesCross;
	sci::GridData<degreeF, 2> instrumentRelativeElevationAnglesCross;
	sci::GridData<metrePerSecondF, 3> instrumentRelativeDopplerVelocitiesCross;
	sci::GridData<degreeF, 2> attitudeCorrectedAzimuthAnglesCross;
	sci::GridData<degreeF, 2> attitudeCorrectedElevationAnglesCross;
	sci::GridData<metrePerSecondF, 3> motionCorrectedDopplerVelocitiesCross;
	sci::GridData<perSteradianPerMetreF, 3> backscattersCross;
	sci::GridData<unitlessF, 3> snrPlusOneCross;
	sci::GridData<uint8_t, 3> dopplerVelocityFlagsCross;
	sci::GridData<uint8_t, 3> backscatterFlagsCross;
	sci::GridData<metreF, 3> rangesCross;
	secondF averagingPeriodCross;
	secondF samplingIntervalCross;
	size_t maxProfilesPerScanCross;
	size_t maxNGatesCross;
	size_t pulsesPerRayCross;
	size_t raysPerPointCross;
	metreF focusCross;
	metrePerSecondF dopplerResolutionCross;
	metreF gateLengthCross;
	size_t pointsPerGateCross;
	
	m_copolarisedProcessor.formatDataForOutput(progressReporter, rangesCo, instrumentRelativeAzimuthAnglesCo, attitudeCorrectedAzimuthAnglesCo, instrumentRelativeElevationAnglesCo,
		attitudeCorrectedElevationAnglesCo, instrumentRelativeDopplerVelocitiesCo, motionCorrectedDopplerVelocitiesCo, backscattersCo, snrPlusOneCo, dopplerVelocityFlagsCo,
		backscatterFlagsCo, startTimesCo, endTimesCo, maxProfilesPerScanCo, maxNGatesCo, pulsesPerRayCo, raysPerPointCo, focusCo, dopplerResolutionCo, gateLengthCo, pointsPerGateCo);

	m_crosspolarisedProcessor.formatDataForOutput(progressReporter, rangesCross, instrumentRelativeAzimuthAnglesCross, attitudeCorrectedAzimuthAnglesCross, instrumentRelativeElevationAnglesCross,
		attitudeCorrectedElevationAnglesCross, instrumentRelativeDopplerVelocitiesCross, motionCorrectedDopplerVelocitiesCross, backscattersCross, snrPlusOneCross, dopplerVelocityFlagsCross,
		backscatterFlagsCross, startTimesCross, endTimesCross, maxProfilesPerScanCross, maxNGatesCross, pulsesPerRayCross, raysPerPointCross, focusCross, dopplerResolutionCross, gateLengthCross, pointsPerGateCross);
		
	//make the attitude corrected angles continuous for moving platforms otherwise the averaging gets messed up
	for (size_t i = 0; i < attitudeCorrectedAzimuthAnglesCo.shape()[1]; ++i) //for each angle
	{
		degreeF offset(0);
		for (size_t j = 1; j < attitudeCorrectedAzimuthAnglesCo.shape()[0]; ++j) //for each scan pattern/timestep
		{
			if (attitudeCorrectedAzimuthAnglesCo[j][i] - attitudeCorrectedAzimuthAnglesCo[j - 1][i] > degreeF(180))
				offset -= degreeF(180);
			else if (attitudeCorrectedAzimuthAnglesCo[j][i] - attitudeCorrectedAzimuthAnglesCo[j - 1][i] < degreeF(-180))
				offset += degreeF(180);
			attitudeCorrectedAzimuthAnglesCo[j][i] += offset;
		}
	}
	for (size_t i = 0; i < attitudeCorrectedAzimuthAnglesCross.shape()[1]; ++i) //for each angle
	{
		degreeF offset(0);
		for (size_t j = 1; j < attitudeCorrectedAzimuthAnglesCross.shape()[0]; ++j) //for each scan pattern/timestep
		{
			if (attitudeCorrectedAzimuthAnglesCross[j][i] - attitudeCorrectedAzimuthAnglesCross[j - 1][i] > degreeF(180))
				offset -= degreeF(180);
			else if (attitudeCorrectedAzimuthAnglesCross[j][i] - attitudeCorrectedAzimuthAnglesCross[j - 1][i] < degreeF(-180))
				offset += degreeF(180);
			attitudeCorrectedAzimuthAnglesCross[j][i] += offset;
		}
	}

	if (startTimesCo.size() < 1)
	{
		progressReporter << sU("The co-polarisation file contains no data - no output file will be created.");
		return;
	}
	if (startTimesCross.size() < 1)
	{
		progressReporter << sU("The cross-polarisation file contains no data - no output file will be created.");
		return;
	}


	secondF samplingInterval = sci::median(std::vector<sci::UtcTime>(startTimesCo.begin() + 1, startTimesCo.end()) - std::vector<sci::UtcTime>(startTimesCo.begin(), startTimesCo.end() - 1));

	

	//The averaging period will be the multiple of ten larger than the original sampling interval
	dataInfo.averagingPeriod = secondF(10.0) *sci::ceil<unitlessF>(samplingInterval/secondF(10.0f));

	//work out the sampling interval
	/*{
		std::vector<sci::TimeInterval> samplingIntervals = std::vector<sci::UtcTime>(startTimesCo.begin() + 1, startTimesCo.end()) - std::vector<sci::UtcTime>(startTimesCo.begin(), startTimesCo.end() - 1);
		samplingIntervalCo = sci::median(samplingIntervals);
		samplingIntervals = std::vector<sci::UtcTime>(startTimesCross.begin() + 1, startTimesCross.end()) - std::vector<sci::UtcTime>(startTimesCross.begin(), startTimesCross.end() - 1);
		samplingIntervalCross = sci::median(samplingIntervals);
		dataInfo.samplingInterval = (samplingIntervalCo + samplingIntervalCross) / unitlessF(2);
	}*/
	//Barbara just wants the sampling interval to be the same as the averagig period
	dataInfo.samplingInterval = dataInfo.averagingPeriod;


	//remove any low snr flags - the snr will be increased by the averaging
	for(auto iter = backscatterFlagsCo.begin(); iter!=backscatterFlagsCo.end(); ++iter)
		if (*iter == lidarSnrBelow1Flag || *iter == lidarSnrBelow2Flag || *iter == lidarSnrBelow3Flag)
			*iter = lidarGoodDataFlag;
	for (auto iter = backscatterFlagsCross.begin(); iter != backscatterFlagsCross.end(); ++iter)
		if (*iter == lidarSnrBelow1Flag || *iter == lidarSnrBelow2Flag || *iter == lidarSnrBelow3Flag)
			*iter = lidarGoodDataFlag;

	//We should have one co for each cross and the co should be jsut before the cross.
	//go through the data and check this is the case. removing any unpaired data.
	//Unpaired data would be the result of e.g. powering down the lidar.
	for(size_t i=0; i<std::max(startTimesCo.shape()[0], startTimesCross.shape()[0]); ++i)
	{
		
		//This code checks for missing co data. If it finds it
		//then it inserts data to match.
		if (i < startTimesCross.shape()[0] && (i == startTimesCo.shape()[0] || startTimesCo[i] > startTimesCross[i]))
		{
			//this is a really slow way to insert data if there are many missing points,
			//but actually I can't think of any case where we would end up with missing
			//the copolarised data, unless we totally messed up the files
			startTimesCo.insert(i, endTimesCross[i]);//deliberately set startTimeCross to endTimeCo to introduce some stagger
			endTimesCo.insert(i, endTimesCross[i]);//start and end times are the same as there is no data
			instrumentRelativeAzimuthAnglesCo.insert(i, instrumentRelativeAzimuthAnglesCross[i]);
			instrumentRelativeElevationAnglesCo.insert(i, instrumentRelativeElevationAnglesCross[i]);
			attitudeCorrectedAzimuthAnglesCo.insert(i, attitudeCorrectedAzimuthAnglesCross[i]);
			attitudeCorrectedElevationAnglesCo.insert(i, attitudeCorrectedElevationAnglesCross[i]);
			backscattersCo.insert(i, std::numeric_limits<perSteradianPerMetreF>::quiet_NaN());
			snrPlusOneCo.insert(i, std::numeric_limits<unitlessF>::quiet_NaN());
			backscatterFlagsCo.insert(i, lidarPaddedBackscatter);
			rangesCo.insert(i, rangesCross[i]);
		}
		
		if (i<startTimesCo.size())
		{
			//This code checks for missing cross data and if it finds any
			//then it inserts padding data. Note that here we check for multiple
			//contiguous missing profiles as it might be that cross polarised data
			//was turned off for a time and it is very slow to add lots of data one
			//element at a time, part way through a data set
			size_t nToInsert = 0;
			if (i == startTimesCross.size())
				nToInsert = startTimesCo.size() - startTimesCross.size();
			else
			{
				while (i + nToInsert + 1 < startTimesCo.size() && startTimesCross[i] > startTimesCo[i + nToInsert + 1])
				{
					++nToInsert;
					if (i + nToInsert + 1 == startTimesCo.size())
						break;
				}
			}
			if(nToInsert > 0)
			{
				startTimesCross.insert(i, endTimesCo.subGrid(i, nToInsert)); //deliberately set startTimeCross to endTimeCo to introduce some stagger
				endTimesCross.insert(i, endTimesCo.subGrid(i, nToInsert)); //start and end times are the same as there is no actual data
				instrumentRelativeAzimuthAnglesCross.insert(i, instrumentRelativeAzimuthAnglesCo.subGrid(i, nToInsert));
				instrumentRelativeElevationAnglesCross.insert(i, instrumentRelativeElevationAnglesCo.subGrid(i, nToInsert));
				attitudeCorrectedAzimuthAnglesCross.insert(i, attitudeCorrectedAzimuthAnglesCo.subGrid(i, nToInsert));
				attitudeCorrectedElevationAnglesCross.insert(i, attitudeCorrectedElevationAnglesCo.subGrid(i, nToInsert));
				rangesCross.insert(i, rangesCo.subGrid(i, nToInsert));

				backscattersCross.insert(i, nToInsert, std::numeric_limits<perSteradianPerMetreF>::quiet_NaN());
				snrPlusOneCross.insert(i, nToInsert, std::numeric_limits<unitlessF>::quiet_NaN());
				backscatterFlagsCross.insert(i, nToInsert, lidarPaddedBackscatter);
			}
			//no need to check all the data we just added
			if(nToInsert > 0)
				i += nToInsert-1;
		}
	}

	//check that the number of range gates in the co and cross data are the same
	std::array<size_t, 3> rangesCoShape = rangesCo.shape();
	std::array<size_t, 3> rangesCrossShape = rangesCross.shape();
	std::array<size_t, 3> minShape;
	for (size_t i = 0; i < 3; ++i)
		minShape[i] = std::min(rangesCoShape[i], rangesCrossShape[i]);
	rangesCo.reshape(minShape);
	backscattersCo.reshape(minShape);
	snrPlusOneCo.reshape(minShape);
	backscatterFlagsCo.reshape(minShape);
	rangesCross.reshape(minShape);
	backscattersCross.reshape(minShape);
	snrPlusOneCross.reshape(minShape);
	backscatterFlagsCross.reshape(minShape);
	
	//Check that the values of the ranges and the angles are the same for cross and co data
	/*for (size_t i = 0; i < rangesCo.size(); ++i)
	{
		bool goodProfile = true;
		for (size_t j = 0; j < instrumentRelativeAzimuthAnglesCo[i].size(); ++j)
		{
			goodProfile = goodProfile && sci::abs(instrumentRelativeAzimuthAnglesCo[i][j] - instrumentRelativeAzimuthAnglesCross[i][j]) < degreeF(0.01f);
			goodProfile = goodProfile && sci::abs(instrumentRelativeElevationAnglesCo[i][j] - instrumentRelativeElevationAnglesCross[i][j]) < degreeF(0.01f);
		}
		for (size_t j = 0; j < rangesCo[i].size(); ++j)
		{
			for (size_t k = 0; k < rangesCo[i][j].size(); ++k)
			goodProfile = goodProfile && sci::abs(rangesCo[i][j][k] - rangesCross[i][j][k]) < metreF(0.001f);
		}
		if (!goodProfile)
		{
			//ensure all data have the same size
			size_t nGates = rangesCo[i].size();
			rangesCross[i]= rangesCross[i];
			backscattersCross[i].resize(nGates);
			snrPlusOneCross.resize(nGates);
			backscatterFlagsCross.resize(nGates);
			backscattersCo[i].resize(nGates);
			snrPlusOneCo.resize(nGates);
			backscatterFlagsCo.resize(nGates);
			for (size_t j = 0; j < backscattersCross[i].size(); ++j)
			{
				size_t nAngles = rangesCo[i][j].size();
				backscattersCross[i][j].assign(nAngles, std::numeric_limits<perSteradianPerMetreF>::quiet_NaN());
				snrPlusOneCross[i][j].assign(nAngles, std::numeric_limits<unitlessF>::quiet_NaN());
				backscatterFlagsCross[i][j].assign(nAngles, lidarNonMatchingRanges);
				backscattersCo[i][j].assign(nAngles, std::numeric_limits<perSteradianPerMetreF>::quiet_NaN());
				snrPlusOneCo[i][j].assign(nAngles, std::numeric_limits<unitlessF>::quiet_NaN());
				backscatterFlagsCo[i][j].assign(nAngles, lidarNonMatchingRanges);
			}
		}
	}*/

	//average over an appropriate time period. For now we're going with 10s
	sci::GridData<sci::UtcTime, 1> averagedTime;
	sci::GridData<metreF, 3> averagedRanges;
	sci::GridData<perSteradianPerMetreF, 3> averagedBackscatterCross;
	sci::GridData<perSteradianPerMetreF, 3> averagedBackscatterCo;
	sci::GridData<unitlessF, 3> depolarisation;
	sci::GridData<unitlessF, 3> averagedSnrPlusOneCross;
	sci::GridData<unitlessF, 3> averagedSnrPlusOneCo;
	sci::GridData<uint8_t, 3> averagedFlagsCross;
	sci::GridData<uint8_t, 3> averagedFlagsCo;
	sci::GridData<uint8_t, 3> depolarisationFlags;
	sci::GridData<degreeF, 2> instrumentRelativeAzimuthAngles;
	sci::GridData<degreeF, 2> instrumentRelativeElevationAngles;
	sci::GridData<degreeF, 2> attitudeCorrectedAzimuthAngles;
	sci::GridData<degreeF, 2> attitudeCorrectedElevationAngles;
	sci::GridData<second, 1> integrationTimes;
	sci::GridData<uint8_t, 1> missmatchedProfileFlags;


	{
		//put this inside its own scope to hide the i variable from the rest of the code

		//This is our averaging code.
		//we start with i=0, then within each pass of the while loop we increment i multiple
		//times until we exceed the integration time. This gives us a range to average
		size_t i = 0;
		while(i<backscattersCross.shape()[0]-1)
		{
			//first time within this averaged block
			sci::UtcTime startTime = startTimesCo[i];
			//these are the ranges at the biginning of the average period, we will check they don't change during the average time
			sci::GridData<metreF, 2> startRanges = rangesCo[i];
			//position at the begining of the averaging period - we will check this doesn't change during the averaging time
			sci::GridData<degreeF, 1> startInstrumentRelativeAzimuthAngles = instrumentRelativeAzimuthAnglesCo[i];
			sci::GridData<degreeF, 1> startInstrumentRelativeElevationAngles = instrumentRelativeElevationAnglesCo[i];
			//sum of the cross and co backscatters over the average time. First dim is range, second is angle
			//set the size up correctly and set all values to 0.0 sr-1 m-1
			sci::GridData<perSteradianPerMetreF, 2> crossSum(backscattersCross[i].shape(), perSteradianPerMetreF(0.0));
			sci::GridData<perSteradianPerMetreF, 2> coSum(backscattersCross[i].shape(), perSteradianPerMetreF(0.0));

			sci::GridData<perSteradianRerMetreCubedF, 2> crossSumRaw(backscattersCross[i].shape(), perSteradianRerMetreCubedF(0.0));
			sci::GridData<perSteradianRerMetreCubedF, 2> coSumRaw(backscattersCross[i].shape(), perSteradianRerMetreCubedF(0.0));
			sci::GridData<perSteradianSquaredPerMetreSixF, 2> crossNoiseSumSquared(backscattersCross[i].shape(), perSteradianSquaredPerMetreSixF(0.0));
			sci::GridData<perSteradianSquaredPerMetreSixF, 2> coNoiseSumSquared(backscattersCross[i].shape(), perSteradianSquaredPerMetreSixF(0.0));
			
			//keep track of the flags while averaging - same dimension as crossSum and coSum
			sci::GridData<uint8_t, 2> worstFlagCo(backscattersCross[i].shape(), lidarGoodDataFlag);
			sci::GridData<uint8_t, 2> worstFlagCr(backscattersCross[i].shape(), lidarGoodDataFlag);
			sci::GridData<uint8_t, 2> worstFlagDepol(backscattersCross[i].shape(), lidarGoodDataFlag);
			
			//average the attitude corrected positions too. These just have one dimension
			sci::GridData<degreeF, 1>attitudeCorrectedAverageAzimuth(instrumentRelativeAzimuthAnglesCo[i].shape(), degreeF(0.0));
			sci::GridData<degreeF, 1>attitudeCorrectedAverageElevation(instrumentRelativeAzimuthAnglesCo[i].shape(), degreeF(0.0));
			//have a variable to ceck we meet all our qc checks during the averaging
			bool goodAverage = true;
			//count how many times we have in our averaging
			size_t count = 0;
			//record our total integration time - not the same as averaging time as the laser is off when moving
			second integrationTime(0.0);

			//this loop increments i and sums the apropriate data until we exceed the averaging period
			while (i < backscattersCross.shape()[0] - 1 && integrationTime < dataInfo.averagingPeriod )
			{
				auto result = instrumentRelativeAzimuthAnglesCo[i] - startInstrumentRelativeAzimuthAngles;
				if (count > 0)
				{
					//compare this profile against the first profile in the average period to see if it is compatible
					//and if not, flag the profile
					if ( sci::abs(rangesCo[i][0][0] - startRanges[0][0]) > metreF(0.1f) //check for change in range resolution
						|| sci::abs(rangesCross[i][0][0]- startRanges[0][0]) > metreF(0.1f) //check for change in range resolution
						|| sci::allTrue(sci::abs(instrumentRelativeAzimuthAnglesCo[i]-startInstrumentRelativeAzimuthAngles) > degreeF(0.01f)) //check for change in azimuth
						|| sci::allTrue(sci::abs(instrumentRelativeElevationAnglesCo[i] - startInstrumentRelativeElevationAngles) > degreeF(0.01f)) //check for change in elevation
						|| (count >0 && startTimesCo[i+1]- startTimesCo[i] > dataInfo.samplingInterval*unitlessF(3.0f))) //check for large gaps
					{
						goodAverage = false;
					}
				}
				integrationTime += (startTimesCross[i] -startTimesCo[i]) * unitlessF(2.0); //calculate the integration time this way because we know co and cross match, but there could be a gap co to co
				//add the current backscatters to the sums
				crossSum += backscattersCross[i];
				coSum += backscattersCo[i];
				crossSumRaw += backscattersCross[i] / rangesCross[i] / rangesCross[i];
				coSumRaw += backscattersCo[i] / rangesCo[i] /rangesCo[i];
				crossNoiseSumSquared += sci::pow<2>((backscattersCross[i] / rangesCross[i] / rangesCross[i]) / (snrPlusOneCross[i] - unitlessF(1.0f)));
				coNoiseSumSquared += sci::pow<2>((backscattersCo[i] / rangesCo[i] / rangesCo[i]) / (snrPlusOneCo[i] - unitlessF(1.0f)));
				//track the worst flag over the averaging period
				for (size_t j = 0; j < worstFlagCo.shape()[0]; ++j)
					for (size_t k = 0; k < worstFlagCo.shape()[1]; ++k)
						worstFlagCo[j][k] = std::max(backscatterFlagsCo[i][j][k], worstFlagCo[j][k]);
				for (size_t j = 0; j < worstFlagCo.shape()[0]; ++j)
					for (size_t k = 0; k < worstFlagCo.shape()[1]; ++k)
						worstFlagCr[j][k] = std::max(backscatterFlagsCross[i][j][k], worstFlagCr[j][k]);
				for (size_t j = 0; j < worstFlagCo.shape()[0]; ++j)
					for (size_t k = 0; k < worstFlagCo.shape()[1]; ++k)
						worstFlagDepol[j][k] = std::max(worstFlagCr[j][k], worstFlagCo[j][k]);
				//add the current attitude corrected positions to the sums
				attitudeCorrectedAverageAzimuth += (attitudeCorrectedAzimuthAnglesCo[i] + attitudeCorrectedAzimuthAnglesCross[i]) / unitlessF(2);
				attitudeCorrectedAverageElevation += (attitudeCorrectedElevationAnglesCo[i] + attitudeCorrectedElevationAnglesCross[i]) / unitlessF(2);

				//increment the count for the average and the curent time index
				++count;
				++i;
			}

			//When we get to here we have summed over a chunk of data and either flagged it as
			//a good or bad chunk



			//add the averages to the appropriate variables
			averagedTime.push_back(startTime);
			averagedRanges.push_back(startRanges);
			averagedFlagsCo.push_back(worstFlagCo);
			averagedFlagsCross.push_back(worstFlagCr);
			attitudeCorrectedAzimuthAngles.push_back(attitudeCorrectedAverageAzimuth/unitlessF(count));
			attitudeCorrectedElevationAngles.push_back(attitudeCorrectedAverageElevation/unitlessF(count));
			integrationTimes.push_back(integrationTime);
			missmatchedProfileFlags.push_back(goodAverage ? lidarGoodDataFlag : lidarCoAndCrossMisaligned);

			//we have to do things a little more convolutedly for multi-d arrays
			//we append an empty multi-d array element and use convert to put the data into this element
			//this is because although the result of the divide has equivalent units to the destination array
			//the units are not equal (e.g. sr-1 m-1 sr m vs unitless). Assignment with vectors would require
			//them to be equal 
			depolarisation.push_back(crossSum / (crossSum + coSum));
			averagedBackscatterCo.push_back(coSum / unitlessF((unitlessF::valueType)count));
			averagedBackscatterCross.push_back(crossSum / unitlessF((unitlessF::valueType)count));
			averagedSnrPlusOneCo.push_back(coSumRaw / sci::root<2>(coNoiseSumSquared) + unitlessF(1.0f));
			averagedSnrPlusOneCross.push_back(crossSumRaw / sci::root<2>(crossNoiseSumSquared) + unitlessF(1.0f));
			instrumentRelativeAzimuthAngles.push_back(startInstrumentRelativeAzimuthAngles); // no need to divide as we don't average these - they are all identical
			instrumentRelativeElevationAngles.push_back(startInstrumentRelativeElevationAngles);  // no need to divide as we don't average these - they are all identical

				
			

			//it's worth reserving some space for the arrays early on to save multiple memory allocations and reassignments
			//this code waits untile we have 5 entries in averagedTime, then estimates the space we will need
			if (averagedTime.size() == 5)
			{
				auto expectedSize3d = backscattersCo.shape();
				expectedSize3d[0] /= (i / 5);
				expectedSize3d[0] += expectedSize3d[0] / 5; //add 20% for good measure

				auto expectedSize2d = instrumentRelativeAzimuthAnglesCo.shape();
				expectedSize2d[0] /= (i / 5);
				expectedSize2d[0] += expectedSize2d[0] / 5; //add 20% for good measure

				auto expectedSize1d = startTimesCo.shape();
				expectedSize1d[0] /= (i / 5);
				expectedSize1d[0] += expectedSize1d[0] / 5; //add 20% for good measure

				averagedBackscatterCo.reserve(expectedSize3d);
				averagedBackscatterCross.reserve(expectedSize3d);
				averagedSnrPlusOneCo.reserve(expectedSize3d);
				averagedSnrPlusOneCross.reserve(expectedSize3d);
				depolarisation.reserve(expectedSize3d);
				averagedFlagsCo.reserve(expectedSize3d);
				averagedFlagsCross.reserve(expectedSize3d);
				averagedRanges.reserve(expectedSize3d);

				instrumentRelativeAzimuthAngles.reserve(expectedSize2d);
				instrumentRelativeElevationAngles.reserve(expectedSize2d);
				attitudeCorrectedAzimuthAngles.reserve(expectedSize2d);
				attitudeCorrectedElevationAngles.reserve(expectedSize2d);

				averagedTime.reserve(expectedSize1d);
				integrationTimes.reserve(expectedSize1d);
				missmatchedProfileFlags.reserve(expectedSize1d);
			}
		}
	}

	//by making the earth frame azimuth continuous we may have strayed out of the +/-180 degree range. Fix that now

	for (auto& angle : attitudeCorrectedAzimuthAngles)
	{
		while (angle < degreeF(-180))
			angle += degreeF(360);
		while (angle > degreeF(180))
			angle -= degreeF(360);
	}

	if (averagedTime.size() == 0)
	{
		progressReporter << sU("Did not find enough good profiles to create any depolarisation averages. No output file will be created.");
		return;
	}

	auto averagedFlagsCoIter = averagedFlagsCo.begin();
	auto averagedSnrPlusOneCoIter = averagedSnrPlusOneCo.begin();
	for (; averagedFlagsCoIter != averagedFlagsCo.end(); ++averagedFlagsCoIter, ++averagedSnrPlusOneCoIter)
	{
		if (*averagedSnrPlusOneCoIter < unitlessF(2.0) && *averagedFlagsCoIter == lidarGoodDataFlag)
			*averagedFlagsCoIter = lidarSnrBelow1Flag;
		if (*averagedSnrPlusOneCoIter < unitlessF(3.0) && *averagedFlagsCoIter == lidarGoodDataFlag)
			*averagedFlagsCoIter = lidarSnrBelow3Flag;
		if (*averagedSnrPlusOneCoIter < unitlessF(4.0) && *averagedFlagsCoIter == lidarGoodDataFlag)
			*averagedFlagsCoIter = lidarSnrBelow1Flag;
	}

	auto averagedFlagsCrossIter = averagedFlagsCross.begin();
	auto averagedSnrPlusOneCrossIter = averagedSnrPlusOneCross.begin();
	for (; averagedFlagsCrossIter != averagedFlagsCross.end(); ++averagedFlagsCrossIter, ++averagedSnrPlusOneCrossIter)
	{
		if (*averagedSnrPlusOneCrossIter < unitlessF(2.0) && *averagedFlagsCrossIter == lidarGoodDataFlag)
			*averagedFlagsCrossIter = lidarSnrBelow1Flag;
		if (*averagedSnrPlusOneCrossIter < unitlessF(3.0) && *averagedFlagsCrossIter == lidarGoodDataFlag)
			*averagedFlagsCrossIter = lidarSnrBelow3Flag;
		if (*averagedSnrPlusOneCrossIter < unitlessF(4.0) && *averagedFlagsCrossIter == lidarGoodDataFlag)
			*averagedFlagsCrossIter = lidarSnrBelow1Flag;
	}


	depolarisationFlags = averagedFlagsCo;
	for (size_t i = 0; i < averagedBackscatterCross.shape()[0]; ++i)
	{
		for (size_t j = 0; j < averagedBackscatterCross.shape()[1]; ++j)
		{
			for (size_t k = 0; k < averagedBackscatterCross.shape()[2]; ++k)
			{
				depolarisationFlags[i][j][k] = std::max(std::max(averagedFlagsCo[i][j][k], averagedFlagsCross[i][j][k]), missmatchedProfileFlags[i]);
			}
		}
	}

	//check the median integration time
	//std::vector<second> sortedIntegrationTimes = sci::sort(integrationTimes);
	//dataInfo.averagingPeriod = secondF(sortedIntegrationTimes[sortedIntegrationTimes.size() / 2]);

	std::vector<sci::NcDimension*> nonTimeDimensions;
	sci::NcDimension rangeIndexDimension(sU("index_of_range"), rangesCo.size() > 0 ? rangesCo.shape()[1] : 0);
	sci::NcDimension angleIndexDimension(sU("index_of_angle"), attitudeCorrectedAzimuthAngles.size() > 0 ? attitudeCorrectedAzimuthAngles.shape()[1] : 0);
	nonTimeDimensions.push_back(&rangeIndexDimension);
	nonTimeDimensions.push_back(&angleIndexDimension);

	sci::stringstream averagingComment;
	averagingComment << sU("Doppler lidar depolorisation profiles, averaged for at least ") << dataInfo.averagingPeriod << sU(" if the ray time is shorter than this.");

	if (dataInfo.processingOptions.comment.length() > 0)
		dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\n");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + averagingComment.str();
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nRange gates may overlap - hence range may not grow in increments of gate_length.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_azimuth_angle is relative to the instrument with 0 degrees being to the \"front\" of the instruments.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_view_angle is relative to the instruments with 0 degrees being \"horizontal\", positive being upwards, negative being downwards.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_azimuth_angle_earth_frame is relative to the geoid with 0 degrees being north.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_view_angle_earth_frame is relative to the geoid with 0 degrees being horizontal, positive being upwards, negative being downwards.");
	

	//set the global attributes
	sci::stringstream pulsesPerRayStr;
	sci::stringstream raysPerPointStr;
	sci::stringstream velocityResolutionStr;
	sci::stringstream nGatesStr;
	sci::stringstream gateLengthStr;
	sci::stringstream maxRangeStr;
	pulsesPerRayStr << pulsesPerRayCo;
	raysPerPointStr << raysPerPointCo;
	velocityResolutionStr << dopplerResolutionCo;
	nGatesStr << maxNGatesCo;
	gateLengthStr << gateLengthCo;
	maxRangeStr << unitlessF(float(maxNGatesCo)) * gateLengthCo / unitlessF(float(pointsPerGateCo));
	sci::NcAttribute wavelengthAttribute(sU("laser_wavelength"), sU("1550 nm"));
	sci::NcAttribute energyAttribute(sU("laser_pulse_energy"), sU("1.0e-05 J"));
	sci::NcAttribute pulseFrequencyAttribute(sU("pulse_repetition_frequency"), sU("15000 s-1"));
	sci::NcAttribute pulsesPerRayAttribute(sU("pulses_per_ray"), pulsesPerRayStr.str());
	sci::NcAttribute raysPerPointAttribute(sU("rays_per_point"), raysPerPointStr.str());
	sci::NcAttribute diameterAttribute(sU("lens_diameter"), sU("0.08 m"));
	sci::NcAttribute divergenceAttribute(sU("beam_divergence"), sU("0.00189 degrees"));
	sci::NcAttribute pulseLengthAttribute(sU("pulse_length"), sU("2e-07 s"));
	sci::NcAttribute samplingFrequencyAttribute(sU("sampling_frequency"), sU("1.5e+07 Hz"));
	sci::NcAttribute focusAttribute(sU("focus"), sU("inf"));
	sci::NcAttribute velocityResolutionAttribute(sU("velocity_resolution"), velocityResolutionStr.str());
	sci::NcAttribute nGatesAttribute(sU("number_of_gates"), nGatesStr.str());
	sci::NcAttribute gateLengthAttribute(sU("gate_length"), gateLengthStr.str());
	sci::NcAttribute fftAttribute(sU("fft_length"), sU("1024"));
	sci::NcAttribute maxRangeAttribute(sU("maximum_range"), maxRangeStr.str());


	std::vector<sci::NcAttribute*> lidarGlobalAttributes{ &wavelengthAttribute, &energyAttribute,
		&pulseFrequencyAttribute, &pulsesPerRayAttribute, &raysPerPointAttribute, &diameterAttribute,
		&divergenceAttribute, &pulseLengthAttribute, &samplingFrequencyAttribute, &focusAttribute,
	&velocityResolutionAttribute, &nGatesAttribute, &gateLengthAttribute, &fftAttribute, &maxRangeAttribute };

	OutputAmfNcFile file(AmfVersion::v1_1_0, directory, m_copolarisedProcessor.getInstrumentInfo(), author, processingSoftwareInfo, m_copolarisedProcessor.getCalibrationInfo(), dataInfo,
		projectInfo, platform, sU("Lidar depolarisation ratio"), averagedTime, nonTimeDimensions, sU(""), lidarGlobalAttributes);

	//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), cm_mean}, { sU("range"), cm_mean } };
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ {sU("time"), cm_point} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAnglesEarthFrame{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsRange{ };
	std::vector<sci::string> coordinatesData{ sU("latitude"), sU("longitude") };

	//std::array<size_t, 2> newShape{ averagedTime.size(), averagedRanges[0].size() };
	//std::array<size_t, 1> newShapeAngle{ averagedTime.size() };
	//std::vector<metreF> flat = sci::flatten(averagedRanges);
	//std::vector<std::vector<metreF>> tempRange= sci::unflatten(flat, newShape);
	AmfNcVariable<metreF> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), sU(""), averagedRanges, true, coordinatesData, cellMethodsRange, sci::GridData<uint8_t,0>(1));
	AmfNcVariable<degreeF> azimuthVariable(sU("sensor_azimuth_angle_instrument_frame"), file, file.getTimeDimension(), sU("Scanning head azimuth angle in the instrument frame of reference"), sU(""), instrumentRelativeAzimuthAngles, true, coordinatesData, cellMethodsAngles, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<degreeF> elevationVariable(sU("sensor_view_angle_instrument_frame"), file, file.getTimeDimension(), sU("Scanning head elevation angle in the instrument frame of reference"), sU(""), instrumentRelativeElevationAngles, true, coordinatesData, cellMethodsAngles, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<degreeF> azimuthVariableEarthFrame(sU("sensor_azimuth_angle_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & angleIndexDimension}, sU("Scanning head azimuth angle in the Earth frame of reference"), sU(""), attitudeCorrectedAzimuthAngles, true, coordinatesData, cellMethodsAnglesEarthFrame, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<degreeF> elevationVariableEarthFrame(sU("sensor_view_angle_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & angleIndexDimension}, sU("Scanning head elevation angle in the Earth frame of reference"), sU(""), attitudeCorrectedElevationAngles, true, coordinatesData, cellMethodsAnglesEarthFrame, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<perSteradianPerMetreF> backscatterCoVariable(sU("attenuated_aerosol_backscatter_coefficient_co"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Attenuated Aerosol Backscatter Coefficient (Planar Polarised)"), sU(""), averagedBackscatterCo, true, coordinatesData, cellMethodsData, averagedFlagsCo);
	AmfNcVariable<unitlessF> snrPlusOneCoVariable(sU("signal_to_noise_ratio_plus_1_co"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Signal to Noise Ratio: SNR+1 (Planar Polarised)"), sU(""), averagedSnrPlusOneCo, true, coordinatesData, cellMethodsData, averagedFlagsCo);
	AmfNcVariable<perSteradianPerMetreF> backscatterCrossVariable(sU("attenuated_aerosol_backscatter_coefficient_cr"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Attenuated Aerosol Backscatter Coefficient (Cross Polarised)"), sU(""), averagedBackscatterCross, true, coordinatesData, cellMethodsData, averagedFlagsCross);
	AmfNcVariable<unitlessF> snrPlusOneCrossVariable(sU("signal_to_noise_ratio_plus_1_cr"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Signal to Noise Ratio: SNR+1 (Cross Polarised)"), sU(""), averagedSnrPlusOneCross, true, coordinatesData, cellMethodsData, averagedFlagsCross);
	AmfNcVariable<unitlessF> depolarisationVariable(sU("depolarisation_ratio"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension}, sU("Volume Linear Depolarization Ratio"), sU(""), depolarisation, true, coordinatesData, cellMethodsData, depolarisationFlags);
	AmfNcFlagVariable backscatterFlagCoVariable(sU("qc_flag_attenuated_aerosol_backscatter_coefficient_co"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension});
	AmfNcFlagVariable backscatterFlagCrossVariable(sU("qc_flag_attenuated_aerosol_backscatter_coefficient_cr"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension});
	AmfNcFlagVariable depolarisationFlagVariable(sU("qc_flag_depolarisation_ratio"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension});

	file.writeTimeAndLocationData(platform);

	file.writeIgnoreDefunctLastDim(rangeVariable, averagedRanges);
	file.writeIgnoreDefunctLastDim(azimuthVariable, instrumentRelativeAzimuthAngles);
	file.write(azimuthVariableEarthFrame, attitudeCorrectedAzimuthAngles);
	file.writeIgnoreDefunctLastDim(elevationVariable, instrumentRelativeElevationAngles);
	file.write(elevationVariableEarthFrame, attitudeCorrectedElevationAngles);
	file.writeIgnoreDefunctLastDim(backscatterCoVariable, averagedBackscatterCo);
	file.writeIgnoreDefunctLastDim(snrPlusOneCoVariable, averagedSnrPlusOneCo);
	file.writeIgnoreDefunctLastDim(backscatterCrossVariable, averagedBackscatterCross);
	file.writeIgnoreDefunctLastDim(snrPlusOneCrossVariable, averagedSnrPlusOneCross);
	file.writeIgnoreDefunctLastDim(depolarisationVariable, depolarisation);
	file.writeIgnoreDefunctLastDim(backscatterFlagCoVariable, averagedFlagsCo);
	file.writeIgnoreDefunctLastDim(backscatterFlagCrossVariable, averagedFlagsCross);
	file.writeIgnoreDefunctLastDim(depolarisationFlagVariable, depolarisationFlags);
}