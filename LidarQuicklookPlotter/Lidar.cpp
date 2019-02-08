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

void LidarBackscatterDopplerProcessor::readData(const std::vector<sci::string> &inputFilenames, ProgressReporter &progressReporter, wxWindow *parent)
{
	for (size_t i = 0; i < inputFilenames.size(); ++i)
	{
		readData(inputFilenames[i], progressReporter, parent, i == 0);
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
		m_headerIndex.push_back(m_hplHeaders.size() - 1);//record which header this profile is linked to
		//Read the profile itself - it takes the header as an input to give details of what needs reading
		readingOkay = m_profiles.back().readFromStream(fin, m_hplHeaders.back());
		if (!readingOkay) //we hit the end of the file while reading this profile
			m_profiles.resize(m_profiles.size() - 1);
		++nRead;
		if (readingOkay)
		{
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

	std::vector<metre> result(m_profiles[profileIndex].nGates() + 1);
	for (size_t i = 0; i < m_profiles.size(); ++i)
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

std::vector<radian> LidarBackscatterDopplerProcessor::getAzimuths() const
{
	std::vector<radian> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
		result[i] = m_profiles[i].getAzimuth();
	return result;
}

std::vector<radian> LidarBackscatterDopplerProcessor::getElevations() const
{
	std::vector<radian> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
		result[i] = m_profiles[i].getElevation();
	return result;
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

