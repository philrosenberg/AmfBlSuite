#pragma once
#include"Campbell.h"
#include<svector/time.h>
#include<svector/svector.h>
#include"HplHeader.h"

class CampbellCeilometerProfile
{
public:
	CampbellCeilometerProfile(const sci::UtcTime &time, const CampbellMessage2 &profile)
		:m_time(time), m_profile(profile)
	{}
	const std::vector<double> &getBetas() const { return m_profile.getData(); }
	std::vector<size_t> getGates() const { return sci::indexvector<size_t>(m_profile.getData().size()); }
	void setTime(const sci::UtcTime &time) { m_time = time; }
	const sci::UtcTime &getTime() const { return m_time; }
	double getResolution() const { return m_profile.getResolution(); }
	double getSampleRate() const { return m_profile.getSampleRate(); }
	double getPulseQuantity() const { return m_profile.getPulseQuantity(); }
private:
	CampbellMessage2 m_profile;
	sci::UtcTime m_time;
};


inline HplHeader getCeilometerHeader(const std::string &filename, const CampbellHeader &firstHeader, const CampbellCeilometerProfile &firstProfile)
{
	HplHeader result;
	result.dopplerResolution = std::numeric_limits<double>::quiet_NaN();
	result.filename = filename;
	result.focusRange = 0;
	result.nGates = 2046;
	result.nRays = 1;
	result.pointsPerGate = firstProfile.getResolution()*3.0e6/firstProfile.getSampleRate();
	result.pulsesPerRay = firstProfile.getPulseQuantity();
	result.rangeGateLength = firstProfile.getResolution();
	result.scanType = st_stare;
	result.startTime = firstProfile.getTime();
	result.systemId = firstHeader.getId();

	return result;
}