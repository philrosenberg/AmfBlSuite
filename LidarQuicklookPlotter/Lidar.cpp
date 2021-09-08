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

void LidarBackscatterDopplerProcessor::readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter)
{
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		//after reading the first file reserve space for the remaining files. This
		//can speed things up a lot
		if (i == 1)
		{
			size_t reserveSize = m_profiles.size()*(inputFilenames.size() + 1); // the +1 gives a bit of slack
			m_profiles.reserve(reserveSize);
			m_hplHeaders.reserve(reserveSize);
			m_headerIndex.reserve(reserveSize);
			m_betaFlags.reserve(reserveSize);
			m_dopplerFlags.reserve(reserveSize);
			m_correctedAzimuths.reserve(reserveSize);
			m_correctedElevations.reserve(reserveSize);
			m_correctedDopplerVelocities.reserve(reserveSize);
		}

		readData(inputFilenames[i], platform, progressReporter, i == 0);
		if (progressReporter.shouldStop())
			break;
	}
}
void LidarBackscatterDopplerProcessor::readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter, bool clear)
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
			//check the time is ascending, we can sometimes cross into the next day, in which case the time recorded for the profile
			//resets to close to 0. Increment the time by a day as needed;
			if (m_profiles.size() > 1)
				while (m_profiles[m_profiles.size() - 1].getTime<sci::UtcTime>() < m_profiles[m_profiles.size() - 2].getTime<sci::UtcTime>())
					m_profiles[m_profiles.size() - 1].setTime(m_profiles[m_profiles.size() - 1].getTime<sci::UtcTime>() + second(24.0*60.0*60.0));
			//correct azimuths and elevations for platform orientation
			m_correctedAzimuths.resize(m_profiles.size(), degreeF(0.0));
			m_correctedElevations.resize(m_profiles.size(), degreeF(0.0));
			second profileDuration = (unitlessF((unitlessF::valueType)m_hplHeaders.back().pulsesPerRay) / sci::Physical<sci::Hertz<1, 3>, typename unitlessF::valueType>(15.0));
			platform.correctDirection(m_profiles.back().getTime<sci::UtcTime>(), m_profiles.back().getTime<sci::UtcTime>() + profileDuration, m_profiles.back().getAzimuth(), m_profiles.back().getElevation(), m_correctedAzimuths.back(), m_correctedElevations.back());
			metrePerSecondF u;
			metrePerSecondF v;
			metrePerSecondF w;
			platform.getInstrumentVelocity(m_profiles.back().getTime<sci::UtcTime>(), m_profiles.back().getTime<sci::UtcTime>() + profileDuration, u, v, w);
			metrePerSecondF offset = u * sci::sin(m_correctedAzimuths.back())*sci::cos(m_correctedElevations.back())
				+ v * sci::cos(m_correctedAzimuths.back())*sci::cos(m_correctedElevations.back())
				+ w * sci::sin(m_correctedElevations.back());
			m_correctedDopplerVelocities.push_back(m_profiles.back().getDopplerVelocities() + offset);

			m_headerIndex.push_back(m_hplHeaders.size() - 1);//record which header this profile is linked to
			std::vector<uint8_t> dopplerVelocityFlags(m_profiles.back().nGates(), lidarGoodDataFlag);
			std::vector<uint8_t> betaFlags(m_profiles.back().nGates(), lidarGoodDataFlag);
			//flag for out of range doppler
			sci::assign(dopplerVelocityFlags, sci::isEq(dopplerVelocityFlags,lidarGoodDataFlag) && (m_profiles.back().getDopplerVelocities() > metrePerSecondF(19.0) || m_profiles.back().getDopplerVelocities() < metrePerSecondF(-19.0)), lidarDopplerOutOfRangeFlag);
			//flag for bad snr - not intensity reported by instrument is snr+1
			sci::assign(dopplerVelocityFlags, sci::isEq(dopplerVelocityFlags, lidarGoodDataFlag) && m_profiles.back().getIntensities() < unitlessF(2.0), lidarSnrBelow1Flag);
			sci::assign(dopplerVelocityFlags, sci::isEq(dopplerVelocityFlags, lidarGoodDataFlag) && m_profiles.back().getIntensities() < unitlessF(3.0), lidarSnrBelow2Flag);
			sci::assign(dopplerVelocityFlags, sci::isEq(dopplerVelocityFlags, lidarGoodDataFlag) && m_profiles.back().getIntensities() < unitlessF(4.0), lidarSnrBelow3Flag);
			sci::assign(betaFlags, sci::isEq(betaFlags, lidarGoodDataFlag) && m_profiles.back().getIntensities() < unitlessF(2.0), lidarSnrBelow1Flag);
			sci::assign(betaFlags, sci::isEq(betaFlags, lidarGoodDataFlag) && m_profiles.back().getIntensities() < unitlessF(3.0), lidarSnrBelow2Flag);
			sci::assign(betaFlags, sci::isEq(betaFlags, lidarGoodDataFlag) && m_profiles.back().getIntensities() < unitlessF(4.0), lidarSnrBelow3Flag);
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
	progressReporter << sU(", Reading done.\n");

	bool gatesGood = true;
	for (size_t i = m_profiles.size() - nRead; i < m_profiles.size(); ++i)
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

