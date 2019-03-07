#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"HplHeader.h"
#include"HplProfile.h"
#include"ProgressReporter.h"
#include<svector/sstring.h>
#include"ProgressReporter.h"

void LidarBackscatterDopplerProcessor::readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter, wxWindow *parent)
{
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		readData(inputFilenames[i], progressReporter, parent, i == 0);
	}

	//correct azimuths and elevations for platform orientation
	m_correctedAzimuths.resize(m_profiles.size(), degree(0.0));
	m_correctedElevations.resize(m_profiles.size(), degree(0.0));
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		platform.correctDirection(m_profiles[i].getTime<sci::UtcTime>(), m_profiles[i].getAzimuth(), m_profiles[i].getElevation(), m_correctedAzimuths[i], m_correctedElevations[i]);
	}
}
void LidarBackscatterDopplerProcessor::readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent, bool clear)
{
	if (clear)
	{
		m_hasData = false;
		m_profiles.clear();
		m_hplHeaders.clear();
		m_headerIndex.clear();
		m_betaFlags.clear();
		m_dopplerFlags.clear();
	}

	std::fstream fin;
	fin.open(sci::nativeUnicode(inputFilename), std::ios::in);
	if (!fin.is_open())
		throw(sU("Could not open lidar file ") + inputFilename + sU("."));

	progressReporter << sU("Reading file.\n");

	//Read the HPL header from the top of the file
	m_hplHeaders.resize(m_hplHeaders.size() + 1);
	fin >> m_hplHeaders.back();

	bool readingOkay = true;
	size_t nRead = 0;
	//Read each profile in turn
	while (readingOkay)
	{
		m_profiles.resize(m_profiles.size() + 1);
			//Read the profile itself - it takes the header as an input to give details of what needs reading
		readingOkay = m_profiles.back().readFromStream(fin, m_hplHeaders.back());
		if (!readingOkay) //we hit the end of the file while reading this profile
			m_profiles.resize(m_profiles.size() - 1);
		++nRead;
		if (readingOkay)
		{
			m_headerIndex.push_back(m_hplHeaders.size() - 1);//record which header this profile is linked to
			std::vector<uint8_t> dopplerVelocityFlags(m_profiles.back().nGates(), lidarGoodDataFlag);
			std::vector<uint8_t> betaFlags(m_profiles.back().nGates(), lidarGoodDataFlag);
			//flag for out of range doppler
			sci::assign(dopplerVelocityFlags, dopplerVelocityFlags == lidarGoodDataFlag && (m_profiles.back().getDopplerVelocities() > metrePerSecond(19.0) || m_profiles.back().getDopplerVelocities() < metrePerSecond(-19.0)), lidarDopplerOutOfRangeFlag);
			//flag for bad snr - not intensity reported by instrument is snr+1
			sci::assign(dopplerVelocityFlags, dopplerVelocityFlags == lidarGoodDataFlag && m_profiles.back().getIntensities() < unitless(2.0), lidarSnrBelow1Flag);
			sci::assign(dopplerVelocityFlags, dopplerVelocityFlags == lidarGoodDataFlag && m_profiles.back().getIntensities() < unitless(3.0), lidarSnrBelow2Flag);
			sci::assign(dopplerVelocityFlags, dopplerVelocityFlags == lidarGoodDataFlag && m_profiles.back().getIntensities() < unitless(4.0), lidarSnrBelow3Flag);
			sci::assign(betaFlags, dopplerVelocityFlags == lidarGoodDataFlag && m_profiles.back().getIntensities() < unitless(2.0), lidarSnrBelow1Flag);
			sci::assign(betaFlags, dopplerVelocityFlags == lidarGoodDataFlag && m_profiles.back().getIntensities() < unitless(3.0), lidarSnrBelow2Flag);
			sci::assign(betaFlags, dopplerVelocityFlags == lidarGoodDataFlag && m_profiles.back().getIntensities() < unitless(4.0), lidarSnrBelow3Flag);
			m_betaFlags.push_back(betaFlags);
			m_dopplerFlags.push_back(dopplerVelocityFlags);

			if (nRead == 1)
				progressReporter << sU("Read profile 1");
			else if (nRead <= 50)
				progressReporter << sU(", ") << nRead;
			else if (nRead % 10 == 0)
				progressReporter << sU(", ") << nRead - 9 << sU("-") << nRead;
		}
		if (progressReporter.shouldStop())
			break;

	}
	if (progressReporter.shouldStop())
	{
		progressReporter << sU(", stopped at user request.\n");
		fin.close();
		return;

	}
	progressReporter << sU(", reading done.\n");

	bool gatesGood = true;
	for (size_t i = m_profiles.size()-nRead; i < m_profiles.size(); ++i)
	{
		std::vector<size_t> gates = m_profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			if (gates[j] != j)
			{
				sci::ostringstream error;
				error << sU("The plotting code currently assumes gates go 0, 1, 2, 3, ... but profile ") << i << sU(" (0 indexed) in file ") << inputFilename << sU(" did not have this layout.");
				throw(error.str());
			}
	}
	m_hasData = true;
}


