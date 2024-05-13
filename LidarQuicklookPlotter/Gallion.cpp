#include"Gallion.h"
#include"ProgressReporter.h"
#include<svector/sminimiser2.h>
#include"AmfNc.h"
#include"Lidar.h"

void readData(const std::vector<sci::string>& inputFilenames, const Platform& platform, ProgressReporter& progressReporter)
{
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		try
		{

		}
		catch(sci::err err)
		{
			progressReporter.setErrorMode();
			progressReporter << err.getErrorMessage() << sU("\n");
			progressReporter.setNormalMode();
		}
	}
}

void GalionWindProfileProcessor::readWindProfile(const sci::string& filename, std::vector<Profile>& profiles, sci::GridData<sci::UtcTime, 2>& times,
	sci::GridData<metrePerSecondF, 2>& horizontalWindSpeeds, sci::GridData<metrePerSecondF, 2>& verticalWindSpeeds, sci::GridData<degreeF, 2>& windDirections,
	sci::GridData<metreSquaredPerSecondSquaredF, 2>& goodnessOfFits, sci::GridData<metreSquaredPerSecondSquaredF, 2>& minimumPowerIntensities, sci::GridData<metreSquaredPerSecondSquaredF, 2>& meanPowerIntensities,
	sci::GridData<metrePerSecondF, 2>& neSpeeds, sci::GridData<metrePerSecondF, 2>& esSpeeds, sci::GridData<metrePerSecondF, 2>& swSpeeds, sci::GridData<metrePerSecondF, 2>& wnSpeeds, ProgressReporter& progressReporter)
{
	/*profiles.resize(0);

	times.reshape({ 0,0 });
	horizontalWindSpeeds.reshape({ 0,0 });
	verticalWindSpeeds.reshape({ 0,0 });
	windDirections.reshape({ 0,0 });
	goodnessOfFits.reshape({ 0,0 });
	minimumPowerIntensities.reshape({ 0,0 });
	meanPowerIntensities.reshape({ 0,0 });
	neSpeeds.reshape({ 0,0 });
	esSpeeds.reshape({ 0,0 });
	swSpeeds.reshape({ 0,0 });
	wnSpeeds.reshape({ 0,0 });

	progressReporter << sU("opening file: ") << filename << sU("\n");
	std::fstream fin;
	fin.open(sci::nativeUnicode(filename), std::ios::in);
	sci::stringstream message;
	message << "could not open file " << filename;
	sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, message.str()));

	std::string line;
	std::getline(fin, line); //read the header line, we don't need it.

	//we first want to count how many altitudes we are using by seeing when the altitudes repeat
	metreF height;
	std::getline(fin, line);
	assertThrow(line.length() > 27, sci::err(sci::SERR_USER, 0, sU("found a cropped line")));
	std::stringstream lineStream;
	lineStream.str(line.substr(24));
	lineStream >> height;
	heights.push_back(height);
	while (1)
	{
		std::getline(fin, line);
		assertThrow(line.length() > 27, sci::err(sci::SERR_USER, 0, sU("found a cropped line")));
		std::stringstream lineStream;
		lineStream.str(line.substr(24));
		lineStream >> height;
		if (height == heights[0])
			break;
		heights.push_back(height);
	}

	progressReporter << sU("Profiles contain ") << heights.size() << sU(" elements\n");

	size_t nHeights = heights.size();
	//there's about 120 bytes per line. Work out ow many bytes we have and reserve space in the grids
	fin.seekg(0, std::ios::end);
	size_t fileLength = fin.tellg();
	size_t approxLines = fileLength / 110; //overestimate by a bit to give some slack
	times.reserve(approxLines);
	horizontalWindSpeeds.reserve(approxLines);
	verticalWindSpeeds.reserve(approxLines);
	windDirections.reserve(approxLines);
	goodnessOfFits.reserve(approxLines);
	minimumPowerIntensities.reserve(approxLines);
	meanPowerIntensities.reserve(approxLines);
	neSpeeds.reserve(approxLines);
	esSpeeds.reserve(approxLines);
	swSpeeds.reserve(approxLines);
	wnSpeeds.reserve(approxLines);

	//return to the beginning of the file;
	fin.seekg(0, std::ios::beg);
	std::getline(fin, line); //read the header line, we still don't need it.
	size_t nProfiles = 0;
	while (!fin.eof() && !fin.bad() && !fin.fail())
	{
		++nProfiles;
		progressReporter << sU("Reading profile ") << nProfiles << sU("\n");
		times.reshape({ nProfiles,nHeights }, std::numeric_limits<sci::UtcTime>::quiet_NaN());
		horizontalWindSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		verticalWindSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		windDirections.reshape({ nProfiles,nHeights }, std::numeric_limits<degreeF>::quiet_NaN());
		goodnessOfFits.reshape({ nProfiles,nHeights }, std::numeric_limits<metreSquaredPerSecondSquaredF>::quiet_NaN());
		minimumPowerIntensities.reshape({ nProfiles,nHeights }, std::numeric_limits<metreSquaredPerSecondSquaredF>::quiet_NaN());
		meanPowerIntensities.reshape({ nProfiles,nHeights }, std::numeric_limits<metreSquaredPerSecondSquaredF>::quiet_NaN());
		neSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		esSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		swSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		wnSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		for (size_t i = 0; i < nHeights; ++i)
		{
			std::getline(fin, line);
			if (line.length() == 0)
				break;
			assertThrow(line.length() > 113, sci::err(sci::SERR_USER, 0, sU("found a cropped line")));
			line[4] = ' ';
			line[7] = ' ';
			line[13] = ' ';
			line[16] = ' ';
			int year;
			int month;
			int day;
			int hour;
			int minute;
			double second;
			std::stringstream lineStream;
			lineStream.str(line);
			lineStream >> year >> month >> day >> hour >> minute >> second >> height
				>> horizontalWindSpeeds[nProfiles - 1][i] >> verticalWindSpeeds[nProfiles - 1][i] >> windDirections[nProfiles - 1][i]
				>> goodnessOfFits[nProfiles - 1][i] >> minimumPowerIntensities[nProfiles - 1][i] >> meanPowerIntensities[nProfiles - 1][i]
				>> neSpeeds[nProfiles - 1][i] >> esSpeeds[nProfiles - 1][i] >> swSpeeds[nProfiles - 1][i]
				>> wnSpeeds[nProfiles - 1][i];
			times[nProfiles - 1][i] = sci::UtcTime(year, month, day, hour, minute, second);
		}
	}

	progressReporter << sU("Reading complete\n");*/
}

