#include "mainFrame.h"
#include "HplHeader.h"
#include "HplProfile.h"
#include "Plotting.h"
#include<wx/dir.h>
#include<wx/filename.h>
#include"TextCtrlProgressReporter.h"
#include"FolderChangesLister.h"
#include"Campbell.h"
#include"Ceilometer.h"

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
	else
	{
		m_logText->SetValue("To run Lidar Quicklook Plotter either run with 2 command line arguments - the first "
			"being the directory to search for data and the second being the directory to store the plots - or "
			"run with no command line arguments and manually select the directories and start processing using "
			"the items under the file menu. If you use the command line then the plotting will start immediately\n"
			"The file structure in the input directory will be searched for data that can be plotted and the same "
			"structure will be mirrored in the output directory. A file called \"previouslyPlottedFiles.txt\" "
			"will be place in the output directory and, as you might expect, lists all previously plotted files. "
			"The software will check every 10 minutes to see if new files have been created that can be plotted. "
			"Note that the last file alphabetically of a given type is assumed to be the latest one created, and "
			"it is assumed that this file may not be complete, therefore it is never listed as previously plotted.\n"
			"You may run multiple instances of this software to plot data from multiple directories, but it may "
			"be prudent to use different output directories for each instance so that clashes do not occur with "
			"the previouslyPlottedFiles.txt file."
			"\n\n");
		if (wxTheApp->argc != 1 && wxTheApp->argc != 3)
		{
			wxTextAttr originalStyle = m_logText->GetDefaultStyle();
			m_logText->SetDefaultStyle(wxTextAttr(*wxRED));
			m_logText->AppendText("You ran Lidar Quicklook Plotter with the wrong number of arguments. Check above for "
				"usage instructions. You may still manually select directories and start the plotting.\n\n");
			m_logText->SetDefaultStyle(originalStyle);
		}
	}
}

void mainFrame::OnExit(wxCommandEvent& event)
{
	Close();
}

sci::UtcTime getCeilometerTime(const std::string &timeDateString)
{
	sci::UtcTime result(
		std::atoi(timeDateString.substr(0, 4).c_str()), //year
		std::atoi(timeDateString.substr(5, 2).c_str()), //month
		std::atoi(timeDateString.substr(8, 2).c_str()), //day
		std::atoi(timeDateString.substr(11, 2).c_str()), //hour
		std::atoi(timeDateString.substr(14, 2).c_str()), //minute
		std::atof(timeDateString.substr(17, std::string::npos).c_str()) //second
	);
	return result;
}

void plotFile(const std::string &inputFilename, const std::string &outputFilename, const std::vector<double> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	//If this is a processed wind profile then we jsut send the file straiht off to the code to plot that
	if (inputFilename.find("Processed_Wind_Profile") != std::string::npos)
	{
		std::fstream fin;
		fin.open(inputFilename.c_str(), std::ios::in);
		if (!fin.is_open())
			throw("Could not open file");
		size_t nPoints;
		fin >> nPoints;
		std::vector<double> height(nPoints);
		std::vector<double> degrees(nPoints);
		std::vector<double> speed(nPoints);

		for (size_t i = 0; i < nPoints; ++i)
		{
			fin >> height[i] >> degrees[i] >> speed[i];
		}
		fin.close();

		for (size_t i = 0; i < maxRanges.size(); ++i)
		{
			std::ostringstream rangeLimitedfilename;
			rangeLimitedfilename << outputFilename;
			if (maxRanges[i] != std::numeric_limits<double>::max())
				rangeLimitedfilename << "_maxRange_" << maxRanges[i];
			plotProcessedWindProfile(height, degrees, speed, rangeLimitedfilename.str(), maxRanges[i], progressReporter, parent);
		}
		return;
	}
	if (inputFilename.find("_ceilometer.csv") != std::string::npos)
	{
		std::fstream fin;
		fin.open(inputFilename.c_str(), std::ios::in | std::ios::binary);
		if (!fin.is_open())
			throw("Could not open file");

		progressReporter << "Reading file " << inputFilename << "\n";

		std::vector<CampbellCeilometerProfile> data;
		CampbellHeader firstHeader;
		const size_t displayInterval = 120;
		std::string firstBatchDate;
		std::string timeDate;
		while (!fin.eof())
		{
			bool badTimeDate = false;
			timeDate = "";
			char character;
			fin.read(&character, 1);
			//size_t counter = 1;
			while (character != ','  && !fin.eof())
			{
				timeDate = timeDate + character;
				fin.read(&character, 1);
			}
			//if (counter == 50)
			//{
			//	fin.close();
			//	throw("Failed to find the comma after the timestamp in a ceilometer file. Reading aborted.");
			//}
			if (fin.eof())
				break;
			size_t firstDash = timeDate.find_first_of('-');
			if (firstDash == std::string::npos || firstDash < 4)
				badTimeDate = true;
			else
			{
				size_t timeDateStart = firstDash - 4;
				timeDate = timeDate.substr(timeDateStart);
				if (timeDate.length() < 19)
					badTimeDate = true;
				else if (timeDate[4] != '-')
					badTimeDate = true;
				else if (timeDate[7] != '-')
					badTimeDate = true;
				else if (timeDate[10] != 'T')
					badTimeDate = true;
				else if (timeDate[13] != ':')
					badTimeDate = true;
				else if (timeDate[16] != ':')
					badTimeDate = true;
			}

			//unless we have a bad time/date we can read in the data
			if (!badTimeDate)
			{
				//I have found one instance where time went backwards in the second entry in
				//a file. I'm not really sure why, but the best thing to do in this case seems
				//to be to discard the first entry.

				//If we have more than one entry already then I'm not sure what to do. For now abort processing this file..
				if (data.size() == 1 && data[0].getTime() > getCeilometerTime(timeDate))
				{
					data.pop_back();
					progressReporter << "The second entry in the file has a timestamp earlier than the first. This may be due to a logging glitch. The first entry will be deleted.\n";
				}
				else if (data.size() > 1 && data[0].getTime() > getCeilometerTime(timeDate))
				{
					throw("Time jumps backwards in this file, it cannot be processed.");
				}
				CampbellHeader header;
				header.readHeader(fin);
				if (data.size() == 0)
					firstHeader = header;
				if (header.getMessageType() == cmt_cs && header.getMessageNumber() == 2)
				{
					if (data.size() == 0)
						progressReporter << "Found CS 002 messages: ";
					if (data.size() % displayInterval == 0)
						firstBatchDate = timeDate;
					if (data.size() % displayInterval == displayInterval - 1)
						progressReporter << firstBatchDate << "-" << timeDate << "(" << displayInterval << " profiles) ";
					CampbellMessage2 profile;
					profile.read(fin);
					data.push_back(CampbellCeilometerProfile(getCeilometerTime(timeDate), profile));
				}
				else
				{
					progressReporter << "Found " << header.getMessageNumber() << " message " << timeDate << ". Halting Read\n";
					break;
				}
			}
			else
				progressReporter << "Found a profile with an incorrectly formatted time/date. This profile will be ignored.\n";
		}
		if (data.size() % displayInterval != displayInterval - 1)
			progressReporter << firstBatchDate << "-" << timeDate << "(" << (data.size() % displayInterval)+1 << " profiles)\n";
		progressReporter << "Completed reading file. " << data.size() << " profiles found\n";
		fin.close();

		if (data.size() > 0)
		{
			for (size_t i = 0; i < maxRanges.size(); ++i)
				plotCeilometerProfiles(getCeilometerHeader(inputFilename, firstHeader, data[0]), data, outputFilename, maxRanges[i], progressReporter, parent);
		}

		return;
	}

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
				else if ( profiles.size() % 10 == 0)
					progressReporter << ", " << profiles.size() - 9 << "-" << profiles.size();
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


		for (size_t i = 0; i < maxRanges.size(); ++i)
		{
			std::ostringstream rangeLimitedfilename;
			rangeLimitedfilename << outputFilename;
			if (maxRanges[i] != std::numeric_limits<double>::max())
				rangeLimitedfilename << "_maxRange_" << maxRanges[i];
			plotProfiles(hplHeader, profiles, rangeLimitedfilename.str(), maxRanges[i], progressReporter, parent);
		}
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
	if (dir.length() == 0)
		return;
	m_inputDirectory = dir;
	(*m_progressReporter) << "Input directory changed to " << m_inputDirectory << "\n";
}

