#pragma once
#include<svector\time.h>
#include<vector>
#include<iostream>
#include"Units.h"
#include<svector/gridview.h>

struct HplHeader;

class HplProfile
{
public:
	bool readFromStream(std::istream &stream, const HplHeader &header);
	template<class T>
	T getTime() const { static_assert(false, "HplProfile::getTime<T> can only be called with T=sci::UtcTime or T=second."); }
	template<>
	sci::UtcTime getTime<sci::UtcTime>() const { return m_time; }
	template<>
	second getTime<second>() const { return second(m_time - sci::UtcTime(1970, 1, 1, 0, 0, 0)); }
	void setTime(const sci::UtcTime &time) { m_time = time; }
	degreeF getAzimuth() const { return m_azimuth; }
	degreeF getElevation() const { return m_elevation; }
	sci::GridData<size_t, 1> getGates() const { return sci::GridData<size_t, 1>(m_gates.begin(), m_gates.end()); }
	sci::GridData<metrePerSecondF, 1> getDopplerVelocities() const { return sci::GridData<metrePerSecondF, 1>(m_dopplerVelocities.begin(), m_dopplerVelocities.end()); }
	sci::GridData<unitlessF, 1> getIntensities() const { return sci::GridData<unitlessF, 1>(m_intensities.begin(), m_intensities.end()); }
	sci::GridData<perSteradianPerMetreF, 1> getBetas() const { return sci::GridData<perSteradianPerMetreF, 1>(m_betas.begin(), m_betas.end()); }
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