#include "mainFrame.h"
#include "HplHeader.h"
#include "HplProfile.h"
#include "Plotting.h"
#include<wx/dir.h>
#include<wx/filename.h>
#include"TextCtrlProgressReporter.h"

const int mainFrame::ID_FILE_EXIT = ::wxNewId();
const int mainFrame::ID_FILE_RUN = ::wxNewId();
const int mainFrame::ID_FILE_STOP = ::wxNewId();
const int mainFrame::ID_FILE_SELECT_INPUT_DIR = ::wxNewId();
const int mainFrame::ID_FILE_SELECT_OUTPUT_DIR = ::wxNewId();
const int mainFrame::ID_HELP_ABOUT = ::wxNewId();
const int mainFrame::ID_CHECK_DATA_TIMER = ::wxNewId();
const int mainFrame::ID_INSTANT_CHECK_DATA_TIMER = ::wxNewId();

BEGIN_EVENT_TABLE(mainFrame, wxFrame)
EVT_MENU(ID_FILE_EXIT, mainFrame::OnExit)
EVT_MENU(ID_FILE_RUN, mainFrame::OnRun)
EVT_MENU(ID_FILE_STOP, mainFrame::OnStop)
EVT_MENU(ID_FILE_SELECT_INPUT_DIR, mainFrame::OnSelectInputDir)
EVT_MENU(ID_FILE_SELECT_OUTPUT_DIR, mainFrame::OnSelectOutputDir)
EVT_MENU(ID_HELP_ABOUT, mainFrame::OnAbout)
EVT_TIMER(ID_CHECK_DATA_TIMER, mainFrame::OnCheckDataTimer)
EVT_TIMER(ID_INSTANT_CHECK_DATA_TIMER, mainFrame::OnCheckDataTimer)
END_EVENT_TABLE()

mainFrame::mainFrame(wxFrame *frame, const wxString& title, const wxString &inputDirectory, const wxString &outputDirectory, bool runImmediately)
	: wxFrame(frame, -1, title)
{
	wxMenuBar* mbar = new wxMenuBar();
	wxMenu* fileMenu = new wxMenu(wxT(""));
	fileMenu->Append(ID_FILE_EXIT, wxT("E&xit\tAlt+F4"), wxT("Exit the application"));
	fileMenu->Append(ID_FILE_RUN, wxT("Run\tCtrl+R"), wxT("Run Code"));
	fileMenu->Append(ID_FILE_STOP, wxT("Stop\tCtrl+X"), wxT("Stop Code"));
	fileMenu->Append(ID_FILE_SELECT_INPUT_DIR, wxT("Select Input Dir"), wxT("Select directory to search for data files"));
	fileMenu->Append(ID_FILE_SELECT_OUTPUT_DIR, wxT("Select Output Dir"), wxT("Select directory to store plots"));
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
	m_instantCheckTimer = new wxTimer(this, ID_INSTANT_CHECK_DATA_TIMER);

	m_plotting = false;

	m_inputDirectory = inputDirectory;
	m_outputDirectory = outputDirectory;

	m_progressReporter.reset(new TextCtrlProgressReporter(m_logText, true, this));
	m_progressReporter->setShouldStop(true);

	if (runImmediately)
		start();
}

void mainFrame::OnExit(wxCommandEvent& event)
{
	Close();
}

