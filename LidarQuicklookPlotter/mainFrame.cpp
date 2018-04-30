#include "mainFrame.h"
#include "HplHeader.h"
#include "HplProfile.h"
#include "Plotting.h"

const int mainFrame::ID_FILE_EXIT = ::wxNewId();
const int mainFrame::ID_FILE_RUN = ::wxNewId();
const int mainFrame::ID_HELP_ABOUT = ::wxNewId();

BEGIN_EVENT_TABLE(mainFrame, wxFrame)
EVT_MENU(ID_FILE_EXIT, mainFrame::OnExit)
EVT_MENU(ID_FILE_RUN, mainFrame::OnRun)
EVT_MENU(ID_HELP_ABOUT, mainFrame::OnAbout)
END_EVENT_TABLE()

mainFrame::mainFrame(wxFrame *frame, const wxString& title)
	: wxFrame(frame, -1, title)
{
	wxMenuBar* mbar = new wxMenuBar();
	wxMenu* fileMenu = new wxMenu(wxT(""));
	fileMenu->Append(ID_FILE_EXIT, wxT("E&xit\tAlt+F4"), wxT("Exit the application"));
	fileMenu->Append(ID_FILE_RUN, wxT("Run\tCtrl+R"), wxT("Run Code"));
	mbar->Append(fileMenu, wxT("&File"));

	wxMenu* helpMenu = new wxMenu(wxT(""));
	helpMenu->Append(ID_HELP_ABOUT, wxT("&About\tF1"), wxT("Show info about this application"));
	mbar->Append(helpMenu, wxT("&Help"));

	SetMenuBar(mbar);
}

void mainFrame::OnExit(wxCommandEvent& event)
{
	Close();
}

void plotFile(const std::string &inputFilename, const std::string &outputFilename, wxWindow *parent)
{
	std::fstream fin;
	HplHeader hplHeader;
	std::vector<HplProfile> profiles;
	try
	{
		fin.open(inputFilename.c_str(), std::ios::in);
		fin >> hplHeader;

		bool readingOkay = true;
		while (readingOkay)
		{
			profiles.resize(profiles.size() + 1);
			readingOkay = profiles.back().readFromStream(fin, hplHeader);
			if (!readingOkay) //we hit the end of the file while reading this profile
				profiles.resize(profiles.size() - 1);
		}

		plotProfiles(hplHeader, profiles, outputFilename, parent);
	}
	catch (char *err)
	{
		wxMessageBox(err);
	}
	catch (std::string err)
	{
		wxMessageBox(err);
	}

	fin.close();
}

void mainFrame::OnRun(wxCommandEvent& event)
{
	//plotFile("../../../data/co/2013/201301/20130124/Stare_05_20130124_16.hpl", "testStare.png", this);
	plotFile("../../../data/co/2013/201301/20130124/Wind_Profile_05_20130124_162124.hpl", "testWindProfile.png", this);
}

void mainFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox(wxT("$projectname$ Version 1.00.0"), wxT("About $projectname$..."));
}

mainFrame::~mainFrame()
{
}