std::vector<second> LidarBackscatterDopplerProcessor::getTimesSeconds() const
{
	std::vector<second> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getTime<second>();
	}
	return result;
}

std::vector<sci::UtcTime> LidarBackscatterDopplerProcessor::getTimesUtcTime() const
{
	std::vector<sci::UtcTime> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getTime<sci::UtcTime>();
	}
	return result;
}

std::vector<std::vector<perSteradianPerMetre>> LidarBackscatterDopplerProcessor::getBetas() const
{
	std::vector<std::vector<perSteradianPerMetre>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getBetas();
	}
	return result;
}

std::vector<std::vector<unitless>> LidarBackscatterDopplerProcessor::getSignalToNoiseRatios() const
{
	std::vector<std::vector<unitless>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getIntensities()-unitless(1.0);
	}
	return result;
}

std::vector<std::vector<metrePerSecond>> LidarBackscatterDopplerProcessor::getDopplerVelocities() const
{
	std::vector<std::vector<metrePerSecond>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getDopplerVelocities();
	}
	return result;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateBoundariesForPlotting(size_t profileIndex) const
{
	//because there may be gate overlapping, for plotting the lower and upper gate boundaries
	//aren't quite what we want.
	metre interval = m_hplHeaders[profileIndex].rangeGateLength;
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (m_profiles[profileIndex].nGates() > 533)
		interval /= unitless(m_hplHeaders[m_headerIndex[profileIndex]].pointsPerGate);

	std::vector<metre> result = getGateCentres(profileIndex) - unitless(0.5)*interval;
	result.push_back(result.back() + interval);

	return result;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateLowerBoundaries(size_t profileIndex) const
{
	if (m_profiles.size() == 0)
		return std::vector<metre>(0);

	std::vector<metre> result(m_profiles[profileIndex].nGates());
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = unitless(i) * m_hplHeaders[m_headerIndex[profileIndex]].rangeGateLength;
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (m_profiles[profileIndex].nGates() > 533)
		result /= unitless(m_hplHeaders[m_headerIndex[profileIndex]].pointsPerGate);
	return result;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateUpperBoundaries(size_t profileIndex) const
{
	return getGateLowerBoundaries(profileIndex) + m_hplHeaders[m_headerIndex[profileIndex]].rangeGateLength;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateCentres(size_t profileIndex) const
{
	return getGateLowerBoundaries(profileIndex) + unitless(0.5) * m_hplHeaders[m_headerIndex[profileIndex]].rangeGateLength;
}

void PlotableLidar::setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent, HplHeader hplHeader)
{
	*window = new splotframe(parent, true);
	(*window)->SetClientSize(1000, 1000);

	sci::ostringstream plotTitle;
	plotTitle << sU("Lidar Backscatter (m#u-1#d sr#u-1#d)") << extraDescriptor << sU("\n") << hplHeader.filename;
	*plot = (*window)->addplot(0.1, 0.1, 0.75, 0.75, false, false, plotTitle.str(), 20.0, 3.0);
	(*plot)->getxaxis()->settitlesize(15);
	(*plot)->getyaxis()->settitledistance(4.5);
	(*plot)->getyaxis()->settitlesize(15);
	(*plot)->getxaxis()->setlabelsize(15);
	(*plot)->getyaxis()->setlabelsize(15);

	splotlegend *legend = (*window)->addlegend(0.86, 0.25, 0.13, 0.65, wxColour(0, 0, 0), 0.0);
	legend->addentry(sU(""), g_lidarColourscale, false, false, 0.05, 0.3, 15, sU(""), 0, 0.05, wxColour(0, 0, 0), 128, false, 150, false);
}

void LidarScanningProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	DataInfo dataInfo;
	dataInfo.continuous = true;
	dataInfo.samplingInterval = second(OutputAmfNcFile::getFillValue());//set to fill value initially - calculate it later
	dataInfo.averagingPeriod = second(OutputAmfNcFile::getFillValue());//set to fill value initially - calculate it later
	dataInfo.startTime = getTimesUtcTime()[0];
	dataInfo.endTime = getTimesUtcTime().back();
	dataInfo.featureType = ft_timeSeriesPoint;
	dataInfo.maxLat = sci::max<degree>(platform.getPlatformInfo().latitudes);
	dataInfo.minLat = sci::min<degree>(platform.getPlatformInfo().latitudes);
	dataInfo.maxLon = sci::max<degree>(platform.getPlatformInfo().longitudes);
	dataInfo.minLon = sci::min<degree>(platform.getPlatformInfo().longitudes);
	dataInfo.options = getProcessingOptions();
	dataInfo.processingLevel = 1;
	dataInfo.productName = sU("backscatter radial winds");
	dataInfo.processingOptions = processingOptions;

	//build up our data arrays. We must account for the fact that the user could change
	//the number of profiles in a scan pattern or the range of the instruemnt during a day
	//so we work through the profiles checking when the filename changes. At this point things could
	//change. The number of gates in each profile can change within a file, but the distance per
	//gate cannot, so we find the longest range per hile and use this
	//work out how many lidar profiles there are per VAD and find the start time of each VAD and the ranges array for each vad
	size_t maxProfilesPerScan = 1;
	size_t thisProfilesPerScan = 1;
	std::vector<sci::UtcTime> allTimes = getTimesUtcTime();
	std::vector<degree> allAzimuths = getAzimuths();
	std::vector<degree> allElevations = getElevations();
	std::vector<std::vector<metrePerSecond>> allDopplerVelocities = getDopplerVelocities();
	std::vector<std::vector<perSteradianPerMetre>> allBackscatters = getBetas();
	std::vector<std::vector<uint8_t>> allDopplerVelocityFlags = getDopplerFlags();
	std::vector<std::vector<uint8_t>> allBackscatterFlags = getBetaFlags();
	std::vector<sci::UtcTime> scanStartTimes;
	std::vector<sci::UtcTime> scanEndTimes;
	scanStartTimes.reserve(allTimes.size() / 6);
	scanEndTimes.reserve(allTimes.size() / 6);
	//a 2d array for ranges (time,range)
	std::vector<std::vector<metre>> ranges;
	ranges.reserve(allTimes.size() / 6);
	//a 2d array for azimuth (time, profile within scan)
	std::vector<std::vector<degree>> azimuthAngles;
	azimuthAngles.reserve(allTimes.size() / 6);
	//a 2d array for elevation angle (time, profile within scan)
	std::vector<std::vector<degree>> elevationAngles;
	elevationAngles.reserve(allTimes.size() / 6);
	//a 3d array for doppler speeds (time, profile within a scan, elevation) - note this is not the order we need to output
	//so immediately after this code we transpose the second two dimensions (time, elevation, profile within a scan)
	std::vector<std::vector<std::vector<metrePerSecond>>> dopplerVelocities;
	dopplerVelocities.reserve(allTimes.size() / 6);
	//a 3d array for backscatters (time, profile within a scan, elevation) - note this is not the order we need to output
	//so immediately after this code we transpose the second two dimensions (time, elevation, profile within a scan)
	std::vector<std::vector<std::vector<perSteradianPerMetre>>> backscatters;
	backscatters.reserve(allTimes.size() / 6);
	//As above, but for the flags
	std::vector < std::vector<std::vector<uint8_t>>> dopplerVelocityFlags(backscatters.size());
	std::vector < std::vector<std::vector<uint8_t>>> backscatterFlags(backscatters.size());
	dopplerVelocityFlags.reserve(allTimes.size() / 6);
	backscatterFlags.reserve(allTimes.size() / 6);

	HplHeader previousHeader;
	size_t maxNGatesThisScan;
	size_t maxNGates;

	if (allTimes.size() > 0)
	{
		//keep track of the previous header as we go throught the data. We compare current to previous
		//header to check if we have moved to the next scan.
		//Set all the variables using the first profile.
		previousHeader = getHeaderForProfile(0);
		scanStartTimes.push_back(allTimes[0]);
		maxNGatesThisScan = getNGates(0);
		maxNGates = maxNGatesThisScan;
		ranges.push_back(getGateCentres(0));
		azimuthAngles.resize(1);
		azimuthAngles.back().reserve(6);
		azimuthAngles.back().push_back(allAzimuths[0]);
		elevationAngles.resize(1);
		elevationAngles.back().reserve(6);
		elevationAngles.back().push_back(allElevations[0]);
		dopplerVelocities.resize(1);
		dopplerVelocities.back().reserve(6);
		dopplerVelocities.back().push_back(allDopplerVelocities[0]);
		backscatters.resize(1);
		backscatters.back().reserve(6);
		backscatters.back().push_back(allBackscatters[0]);
		dopplerVelocityFlags.resize(1);
		dopplerVelocityFlags.back().reserve(6);
		dopplerVelocityFlags.back().push_back(allDopplerVelocityFlags[0]);
		backscatterFlags.resize(1);
		backscatterFlags.back().reserve(6);
		backscatterFlags.back().push_back(allBackscatterFlags[0]);


		//Work through all the profiles finding the needed data
		for (size_t i = 1; i < allTimes.size(); ++i)
		{
			if (getHeaderForProfile(i).filename == previousHeader.filename)
			{
				++thisProfilesPerScan;
				//we want to find the maximum number of gates in the whole set
				//For a particular scan we want to set the ranges using the profile with the most gates, but
				//we can't use the longest profile in the whole dataset as the gate length may change between
				//scans.
				if (getNGates(i) > maxNGatesThisScan)
				{
					maxNGatesThisScan = getNGates(i);
					maxNGates = std::max(maxNGates, maxNGatesThisScan);
					ranges.back() = getGateCentres(i);
				}
			}
			else
			{
				previousHeader = getHeaderForProfile(i);
				maxProfilesPerScan = std::max(maxProfilesPerScan, thisProfilesPerScan);
				thisProfilesPerScan = 1;
				scanStartTimes.push_back(allTimes[i]);
				scanEndTimes.push_back(allTimes[i - 1] + (unitless(getHeaderForProfile(i).pulsesPerRay * getHeaderForProfile(i).nRays) / sci::Physical<sci::Hertz<1, 3>>(15.0)).value<second>()); //this is the time of the last profile in the scan plus the duration of this profile
				maxNGatesThisScan = getNGates(i);
				ranges.push_back(getGateCentres(i));
				azimuthAngles.resize(azimuthAngles.size() + 1);
				azimuthAngles.back().reserve(maxProfilesPerScan);
				elevationAngles.resize(elevationAngles.size() + 1);
				elevationAngles.back().reserve(maxProfilesPerScan);
				dopplerVelocities.resize(dopplerVelocities.size() + 1);
				dopplerVelocities.back().reserve(maxProfilesPerScan);
				backscatters.resize(backscatters.size() + 1);
				backscatters.back().reserve(maxProfilesPerScan);
				dopplerVelocityFlags.resize(dopplerVelocityFlags.size() + 1);
				dopplerVelocityFlags.back().reserve(maxProfilesPerScan);
				backscatterFlags.resize(backscatterFlags.size() + 1);
				backscatterFlags.back().reserve(maxProfilesPerScan);
			}
			azimuthAngles.back().push_back(allAzimuths[i]);
			elevationAngles.back().push_back(allElevations[i]);
			dopplerVelocities.back().push_back(allDopplerVelocities[i]);
			backscatters.back().push_back(allBackscatters[i]);
			dopplerVelocityFlags.back().push_back(allDopplerVelocityFlags[i]);
			backscatterFlags.back().push_back(allBackscatterFlags[i]);
		}
		//Some final items that need doing regaring the last scan/profile.
		scanEndTimes.push_back(allTimes.back() + (unitless(getHeaderForProfile(allTimes.size() - 1).pulsesPerRay * getHeaderForProfile(allTimes.size() - 1).nRays) / sci::Physical<sci::Hertz<1, 3>>(15.0)).value<second>()); //this is the time of the last profile in the scan plus the duration of this profile);
		maxProfilesPerScan = std::max(maxProfilesPerScan, thisProfilesPerScan);
	}

	//Now we must transpose each element of the doppler and backscatter data
	//so they are indexed by the range before the profile within the scan.
	for (size_t i = 0; i < dopplerVelocities.size(); ++i)
	{
		dopplerVelocities[i] = sci::transpose(dopplerVelocities[i]);
		backscatters[i] = sci::transpose(backscatters[i]);
		dopplerVelocityFlags[i] = sci::transpose(dopplerVelocityFlags[i]);
		backscatterFlags[i] = sci::transpose(backscatterFlags[i]);
	}


	//expand the arrays, padding with fill value as needed
	for (size_t i = 0; i < ranges.size(); ++i)
	{
		ranges[i].resize(maxNGates, metre(OutputAmfNcFile::getFillValue()));
		azimuthAngles[i].resize(maxProfilesPerScan, degree(OutputAmfNcFile::getFillValue()));
		elevationAngles[i].resize(maxProfilesPerScan, degree(OutputAmfNcFile::getFillValue()));
		dopplerVelocities[i].resize(maxNGates);
		backscatters[i].resize(maxNGates);
		for (size_t j = 0; j < maxNGates; ++j)
		{
			dopplerVelocities[i][j].resize(maxProfilesPerScan, metrePerSecond(OutputAmfNcFile::getFillValue()));
			backscatters[i][j].resize(maxProfilesPerScan, perSteradianPerMetre(OutputAmfNcFile::getFillValue()));
			dopplerVelocityFlags[i][j].resize(maxProfilesPerScan, lidarNoDataFlag);
			backscatterFlags[i][j].resize(maxProfilesPerScan, lidarNoDataFlag);
		}
	}

	//work out the averaging time - this is the difference between the scan start and end times.
	//use the median as the value for the file
	if (scanStartTimes.size() > 0)
	{
		std::vector<sci::TimeInterval> averagingTimes = scanEndTimes - scanStartTimes;
		std::vector < sci::TimeInterval> sortedAveragingTimes = averagingTimes;
		std::sort(sortedAveragingTimes.begin(), sortedAveragingTimes.end());
		dataInfo.averagingPeriod = second(sortedAveragingTimes[sortedAveragingTimes.size() / 2]);
	}

	//work out the sampling interval - this is the difference between each scan time.
	//use the median as the value for the file
	if (scanStartTimes.size() > 1)
	{
		std::vector<sci::TimeInterval> samplingIntervals = std::vector<sci::UtcTime>(scanStartTimes.begin()+1, scanStartTimes.end()) - std::vector<sci::UtcTime>(scanStartTimes.begin(), scanStartTimes.end() - 1);
		std::vector < sci::TimeInterval> sortedSamplingIntervals = samplingIntervals;
		std::sort(sortedSamplingIntervals.begin(), sortedSamplingIntervals.end());
		dataInfo.samplingInterval = second(sortedSamplingIntervals[sortedSamplingIntervals.size() / 2]);
	}

	sci::NcDimension rangeIndexDimension(sU("index_of_range"), maxNGates);
	sci::NcDimension angleIndexDimension(sU("index_of_angle"), maxProfilesPerScan);
	std::vector<sci::NcDimension*> nonTimeDimensions{ &rangeIndexDimension, &angleIndexDimension };



	OutputAmfNcFile file(directory, getInstrumentInfo(), author, processingSoftwareInfo, getCalibrationInfo(), dataInfo,
		projectInfo, platform, scanStartTimes, platform.getPlatformInfo().longitudes[0], platform.getPlatformInfo().latitudes[0], nonTimeDimensions);

	AmfNcVariable<metre> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), metre(0), metre(10000.0));
	AmfNcVariable<degree> azimuthVariable(sU("sensor_azimuth_angle"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning head azimuth angle"), degree(0), degree(360.0));
	AmfNcVariable<degree> elevationVariable(sU("sensor_view_angle"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning head elevation angle"), degree(-90.0), degree(90.0));
	AmfNcVariable<metrePerSecond> dopplerVariable(sU("los_radial_wind_speed"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Line of sight radial wind speed"), metrePerSecond(-19.0), metrePerSecond(19.0));
	AmfNcVariable<perSteradianPerMetre> backscatterVariable(sU("attenuated_aerosol_backscatter_coefficient"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Attenuated Aerosol Backscatter Coefficient"), perSteradianPerMetre(0.0), perSteradianPerMetre(1e-3));
	AmfNcFlagVariable dopplerFlagVariable(sU("los_radial_wind_speed_qc_flag"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension });
	AmfNcFlagVariable backscatterFlagVariable(sU("attenuated_aerosol_backscatter_coefficient_qc_flag"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension });

	file.write(rangeVariable, ranges);
	file.write(azimuthVariable, azimuthAngles);
	file.write(elevationVariable, elevationAngles);
	file.write(dopplerVariable, dopplerVelocities);
	file.write(backscatterVariable, backscatters);
}