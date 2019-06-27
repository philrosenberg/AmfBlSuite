#pragma once
#include <string>
#include <iostream>
#include <svector\time.h>
#include "Units.h"

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
	sci::string filename;
	unsigned int systemId;
	size_t nGates;
	metre rangeGateLength;
	size_t pointsPerGate;
	size_t pulsesPerRay;
	size_t nRays;
	ScanType scanType;
	size_t focusRange;
	sci::UtcTime startTime;
	double dopplerResolution;
	bool oldType;
};

std::istream & operator>> (std::istream & stream, HplHeader &);
