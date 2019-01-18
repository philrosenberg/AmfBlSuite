#include"plotting.h"
#include<svector\splot.h>
#include"HplHeader.h"
#include"HplProfile.h"
#include<fstream>
#include<wx/filename.h>
#include"ProgressReporter.h"
#include"Ceilometer.h"

void plotStareProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadPlanProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadUnrolledProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, double viewAzimuth, size_t nSegmentsMin, double maxRange, splot2d *plot);
void plotRhiProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, ProgressReporter& progressReporter, wxWindow *parent);




WindowCleaner::WindowCleaner(wxWindow *window)
	: m_window(window)
{}
WindowCleaner::~WindowCleaner()
{
	m_window->Destroy();
}


void plotProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, double maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	if (header.scanType == st_rhi)
		plotRhiProfiles(header, profiles, filename, progressReporter, parent);
	else if (header.scanType == st_vad || header.scanType == st_wind)
	{
		plotVadPlanProfiles(header, profiles, filename, 24, maxRange, progressReporter, parent);
		plotVadConeProfiles(header, profiles, filename + sci::string(sU("cone.png")), 1, maxRange, progressReporter, parent);
		plotVadUnrolledProfiles(header, profiles, filename + sci::string(sU("unrolled.png")), 1, maxRange, progressReporter, parent);
	}
	else //stare or user
		plotStareProfiles(header, profiles, filename, maxRange, progressReporter, parent);
}

sci::string getScanTypeString (const HplHeader &header)
{
	if (header.scanType == st_stare)
		return sU("Stare");
	if (header.scanType == st_rhi)
		return sU("RHI");
	if (header.scanType == st_vad)
		return sU("VAD");
	if (header.scanType == st_wind)
		return sU("Wind Profile");
	if (header.scanType == st_user1)
		return sU("User Scan 1");
	if (header.scanType == st_user2)
		return sU("User Scan 2");
	if (header.scanType == st_user3)
		return sU("User Scan 3");
	if (header.scanType == st_user4)
		return sU("User Scan 4");

	return sU("Unknown Scan Type");
}

void setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent, const HplHeader &header)
{
	*window = new splotframe(parent, true);
	(*window)->SetClientSize(1000, 1000);

	sci::ostringstream plotTitle;
	plotTitle << sU("Lidar Backscatter (m#u-1#d sr#u-1#d)") << extraDescriptor << sU("\n") << header.filename;
	*plot = (*window)->addplot(0.1, 0.1, 0.75, 0.75, false, false, plotTitle.str(), 20.0, 3.0);
	(*plot)->getxaxis()->settitlesize(15);
	(*plot)->getyaxis()->settitledistance(4.5);
	(*plot)->getyaxis()->settitlesize(15);
	(*plot)->getxaxis()->setlabelsize(15);
	(*plot)->getyaxis()->setlabelsize(15);

	splotlegend *legend = (*window)->addlegend(0.86, 0.25, 0.13, 0.65, wxColour(0, 0, 0), 0.0);
	legend->addentry("", g_lidarColourscale, false, false, 0.05, 0.3, 15, "", 0, 0.05, wxColour(0, 0, 0), 128, false, 150, false);
}

void createDirectoryAndWritePlot(splotframe *plotCanvas, sci::string filename, size_t width, size_t height, ProgressReporter &progressReporter)
{
	size_t lastSlashPosition = filename.find_last_of(sU("/\\"));
	sci::string directory;
	if (lastSlashPosition == sci::string::npos)
		directory = sU(".");
	else
		directory = filename.substr(0, lastSlashPosition + 1);
	
	if (!wxDirExists(sci::nativeUnicode(directory)))
		wxFileName::Mkdir(sci::nativeUnicode(directory), 770, wxPATH_MKDIR_FULL);
	if (!wxDirExists(sci::nativeUnicode(directory)))
	{
		sci::ostringstream message;
		message << sU("The output directory ") << directory << sU("does not exist and could not be created.");
		throw (message.str());
	}

	sci::string lastFourChars = filename.length() < 4 ? sU("") : filename.substr(filename.length() - 4, sci::string::npos);
	if (lastFourChars != sU(".png") && lastFourChars != sU(".PNG"))
		filename = filename + sU(".png");

	progressReporter << sU("Rendering plot to file - the application may be unresponsive for a minute.\n");
	if (progressReporter.shouldStop())
	{
		return;
	}
	if( !plotCanvas->writetofile(filename, width, height, 1.0))
	{
		sci::ostringstream message;
		message << sU("The output file ") << filename << sU(" could not be created.");
		throw (message.str());
	}
	progressReporter << sU("Plot rendered to ") << filename << sU("\n");
}




