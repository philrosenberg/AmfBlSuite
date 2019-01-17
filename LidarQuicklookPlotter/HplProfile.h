#pragma once
#include<svector\time.h>
#include<vector>
#include<iostream>
#include"Units.h"

struct HplHeader;

class HplProfile
{
public:
	bool readFromStream(std::istream &stream, const HplHeader &header);
	template<class T>
	T getTime() const { static_assert(false "HplProfile::getTime<T> can only be called with T=sci::UtcTime or T=second."); }
	template<>
	sci::UtcTime getTime<sci::UtcTime>() const { return m_time; }
	template<>
	second getTime<second>() const { return second(m_time.getUnixTime()); }
	radian getAzimuth() const { return m_azimuth; }
	radian getElevation() const { return m_elevation; }
	std::vector<size_t> getGates() const { return m_gates; }
	std::vector<metrePerSecond> getDopplerVelocities() const { return m_dopplerVelocities; }
	std::vector<unitless> getIntensities() const { return m_intensities; }
	std::vector<perSteradianPerMetre> getBetas() const { return m_betas; }
	size_t nGates() const { return m_gates.size(); }
private:
	sci::UtcTime m_time;
	radian m_azimuth;
	radian m_elevation;
	radian m_pitch;
	radian m_roll;
	std::vector<size_t> m_gates;
	std::vector<metrePerSecond> m_dopplerVelocities;
	std::vector<unitless> m_intensities;
	std::vector<perSteradianPerMetre> m_betas;
};