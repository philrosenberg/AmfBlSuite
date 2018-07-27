#include "mainFrame.h"
#include "HplHeader.h"
#include "HplProfile.h"
#include "Plotting.h"
#include<wx/dir.h>
#include<wx/filename.h>

const int mainFrame::ID_FILE_EXIT = ::wxNewId();
const int mainFrame::ID_FILE_RUN = ::wxNewId();
const int mainFrame::ID_FILE_STOP = ::wxNewId();
const int mainFrame::ID_HELP_ABOUT = ::wxNewId();
const int mainFrame::ID_CHECK_DATA_TIMER = ::wxNewId();

BEGIN_EVENT_TABLE(mainFrame, wxFrame)
EVT_MENU(ID_FILE_EXIT, mainFrame::OnExit)
EVT_MENU(ID_FILE_RUN, mainFrame::OnRun)
EVT_MENU(ID_FILE_STOP, mainFrame::OnStop)
EVT_MENU(ID_HELP_ABOUT, mainFrame::OnAbout)
EVT_TIMER(ID_CHECK_DATA_TIMER, mainFrame::OnCheckDataTimer)
END_EVENT_TABLE()

mainFrame::mainFrame(wxFrame *frame, const wxString& title)
	: wxFrame(frame, -1, title)
{
	wxMenuBar* mbar = new wxMenuBar();
	wxMenu* fileMenu = new wxMenu(wxT(""));
	fileMenu->Append(ID_FILE_EXIT, wxT("E&xit\tAlt+F4"), wxT("Exit the application"));
	fileMenu->Append(ID_FILE_RUN, wxT("Run\tCtrl+R"), wxT("Run Code"));
	fileMenu->Append(ID_FILE_STOP, wxT("Stop\tCtrl+X"), wxT("Stop Code"));
	mbar->Append(fileMenu, wxT("&File"));

	wxMenu* helpMenu = new wxMenu(wxT(""));
	helpMenu->Append(ID_HELP_ABOUT, wxT("&About\tF1"), wxT("Show info about this application"));
	mbar->Append(helpMenu, wxT("&Help"));
	SetMenuBar(mbar);

	wxPanel *panel = new wxPanel(this);

	wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

	m_logText = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(300, 500), wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH);
	m_logText->SetMinSize(wxSize(100, 20));

	topSizer->Add(m_logText, 1, wxEXPAND);

	panel->SetSizer(topSizer);
	topSizer->SetSizeHints(panel);

	m_checkForNewDataTimer = new wxTimer(this, ID_CHECK_DATA_TIMER);

	m_started = false;
	m_plotting = false;
	m_shouldStop = false;

	m_inputDirectory = "D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\doppler_lidar_backup_ds1\\Data\\Proc\\2018";
	m_outputDirectory = "D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\quicklooks";
}

void mainFrame::OnExit(wxCommandEvent& event)
{
	Close();
}