bool GalionAdvancedProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	sci::string dateString = fileName.substr(fileName.length() - 15, 8);
	int fileDay = std::atoi(sci::toUtf8(dateString.substr(0, 2)).c_str());
	int fileMonth = std::atoi(sci::toUtf8(dateString.substr(2, 2)).c_str());
	int fileYear = std::atoi(sci::toUtf8(dateString.substr(4, 2)).c_str());
	int fileHour = std::atoi(sci::toUtf8(dateString.substr(6, 2)).c_str());
	if (fileDay == 0 || fileMonth == 0 || fileYear == 0)
		return false;
	fileYear += 2000;
	sci::UtcTime fileStartTime(fileYear, fileMonth, fileDay, fileHour, 0, 0.0);
	sci::UtcTime fileEndTime = fileStartTime + hour(1);
	return fileStartTime < endTime && fileEndTime > startTime;
}

void GalionAdvancedProcessor::readData(const std::vector<sci::string>& inputFilenames, const Platform& platform, ProgressReporter& progressReporter)
{
	m_profiles.clear();
	m_hasData = false;
	std::vector<degreeF> eleveations;
	std::vector<degreeF> azimuths;
	try
	{
		for (size_t i = 0; i < inputFilenames.size(); ++i)
		{
			std::vector<Profile> profiles;
			readAdvancedFile(inputFilenames[i], profiles, progressReporter);
			for (size_t j = 0; j < profiles.size(); ++j)
			{
				eleveations.push_back(profiles[j].elevation);
				azimuths.push_back(profiles[j].azimuth);
			}


			//split the profiles into scans
			//we assume that if the azimuth or elevation or both remain constant from one profile to the next, then this forms one scan
			//pattern, i.e. rhi, vad or stare
			
			
			bool previousSameElevation = false;
			bool previousSameAzimuth = false;
			size_t patternIndex = -1;
			size_t profileIndex;
			for (size_t j = 0; j < profiles.size(); ++j)
			{
				bool sameElevation = j > 0 ? profiles[j].elevation == profiles[j-1].elevation : false;
				bool sameAzimuth = j > 0 ? profiles[j].azimuth == profiles[j-1].azimuth : false;

				bool sameScan = true;
				if (previousSameElevation && previousSameAzimuth) //we were doing a stare scan pattern
					sameScan == sameElevation && sameAzimuth;
				else if (previousSameElevation) // we were doing a vad scan pattern
					sameScan = sameElevation;
				else if (previousSameAzimuth) // we were doing an rhi scan patter
					sameScan = sameAzimuth;
				else //previous profile was the first profiel of a new scan pattern of an unknown type
					sameScan = sameAzimuth || sameElevation;

				//if we have a single stare at 90, 0, this can mess up another scan starting at azimuth 0
				if (j> 0 && j<profiles.size()-1 && sameScan && //not the first or last profile
					!previousSameAzimuth && !previousSameElevation && // this is potentially the second profile in a scan
					!(sameAzimuth && sameElevation) && //not the same stare direction
					profiles[j-1].elevation == degreeF(90) && profiles[j-1].azimuth == degreeF(0) &&//the previous profiel was a vertical stare
					profiles[j].elevation == profiles[j + 1].elevation) //the scan matches the elevation of the next profile
				{
					sameScan = false; //probably the last profile was a single vertical stare followed by a vad
					sameAzimuth = false; //this makes it equivalent the the previous scan being 90,1 which is what the user could have done
				}

				if (!sameScan)
				{
					//The scan pattern has changed - update the index at which we store this data
					++patternIndex;
					profileIndex = 0;
				}
				else ++profileIndex;

				if (patternIndex > m_profiles.size() - 1 || m_profiles.size() == 0)
					m_profiles.resize(patternIndex + 1);

				//pull the actaul scans of this type 
				sci::GridData<Profile, 2>& scans = m_profiles[patternIndex];

				std::array<size_t, 2> shape = scans.shape();

				//add a new scan or profile if needed
				std::array<size_t, 2> newShape = shape;
				if (profileIndex == 0)
					newShape[0] += 1;
				if (profileIndex > newShape[1] - 1 || newShape[1] == 0)
				{
					if (newShape[0] == 1) // this is the first set of scans, we can expand in the second dimension
						newShape[1] = profileIndex + 1;
					else
						throw(sci::err(sci::SERR_USER, 0, sU("The number of scan patterns for the wind lidar are not consistent through the day - processing will stop")));
				}
				scans.reshape(newShape);
				size_t scanIndex = newShape[0] - 1;

				scans[scanIndex][profileIndex] = profiles[j];

				previousSameAzimuth = sameAzimuth;
				previousSameElevation = sameElevation;
			}
			
		}
		/*
		//now we have grouped all the profiles into scans we can do wind profiles
		std::vector<bool> isVad(m_profiles.size());
		for (size_t i = 0; i < isVad.size(); ++i)
			isVad[i] = m_profiles[i].shape()[0] > 0 //at least one instance of this scan
				&& m_profiles[i].shape()[1] >= 4 // at least 4 directions
				&& m_profiles[i][0][0].elevation == m_profiles[i][0][1].elevation //all profiles have the same elevation
				&& m_profiles[i][0][0].azimuth != m_profiles[i][0][1].azimuth //profiles have different azimuths - should really check all profiles have different azimuths, but life is tto short
				&& m_profiles[i][0][0].elevation >= degreeF(30); // elevation larger than 30 degrees

		size_t nVad = 0;
		for (size_t i = 0; i < isVad.size(); ++i)
			if (isVad[i])
				nVad += m_profiles[i].shape()[0];

		m_windU.resize(nVad);
		m_windV.resize(nVad);
		m_windW.resize(nVad);
		m_windStartTimes.resize(nVad);
		m_windEndTimes.resize(nVad);
		m_windHeightInterval.resize(nVad);

		size_t windProfileIndex = -1;
		//see "Scan strategies for wind profiling with Doppler lidar –an large - eddy simulation(LES) - based evaluation" AMT 15, 2839-2856 equations (2) and (3)
		for (size_t i = 0; i < m_profiles.size(); ++i)
		{
			if (isVad[i])
			{
				sci::GridData<Profile, 2 >& scans = m_profiles[i];
				for (size_t j = 0; j < scans.shape()[0]; ++j)
				{
					auto profiles = scans[j]; // should be a 1d view into the scans grid
					++windProfileIndex;
					m_windU[windProfileIndex].resize(profiles[0].velocity.size());
					m_windV[windProfileIndex].resize(profiles[0].velocity.size());
					m_windW[windProfileIndex].resize(profiles[0].velocity.size());
					m_windStartTimes[windProfileIndex] = profiles[0].time;
					m_windEndTimes[windProfileIndex] = profiles[profiles.size()-1].time;
					m_windHeightInterval[windProfileIndex] = m_rangeInterval *sci::sin( profiles[0].elevation);

					//The A matrix from equation 3. Row is the first index, column is the second
					sci::GridData<unitlessF, 2> A({ profiles.size(), 3 });
					for (size_t k = 0; k < A.shape()[0]; ++k)
					{
						degreeF azimuth = profiles[k].azimuth;
						degreeF elevation = profiles[k].elevation;
						A[k][0] = sci::sin(azimuth) * sci::sin(degreeF(90) - elevation);
						A[k][1] = sci::cos(azimuth) * sci::sin(degreeF(90) - elevation);
						A[k][2] = sci::cos(degreeF(90) - elevation);
					}

					//lamda function to stop the minimisation routine as needed - note that it just never
					//stops as we instead limit the iterations
					auto stopper = [&](const sci::GridData<metrePerSecondF, 1>&, metreSquaredPerSecondSquaredF residual)
					{
						return residual == metreSquaredPerSecondSquaredF(0);
					};

					//test
					sci::GridData<metrePerSecondF, 1> testVelocities(3);
					testVelocities[0] = metrePerSecondF(10);
					testVelocities[1] = metrePerSecondF(10);
					testVelocities[2] = metrePerSecondF(1);
					sci::GridData<metrePerSecondF, 1> uvw(3, metrePerSecondF(0)); //initial guess, all zeros
					sci::GridData<metrePerSecondF, 1> uvwScale(3);
					uvwScale[0] = metrePerSecondF(1);
					uvwScale[1] = metrePerSecondF(1);
					uvwScale[2] = metrePerSecondF(0.1);
					sci::GridData<metrePerSecondF, 1> dopplerVelocities(profiles.size(), metrePerSecondF(0));
					for (size_t l = 0; l < A.shape()[0]; ++l)
						for (size_t k = 0; k < A.shape()[1]; ++k)
							dopplerVelocities[l] += A[l][k] * testVelocities[k];
					sci::minimisePowell(A, dopplerVelocities, stopper, uvw, uvwScale, 9, 9);



					for (size_t l = 0; l < profiles[0].velocity.size(); ++l)
					{
						sci::GridData<metrePerSecondF, 1> dopplerVelocities(profiles.size());
						for (size_t k = 0; k < profiles.size(); ++k)
							dopplerVelocities[k] = profiles[k].velocity[l];
						sci::GridData<metrePerSecondF, 1> uvw(3, metrePerSecondF(0)); //initial guess, all zeros
						sci::GridData<metrePerSecondF, 1> uvwScale(3);
						uvwScale[0] = metrePerSecondF(10);
						uvwScale[1] = metrePerSecondF(10);
						uvwScale[2] = metrePerSecondF(1);


						sci::minimisePowell(A, dopplerVelocities, stopper, uvw, uvwScale, 9, 9);
						m_windU[windProfileIndex][l] = uvw[0];
						m_windV[windProfileIndex][l] = uvw[1];
						m_windW[windProfileIndex][l] = uvw[2];
					}

				}
			}

		}
		*/
	}
	catch (...)
	{
		m_profiles.clear();
		throw;
	}
	m_hasData = true;
}

