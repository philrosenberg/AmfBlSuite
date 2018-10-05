#pragma once
#include"Campbell.h"
#include<svector/time.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include"HplHeader.h"
#include"Units.h"

class CampbellCeilometerProfile
{
public:
	CampbellCeilometerProfile(const sci::UtcTime &time, const CampbellMessage2 &profile)
		:m_time(time), m_profile(profile)
	{}
	const std::vector<steradianPerKilometre> &getBetas() const { return m_profile.getData(); }
	std::vector<size_t> getGates() const { return sci::indexvector<size_t>(m_profile.getData().size()); }
	void setTime(const sci::UtcTime &time) { m_time = time; }
	const sci::UtcTime &getTime() const { return m_time; }
	metre getCloudBase1() const { return m_profile.getHeight1(); }
	metre getCloudBase2() const { return m_profile.getHeight2(); }
	metre getCloudBase3() const { return m_profile.getHeight3(); }
	metre getCloudBase4() const { return m_profile.getHeight4(); }
	metre getVisibility() const { return m_profile.getVisibility(); }
	double getHighestSignal() const { return m_profile.getHighestSignal(); }
	percent getWindowTransmission() const { return m_profile.getWindowTransmission(); }
	metre getResolution() const { return m_profile.getResolution(); }
	percent getLaserPulseEnergy() const { return m_profile.getLaserPulseEnergy(); }
	kelvin getLaserTemperature() const { return m_profile.getLaserTemperature(); }
	radian getTiltAngle() const { return m_profile.getTiltAngle(); }
	millivolt getBackground() const { return m_profile.getBackground(); }
	size_t getPulseQuantity() const { return m_profile.getPulseQuantity(); }
	hertz getSampleRate() const { return m_profile.getSampleRate(); }
	char getProfileFlag() const;
	std::vector<char> getGateFlags() const;
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
	result.pointsPerGate = 1;
	result.pulsesPerRay = firstProfile.getPulseQuantity();
	result.rangeGateLength = firstProfile.getResolution();
	result.scanType = st_stare;
	result.startTime = firstProfile.getTime();
	result.systemId = firstHeader.getId();

	return result;
}

void writeToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, sci::string directory);