void plotFile(const std::string &inputFilename, const std::string &outputFilename, double maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{

	std::fstream fin;
	HplHeader hplHeader;
	std::vector<HplProfile> profiles;
	try
	{
		fin.open(inputFilename.c_str(), std::ios::in);
		if (!fin.is_open())
			throw("Could not open file");

		progressReporter << "Reading file.\n";
		fin >> hplHeader;

		bool readingOkay = true;
		while (readingOkay)
		{
			profiles.resize(profiles.size() + 1);
			readingOkay = profiles.back().readFromStream(fin, hplHeader);
			if (!readingOkay) //we hit the end of the file while reading this profile
				profiles.resize(profiles.size() - 1);
			if (readingOkay)
			{
				if (profiles.size() == 1)
					progressReporter << "Read profile 1";
				else if (profiles.size() <= 50)
					progressReporter << ", " << profiles.size();
				else if (profiles.size() <=500 && profiles.size() % 10 == 0)
					progressReporter << ", " << profiles.size() - 9 << "-" << profiles.size();
				else if (profiles.size() % 100 == 0)
					progressReporter << ", " << profiles.size() - 99 << "-" << profiles.size();
			}
			if (progressReporter.shouldStop())
				break;
		}
		if (progressReporter.shouldStop())
		{
			progressReporter << "\n";
			fin.close();
			return;

		}
		progressReporter << ", done.\n";

		plotProfiles(hplHeader, profiles, outputFilename, maxRange, progressReporter, parent);
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

void mainFrame::OnSelectInputDir(wxCommandEvent& event)
{
	if (m_plotting || !m_progressReporter->shouldStop())
	{
		wxMessageBox("Please stop processing before attempting to change a directory.");
		return;
	}
	std::string dir = wxDirSelector("Select the input directory to search for data files.",m_inputDirectory, wxDD_DIR_MUST_EXIST|wxDD_CHANGE_DIR);
	if (!dir.length() == 0)
		m_inputDirectory = dir;
	(*m_progressReporter) << "Input directory changed to " << m_inputDirectory;
}

void mainFrame::OnSelectOutputDir(wxCommandEvent& event)
{
	if (m_plotting || !m_progressReporter->shouldStop())
	{
		wxMessageBox("Please stop processing before attempting to change a directory.");
		return;
	}
	std::string dir = wxDirSelector("Select the output directory to store plots.", m_outputDirectory, wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR);
	if (!dir.length() == 0)
		m_outputDirectory = dir;
	(*m_progressReporter) << "Output directory changed to " << m_outputDirectory;
}

void mainFrame::start()
{
	if (!m_progressReporter->shouldStop())
		return;
	if (m_progressReporter->shouldStop() && m_plotting)
	{
		//The user has requested to stop but before the stop has happened they requested to start again
		//Tell them to be patient
		wxMessageBox("Although you have requested plotting to stop, the software is just waiting for code to finish execution before it can do so. Please wait for the current processing to halt before trying to restart.");
	}
	m_progressReporter->setShouldStop(false);
	m_logText->AppendText("Starting\n");
	m_checkForNewDataTimer->Start(300000);//Use this timer to check for new data every 5 mins
	m_instantCheckTimer->StartOnce(1);//Use this timer to check for new data now
}

void mainFrame::stop()
{
	if (m_progressReporter->shouldStop())
		return;
	m_checkForNewDataTimer->Stop();
	m_progressReporter->setShouldStop(true);
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

std::vector<std::string> getDirectoryListing(const std::string &directory, const std::string &filespec)
{
	wxArrayString files;
	wxDir::GetAllFiles(directory, &files, filespec);
	std::vector<std::string> result(files.size());
	for (size_t i = 0; i < files.size(); ++i)
		result[i] = std::string(files[i]);
	return result;
}

void mainFrame::plot()
{
	if (m_plotting)
		return;
	m_plotting = true;
	//There must be no code that can throw without being caught until m_plotting is set back to false
	try
	{
		if(!m_progressReporter->shouldStop())
			plot("*Stare_??_????????_??.hpl");
		if (!m_progressReporter->shouldStop())
			plot("*VAD_??_????????_??????.hpl");
		if (!m_progressReporter->shouldStop())
			(*m_progressReporter) << "Generated plots for all files found. Waiting approx 5 mins to check again.\n\n";
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

	m_plotting = false;
	if (m_progressReporter->shouldStop())
		m_logText->AppendText("Stopped\n\n");
}

//Plot just a specific file type - the last file found alphabetically
//is assumed to be the last file chronologically (assuming you have a
//sensible naming structure) and it is assumed that this file may be
//incomplete, so will be replotted next time just in case.
void mainFrame::plot(const std::string &filter)
{
	//ensure that the input and output directories end with a slash or are empty
	if (m_inputDirectory.length() > 0 && m_inputDirectory.back()!= '/' && m_inputDirectory.back()!= '\\')
		m_inputDirectory = m_inputDirectory+"/";
	if (m_outputDirectory.length() > 0 && m_outputDirectory.back() != '/' && m_outputDirectory.back() != '\\')
		m_outputDirectory = m_outputDirectory + "/";

	std::vector<std::string> plottedFiles;
	(*m_progressReporter) << "Looking for data files to plot.\n";
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
	fin.close();

	//Find all the files in the input directory
	std::vector<std::string> allFiles = getDirectoryListing(m_inputDirectory, filter);

	//Sort the files in alphabetical order - this will also put them in time order
	//which is important for hunting out the last file of any type which may have been
	//incomplete.
	if(allFiles.size() > 0)
		std::sort(allFiles.begin(), allFiles.end());

	//Remember the name of the last file in the list - we assume thismay be incomplete
	//so don't let it get remembered as a previously plotted file
	std::string lastFileToPlot;
	if (allFiles.size() > 0)
		lastFileToPlot = allFiles.back();

	//Filter for just files we are interested in
	//std::vector<std::string> filesOfInterest;
	//filesOfInterest.reserve(allFiles.size());
	//for (size_t i = 0; i < allFiles.size(); ++i)
	//{
	//	if (allFiles[i].find("Stare", m_inputDirectory.length()) != std::string::npos)
	//		filesOfInterest.push_back(allFiles[i]);
	//	if (allFiles[i].find("VAD", m_inputDirectory.length()) != std::string::npos)
	//		filesOfInterest.push_back(allFiles[i]);
	//}
	//The code above was replaced by wildcard filtering when we hunt for files
	//however it has been left in place in case we need to do something similar in
	//the future
	std::vector<std::string> filesOfInterest = allFiles;

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
		(*m_progressReporter) << "Found no new files to plot.\n";
	else
	{
		(*m_progressReporter) << "Found the following new files to plot matching the filter " << filter << " :\n";
		for(size_t i=0; i<filesToPlot.size(); ++i)
			(*m_progressReporter) << "\t" << filesToPlot[i] << "\n";

		for (size_t i = 0; i < filesToPlot.size(); ++i)
		{
			(*m_progressReporter) << "Processing " << filesToPlot[i] << "\n";
			std::string outputFile = m_outputDirectory + filesToPlot[i].substr(m_inputDirectory.length(), std::string::npos);
			try
			{
				plotFile(filesToPlot[i], outputFile, std::numeric_limits<double>::max(), *m_progressReporter, this);
				
				if (m_progressReporter->shouldStop())
				{
					(*m_progressReporter) << "Operation halted at user request.\n";
					break;
				}

				//remember which files have been plotted
				if (filesToPlot[i] != lastFileToPlot)
				{
					std::fstream fout;
					fout.open(previouslyPlottedFilename.c_str(), std::ios::app);
					fout << filesToPlot[i] << "\n";
					fout.close();
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

			if (m_progressReporter->shouldStop())
			{
				(*m_progressReporter) << "Operation halted at user request.\n";
				break;
			}
		}

		//plotFile("../../../data/co/2013/201301/20130124/Stare_05_20130124_16.hpl", "testStare.png", this);
		//plotFile("../../../data/co/2013/201301/20130124/Wind_Profile_05_20130124_162124.hpl", "testWindProfile.png", this);
		//plotFile("VAD_18_20180712_090610.hpl", "moccha_clutter_1000.png", 1000, this);
		//plotFile("D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\doppler_lidar_backup_ds1\\Data\\Proc\\2018\\201807\\20180711\\Stare_18_20180711_20.hpl", "moccha_test_stare_cubehelix.png", 1000, this);
		//plotFile("D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\doppler_lidar_backup_ds1\\Data\\Proc\\2018\\201807\\20180711\\VAD_18_20180711_081948.hpl", "moccha_test_vad_cubehelix.png", 10000, this);
	}
	if(!m_progressReporter->shouldStop())
		(*m_progressReporter) << "Generated plots for all files matching filter " << filter << "\n";
	
}

void mainFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox(wxT("$projectname$ Version 1.00.0"), wxT("About $projectname$..."));
}

mainFrame::~mainFrame()
{
}