void GalionAdvancedProcessor::readAdvancedFile(const sci::string& filename, std::vector<Profile>& profiles, ProgressReporter& progressReporter)
{
	std::fstream fin;
	fin.open(sci::nativeUnicode(filename), std::ios::in);
	std::string line;
	std::getline(fin, line);
	size_t nRays;

	//read the header lines
	if (line.substr(0, 9) != "Filename:")
	{
		progressReporter.setWarningMode();
		progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line 1 did not begin with \"Fileame:\"\n");
		progressReporter.setNormalMode();
		return;
	}
	std::getline(fin, line);
	if (line.substr(0, 14) != "Campaign code:")
	{
		progressReporter.setWarningMode();
		progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line 2 did not begin with \"Campaign code:\"\n");
		progressReporter.setNormalMode();
		return;
	}
	std::getline(fin, line);
	if (line.substr(0, 16) != "Campaign number:")
	{
		progressReporter.setWarningMode();
		progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line 3 did not begin with \"Campaign number:\"\n");
		progressReporter.setNormalMode();
		return;
	}
	std::getline(fin, line);
	if (line.substr(0, 13) != "Rays in scan:")
	{
		progressReporter.setWarningMode();
		progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line 4 did not begin with \"Rays in scan:\"\n");
		progressReporter.setNormalMode();
		return;
	}
	nRays = std::atoi(line.substr(13).c_str());
	if (nRays == 0)
	{
		progressReporter.setWarningMode();
		progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Rays in scan was either zero or the header is corrupted\n");
		progressReporter.setNormalMode();
		return;
	}
	std::getline(fin, line);
	if (line.substr(0, 11) != "Start time:")
	{
		progressReporter.setWarningMode();
		progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line 5 did not begin with \"Start time:\"\n");
		progressReporter.setNormalMode();
		return;
	}
	std::getline(fin, line);
	if (line != "Range gate	Doppler	Intensity	Ray time	Az	El	Pitch	Roll")
	{
		progressReporter.setWarningMode();
		progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line 6 was not the header line \"Range gate	Doppler	Intensity	Ray time	Az	El	Pitch	Roll\"\n");
		progressReporter.setNormalMode();
		return;
	}

	const size_t gatesPerRay = 136;
	size_t previousGate = gatesPerRay - 1;
	profiles.resize(nRays);
	for (size_t i = 0; i < nRays; ++i)
	{
		profiles[i].snr.resize(gatesPerRay);
		profiles[i].velocity.resize(gatesPerRay);
		for (size_t j = 0; j < gatesPerRay; ++j)
		{
			std::getline(fin, line);
			std::vector<std::string> columns = sci::splitstring(line, " \t", true);
			if (columns.size() != 9)
			{
				progressReporter.setWarningMode();
				progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line ") << i * gatesPerRay + j + 7 << sU(" does not have 9 whitespace separated columns\n");
				progressReporter.setNormalMode();
				profiles.resize(0);
				return;
			}
			size_t gate = std::atoi(columns[0].c_str());
			if (previousGate == gatesPerRay - 1)
				previousGate = -1;
			if (previousGate != gate - 1)
			{
				progressReporter.setWarningMode();
				progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line ") << i * gatesPerRay + j + 7 << sU(" invalid gate number\n");
				progressReporter.setNormalMode();
				profiles.resize(0);
				return;
			}
			if (gate == 0)
			{
				std::vector<std::string> dateColumns = sci::splitstring(columns[3], "-", false);
				std::vector<std::string> timeColumns = sci::splitstring(columns[4], ":", false);
				if (dateColumns.size() != 3 || timeColumns.size() != 3)
				{
					progressReporter.setWarningMode();
					progressReporter << sU("Invalid Gallion Advance file: ") << filename << sU(" Line ") << i * gatesPerRay + j + 7 << sU(" invalid date/time\n");
					progressReporter.setNormalMode();
					profiles.resize(0);
					return;
				}
				profiles[i].time = sci::UtcTime(std::atoi(dateColumns[0].c_str()), std::atoi(dateColumns[1].c_str()), std::atoi(dateColumns[2].c_str()),
					std::atoi(timeColumns[0].c_str()), std::atoi(timeColumns[1].c_str()), std::atof(timeColumns[2].c_str()));
				profiles[i].azimuth = degreeF(std::atof(columns[5].c_str()));
				profiles[i].elevation = degreeF(std::atof(columns[6].c_str()));
				profiles[i].pitch = degreeF(std::atof(columns[7].c_str()));
				profiles[i].roll = degreeF(std::atof(columns[8].c_str()));
			}
			profiles[i].velocity[j] = metrePerSecondF(std::atof(columns[1].c_str()));
			profiles[i].snr[j] = unitlessF(std::atof(columns[2].c_str()));
			previousGate = gate;
		}

	}
}

