#pragma once
#include<wx/xml/xml.h>
#include<svector/sstring.h>
#include"AmfNc.h"

void setup(sci::string setupFilename, sci::string infoFilename, ProgressReporter &progressReporter,
	ProcessingOptions &processingOptions, PersonInfo &author, ProjectInfo &projectInfo, std::shared_ptr<Platform> &platform,
	InstrumentInfo &lidarInstrumentInfo, CalibrationInfo &lidarCalibrationInfo);