void plotFile(const std::string &inputFilename, const std::string &outputFilename, double maxRange, wxWindow *parent)
{
	std::fstream fin;
	HplHeader hplHeader;
	std::vector<HplProfile> profiles;
	try
	{
		fin.open(inputFilename.c_str(), std::ios::in);
		if (!fin.is_open())
			throw("Could not open file");
		fin >> hplHeader;

		bool readingOkay = true;
		while (readingOkay)
		{
			profiles.resize(profiles.size() + 1);
			readingOkay = profiles.back().readFromStream(fin, hplHeader);
			if (!readingOkay) //we hit the end of the file while reading this profile
				profiles.resize(profiles.size() - 1);
		}

		plotProfiles(hplHeader, profiles, outputFilename, maxRange, parent);
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
	start();
}

void mainFrame::OnStop(wxCommandEvent& event)
{
	stop();
}

void mainFrame::start()
{
	if (m_started)
		return;
	m_logText->AppendText("Starting\n");
	m_checkForNewDataTimer->Start(300000);//5 mins
	plot();
}

void mainFrame::stop()
{
	if (!m_started)
		return;
	m_checkForNewDataTimer->Stop();
	m_shouldStop = true;
	if( m_plotting)
		m_logText->AppendText("Stopping\n");
	else
		m_logText->AppendText("Stopped\n\n");
}

void mainFrame::OnCheckDataTimer(wxTimerEvent& event)
{
	if (m_plotting)
		return;
	plot();
}

std::vector<std::string> getDirectoryListing(const std::string &directory)
{
	wxArrayString files;
	wxDir::GetAllFiles(directory, &files);
	std::vector<std::string> result(files.size());
	for (size_t i = 0; i < files.size(); ++i)
		result[i] = std::string(files[i]);
	return result;
}

void mainFrame::plot()
{
	m_plotting = true;
	std::vector<std::string> plottedFiles;
	try
	{
		std::ostream logStream(m_logText);
		logStream << "Looking for data files to plot.\n";
		//check the output directory exists
		if (!wxDirExists(m_outputDirectory))
			wxFileName::Mkdir(m_outputDirectory, 770, wxPATH_MKDIR_FULL);
		if (!wxDirExists(m_outputDirectory))
		{
			std::ostringstream message;
			message << "The output directory " << m_outputDirectory << " does not exist and could not be created.";
			throw(message.str());
		}

		//check the input directory exists
		if(!wxDirExists(m_inputDirectory))
		{
			std::ostringstream message;
			message << "The input directory " << m_inputDirectory << " does not exist.";
			throw(message.str());
		}

		//get a list of all the files previously plotted
		std::vector<std::string> previouslyPlottedFiles;
		std::string previouslyPlottedFilename = m_outputDirectory + "previouslyPlottedFiles.txt";

		std::fstream fin;
		fin.open(previouslyPlottedFilename.c_str(), std::ios::in);
		std::string filename;
		std::getline(fin, filename);
		while (filename.length() > 0)
		{
			previouslyPlottedFiles.push_back(filename);
			std::getline(fin, filename);
		}

		//Find all the files in the input directory
		std::vector<std::string> allFiles = getDirectoryListing(m_inputDirectory);

		//Filter for just files we are interested in
		std::vector<std::string> filesOfInterest;
		filesOfInterest.reserve(allFiles.size());
		for (size_t i = 0; i < allFiles.size(); ++i)
		{
			if (allFiles[i].find("Stare", m_inputDirectory.length()) != std::string::npos)
				filesOfInterest.push_back(allFiles[i]);
			if (allFiles[i].find("VAD", m_inputDirectory.length()) != std::string::npos)
				filesOfInterest.push_back(allFiles[i]);
		}


		std::vector<std::string> filesToPlot;
		filesToPlot.reserve(filesOfInterest.size());
		for (size_t i = 0; i < filesOfInterest.size(); ++i)
		{
			bool alreadyPlotted = false;
			for (size_t j = 0; j < previouslyPlottedFiles.size(); ++j)
			{
				if (filesOfInterest[i] == previouslyPlottedFiles[j])
				{
					alreadyPlotted = true;
					break;
				}
			}
			if (!alreadyPlotted)
				filesToPlot.push_back(filesOfInterest[i]);
		}

		if (filesToPlot.size() == 0)
			logStream << "Found no new files to plot.\n";

		else
		{
			//Sort the files in alphabetical order - this will also put them in time order
			//which is important for hunting out the last file of any type which may have been
			//incomplete.
			std::sort(filesToPlot.begin(), filesToPlot.end());

			logStream << "Found the following new files to plot:\n";
			for(size_t i=0; i<filesToPlot.size(); ++i)
				logStream << "\t" << filesToPlot[i] << "\n";

			std::fstream fout;
			fout.open(previouslyPlottedFilename.c_str(), std::ios::app);
			try
			{
				for (size_t i = 0; i < filesToPlot.size(); ++i)
				{
					logStream << "Plotting " << filesToPlot[i] << "\n";
					std::string outputFile = m_outputDirectory + filesToPlot[i].substr(m_inputDirectory.length(), std::string::npos);
					plotFile(filesToPlot[i], outputFile, std::numeric_limits<double>::max(), this);
					logStream << "File plotted to " << outputFile << "\n";

					//remember which files have been plotted
					fout << filesToPlot[i] << "\n";
				}
			}
			catch (...)
			{
				fout.close();
				throw;
			}


			//plotFile("../../../data/co/2013/201301/20130124/Stare_05_20130124_16.hpl", "testStare.png", this);
			//plotFile("../../../data/co/2013/201301/20130124/Wind_Profile_05_20130124_162124.hpl", "testWindProfile.png", this);
			//plotFile("VAD_18_20180712_090610.hpl", "moccha_clutter_1000.png", 1000, this);
			//plotFile("D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\doppler_lidar_backup_ds1\\Data\\Proc\\2018\\201807\\20180711\\Stare_18_20180711_20.hpl", "moccha_test_stare_cubehelix.png", 1000, this);
			//plotFile("D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\doppler_lidar_backup_ds1\\Data\\Proc\\2018\\201807\\20180711\\VAD_18_20180711_081948.hpl", "moccha_test_vad_cubehelix.png", 10000, this);
		}
	}
	catch (sci::err err)
	{
		wxTextAttr originalStyle = m_logText->GetDefaultStyle();
		m_logText->SetDefaultStyle(wxTextAttr(*wxRED));
		std::ostream stream(m_logText);
		stream << "Error: " << err.getErrorCategory() << ":" << err.getErrorCode() << " " << err.getErrorMessage() << "\n";
		m_logText->SetDefaultStyle(originalStyle);
	}
	catch (char *errMessage)
	{
		wxTextAttr originalStyle = m_logText->GetDefaultStyle();
		m_logText->SetDefaultStyle(wxTextAttr(*wxRED));
		std::ostream stream(m_logText);
		stream << "Error: " << errMessage << "\n";
		m_logText->SetDefaultStyle(originalStyle);
	}
	catch (std::string errMessage)
	{
		wxTextAttr originalStyle = m_logText->GetDefaultStyle();
		m_logText->SetDefaultStyle(wxTextAttr(*wxRED));
		std::ostream stream(m_logText);
		stream << "Error: " << errMessage << "\n";
		m_logText->SetDefaultStyle(originalStyle);
	}
	catch (wxString errMessage)
	{
		wxTextAttr originalStyle = m_logText->GetDefaultStyle();
		m_logText->SetDefaultStyle(wxTextAttr(*wxRED));
		std::ostream stream(m_logText);
		stream << "Error: " << errMessage << "\n";
		m_logText->SetDefaultStyle(originalStyle);
	}
	catch (...)
	{
		wxTextAttr originalStyle = m_logText->GetDefaultStyle();
		m_logText->SetDefaultStyle(wxTextAttr(*wxRED));
		std::ostream stream(m_logText);
		stream << "Error: Unknown\n";
		m_logText->SetDefaultStyle(originalStyle);
	}

	if(m_shouldStop)
		m_logText->AppendText("Stopped\n\n");

	m_plotting = false;
	m_shouldStop = false;
}

void mainFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox(wxT("$projectname$ Version 1.00.0"), wxT("About $projectname$..."));
}

mainFrame::~mainFrame()
{
}