#pragma once
#include<svector/time.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include"HplHeader.h"
#include"Units.h"
#include"AmfNc.h"
#include"Plotting.h"

class ProgressReporter;
class wxWindow;

class LidarProcessor : public InstrumentPlotter
{
public:
	static void writeToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles,
		sci::string directory, const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo,
		const ProjectInfo &projectInfo, const PlatformInfo &platformInfo, const sci::string &comment);

	void readDataAndPlot(const std::string &inputFilename, const std::string &outputFilename,
		const std::vector<double> maxRanges, ProgressReporter &progressReporter, wxWindow *parent);

	static void plotCeilometerProfiles(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles,
		std::string filename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent);
};