std::vector<std::vector<perSteradianPerMetreF>> LidarBackscatterDopplerProcessor::getBetas() const
{
	std::vector<std::vector<perSteradianPerMetreF>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getBetas();
	}
	return result;
}

std::vector<std::vector<unitlessF>> LidarBackscatterDopplerProcessor::getSignalToNoiseRatios() const
{
	std::vector<std::vector<unitlessF>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getIntensities() - unitlessF(1.0);
	}
	return result;
}

std::vector<std::vector<unitlessF>> LidarBackscatterDopplerProcessor::getSignalToNoiseRatiosPlusOne() const
{
	std::vector<std::vector<unitlessF>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getIntensities();
	}
	return result;
}

std::vector<std::vector<metrePerSecondF>> LidarBackscatterDopplerProcessor::getInstrumentRelativeDopplerVelocities() const
{
	std::vector<std::vector<metrePerSecondF>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getDopplerVelocities();
	}
	return result;
}

std::vector<degreeF> LidarBackscatterDopplerProcessor::getInstrumentRelativeElevations() const
{
	std::vector<degreeF> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getElevation();
	}
	return result;
}
std::vector<degreeF> LidarBackscatterDopplerProcessor::getInstrumentRelativeAzimuths() const
{
	std::vector<degreeF> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getAzimuth();
	}
	return result;
}

std::vector<metreF> LidarBackscatterDopplerProcessor::getGateBoundariesForPlotting(size_t profileIndex) const
{
	//because there may be gate overlapping, for plotting the lower and upper gate boundaries
	//aren't quite what we want.
	metreF interval = m_hplHeaders[profileIndex].rangeGateLength;
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (m_profiles[profileIndex].nGates() > 533)
		interval /= unitlessF((unitlessF::valueType)m_hplHeaders[m_headerIndex[profileIndex]].pointsPerGate);

	std::vector<metreF> result = getGateCentres(profileIndex) - unitlessF(0.5)*interval;
	result.push_back(result.back() + interval);

	return result;
}

std::vector<metreF> LidarBackscatterDopplerProcessor::getGateLowerBoundaries(size_t profileIndex) const
{
	if (m_profiles.size() == 0)
		return std::vector<metreF>(0);

	std::vector<metreF> result(m_profiles[profileIndex].nGates());
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = unitlessF((unitlessF::valueType)i) * m_hplHeaders[m_headerIndex[profileIndex]].rangeGateLength;
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (m_profiles[profileIndex].nGates() > 533)
		result /= unitlessF((unitlessF::valueType)m_hplHeaders[m_headerIndex[profileIndex]].pointsPerGate);
	return result;
}

std::vector<metreF> LidarBackscatterDopplerProcessor::getGateUpperBoundaries(size_t profileIndex) const
{
	return getGateLowerBoundaries(profileIndex) + m_hplHeaders[m_headerIndex[profileIndex]].rangeGateLength;
}

std::vector<metreF> LidarBackscatterDopplerProcessor::getGateCentres(size_t profileIndex) const
{
	return getGateLowerBoundaries(profileIndex) + unitlessF(0.5) * m_hplHeaders[m_headerIndex[profileIndex]].rangeGateLength;
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

	splotlegend *legend = (*window)->addlegend(0.86, 0.25, 0.13, 0.65, wxColour(0, 0, 0), 0);
	legend->addentry(sU(""), g_lidarColourscale, false, false, 0.05, 0.3, 15, sU(""), 0, 0.05, wxColour(0, 0, 0), 128, false, 150, false);
}

