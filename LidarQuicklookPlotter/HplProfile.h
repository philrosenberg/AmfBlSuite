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
	void setTime(const sci::UtcTime &time) { m_time = time; }
	degreeF getAzimuth() const { return m_azimuth; }
	degreeF getElevation() const { return m_elevation; }
	std::vector<size_t> getGates() const { return m_gates; }
	std::vector<metrePerSecondF> getDopplerVelocities() const { return m_dopplerVelocities; }
	std::vector<unitlessF> getIntensities() const { return m_intensities; }
	std::vector<perSteradianPerMetreF> getBetas() const { return m_betas; }
	size_t nGates() const { return m_gates.size(); }
private:
	sci::UtcTime m_time;
	degreeF m_azimuth;
	degreeF m_elevation;
	degreeF m_pitch;
	degreeF m_roll;
	std::vector<size_t> m_gates;
	std::vector<metrePerSecondF> m_dopplerVelocities;
	std::vector<unitlessF> m_intensities;
	std::vector<perSteradianPerMetreF> m_betas;
};