void mainFrame::OnSelectOutputDir(wxCommandEvent& event)
{
	if (m_plotting || !m_progressReporter->shouldStop())
	{
		wxMessageBox("Please stop processing before attempting to change a directory.");
		return;
	}
	std::string dir = wxDirSelector("Select the output directory to store plots.", m_outputDirectory, wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR);
	if (dir.length() == 0)
		return;
	m_outputDirectory = dir;
	(*m_progressReporter) << "Output directory changed to " << m_outputDirectory << "\n";
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
	m_checkForNewDataTimer->Start(600000);//Use this timer to check for new data every 5 mins
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
		//run the plot routines for the different type of profiles
		if (!m_progressReporter->shouldStop())
		plot("*_ceilometer.csv");
		if (!m_progressReporter->shouldStop())
			plot("*Processed_Wind_Profile_??_????????_??????.hpl");
		if(!m_progressReporter->shouldStop())
			plot("*Stare_??_????????_??.hpl");
		if (!m_progressReporter->shouldStop())
			plot("*VAD_??_????????_??????.hpl");
		if (!m_progressReporter->shouldStop())
			plot("*User*.hpl");

		//Tell the user we are done for now
		if (!m_progressReporter->shouldStop())
			(*m_progressReporter) << "Generated plots for all files found. Waiting approx 10 mins to check again.\n\n";
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


	//check for new files
	ExistedFolderChangesLister changesLister(m_inputDirectory, m_outputDirectory + "previouslyPlottedFiles.txt");
	std::vector<std::string> filesToPlot = changesLister.getChanges(filter);


	if (filesToPlot.size() == 0)
		(*m_progressReporter) << "Found no new files to plot.\n";
	else
	{
		(*m_progressReporter) << "Found the following new files to plot matching the filter " << filter << " :\n";
		for(size_t i=0; i<filesToPlot.size(); ++i)
			(*m_progressReporter) << "\t" << filesToPlot[i] << "\n";


		std::string lastFileToPlot = filesToPlot.back();

		for (size_t i = 0; i < filesToPlot.size(); ++i)
		{
			(*m_progressReporter) << "Processing " << filesToPlot[i] << "\n";
			std::string outputFile = m_outputDirectory + filesToPlot[i].substr(m_inputDirectory.length(), std::string::npos);
			try
			{
				plotFile(filesToPlot[i], outputFile, { std::numeric_limits<double>::max(), 2000.0, 1000.0 }, *m_progressReporter, this);
				
				if (m_progressReporter->shouldStop())
				{
					(*m_progressReporter) << "Operation halted at user request.\n";
					break;
				}
				
				//remember which files have been plotted
				if (filesToPlot[i] != lastFileToPlot)
				{
					changesLister.updateSnapshotFile(filesToPlot[i]);
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
	wxMessageBox(wxT("Lidar Quicklook Plotter Version 1.00.0"), wxT("About Lidar Quicklook Plotter..."));
}

mainFrame::~mainFrame()
{
}