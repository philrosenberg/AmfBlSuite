#pragma once
#include <string>
#include <iostream>
#include <svector/time.h>
#include "Units.h"
#include <svector/sstring.h>

enum class ScanType
{
	stare,
	rhi,
	vad,
	wind,
	user1,
	user2,
	user3,
	user4
};

#pragma warning(push)
#pragma warning(disable : 26495)
struct HplHeader
{
	sci::string filename;
	unsigned int systemId;
	size_t nGates;
	metreF rangeGateLength;
	size_t pointsPerGate;
	size_t pulsesPerRay;
	size_t nRays;
	ScanType scanType;
	size_t focusRange;
	sci::UtcTime startTime;
	double dopplerResolution;
	bool oldType;
};
#pragma warning(pop)

std::istream & operator>> (std::istream & stream, HplHeader &);
