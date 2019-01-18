#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"HplHeader.h"
#include"HplProfile.h"
#include"ProgressReporter.h"
#include<svector/sstring.h>

void LidarBackscatterDopplerProcessor::readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent)
{
	m_hasData = false;
	m_profiles.clear();

	std::fstream fin;
	fin.open(sci::nativeUnicode(inputFilename), std::ios::in);
	if (!fin.is_open())
		throw(sU("Could not open lidar file ") + inputFilename + sU("."));

	progressReporter << sU("Reading file.\n");
	fin >> m_hplHeader;

	bool readingOkay = true;
	while (readingOkay)
	{
		m_profiles.resize(m_profiles.size() + 1);
		readingOkay = m_profiles.back().readFromStream(fin, m_hplHeader);
		if (!readingOkay) //we hit the end of the file while reading this profile
			m_profiles.resize(m_profiles.size() - 1);
		if (readingOkay)
		{
			if (m_profiles.size() == 1)
				progressReporter << sU("Read profile 1");
			else if (m_profiles.size() <= 50)
				progressReporter << sU(", ") << m_profiles.size();
			else if (m_profiles.size() % 10 == 0)
				progressReporter << sU(", ") << m_profiles.size() - 9 << "-" << m_profiles.size();
		}
		if (progressReporter.shouldStop())
			break;
	}
	if (progressReporter.shouldStop())
	{
		progressReporter << ", stopped at user request.\n";
		fin.close();
		return;

	}
	progressReporter << ", reading done.\n";

	bool gatesGood = true;
	for (size_t i = 0; i < m_profiles.size(); ++i)
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


std::vector<second> LidarBackscatterDopplerProcessor::getTimes() const
{
	std::vector<second> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getTime<second>();
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

std::vector<metre> LidarBackscatterDopplerProcessor::getGateBoundariesForPlotting() const
{
	//because thee may be gate overlapping, for plotting the lower and upper gate boundaries
	//aren't quite what we want.
	metre interval = m_hplHeader.rangeGateLength;
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (m_profiles[0].nGates() > 533)
		interval /= unitless(m_hplHeader.pointsPerGate);

	std::vector<metre> result = getGateCentres() - unitless(0.5)*interval;
	result.push_back(result.back() + interval);

	return result;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateLowerBoundaries() const
{
	if (m_profiles.size() == 0)
		return std::vector<metre>(0);

	std::vector<metre> result(m_profiles[0].nGates() + 1);
	for (size_t i = 0; i < m_profiles.size(); ++i)
		result[i] = unitless(i) * m_hplHeader.rangeGateLength;
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (m_profiles[0].nGates() > 533)
		result /= unitless(m_hplHeader.pointsPerGate);
	return result;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateUpperBoundaries() const
{
	return getGateLowerBoundaries() + m_hplHeader.rangeGateLength;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateCentres() const
{
	return getGateLowerBoundaries() + unitless(0.5) * m_hplHeader.rangeGateLength;
}
