#pragma once
#include<wx/xml/xml.h>
#include<svector/sstring.h>
#include"AmfNc.h"

class InstrumentProcessor;

void setup(sci::string setupFilename, sci::string infoFilename, ProgressReporter &progressReporter,
	ProcessingOptions &processingOptions, PersonInfo &author, ProjectInfo &projectInfo, std::shared_ptr<Platform> &platform,
	std::vector<std::shared_ptr<InstrumentProcessor>> &instrumentProcessors);
void setupProcessingOptionsOnly(sci::string setupFilename, ProcessingOptions &processingOptions);
