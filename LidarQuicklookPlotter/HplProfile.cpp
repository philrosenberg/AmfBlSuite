#include"HplProfile.h"
#include"HplHeader.h"

bool HplProfile::readFromStream(std::istream &stream, const HplHeader &header)
{
	double decimalHour;
	stream >> decimalHour >> m_azimuthDeg >> m_elevationDeg;
	if (stream.eof())
	{
		return false;
	}

	unsigned int hour = (unsigned int)std::floor(decimalHour);
	unsigned int minute = std::floor((decimalHour - hour)*60.0);
	unsigned int second = ((decimalHour - hour)*60.0 - minute) * 60;
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