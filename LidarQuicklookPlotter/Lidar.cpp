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
		readData(inputFilenames[i], platform, progressReporter, parent, i == 0);
		if (progressReporter.shouldStop())
			break;
	}
}
void LidarBackscatterDopplerProcessor::readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter, wxWindow *parent, bool clear)
{
	if (clear)
	{
		m_hasData = false;
		m_profiles.clear();
		m_hplHeaders.clear();
		m_headerIndex.clear();
		m_betaFlags.clear();
		m_dopplerFlags.clear();
		m_correctedAzimuths.clear();
		m_correctedElevations.clear();
		m_correctedDopplerVelocities.clear();
	}

	std::fstream fin;
	fin.open(sci::nativeUnicode(inputFilename), std::ios::in);
	if (!fin.is_open())
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, sU("Could not open lidar file ") + inputFilename + sU(".")));

	progressReporter << sU("Reading file ") << inputFilename << sU(".\n");

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

			//correct azimuths and elevations for platform orientation
			m_correctedAzimuths.resize(m_profiles.size(), degree(0.0));
			m_correctedElevations.resize(m_profiles.size(), degree(0.0));
			second profileDuration = (unitless(m_hplHeaders.back().pulsesPerRay) / sci::Physical<sci::Hertz<1, 3>>(15.0));
			platform.correctDirection(m_profiles.back().getTime<sci::UtcTime>(), m_profiles.back().getTime<sci::UtcTime>()+ profileDuration, m_profiles.back().getAzimuth(), m_profiles.back().getElevation(), m_correctedAzimuths.back(), m_correctedElevations.back());
			metrePerSecond u;
			metrePerSecond v;
			metrePerSecond w;
			platform.getInstrumentVelocity(m_profiles.back().getTime<sci::UtcTime>(), m_profiles.back().getTime<sci::UtcTime>() + profileDuration, u, v, w);
			metrePerSecond offset = u * sci::sin(m_correctedAzimuths.back())*sci::cos(m_correctedElevations.back())
				+ v * sci::cos(m_correctedAzimuths.back())*sci::cos(m_correctedElevations.back())
				+ w * sci::sin(m_correctedElevations.back());
			m_correctedDopplerVelocities.push_back(m_profiles.back().getDopplerVelocities()+offset);

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
		sci::replacenans(m_correctedDopplerVelocities, metrePerSecond(OutputAmfNcFile::getFillValue()));
		sci::replacenans(m_correctedElevations, degree(OutputAmfNcFile::getFillValue()));
		sci::replacenans(m_correctedAzimuths, degree(OutputAmfNcFile::getFillValue()));
		if (progressReporter.shouldStop())
			break;

	}
	if (progressReporter.shouldStop())
	{
		progressReporter << sU(", stopped at user request.\n");
		fin.close();
		return;

	}
	progressReporter << sU(", Reading done.\n");

	bool gatesGood = true;
	for (size_t i = m_profiles.size()-nRead; i < m_profiles.size(); ++i)
	{
		std::vector<size_t> gates = m_profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			if (gates[j] != j)
			{
				sci::ostringstream error;
				error << sU("The plotting code currently assumes gates go 0, 1, 2, 3, ... but profile ") << i << sU(" (0 indexed) in file ") << inputFilename << sU(" did not have this layout.");
				sci::assertThrow(false, sci::err(sci::SERR_USER, 0, error.str()));
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

std::vector<std::vector<metrePerSecond>> LidarBackscatterDopplerProcessor::getInstrumentRelativeDopplerVelocities() const
{
	std::vector<std::vector<metrePerSecond>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getDopplerVelocities();
	}
	return result;
}

std::vector<degree> LidarBackscatterDopplerProcessor::getInstrumentRelativeElevations() const
{
	std::vector<degree> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getElevation();
	}
	return result;
}
std::vector<degree> LidarBackscatterDopplerProcessor::getInstrumentRelativeAzimuths() const
{
	std::vector<degree> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getAzimuth();
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
	std::vector<degree> allAttitudeCorrectedAzimuths = getAttitudeCorrectedAzimuths();
	std::vector<degree> allAttitudeCorrectedElevations = getAttitudeCorrectedElevations();
	std::vector<degree> allInstrumentRelativeAzimuths = getInstrumentRelativeAzimuths();
	std::vector<degree> allInstrumentRelativeElevations = getInstrumentRelativeElevations();
	std::vector<std::vector<metrePerSecond>> allMotionCorrectedDopplerVelocities = getMotionCorrectedDopplerVelocities();
	std::vector<std::vector<metrePerSecond>> allInstrumentRelativeDopplerVelocities = getInstrumentRelativeDopplerVelocities();
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
	std::vector<std::vector<degree>> instrumentRelativeAzimuthAngles;
	instrumentRelativeAzimuthAngles.reserve(allTimes.size() / 6);
	std::vector<std::vector<degree>> attitudeCorrectedAzimuthAngles;
	attitudeCorrectedAzimuthAngles.reserve(allTimes.size() / 6);
	//a 2d array for elevation angle (time, profile within scan)
	std::vector<std::vector<degree>> instrumentRelativeElevationAngles;
	instrumentRelativeElevationAngles.reserve(allTimes.size() / 6);
	std::vector<std::vector<degree>> attitudeCorrectedElevationAngles;
	attitudeCorrectedElevationAngles.reserve(allTimes.size() / 6);
	//a 3d array for doppler speeds (time, profile within a scan, elevation) - note this is not the order we need to output
	//so immediately after this code we transpose the second two dimensions (time, elevation, profile within a scan)
	std::vector<std::vector<std::vector<metrePerSecond>>> instrumentRelativeDopplerVelocities;
	instrumentRelativeDopplerVelocities.reserve(allTimes.size() / 6);
	std::vector<std::vector<std::vector<metrePerSecond>>> motionCorrectedDopplerVelocities;
	motionCorrectedDopplerVelocities.reserve(allTimes.size() / 6);
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
		instrumentRelativeAzimuthAngles.resize(1);
		instrumentRelativeAzimuthAngles.back().reserve(6);
		instrumentRelativeAzimuthAngles.back().push_back(allInstrumentRelativeAzimuths[0]);
		attitudeCorrectedAzimuthAngles.resize(1);
		attitudeCorrectedAzimuthAngles.back().reserve(6);
		attitudeCorrectedAzimuthAngles.back().push_back(allAttitudeCorrectedAzimuths[0]);
		instrumentRelativeElevationAngles.resize(1);
		instrumentRelativeElevationAngles.back().reserve(6);
		instrumentRelativeElevationAngles.back().push_back(allInstrumentRelativeElevations[0]);
		attitudeCorrectedElevationAngles.resize(1);
		attitudeCorrectedElevationAngles.back().reserve(6);
		attitudeCorrectedElevationAngles.back().push_back(allAttitudeCorrectedElevations[0]);
		instrumentRelativeDopplerVelocities.resize(1);
		instrumentRelativeDopplerVelocities.back().reserve(6);
		instrumentRelativeDopplerVelocities.back().push_back(allInstrumentRelativeDopplerVelocities[0]);
		motionCorrectedDopplerVelocities.resize(1);
		motionCorrectedDopplerVelocities.back().reserve(6);
		motionCorrectedDopplerVelocities.back().push_back(allMotionCorrectedDopplerVelocities[0]);
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
				scanEndTimes.push_back(allTimes[i - 1] + (unitless(getHeaderForProfile(i).pulsesPerRay * getHeaderForProfile(i).nRays) / sci::Physical<sci::Hertz<1, 3>>(15.0))); //this is the time of the last profile in the scan plus the duration of this profile
				maxNGatesThisScan = getNGates(i);
				ranges.push_back(getGateCentres(i));
				instrumentRelativeAzimuthAngles.resize(instrumentRelativeAzimuthAngles.size() + 1);
				instrumentRelativeAzimuthAngles.back().reserve(maxProfilesPerScan);
				attitudeCorrectedAzimuthAngles.resize(attitudeCorrectedAzimuthAngles.size() + 1);
				attitudeCorrectedAzimuthAngles.back().reserve(maxProfilesPerScan);
				instrumentRelativeElevationAngles.resize(instrumentRelativeElevationAngles.size() + 1);
				instrumentRelativeElevationAngles.back().reserve(maxProfilesPerScan);
				attitudeCorrectedElevationAngles.resize(attitudeCorrectedElevationAngles.size() + 1);
				attitudeCorrectedElevationAngles.back().reserve(maxProfilesPerScan);
				instrumentRelativeDopplerVelocities.resize(instrumentRelativeDopplerVelocities.size() + 1);
				instrumentRelativeDopplerVelocities.back().reserve(maxProfilesPerScan);
				motionCorrectedDopplerVelocities.resize(motionCorrectedDopplerVelocities.size() + 1);
				motionCorrectedDopplerVelocities.back().reserve(maxProfilesPerScan);
				backscatters.resize(backscatters.size() + 1);
				backscatters.back().reserve(maxProfilesPerScan);
				dopplerVelocityFlags.resize(dopplerVelocityFlags.size() + 1);
				dopplerVelocityFlags.back().reserve(maxProfilesPerScan);
				backscatterFlags.resize(backscatterFlags.size() + 1);
				backscatterFlags.back().reserve(maxProfilesPerScan);
			}
			instrumentRelativeAzimuthAngles.back().push_back(allInstrumentRelativeAzimuths[i]);
			attitudeCorrectedAzimuthAngles.back().push_back(allAttitudeCorrectedAzimuths[i]);
			instrumentRelativeElevationAngles.back().push_back(allInstrumentRelativeElevations[i]);
			attitudeCorrectedElevationAngles.back().push_back(allAttitudeCorrectedElevations[i]);
			instrumentRelativeDopplerVelocities.back().push_back(allInstrumentRelativeDopplerVelocities[i]);
			motionCorrectedDopplerVelocities.back().push_back(allMotionCorrectedDopplerVelocities[i]);
			backscatters.back().push_back(allBackscatters[i]);
			dopplerVelocityFlags.back().push_back(allDopplerVelocityFlags[i]);
			backscatterFlags.back().push_back(allBackscatterFlags[i]);
		}
		//Some final items that need doing regaring the last scan/profile.
		scanEndTimes.push_back(allTimes.back() + (unitless(getHeaderForProfile(allTimes.size() - 1).pulsesPerRay * getHeaderForProfile(allTimes.size() - 1).nRays) / sci::Physical<sci::Hertz<1, 3>>(15.0))); //this is the time of the last profile in the scan plus the duration of this profile);
		maxProfilesPerScan = std::max(maxProfilesPerScan, thisProfilesPerScan);
	}

	//Now we must transpose each element of the doppler and backscatter data
	//so they are indexed by the range before the profile within the scan.
	for (size_t i = 0; i < backscatters.size(); ++i)
	{
		instrumentRelativeDopplerVelocities[i] = sci::transpose(instrumentRelativeDopplerVelocities[i]);
		motionCorrectedDopplerVelocities[i] = sci::transpose(motionCorrectedDopplerVelocities[i]);
		backscatters[i] = sci::transpose(backscatters[i]);
		dopplerVelocityFlags[i] = sci::transpose(dopplerVelocityFlags[i]);
		backscatterFlags[i] = sci::transpose(backscatterFlags[i]);
	}


	//expand the arrays, padding with fill value as needed
	for (size_t i = 0; i < ranges.size(); ++i)
	{
		ranges[i].resize(maxNGates, metre(OutputAmfNcFile::getFillValue()));
		instrumentRelativeAzimuthAngles[i].resize(maxProfilesPerScan, degree(OutputAmfNcFile::getFillValue()));
		attitudeCorrectedAzimuthAngles[i].resize(maxProfilesPerScan, degree(OutputAmfNcFile::getFillValue()));
		instrumentRelativeElevationAngles[i].resize(maxProfilesPerScan, degree(OutputAmfNcFile::getFillValue()));
		attitudeCorrectedElevationAngles[i].resize(maxProfilesPerScan, degree(OutputAmfNcFile::getFillValue()));
		instrumentRelativeDopplerVelocities[i].resize(maxNGates);
		motionCorrectedDopplerVelocities[i].resize(maxNGates);
		backscatters[i].resize(maxNGates);
		for (size_t j = 0; j < maxNGates; ++j)
		{
			instrumentRelativeDopplerVelocities[i][j].resize(maxProfilesPerScan, metrePerSecond(OutputAmfNcFile::getFillValue()));
			motionCorrectedDopplerVelocities[i][j].resize(maxProfilesPerScan, metrePerSecond(OutputAmfNcFile::getFillValue()));
			backscatters[i][j].resize(maxProfilesPerScan, perSteradianPerMetre(OutputAmfNcFile::getFillValue()));
			dopplerVelocityFlags[i][j].resize(maxProfilesPerScan, lidarUserChangedGatesFlag);
			backscatterFlags[i][j].resize(maxProfilesPerScan, lidarUserChangedGatesFlag);
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
		projectInfo, platform, scanStartTimes, nonTimeDimensions);

	AmfNcVariable<metre> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), sU("range"), sci::min<metre>(ranges), sci::max<metre>(ranges));
	AmfNcVariable<degree> azimuthVariable(sU("sensor_azimuth_angle"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning head azimuth angle"), sU("sensor_azimuth_angle"), sci::min<degree>(instrumentRelativeAzimuthAngles), sci::max<degree>(instrumentRelativeAzimuthAngles), sU("Relative to the instrument with 0 degrees being to the \"front\" of the instruments."));
	AmfNcVariable<degree> elevationVariable(sU("sensor_view_angle"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning head elevation angle"), sU("sensor_view_angle"), sci::min<degree>(instrumentRelativeElevationAngles), sci::max<degree>(instrumentRelativeElevationAngles), sU("Relative to the instruments with 0 degrees being \"horizontal\", positive being upwards, negative being downwards."));
	AmfNcVariable<degree> azimuthVariableEarthFrame(sU("sensor_azimuth_angle_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning head azimuth angle Earth Frame"), sU(""), sci::min<degree>(attitudeCorrectedAzimuthAngles), sci::max<degree>(attitudeCorrectedAzimuthAngles), sU("Relative to the geoid with 0 degrees being north."));
	AmfNcVariable<degree> elevationVariableEarthFrame(sU("sensor_view_angle_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning head elevation angle Earth Frame"), sU(""), sci::min<degree>(attitudeCorrectedElevationAngles), sci::max<degree>(attitudeCorrectedElevationAngles), sU("Relative to the geoid with 0 degrees being horizontal, positive being upwards, negative being downwards."));
	AmfNcVariable<metrePerSecond> dopplerVariable(sU("radial_velocity_of_scatterers_away_from_instrument"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Radial Velocity of Scatterers Away From Instrument"), sU("radial_velocity_of_scatterers_away_from_instrument"), sci::min<metrePerSecond>(instrumentRelativeDopplerVelocities), sci::max<metrePerSecond>(instrumentRelativeDopplerVelocities), sU("Instrument relative. Positive is away, negative is towards."));
	AmfNcVariable<metrePerSecond> dopplerVariableEarthFrame(sU("radial_velocity_of_scatterers_away_from_instrument_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Radial Velocity of Scatterers Away From Instrument Earth Frame"), sU(""), sci::min<metrePerSecond>(motionCorrectedDopplerVelocities), sci::max<metrePerSecond>(motionCorrectedDopplerVelocities), sU("Motion relative to the geoid in the direction specified by sensor_azimuth_angle_earth_frame and sensor_view_angle_earth_frame. Positive is in the direction specified, negative is in the opposite direction specified."));
	AmfNcVariable<perSteradianPerMetre> backscatterVariable(sU("attenuated_aerosol_backscatter_coefficient"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Attenuated Aerosol Backscatter Coefficient"), sU(""), sci::min<perSteradianPerMetre>(backscatters), sci::max<perSteradianPerMetre>(backscatters));
	AmfNcFlagVariable dopplerFlagVariable(sU("radial_velocity_of_scatterers_away_from_instrument_qc_flag"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension });
	AmfNcFlagVariable backscatterFlagVariable(sU("attenuated_aerosol_backscatter_coefficient_qc_flag"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension });

	file.writeTimeAndLocationData(platform);

	file.write(rangeVariable, ranges);
	file.write(azimuthVariable, instrumentRelativeAzimuthAngles);
	file.write(azimuthVariableEarthFrame, attitudeCorrectedAzimuthAngles);
	file.write(elevationVariable, instrumentRelativeElevationAngles);
	file.write(elevationVariableEarthFrame, attitudeCorrectedElevationAngles);
	file.write(dopplerVariable, instrumentRelativeDopplerVelocities);
	file.write(dopplerVariableEarthFrame, motionCorrectedDopplerVelocities);
	file.write(backscatterVariable, backscatters);
}