#pragma once
#include <string>
#include <iostream>
#include <svector\time.h>

enum ScanType
{
	st_stare,
	st_rhi,
	st_vad,
	st_wind,
	st_user1,
	st_user2,
	st_user3,
	st_user4
};

struct HplHeader
{
	std::string filename;
	unsigned int systemId;
	size_t nGates;
	double rangeGateLength;
	size_t pointsPerGate;
	size_t pulsesPerRay;
	size_t nRays;
	ScanType scanType;
	size_t focusRange;
	sci::UtcTime startTime;
	double dopplerResolution;
};

std::istream & operator>> (std::istream & stream, HplHeader &);
