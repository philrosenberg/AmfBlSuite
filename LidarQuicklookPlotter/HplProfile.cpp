#define _USE_MATH_DEFINES
#include"HplProfile.h"
#include"HplHeader.h"
#include<cmath>

bool HplProfile::readFromStream(std::istream &stream, const HplHeader &header)
{
	double decimalHour;
	double azimuthDeg;
	double elevationDeg;
	double pitchDeg;
	double rollDeg;
	stream >> decimalHour >> azimuthDeg >> elevationDeg >> pitchDeg >> rollDeg;
	m_azimuth = degree(azimuthDeg);
	m_elevation = degree(elevationDeg);
	m_pitch = degree(pitchDeg);
	m_roll = degree(rollDeg);
	if (stream.eof())
	{
		return false;
	}

	unsigned int hour = (unsigned int)std::floor(decimalHour);
	unsigned int minute = (int)std::floor((decimalHour - hour)*60.0);
	double second = ((decimalHour - hour)*60.0 - minute) * 60;
	m_time = header.startTime;
	m_time.setTime(hour, minute, second);

	if (minute > 59)
		throw("Found a minute value bigger than 59, the file may be corrupt or be incorrectly formatted.");

	if (second >= 60.0)
		throw("Found a second value greater than or equal to 60, the file may be corrupt or be incorrectly formatted.");

	size_t nGates = header.nGates;
	m_gates.resize(nGates);
	m_dopplerVelocities.resize(nGates);
	m_intensities.resize(nGates);
	m_betas.resize(nGates);
	for (size_t i = 0; i < nGates; ++i)
	{
		stream >> m_gates[i] >> m_dopplerVelocities[i] >> m_intensities[i] >> m_betas[i];
		if (i != nGates - 1 && stream.eof())
			return false;
	}

	return true;
}