void GalionAdvancedProcessor::writeToNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
{
	std::vector<sci::string> errorMessages;
	/*try
	{
		writeWindProfilesToNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter,
			m_instrumentInfo, m_calibrationInfo);
	}
	catch (sci::err err)
	{
		errorMessages.push_back(err.getErrorMessage());
	}*/
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		try
		{
			writeScanToNc(directory, i, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter,
				m_instrumentInfo, m_calibrationInfo);
		}
		catch (sci::err err)
		{
			errorMessages.push_back(err.getErrorMessage());
		}
	}

	if (errorMessages.size() > 0)
	{
		sci::string message = errorMessages[0];
		for (size_t i = 1; i < errorMessages.size(); ++i)
			message += sU("\n") + errorMessages[i];
		throw(sci::err(sci::SERR_USER, 0, message));
	}
}

void GalionAdvancedProcessor::writeWindProfilesToNc(const sci::string& directory, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter,
	const InstrumentInfo& instrumentInfo, const CalibrationInfo& calibrationInfo) const
{

	if (m_windStartTimes.size() == 0)
		return;

	DataInfo dataInfo;
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();
	dataInfo.continuous = true;
	dataInfo.startTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	dataInfo.endTime = sci::UtcTime(0, 0, 0, 0, 0, 0.0);
	if (m_windStartTimes.size() > 0)
	{
		dataInfo.startTime = m_windStartTimes[0];
		dataInfo.endTime = m_windStartTimes.back();
	}
	dataInfo.featureType = FeatureType::timeSeriesProfile;
	dataInfo.options = std::vector<sci::string>(0);
	dataInfo.processingLevel = 2;
	dataInfo.productName = sU("mean winds profile");
	dataInfo.processingOptions = processingOptions;

	//work out the averaging time - this is the difference between the scan start and end times.
	//use the median as the value for the file
	if (m_windStartTimes.size() > 0)
	{
		sci::GridData<sci::TimeInterval, 1> averagingTimes = m_windEndTimes - m_windStartTimes;
		dataInfo.averagingPeriod = sci::findKthBiggestValue(averagingTimes, averagingTimes.size() / 2);
	}

	//work out the sampling interval - this is the difference between each start time.
	//use the median as the value for the file
	if (m_windStartTimes.size() > 1)
	{
		sci::GridData<sci::TimeInterval, 1> samplingIntervals = sci::GridData<sci::UtcTime, 1>(m_windStartTimes.begin() + 1, m_windStartTimes.end()) - sci::GridData<sci::UtcTime, 1>(m_windStartTimes.begin(), m_windStartTimes.end() - 1);
		dataInfo.samplingInterval = sci::findKthBiggestValue(samplingIntervals, samplingIntervals.size() / 2);
	}

	std::vector<sci::NcDimension*> nonTimeDimensions;

	sci::NcDimension indexDimension(sU("index"), m_windU[0].size());
	nonTimeDimensions.push_back(&indexDimension);
	std::vector<sci::NcAttribute*> lidarGlobalAttributes{ };
	OutputAmfNcFile file(AmfVersion::v1_1_0, directory, instrumentInfo, author, processingSoftwareInfo, calibrationInfo, dataInfo,
		projectInfo, platform, sU("doppler lidar wind profile"), m_windStartTimes, nonTimeDimensions);

	for (size_t i = 0; i < m_windU.size(); ++i)
		sci::assertThrow(m_windU[i].size() == m_windU[0].size(), sci::err(sci::SERR_USER, 0, sU("Not all wind profiles have the same number of elements - processing aborted")));

	if (m_windU[0].size() > 0)
	{
		sci::GridData<metreF, 2> altitudes({ m_windStartTimes.size(), m_windU[0].size()}, std::numeric_limits<metreF>::quiet_NaN());
		sci::GridData<metrePerSecondF, 2> windSpeeds({ m_windStartTimes.size(), m_windU[0].size() }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		sci::GridData<degreeF, 2> windDirections({ m_windStartTimes.size(), m_windU[0].size() }, std::numeric_limits<degreeF>::quiet_NaN());
		sci::GridData<uint8_t, 2> windFlags({ m_windStartTimes.size(), m_windU[0].size() }, lidarMissingDataFlag);

		//copy data from profiles to the grid
		sci::assertThrow(platform.getFixed(), sci::err(sci::SERR_USER, 0, sU("Cannot yet deal with wind profiles on moving platforms")));
		AttitudeAverage elevation;
		AttitudeAverage azimuth;
		AttitudeAverage roll;
		degreeF latitude;
		degreeF longitude;
		metreF altitude;
		platform.getPlatformAttitudes(m_windStartTimes[0], m_windStartTimes[0], elevation, azimuth, roll);
		platform.getLocation(m_windStartTimes[0], m_windStartTimes[0], latitude, longitude, altitude);

		for (size_t i = 0; i < m_windStartTimes.size(); ++i)
		{
			for (size_t j = 0; j < m_windU[i].size(); ++j)
			{
				altitudes[i][j] = m_windHeightInterval[i] * unitlessF(j - 0.5)+altitude;
				windSpeeds[i][j] = sci::sqrt(m_windU[i][j] * m_windU[i][j] + m_windV[i][j] * m_windV[i][j]);
				windDirections[i][j] = -sci::atan2(m_windV[i][j], m_windU[i][j]) + degreeF(180) + azimuth.m_mean;
			}
		}
		//ensure we are on 0-360 range, not -180-180 range for wind directions
		for (auto& dir : windDirections)
		{
			if (dir < degreeF(0))
				dir += degreeF(360);
			if(dir > degreeF(360))
				dir -= degreeF(360);
		}

		//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
		//std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), cm_mean}, { sU("altitude"), cm_mean } };
		std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::mean} };
		std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };
		AmfNcAltitudeVariable altitudeVariable(file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & indexDimension }, altitudes, FeatureType::timeSeriesProfile);
		AmfNcVariable<metrePerSecondF> windSpeedVariable(sU("wind_speed"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & indexDimension }, sU("Mean Wind Speed"), sU("wind_speed"), windSpeeds, true, coordinates, cellMethods, windFlags);
		AmfNcVariable<degreeF> windDirectionVariable(sU("wind_from_direction"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & indexDimension }, sU("Wind From Direction"), sU("wind_from_direction"), windDirections, true, coordinates, cellMethods, windFlags);
		//AmfNcFlagVariable windFlagVariable(sU("qc_flag"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & indexDimension });

		file.writeTimeAndLocationData(platform);

		file.write(altitudeVariable, altitudes);
		file.write(windSpeedVariable, windSpeeds);
		file.write(windDirectionVariable, windDirections);
		//file.write(windFlagVariable, windFlags);
	}
}