void LidarScanningProcessor::formatDataForOutput(ProgressReporter& progressReporter,
	std::vector<std::vector<metreF>> &ranges,
	std::vector<std::vector<degreeF>> &instrumentRelativeAzimuthAngles,
	std::vector<std::vector<degreeF>> &attitudeCorrectedAzimuthAngles,
	std::vector<std::vector<degreeF>> &instrumentRelativeElevationAngles,
	std::vector<std::vector<degreeF>> &attitudeCorrectedElevationAngles,
	std::vector<std::vector<std::vector<metrePerSecondF>>> &instrumentRelativeDopplerVelocities,
	std::vector<std::vector<std::vector<metrePerSecondF>>> &motionCorrectedDopplerVelocities,
	std::vector<std::vector<std::vector<perSteradianPerMetreF>>> &backscatters,
	std::vector<std::vector<std::vector<unitlessF>>> &snrsPlusOne,
	std::vector < std::vector<std::vector<uint8_t>>> &dopplerVelocityFlags,
	std::vector < std::vector<std::vector<uint8_t>>> &backscatterFlags,
	std::vector<sci::UtcTime> &scanStartTimes,
	std::vector<sci::UtcTime> &scanEndTimes,
	size_t &maxProfilesPerScan,
	size_t &maxNGates,
	size_t &pulsesPerRay,
	size_t &raysPerPoint,
	metreF &focus,
	metrePerSecondF &dopplerResolution,
	metreF &gateLength,
	size_t &pointsPerGate)
{
	//build up our data arrays. We must account for the fact that the user could change
	//the number of profiles in a scan pattern or the range of the instruemnt during a day
	//so we work through the profiles checking when the filename changes. At this point things could
	//change. The number of gates in each profile can change within a file, but the distance per
	//gate cannot, so we find the longest range per hile and use this
	//work out how many lidar profiles there are per VAD and find the start time of each VAD and the ranges array for each vad
	maxProfilesPerScan = 1;
	size_t thisProfilesPerScan = 1;
	std::vector<sci::UtcTime> allTimes = getTimesUtcTime();
	std::vector<degreeF> allAttitudeCorrectedAzimuths = getAttitudeCorrectedAzimuths();
	std::vector<degreeF> allAttitudeCorrectedElevations = getAttitudeCorrectedElevations();
	std::vector<degreeF> allInstrumentRelativeAzimuths = getInstrumentRelativeAzimuths();
	std::vector<degreeF> allInstrumentRelativeElevations = getInstrumentRelativeElevations();
	std::vector<std::vector<metrePerSecondF>> allMotionCorrectedDopplerVelocities = getMotionCorrectedDopplerVelocities();
	std::vector<std::vector<metrePerSecondF>> allInstrumentRelativeDopplerVelocities = getInstrumentRelativeDopplerVelocities();
	std::vector<std::vector<perSteradianPerMetreF>> allBackscatters = getBetas();
	std::vector<std::vector<unitlessF>> allSnrsPlusOne = getSignalToNoiseRatiosPlusOne();
	std::vector<std::vector<uint8_t>> allDopplerVelocityFlags = getDopplerFlags();
	std::vector<std::vector<uint8_t>> allBackscatterFlags = getBetaFlags();
	scanStartTimes.reserve(allTimes.size() / 6);
	scanEndTimes.reserve(allTimes.size() / 6);
	//a 2d array for ranges (time,range)
	ranges.reserve(allTimes.size() / 6);
	//a 2d array for azimuth (time, profile within scan)
	instrumentRelativeAzimuthAngles.reserve(allTimes.size() / 6);
	attitudeCorrectedAzimuthAngles.reserve(allTimes.size() / 6);
	//a 2d array for elevation angle (time, profile within scan)
	instrumentRelativeElevationAngles.reserve(allTimes.size() / 6);
	attitudeCorrectedElevationAngles.reserve(allTimes.size() / 6);
	//a 3d array for doppler speeds (time, profile within a scan, elevation) - note this is not the order we need to output
	//so immediately after this code we transpose the second two dimensions (time, elevation, profile within a scan)
	instrumentRelativeDopplerVelocities.reserve(allTimes.size() / 6);
	motionCorrectedDopplerVelocities.reserve(allTimes.size() / 6);
	//a 3d array for backscatters and snr+1 (time, profile within a scan, elevation) - note this is not the order we need to output
	//so immediately after this code we transpose the second two dimensions (time, elevation, profile within a scan)
	backscatters.reserve(allTimes.size() / 6);
	snrsPlusOne.reserve(allTimes.size() / 6);
	//As above, but for the flags
	dopplerVelocityFlags.resize(backscatters.size());
	backscatterFlags.resize(backscatters.size());
	dopplerVelocityFlags.reserve(allTimes.size() / 6);
	backscatterFlags.reserve(allTimes.size() / 6);

	HplHeader previousHeader;
	size_t maxNGatesThisScan;
	maxNGates = 0;

	//To Do: these parameters may change if the settings are adjusted during a day
	//Need to deal with that properly
	pulsesPerRay = 0;
	raysPerPoint = 0;
	focus = metreF(0.0f);
	dopplerResolution = metrePerSecondF(0.0f);
	gateLength = metreF(0.0f);
	pointsPerGate = 0;

	if (allTimes.size() > 0)
	{
		//keep track of the previous header as we go throught the data. We compare current to previous
		//header to check if we have moved to the next scan.
		//Set all the variables using the first profile.
		previousHeader = getHeaderForProfile(0);
		scanStartTimes.push_back(allTimes[0]);
		maxNGatesThisScan = getNGates(0);
		maxNGates = maxNGatesThisScan;
		pulsesPerRay = getPulsesPerRay(0);
		raysPerPoint = getNRays(0);
		focus = getFocus(0);
		dopplerResolution = getDopplerResolution(0);
		gateLength = getGateLength(0);
		pointsPerGate = getNPointsPerGate(0);
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
		snrsPlusOne.resize(1);
		snrsPlusOne.back().reserve(6);
		snrsPlusOne.back().push_back(allSnrsPlusOne[0]);
		dopplerVelocityFlags.resize(1);
		dopplerVelocityFlags.back().reserve(6);
		dopplerVelocityFlags.back().push_back(allDopplerVelocityFlags[0]);
		backscatterFlags.resize(1);
		backscatterFlags.back().reserve(6);
		backscatterFlags.back().push_back(allBackscatterFlags[0]);


		//Work through all the profiles finding the needed data
		for (size_t i = 1; i < allTimes.size(); ++i)
		{
			sci::assertThrow(
				pulsesPerRay == getPulsesPerRay(i) && raysPerPoint == getNRays(i) && focus == getFocus(i) && dopplerResolution == getDopplerResolution(i) && gateLength == getGateLength(i),
				sci::err(sci::SERR_USER, 0, sU("Pulses per ray, rays per point, focus, doppler resolution or gate length have been changed during the day - cannot process.")));
			if (getHeaderForProfile(i).filename == previousHeader.filename && getHeaderForProfile(i).filename.substr(0, 5) != sU("Stare"))
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
				scanEndTimes.push_back(allTimes[i - 1] + (unitlessF((unitlessF::valueType)(getHeaderForProfile(i).pulsesPerRay * getHeaderForProfile(i).nRays)) / sci::Physical<sci::Hertz<1, 3>, typename unitlessF::valueType>(15.0))); //this is the time of the last profile in the scan plus the duration of this profile
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
				snrsPlusOne.resize(snrsPlusOne.size() + 1);
				snrsPlusOne.back().reserve(maxProfilesPerScan);
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
			snrsPlusOne.back().push_back(allSnrsPlusOne[i]);
			dopplerVelocityFlags.back().push_back(allDopplerVelocityFlags[i]);
			backscatterFlags.back().push_back(allBackscatterFlags[i]);
		}
		//Some final items that need doing regaring the last scan/profile.
		scanEndTimes.push_back(allTimes.back() + (unitlessF((unitlessF::valueType)(getHeaderForProfile(allTimes.size() - 1).pulsesPerRay * getHeaderForProfile(allTimes.size() - 1).nRays)) / sci::Physical<sci::Hertz<1, 3>, typename unitlessF::valueType>(15.0))); //this is the time of the last profile in the scan plus the duration of this profile);
		maxProfilesPerScan = std::max(maxProfilesPerScan, thisProfilesPerScan);
	}

	//Now we must transpose each element of the doppler and backscatter data
	//so they are indexed by the range before the profile within the scan.
	for (size_t i = 0; i < backscatters.size(); ++i)
	{
		instrumentRelativeDopplerVelocities[i] = sci::transpose(instrumentRelativeDopplerVelocities[i]);
		motionCorrectedDopplerVelocities[i] = sci::transpose(motionCorrectedDopplerVelocities[i]);
		backscatters[i] = sci::transpose(backscatters[i]);
		snrsPlusOne[i] = sci::transpose(snrsPlusOne[i]);
		dopplerVelocityFlags[i] = sci::transpose(dopplerVelocityFlags[i]);
		backscatterFlags[i] = sci::transpose(backscatterFlags[i]);
	}


	//expand the arrays, padding with fill value as needed
	for (size_t i = 0; i < ranges.size(); ++i)
	{
		ranges[i].resize(maxNGates, std::numeric_limits<metreF>::quiet_NaN());
		instrumentRelativeAzimuthAngles[i].resize(maxProfilesPerScan, std::numeric_limits<degreeF>::quiet_NaN());
		attitudeCorrectedAzimuthAngles[i].resize(maxProfilesPerScan, std::numeric_limits<degreeF>::quiet_NaN());
		instrumentRelativeElevationAngles[i].resize(maxProfilesPerScan, std::numeric_limits<degreeF>::quiet_NaN());
		attitudeCorrectedElevationAngles[i].resize(maxProfilesPerScan, std::numeric_limits<degreeF>::quiet_NaN());
		instrumentRelativeDopplerVelocities[i].resize(maxNGates);
		motionCorrectedDopplerVelocities[i].resize(maxNGates);
		backscatters[i].resize(maxNGates);
		snrsPlusOne[i].resize(maxNGates);
		for (size_t j = 0; j < maxNGates; ++j)
		{
			instrumentRelativeDopplerVelocities[i][j].resize(maxProfilesPerScan, std::numeric_limits<metrePerSecondF>::quiet_NaN());
			motionCorrectedDopplerVelocities[i][j].resize(maxProfilesPerScan, std::numeric_limits<metrePerSecondF>::quiet_NaN());
			backscatters[i][j].resize(maxProfilesPerScan, std::numeric_limits<perSteradianPerMetreF>::quiet_NaN());
			snrsPlusOne[i][j].resize(maxProfilesPerScan, std::numeric_limits<unitlessF>::quiet_NaN());
			dopplerVelocityFlags[i][j].resize(maxProfilesPerScan, lidarUserChangedGatesFlag);
			backscatterFlags[i][j].resize(maxProfilesPerScan, lidarUserChangedGatesFlag);
		}
	}
}

void LidarScanningProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	DataInfo dataInfo;
	dataInfo.continuous = true;
	dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();//set to fill value initially - calculate it later
	dataInfo.startTime = getTimesUtcTime()[0];
	dataInfo.endTime = getTimesUtcTime().back();
	dataInfo.featureType = FeatureType::timeSeriesProfile;
	dataInfo.options = getProcessingOptions();
	dataInfo.processingLevel = 1;
	dataInfo.productName = sU("aerosol backscatter radial winds");
	dataInfo.processingOptions = processingOptions;

	std::vector<std::vector<metreF>> ranges;
	std::vector<std::vector<degreeF>> instrumentRelativeAzimuthAngles;
	std::vector<std::vector<degreeF>> attitudeCorrectedAzimuthAngles;
	std::vector<std::vector<degreeF>> instrumentRelativeElevationAngles;
	std::vector<std::vector<degreeF>> attitudeCorrectedElevationAngles;
	std::vector<std::vector<std::vector<metrePerSecondF>>> instrumentRelativeDopplerVelocities;
	std::vector<std::vector<std::vector<metrePerSecondF>>> motionCorrectedDopplerVelocities;
	std::vector<std::vector<std::vector<perSteradianPerMetreF>>> backscatters;
	std::vector<std::vector<std::vector<unitlessF>>> snrsPlusOne;
	std::vector < std::vector<std::vector<uint8_t>>> dopplerVelocityFlags(backscatters.size());
	std::vector < std::vector<std::vector<uint8_t>>> backscatterFlags(backscatters.size());
	std::vector<sci::UtcTime> scanStartTimes;
	std::vector<sci::UtcTime> scanEndTimes;

	size_t maxProfilesPerScan;
	size_t maxNGates;
	size_t pulsesPerRay;
	size_t raysPerPoint;
	metreF focus;
	metrePerSecondF dopplerResolution;
	metreF gateLength;
	size_t pointsPerGate;

	LidarScanningProcessor::formatDataForOutput(progressReporter,
		ranges,
		instrumentRelativeAzimuthAngles,
		attitudeCorrectedAzimuthAngles,
		instrumentRelativeElevationAngles,
		attitudeCorrectedElevationAngles,
		instrumentRelativeDopplerVelocities,
		motionCorrectedDopplerVelocities,
		backscatters,
		snrsPlusOne,
		dopplerVelocityFlags,
		backscatterFlags,
		scanStartTimes,
		scanEndTimes,
		maxProfilesPerScan,
		maxNGates,
		pulsesPerRay,
		raysPerPoint,
		focus,
		dopplerResolution,
		gateLength,
		pointsPerGate);



	//work out the averaging time - this is the difference between the scan start and end times.
	//use the median as the value for the file
	if (scanStartTimes.size() > 0)
	{
		std::vector<sci::TimeInterval> averagingTimes = scanEndTimes - scanStartTimes;
		std::vector < sci::TimeInterval> sortedAveragingTimes = averagingTimes;
		std::sort(sortedAveragingTimes.begin(), sortedAveragingTimes.end());
		dataInfo.averagingPeriod = secondF(sortedAveragingTimes[sortedAveragingTimes.size() / 2]);
	}

	//work out the sampling interval - this is the difference between each scan time.
	//use the median as the value for the file
	if (scanStartTimes.size() > 1)
	{
		std::vector<sci::TimeInterval> samplingIntervals = std::vector<sci::UtcTime>(scanStartTimes.begin() + 1, scanStartTimes.end()) - std::vector<sci::UtcTime>(scanStartTimes.begin(), scanStartTimes.end() - 1);
		std::vector < sci::TimeInterval> sortedSamplingIntervals = samplingIntervals;
		std::sort(sortedSamplingIntervals.begin(), sortedSamplingIntervals.end());
		dataInfo.samplingInterval = secondF(sortedSamplingIntervals[sortedSamplingIntervals.size() / 2]);
	}

	sci::NcDimension rangeIndexDimension(sU("index_of_range"), maxNGates);
	sci::NcDimension angleIndexDimension(sU("index_of_angle"), maxProfilesPerScan);
	std::vector<sci::NcDimension*> nonTimeDimensions{ &rangeIndexDimension, &angleIndexDimension };

	//add standard to the processing options - this is as opposed to advanced which is a reprocessing of the raw data
	dataInfo.options.push_back(sU("standard"));

	//add comments
	if (dataInfo.processingOptions.comment.length() > 0)
		dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\n");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("Range gates may overlap - hence range may not grow in increments of gate_length.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_azimuth_angle is relative to the instrument with 0 degrees being to the \"front\" of the instruments.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_view_angle is relative to the instruments with 0 degrees being \"horizontal\", positive being upwards, negative being downwards.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_azimuth_angle_earth_frame is relative to the geoid with 0 degrees being north.");
	dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nsensor_view_angle_earth_frame is relative to the geoid with 0 degrees being horizontal, positive being upwards, negative being downwards.");
	//dataInfo.processingOptions.comment = dataInfo.processingOptions.comment + sU("\nradial_velocity_of_scatterers_away_from_instrument_earth_frame is motion relative to the geoid in the direction specified by sensor_azimuth_angle_earth_frame and sensor_view_angle_earth_frame. Positive is in the direction specified, negative is in the opposite direction specified.");
	
	//set the global attributes
	sci::stringstream pulsesPerRayStr;
	sci::stringstream raysPerPointStr;
	sci::stringstream velocityResolutionStr;
	sci::stringstream nGatesStr;
	sci::stringstream gateLengthStr;
	sci::stringstream maxRangeStr;
	pulsesPerRayStr << pulsesPerRay;
	raysPerPointStr << raysPerPoint;
	velocityResolutionStr << dopplerResolution;
	nGatesStr << maxNGates;
	gateLengthStr << gateLength;
	maxRangeStr << unitlessF(float(maxNGates)) * gateLength / unitlessF(float(pointsPerGate));
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
	&velocityResolutionAttribute, &nGatesAttribute, &gateLengthAttribute, &fftAttribute, &maxRangeAttribute};

	OutputAmfNcFile file(AmfVersion::v1_1_0, directory, getInstrumentInfo(), author, processingSoftwareInfo, getCalibrationInfo(), dataInfo,
		projectInfo, platform, sU("doppler lidar scan"), scanStartTimes, nonTimeDimensions, lidarGlobalAttributes);

	//this is what I think it should be, but CEDA want just time: mean, but left this here in case someone changes their mind
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), cm_mean}, { sU("range"), cm_mean }, {sU("sensor_azimuth_angle"), cm_point}, {sU("sensor_view_angle"), cm_point} };
	//std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ {sU("time"), cm_point} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsData{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAngles{ };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsAnglesEarthFrame{ {sU("time"), CellMethod::mean} };
	std::vector<std::pair<sci::string, CellMethod>>cellMethodsRange{ };
	std::vector<sci::string> coordinates{ sU("latitude"), sU("longitude") };


	//create the variables - note we set swap to true for these variables which swaps the data into the varaibles rather than copying it. This saved memory
	AmfNcVariable<metreF, decltype(ranges), true> rangeVariable(sU("range"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension }, sU("Distance of Measurement Volume Centre Point from Instrument"), sU("range"), ranges, true, coordinates, cellMethodsRange, 1);
	AmfNcVariable<degreeF, decltype(instrumentRelativeAzimuthAngles), true> azimuthVariable(sU("sensor_azimuth_angle_instrument_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning Head Azimuth Angle"), sU("sensor_azimuth_angle"), instrumentRelativeAzimuthAngles, true, coordinates, cellMethodsAngles, 1);
	AmfNcVariable<degreeF, decltype(instrumentRelativeElevationAngles), true> elevationVariable(sU("sensor_view_angle_instrument_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning Head Elevation Angle"), sU("sensor_view_angle"), instrumentRelativeElevationAngles, true, coordinates, cellMethodsAngles, 1);
	AmfNcVariable<degreeF, decltype(attitudeCorrectedAzimuthAngles), true> azimuthVariableEarthFrame(sU("sensor_azimuth_angle_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning Head Azimuth Angle Earth Frame"), sU(""), attitudeCorrectedAzimuthAngles, true, coordinates, cellMethodsAnglesEarthFrame, 1);
	AmfNcVariable<degreeF, decltype(attitudeCorrectedElevationAngles), true> elevationVariableEarthFrame(sU("sensor_view_angle_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &angleIndexDimension }, sU("Scanning Head Elevation Angle Earth Frame"), sU(""), attitudeCorrectedElevationAngles, true, coordinates, cellMethodsAnglesEarthFrame, 1);
	AmfNcVariable<metrePerSecondF, decltype(instrumentRelativeDopplerVelocities), true> dopplerVariable(sU("radial_velocity_of_scatterers_away_from_instrument"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Radial Velocity of Scatterers Away From Instrument"), sU("radial_velocity_of_scatterers_away_from_instrument"), instrumentRelativeDopplerVelocities, true, coordinates, cellMethodsData, dopplerVelocityFlags);
	//AmfNcVariable<metrePerSecondF, decltype(motionCorrectedDopplerVelocities), true> dopplerVariableEarthFrame(sU("radial_velocity_of_scatterers_away_from_instrument_earth_frame"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Radial Velocity of Scatterers Away From Instrument Earth Frame"), sU(""), motionCorrectedDopplerVelocities, true, coordinates, cellMethodsData);
	AmfNcVariable<perSteradianPerMetreF, decltype(backscatters), true> backscatterVariable(sU("attenuated_aerosol_backscatter_coefficient"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Attenuated Aerosol Backscatter Coefficient"), sU(""), backscatters, true, coordinates, cellMethodsData, backscatterFlags);
	AmfNcVariable<unitlessF, decltype(snrsPlusOne), true> snrsPlusOneVariable(sU("signal_to_noise_ratio_plus_1"), file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension }, sU("Signal to Noise Ratio: SNR+1"), sU(""), snrsPlusOne, true, coordinates, cellMethodsData, backscatterFlags);
	AmfNcFlagVariable dopplerFlagVariable(sU("qc_flag_radial_velocity_of_scatterers_away_from_instrument"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension });
	AmfNcFlagVariable backscatterFlagVariable(sU("qc_flag_backscatter"), lidarDopplerFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension(), &rangeIndexDimension, &angleIndexDimension });

	file.writeTimeAndLocationData(platform);

	file.write(rangeVariable);
	file.write(azimuthVariable);
	file.write(azimuthVariableEarthFrame);
	file.write(elevationVariable);
	file.write(elevationVariableEarthFrame);
	file.write(dopplerVariable);
	//file.write(dopplerVariableEarthFrame);
	file.write(backscatterVariable);
	file.write(snrsPlusOneVariable);
	file.write(dopplerFlagVariable, dopplerVelocityFlags);
	file.write(backscatterFlagVariable, backscatterFlags);
}

std::vector<std::vector<sci::string>> HplFileLidar::groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	if (newFiles.size() == 0)
		return std::vector<std::vector<sci::string>>(0);

	sci::string croppedFilename = newFiles[0];
	if (croppedFilename.find_last_of(sU("/\\")) != sci::string::npos)
		croppedFilename = croppedFilename.substr(croppedFilename.find_last_of(sU("/\\")) + 1);

	//find how many underscores there are. This will tell us if this is an old or new style file.
	size_t nUnderscores = 0;
	size_t firstUnderscorePosition = 0;
	size_t secondUnderscorePosition = 0;
	for (size_t i = m_filePrefix.length()+1; i < croppedFilename.length(); ++i)
	{
		if (croppedFilename[i] == sU('_'))
		{
			++nUnderscores;
			if (nUnderscores == 1)
				firstUnderscorePosition = i;
			else if (nUnderscores == 2)
				secondUnderscorePosition = i;
		}
	}

	//we shouldn't need to do checks on the filenames as they should have already passed through other checking functions

	//extract the date and time characters from the filename and put them in a string with spaces between
	//in the format YYYY MM DD hh mm ss
	sci::stringstream dateStream;
	if (nUnderscores == 1)
	{
		//this is an old style filename
		return InstrumentProcessor::groupInputFilesbyOutputFiles(newFiles, allFiles, m_filePrefix.length()+1, 6);
	}
	else
	{
		//this is an new style filename
		//this is an old style filename
		return InstrumentProcessor::groupInputFilesbyOutputFiles(newFiles, allFiles, firstUnderscorePosition + 1, 8);
		
	}
}

sci::UtcTime HplFileLidar::extractDateFromLidarFilename(const sci::string &filename) const
{
	//trim off the directory and the prefix
	sci::string croppedFilename;
	if (filename.find_last_of(sU("/\\")) != sci::string::npos)
		croppedFilename = filename.substr(filename.find_last_of(sU("/\\")) + 2 + m_filePrefix.length());
	else
		croppedFilename = filename.substr(m_filePrefix.length() + 1);

	//find how many underscores there are. This will tell us if this is an old or new style file.
	size_t nUnderscores = 0;
	size_t firstUnderscorePosition = 0;
	size_t secondUnderscorePosition = 0;
	for (size_t i = 0; i < croppedFilename.length(); ++i)
	{
		if (croppedFilename[i] == sU('_'))
		{
			++nUnderscores;
			if (nUnderscores == 1)
				firstUnderscorePosition = i;
			else if (nUnderscores == 2)
				secondUnderscorePosition = i;
		}
	}

	//check we have one or two underscores in the filename
	sci::stringstream messageStream;
	messageStream << sU("The file ") << filename << sU(" has been identified as a lidar file, but the file name format does not match the expected pattern.");
	sci::assertThrow(nUnderscores == 0 || nUnderscores == 1 || nUnderscores == 2, sci::err(sci::SERR_USER, 0, messageStream.str()));

	//extract the date and time characters from the filename and put them in a string with spaces between
	//in the format YYYY MM DD hh mm ss
	sci::stringstream dateStream;
	if (nUnderscores == 0)
	{
		//some old style filenames seem to be just Stare_ddmmyy.hpl, this should be one of them
		//this is an old style filename
		sci::assertThrow(croppedFilename.length() == 10, sci::err(sci::SERR_USER, 0, messageStream.str()));
		sci::assertThrow(croppedFilename.substr(6) == sU(".hpl"), sci::err(sci::SERR_USER, 0, messageStream.str()));

		dateStream << sU("20") << croppedFilename.substr(4, 2);
		dateStream << sU(" ") << croppedFilename.substr(2, 2);
		dateStream << sU(" ") << croppedFilename.substr(0, 2);
		dateStream << sU(" 00 00 00");
	}
	else if (nUnderscores == 1)
	{
		//this is an old style filename
		sci::assertThrow(firstUnderscorePosition == 6, sci::err(sci::SERR_USER, 0, messageStream.str()));

		dateStream << sU("20") << croppedFilename.substr(4, 2);
		dateStream << sU(" ") << croppedFilename.substr(2, 2);
		dateStream << sU(" ") << croppedFilename.substr(0, 2);
		dateStream << sU(" ") << croppedFilename.substr(7, 2);
		if (m_filePrefix==sU("Stare"))
		{
			dateStream << sU(" 00 00");
		}
		else
		{
			dateStream << sU(" ") << croppedFilename.substr(9, 2);
			dateStream << sU(" ") << croppedFilename.substr(11, 2);
		}
	}
	else
	{
		//this is a new style filename
		sci::assertThrow((secondUnderscorePosition - firstUnderscorePosition) == 9, sci::err(sci::SERR_USER, 0, messageStream.str()));

		dateStream << croppedFilename.substr(firstUnderscorePosition + 1, 4);
		dateStream << sU(" ") << croppedFilename.substr(firstUnderscorePosition + 5, 2);
		dateStream << sU(" ") << croppedFilename.substr(firstUnderscorePosition + 7, 2);
		dateStream << sU(" ") << croppedFilename.substr(secondUnderscorePosition + 1, 2);
		if (m_filePrefix == sU("Stare"))
		{
			dateStream << sU(" 00 00");
		}
		else
		{
			dateStream << sU(" ") << croppedFilename.substr(secondUnderscorePosition + 3, 2);
			dateStream << sU(" ") << croppedFilename.substr(secondUnderscorePosition + 5, 2);
		}
	}

	//extract the values from the string
	int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	double second;
	dateStream >> year >> month >> day >> hour >> minute >> second;

	//return the date
	return sci::UtcTime(year, month, day, hour, minute, second);
}