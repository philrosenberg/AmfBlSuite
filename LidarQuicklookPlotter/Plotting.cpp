#include"plotting.h"
#include<svector\splot.h>
#include"HplHeader.h"
#include"HplProfile.h"
#include<fstream>
#include<wx/filename.h>
#include"ProgressReporter.h"
#include"Ceilometer.h"
#include<wx/dir.h>
#include<wx/regex.h>



WindowCleaner::WindowCleaner(wxWindow *window)
	: m_window(window)
{}
WindowCleaner::~WindowCleaner()
{
	m_window->Destroy();
}



sci::string getScanTypeString(const HplHeader &header)
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
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, message.str()));
	}

	sci::string lastFourChars = filename.length() < 4 ? sU("") : filename.substr(filename.length() - 4, sci::string::npos);
	if (lastFourChars != sU(".png") && lastFourChars != sU(".PNG"))
		filename = filename + sU(".png");

	progressReporter << sU("Rendering plot to file - the application may be unresponsive for a minute.\n");
	if (progressReporter.shouldStop())
	{
		return;
	}
	if (!plotCanvas->writetofile(filename, width, height, 1.0))
	{
		sci::ostringstream message;
		message << sU("The output file ") << filename << sU(" could not be created.");
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, message.str()));
	}
	progressReporter << sU("Plot rendered to ") << filename << sU("\n");
}

