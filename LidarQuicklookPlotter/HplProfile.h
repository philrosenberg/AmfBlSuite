#pragma once
#include<svector\time.h>
#include<vector>
#include<iostream>

struct HplHeader;

class HplProfile
{
public:
	bool readFromStream(std::istream &stream, const HplHeader &header);
	sci::UtcTime getTime() const { return m_time; }
	double getAzimuthDegrees() const { return m_azimuthDeg; }
	double getElevationDegrees() const { return m_elevationDeg; }
	std::vector<size_t> getGates() const { return m_gates; }
	std::vector<double> getDopplerVelocities() const { return m_dopplerVelocities; }
	std::vector<double> getIntensities() const { return m_intensities; }
	std::vector<double> getBetas() const { return m_betas; }
private:
	sci::UtcTime m_time;
	double m_azimuthDeg;
	double m_elevationDeg;
	double m_pitch;
	double m_roll;
	std::vector<size_t> m_gates;
	std::vector<double> m_dopplerVelocities;
	std::vector<double> m_intensities;
	std::vector<double> m_betas;
};