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
#include"AmfNc.h"
#include<svector/serr.h>
#include<svector/time.h>

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

class ProcessFlagger
{
public:
	ProcessFlagger(bool *flag, bool setValue = true, bool unsetValue = false)
		:m_flag(flag), m_setValue(setValue), m_unsetValue(unsetValue)
	{
		*flag = setValue;
	}
	~ProcessFlagger()
	{
		*m_flag = m_unsetValue;
	}
private:
	bool * const m_flag;
	const bool m_setValue;
	const bool m_unsetValue;

};

mainFrame::mainFrame(wxFrame *frame, const wxString& title, const wxString &inputDirectory, const wxString &outputDirectory, bool runImmediately)
	: wxFrame(frame, -1, title)
{
	InstrumentInfo instrumentInfo = { sU("saturn_v"), sU("Rocket"), sU("NASA"), sU("Saturn"), sU("V"), sU("Amstrad"), sU("1512") };
	PersonInfo author = { sU("Neil"), sU("armstrong@nans.org"), sU("armsrong.orchid.org"), sU("NASA") };
	ProcessingSoftwareInfo processingsoftwareInfo = { sU("http://mycode.git"), sU("1001") };
	CalibrationInfo calibrationInfo = { sci::UtcTime::now(), sci::UtcTime::now(), sU("someurl.org")};
	DataInfo dataInfo = { 10.0, sU("s"), 5.0, sU("s"), 0, ft_timeSeriesPoint, 0.0, 0.0, 0, 360, sci::UtcTime::now(), sci::UtcTime::now(), sU("Another go"), true, sU("Moon data"), {} };
	ProjectInfo projectInfo{ sU("Apolo"), {sU("Nixon"), sU("nixon@thewhitehouse.org"), sU("none"), sU("US government")} };
	PlatformInfo platformInfo{ sU("Saturn"), pt_moving, dm_air, 1.0e6, {sU("Moon"), sU("Cape Canavral")} };
	sci::string comment = sU("Fingers crossed");
	/*try
	{
		OutputAmfNcFile test(sU(""), instrumentInfo, author, processingsoftwareInfo,
			calibrationInfo, dataInfo, projectInfo, platformInfo, comment);
	}
	catch (sci::err err)
	{
		wxMessageBox(err.getErrorMessage());
	}*/

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

	m_inputDirectory = sci::fromWxString(inputDirectory);
	m_outputDirectory = sci::fromWxString(outputDirectory);

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

void mainFrame::OnRun(wxCommandEvent& event)
{
	//sci::string ampUnit = sci::Amp<>::getShortRepresentation();
	//sci::string ampSqUnit = sci::Amp<2>::getShortRepresentation();
	//sci::string milliAmpSqUnit = sci::Amp<2, -3>::getShortRepresentation(sU("#u"), sU("#d"));
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
	sci::string dir = sci::fromWxString(wxDirSelector("Select the input directory to search for data files.",sci::nativeUnicode(m_inputDirectory), wxDD_DIR_MUST_EXIST|wxDD_CHANGE_DIR));
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
	sci::string dir = sci::fromWxString(wxDirSelector("Select the output directory to store plots.", sci::nativeUnicode(m_outputDirectory), wxDD_DIR_MUST_EXIST | wxDD_CHANGE_DIR));
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
	process();
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

void mainFrame::process()
{
	if (m_plotting)
		return;
	//this sets the plotting flag true. It will be automatically set
	//back to false on exit of this function when plottingFlagger
	//goes out of scope and is destroyed - even if this is due to an
	//unhandled exception.
	//We do this because we may yeild during this function to allow
	//user interaction and screen updates which may allow this function
	//to get called again when we are not ready for it.
	ProcessFlagger plottingFlagger(&m_plotting);

	process(sU("*_ceilometer.csv"), CeilometerProcessor());
	process(sU("*Processed_Wind_Profile_??_????????_??????.hpl"), InstrumentPlotter());
	process(sU("*Stare_??_????????_??.hpl"), InstrumentPlotter());
	process(sU("*VAD_??_????????_??????.hpl"), InstrumentPlotter());
	process(sU("*User*.hpl"), InstrumentPlotter());
	//Tell the user we are done for now
	if (!m_progressReporter->shouldStop())
		(*m_progressReporter) << "Generated plots for all files found. Waiting approx 10 mins to check again.\n\n";

	if (m_progressReporter->shouldStop())
		m_logText->AppendText("Stopped\n\n");
}

void mainFrame::process(const sci::string &filter, InstrumentPlotter &plotter)
{
	ExistedFolderChangesLister changesLister(m_inputDirectory, m_outputDirectory + sU("previouslyPlottedFiles.txt"));

	try
	{
		if (!m_progressReporter->shouldStop())
		{
			readDataAndPlot(checkForNewFiles(filter, changesLister), changesLister, plotter);
			if (!m_progressReporter->shouldStop())
				(*m_progressReporter) << "Generated plots for all files matching filter " << filter << "\n";
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
	catch (sci::string errMessage)
	{
		wxTextAttr originalStyle = m_logText->GetDefaultStyle();
		m_logText->SetDefaultStyle(wxTextAttr(*wxRED));
		std::ostream stream(m_logText);
		stream << sU("Error: ") << sci::nativeUnicode(errMessage) << sU("\n");
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
}

//Check for new files - the last file found alphabetically
//is assumed to be the last file chronologically (assuming you have a
//sensible naming structure) and it is assumed that this file may be
//incomplete, so will be reported next time just in case.
std::vector<sci::string> mainFrame::checkForNewFiles(const sci::string &filter, const ExistedFolderChangesLister &changesLister)
{
	//ensure that the input and output directories end with a slash or are empty
	if (m_inputDirectory.length() > 0 && m_inputDirectory.back()!= sU('/') && m_inputDirectory.back()!= sU('\\'))
		m_inputDirectory = m_inputDirectory+sU("/");
	if (m_outputDirectory.length() > 0 && m_outputDirectory.back() != sU('/') && m_outputDirectory.back() != sU('\\'))
		m_outputDirectory = m_outputDirectory + sU("/");

	std::vector<std::string> plottedFiles;
	(*m_progressReporter) << "Looking for data files to plot.\n";
	//check the output directory exists
	if (!wxDirExists(sci::nativeUnicode(m_outputDirectory)))
		wxFileName::Mkdir(sci::nativeUnicode(m_outputDirectory), 770, wxPATH_MKDIR_FULL);
	if (!wxDirExists(sci::nativeUnicode(m_outputDirectory)))
	{
		sci::ostringstream message;
		message << sU("The output directory ") << m_outputDirectory << sU(" does not exist and could not be created.");
		throw(message.str());
	}

	//check the input directory exists
	if(!wxDirExists(sci::nativeUnicode(m_inputDirectory)))
	{
		sci::ostringstream message;
		message << sU("The input directory ") << m_inputDirectory << sU(" does not exist.");
		throw(message.str());
	}


	//check for new files
	std::vector<sci::string> filesToPlot = changesLister.getChanges(filter);


	if (filesToPlot.size() == 0)
		(*m_progressReporter) << sU("Found no new files to plot.\n");
	else
	{
		(*m_progressReporter) << sU("Found the following new files to plot matching the filter ") << filter << sU(" :\n");
		for(size_t i=0; i<filesToPlot.size(); ++i)
			(*m_progressReporter) << sU("\t") << filesToPlot[i] << sU("\n");
	}

	return filesToPlot;
}

void mainFrame::readDataAndPlot(std::vector<sci::string> &filesToPlot, const ExistedFolderChangesLister &changesLister, InstrumentPlotter &plotter)
{
	sci::string lastFileToPlot = filesToPlot.back();

	for (size_t i = 0; i < filesToPlot.size(); ++i)
	{
		(*m_progressReporter) << sU("Processing ") << filesToPlot[i] << sU("\n");
		sci::string outputFile = m_outputDirectory + filesToPlot[i].substr(m_inputDirectory.length(), sci::string::npos);
		try
		{
			plotter.readData(filesToPlot[i], *m_progressReporter, this);

			if (m_progressReporter->shouldStop())
			{
				(*m_progressReporter) << sU("Operation halted at user request.\n");
				break;
			}

			if (plotter.hasData())
				plotter.plotData(outputFile, { std::numeric_limits<double>::max(), 2000.0, 1000.0 }, *m_progressReporter, this);

			if (m_progressReporter->shouldStop())
			{
				(*m_progressReporter) << sU("Operation halted at user request.\n");
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
}

void mainFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox(wxT("Lidar Quicklook Plotter Version 1.00.0"), wxT("About Lidar Quicklook Plotter..."));
}

mainFrame::~mainFrame()
{
}