void GalionAdvancedProcessor::writeScanToNc(const sci::string& directory, size_t scanIndex, const PersonInfo& author,
	const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
	const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter,
	const InstrumentInfo &instrumentInfo, const CalibrationInfo& calibrationInfo) const
{

	const sci::GridData<Profile, 2>& scan = m_profiles[scanIndex];
	if (scan.shape()[0] == 0)
		return;
	for (auto const& p : scan)
		assertThrow(p.snr.size() == scan[0][0].snr.size(), sci::err(sci::SERR_USER, 0, sU("The scan has profiles of different lengths and therefore cannot be processed.")));

	sci::string scanName = sU("fixed");
	if (scan.shape()[1] > 1)
	{
		if (scan[0][0].azimuth == scan[0][1].azimuth && scan[0][0].elevation == scan[0][1].elevation)
		{
			//still fixed do nothing
		}
		else if (scan[0][0].azimuth == scan[0][1].azimuth)
		{
			scanName = sU("rhi");
		}
		else if (scan[0][0].elevation == scan[0][1].elevation)
		{
			scanName = sU("ppi");
		}
	}

	if (scanName == sU("fixed") && scan.shape()[1] != 1)
		throw(sci::err(sci::SERR_USER, 0, sU("Cannot yet process a stare scan with more than 1 profile")));

	sci::ostringstream indexString;
	indexString << (scanIndex < 10 ? sU("0") : sU("")) << scanIndex + 1;
	scanName += indexString.str();

	DataInfo dataInfo;
	dataInfo.continuous = true;
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.startTime = scan[0][0].time;;
	dataInfo.endTime = scan[scan.shape()[0]-1][0].time;
	dataInfo.featureType = FeatureType::timeSeriesProfile;
	dataInfo.options = { scanName };
	dataInfo.processingLevel = 1;
	dataInfo.productName = sU("aerosol backscatter radial winds");
	dataInfo.processingOptions = processingOptions;

	//axis order is time, range, angle
	std::array<size_t, 3> shape;
	shape[0] = scan.shape()[0];
	shape[1] = scan[0][0].snr.size();
	shape[2] = scan.shape()[1];
	std::array<size_t, 2> angleShape{shape[0], shape[2]};

	sci::GridData<metreF, 3> ranges(shape);
	sci::GridData<degreeF, 2> instrumentRelativeAzimuthAngles(angleShape);
	sci::GridData<degreeF, 2> attitudeCorrectedAzimuthAngles(angleShape);
	sci::GridData<degreeF, 2> instrumentRelativeElevationAngles(angleShape);
	sci::GridData<degreeF, 2> attitudeCorrectedElevationAngles(angleShape);
	sci::GridData<metrePerSecondF, 3> instrumentRelativeDopplerVelocities(shape);
	sci::GridData<metrePerSecondF, 3> motionCorrectedDopplerVelocities(shape);
	sci::GridData<unitlessF, 3> snrsPlusOne(shape);
	sci::GridData<uint8_t, 3> dopplerVelocityFlags(shape);
	sci::GridData<sci::UtcTime, 1> scanStartTimes(shape[0]);
	sci::GridData<sci::UtcTime, 1> scanEndTimes(shape[0]);

	//size_t pulsesPerRay;
	//size_t raysPerPoint;
	//metreF focus;
	//metrePerSecondF dopplerResolution;

	//ranges
	for (size_t i = 0; i < shape[0]; ++i)
		for (size_t j = 0; j < shape[1]; ++j)
			for (size_t k = 0; k < shape[2]; ++k)
				ranges[i][j][k] = m_rangeInterval * unitlessF(j + 0.5);
	//doppler
	for (size_t i = 0; i < shape[0]; ++i)
		for (size_t j = 0; j < shape[1]; ++j)
			for (size_t k = 0; k < shape[2]; ++k)
				instrumentRelativeDopplerVelocities[i][j][k] = scan[i][k].velocity[j];
	//snr
	for (size_t i = 0; i < shape[0]; ++i)
		for (size_t j = 0; j < shape[1]; ++j)
			for (size_t k = 0; k < shape[2]; ++k)
				snrsPlusOne[i][j][k] = scan[i][k].snr[j];

	//doppler flags
	for (size_t i = 0; i < shape[0]; ++i)
	{
		for (size_t j = 0; j < shape[1]; ++j)
		{
			for (size_t k = 0; k < shape[2]; ++k)
			{
				if (snrsPlusOne[i][j][k] < unitlessF(2))
					dopplerVelocityFlags[i][j][k] = lidarSnrBelow1Flag;
				else if (snrsPlusOne[i][j][k] < unitlessF(3))
					dopplerVelocityFlags[i][j][k] = lidarSnrBelow2Flag;
				else if (snrsPlusOne[i][j][k] < unitlessF(4))
					dopplerVelocityFlags[i][j][k] = lidarSnrBelow3Flag;
				else if (sci::abs<metrePerSecondF>(instrumentRelativeDopplerVelocities[i][j][k]) > metrePerSecondF(19))
					dopplerVelocityFlags[i][j][k] = lidarDopplerOutOfRangeFlag;
				else
					dopplerVelocityFlags[i][j][k] = lidarGoodDataFlag;
			}
		}
	}

	//angles
	for (size_t i = 0; i < angleShape[0]; ++i)
		for (size_t j = 0; j < angleShape[1]; ++j)
			instrumentRelativeAzimuthAngles[i][j] = scan[i][j].azimuth;
	for (size_t i = 0; i < angleShape[0]; ++i)
		for (size_t j = 0; j < angleShape[1]; ++j)
			instrumentRelativeElevationAngles[i][j] = scan[i][j].elevation;

	//times
	for (size_t i = 0; i < shape[0]; ++i)
		scanStartTimes[i] = scan[i][0].time;
	for (size_t i = 0; i < shape[0]; ++i)
		scanEndTimes[i] = scan[i][shape[2]-1].time;

	//do attitude corrections
	for (size_t i = 0; i < angleShape[0]; ++i)
		for (size_t j = 0; j < angleShape[1]; ++j)
			platform.correctDirection(scanStartTimes[i], scanEndTimes[i], instrumentRelativeAzimuthAngles[i][j], instrumentRelativeElevationAngles[i][j],
				attitudeCorrectedAzimuthAngles[i][j], attitudeCorrectedElevationAngles[i][j]);

	if (platform.getFixed())
	{
		motionCorrectedDopplerVelocities = instrumentRelativeDopplerVelocities;
	}
	else
	{
		for (size_t i = 0; i < shape[0]; ++i)
		{
			for (size_t j = 0; j < shape[1]; ++j)
			{
				for (size_t k = 0; k < shape[2]; ++k)
				{
					platform.correctVelocityVector(scanStartTimes[i], scanEndTimes[i], attitudeCorrectedAzimuthAngles[i][k], attitudeCorrectedElevationAngles[i][k],
						instrumentRelativeDopplerVelocities, motionCorrectedDopplerVelocities);
				}
			}
		}
	}



	//work out the averaging time - this is the difference between the scan start and end times.
	//use the median as the value for the file
	if (scanStartTimes.size() > 0)
		dataInfo.averagingPeriod = sci::median(scanEndTimes - scanStartTimes);

	//work out the sampling interval - this is the difference between each scan time.
	//use the median as the value for the file
	if (scanStartTimes.size() > 1)
	{
		sci::GridData<sci::TimeInterval, 1> samplingIntervals = sci::GridData<sci::UtcTime, 1>(scanStartTimes.begin() + 1, scanStartTimes.end()) - sci::GridData<sci::UtcTime, 1>(scanStartTimes.begin(), scanStartTimes.end() - 1);
		dataInfo.samplingInterval = sci::median(samplingIntervals);
	}

	sci::NcDimension rangeIndexDimension(sU("index_of_range"), shape[1]);
	sci::NcDimension angleIndexDimension(sU("index_of_angle"), shape[2]);
	std::vector<sci::NcDimension*> nonTimeDimensions{ &rangeIndexDimension, & angleIndexDimension };

	//add standard to the processing options - this is as opposed to advanced which is a reprocessing of the raw data
	dataInfo.options.push_back(sU("standard"));

	//add comments
	if (dataInfo.processingOptions.comment.length() > 0)
		dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\n");
	//dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("Range gates may overlap - hence range may not grow in increments of gate_length.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_azimuth_angle is relative to the instrument with 0 degrees being to the \"front\" of the instruments.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_view_angle is relative to the instruments with 0 degrees being \"horizontal\", positive being upwards, negative being downwards.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_azimuth_angle_earth_frame is relative to the geoid with 0 degrees being north.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_view_angle_earth_frame is relative to the geoid with 0 degrees being horizontal, positive being upwards, negative being downwards.");
	//dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nradial_velocity_of_scatterers_away_from_instrument_earth_frame is motion relative to the geoid in the direction specified by sensor_azimuth_angle_earth_frame and sensor_view_angle_earth_frame. Positive is in the direction specified, negative is in the opposite direction specified.");

	//set the global attributes
	//sci::stringstream pulsesPerRayStr;
	//sci::stringstream raysPerPointStr;
	//sci::stringstream velocityResolutionStr;
	sci::stringstream nGatesStr;
	sci::stringstream gateLengthStr;
	sci::stringstream maxRangeStr;
	//pulsesPerRayStr << pulsesPerRay;
	//raysPerPointStr << raysPerPoint;
	//velocityResolutionStr << dopplerResolution;
	nGatesStr << shape[2];
	gateLengthStr << m_rangeInterval;
	maxRangeStr << unitlessF(float(shape[2])) * m_rangeInterval;
	sci::NcAttribute wavelengthAttribute(sU("laser_wavelength"), sU("1550 nm"));
	sci::NcAttribute energyAttribute(sU("laser_pulse_energy"), sU("1.0e-05 J"));
	sci::NcAttribute pulseFrequencyAttribute(sU("pulse_repetition_frequency"), sU("15000 s-1"));
	//sci::NcAttribute pulsesPerRayAttribute(sU("pulses_per_ray"), pulsesPerRayStr.str());
	//sci::NcAttribute raysPerPointAttribute(sU("rays_per_point"), raysPerPointStr.str());
	sci::NcAttribute diameterAttribute(sU("lens_diameter"), sU("0.08 m"));
	sci::NcAttribute divergenceAttribute(sU("beam_divergence"), sU("0.00189 degrees"));
	sci::NcAttribute pulseLengthAttribute(sU("pulse_length"), sU("2e-07 s"));
	sci::NcAttribute samplingFrequencyAttribute(sU("sampling_frequency"), sU("1.5e+07 Hz"));
	sci::NcAttribute focusAttribute(sU("focus"), sU("inf"));
	//sci::NcAttribute velocityResolutionAttribute(sU("velocity_resolution"), velocityResolutionStr.str());
	sci::NcAttribute nGatesAttribute(sU("number_of_gates"), nGatesStr.str());
	sci::NcAttribute gateLengthAttribute(sU("gate_length"), gateLengthStr.str());
	sci::NcAttribute fftAttribute(sU("fft_length"), sU("1024"));
	sci::NcAttribute maxRangeAttribute(sU("maximum_range"), maxRangeStr.str());


	//std::vector<sci::NcAttribute*> lidarGlobalAttributes{ &wavelengthAttribute, & energyAttribute,
	//	& pulseFrequencyAttribute, & pulsesPerRayAttribute, & raysPerPointAttribute, & diameterAttribute,
	//	& divergenceAttribute, & pulseLengthAttribute, & samplingFrequencyAttribute, & focusAttribute,
	//	& velocityResolutionAttribute, & nGatesAttribute, & gateLengthAttribute, & fftAttribute, & maxRangeAttribute};

	std::vector<sci::NcAttribute*> lidarGlobalAttributes{ &wavelengthAttribute, & energyAttribute,
		& pulseFrequencyAttribute, & diameterAttribute,		& divergenceAttribute, & pulseLengthAttribute,
		& samplingFrequencyAttribute, & focusAttribute,& nGatesAttribute, & gateLengthAttribute,
		& fftAttribute, & maxRangeAttribute};

	OutputAmfNcFile file(AmfVersion::v1_1_0, directory, instrumentInfo, author, processingSoftwareInfo, calibrationInfo, dataInfo,
		projectInfo, platform, sU("doppler lidar scan"), scanStartTimes, nonTimeDimensions, sU(""), lidarGlobalAttributes);

	//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), cm_mean}, { sU("range"), cm_mean }, {sU("sensor_azimuth_angle"), cm_point}, {sU("sensor_view_angle"), cm_point} };
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ {sU("time"), cm_point} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAnglesEarthFrame{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsRange{ };
	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };


	sci::GridData<uint8_t, 0> includeAllFlag(1);
	//create the variables - note we set swap to true for these variables which swaps the data into the varaibles rather than copying it. This saved memory
	AmfNcVariable<metreF> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & rangeIndexDimension, & angleIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), sU("range"), ranges, true, coordinates, cellMethodsRange, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<degreeF> azimuthVariable(sU("sensor_azimuth_angle_instrument_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & angleIndexDimension }, sU("Scanning head azimuth angle in the instrument frame of reference"), sU(""), instrumentRelativeAzimuthAngles, true, coordinates, cellMethodsAngles, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<degreeF> elevationVariable(sU("sensor_view_angle_instrument_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & angleIndexDimension }, sU("Scanning head elevation angle in the instrument frame of reference"), sU(""), instrumentRelativeElevationAngles, true, coordinates, cellMethodsAngles, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<degreeF> azimuthVariableEarthFrame(sU("sensor_azimuth_angle_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & angleIndexDimension }, sU("Scanning head azimuth angle in the Earth frame of reference"), sU(""), attitudeCorrectedAzimuthAngles, true, coordinates, cellMethodsAnglesEarthFrame, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<degreeF> elevationVariableEarthFrame(sU("sensor_view_angle_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & angleIndexDimension }, sU("Scanning head elevation angle in the Earth frame of reference"), sU(""), attitudeCorrectedElevationAngles, true, coordinates, cellMethodsAnglesEarthFrame, sci::GridData<uint8_t, 0>(1));
	AmfNcVariable<metrePerSecondF> dopplerVariable(sU("radial_velocity_of_scatterers_away_from_instrument"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & rangeIndexDimension, & angleIndexDimension }, sU("Radial Velocity of Scatterers Away From Instrument"), sU("radial_velocity_of_scatterers_away_from_instrument"), instrumentRelativeDopplerVelocities, true, coordinates, cellMethodsData, dopplerVelocityFlags);
	//AmfNcVariable<metrePerSecondF, decltype(motionCorrectedDopplerVelocities), true> dopplerVariableEarthFrame(sU("radial_velocity_of_scatterers_away_from_instrument_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Radial Velocity of Scatterers Away From Instrument Earth Frame"), sU(""), motionCorrectedDopplerVelocities, true, coordinates, cellMethodsData);
	//AmfNcVariable<perSteradianPerMetreF> backscatterVariable(sU("attenuated_aerosol_backscatter_coefficient"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & rangeIndexDimension, & angleIndexDimension }, sU("Attenuated Aerosol Backscatter Coefficient"), sU(""), backscatters, true, coordinates, cellMethodsData, backscatterFlags);
	AmfNcVariable<unitlessF> snrsPlusOneVariable(sU("signal_to_noise_ratio_plus_1"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & rangeIndexDimension, & angleIndexDimension }, sU("Signal to Noise Ratio: SNR+1"), sU(""), snrsPlusOne, true, coordinates, cellMethodsData, sci::GridData<uint8_t, 0>(1));
	AmfNcFlagVariable dopplerFlagVariable(sU("qc_flag_radial_velocity_of_scatterers_away_from_instrument"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & rangeIndexDimension, & angleIndexDimension });
	//AmfNcFlagVariable backscatterFlagVariable(sU("qc_flag_backscatter"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), & rangeIndexDimension, & angleIndexDimension });

	file.writeTimeAndLocationData(platform);

	file.write(rangeVariable, ranges);
	file.write(azimuthVariable, instrumentRelativeAzimuthAngles);
	file.write(azimuthVariableEarthFrame, attitudeCorrectedAzimuthAngles);
	file.write(elevationVariable, instrumentRelativeElevationAngles);
	file.write(elevationVariableEarthFrame, attitudeCorrectedElevationAngles);
	file.write(dopplerVariable, instrumentRelativeDopplerVelocities);
	//file.write(dopplerVariableEarthFrame);
	//file.write(backscatterVariable, backscatters);
	file.write(snrsPlusOneVariable, snrsPlusOne);
	file.write(dopplerFlagVariable, dopplerVelocityFlags);
	//file.write(backscatterFlagVariable, backscatterFlags);
}