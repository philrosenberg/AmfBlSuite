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
#include "Setup.h"
#include <wx/evtloop.h>

const int mainFrame::ID_FILE_EXIT = ::wxNewId();
const int mainFrame::ID_FILE_RUN = ::wxNewId();
const int mainFrame::ID_FILE_STOP = ::wxNewId();
const int mainFrame::ID_FILE_SELECT_SETUP_FILE = ::wxNewId();
const int mainFrame::ID_HELP_ABOUT = ::wxNewId();
const int mainFrame::ID_CHECK_DATA_TIMER = ::wxNewId();
const int mainFrame::ID_INSTANT_CHECK_DATA_TIMER = ::wxNewId();

BEGIN_EVENT_TABLE(mainFrame, wxFrame)
EVT_MENU(ID_FILE_EXIT, mainFrame::OnExit)
EVT_MENU(ID_FILE_RUN, mainFrame::OnRun)
EVT_MENU(ID_FILE_STOP, mainFrame::OnStop)
EVT_MENU(ID_FILE_SELECT_SETUP_FILE, mainFrame::OnSelectSetupFile)
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

mainFrame::mainFrame(wxFrame *frame, const wxString& title, const wxString &settingsFile)
	: wxFrame(frame, -1, title)
{
	setSetupFile(sci::fromWxString(settingsFile));

	m_processingSoftwareInfo.url = sU("https://github.com/philrosenberg/AmfBlSuite.git");
	m_processingSoftwareInfo.version = sU("1.1.20");

	wxMenuBar* mbar = new wxMenuBar();
	wxMenu* fileMenu = new wxMenu(wxT(""));
	fileMenu->Append(ID_FILE_EXIT, wxT("E&xit\tAlt+F4"), wxT("Exit the application"));
	fileMenu->Append(ID_FILE_RUN, wxT("Run\tCtrl+R"), wxT("Run Code"));
	fileMenu->Append(ID_FILE_STOP, wxT("Stop\tCtrl+X"), wxT("Stop Code"));
	fileMenu->Append(ID_FILE_SELECT_SETUP_FILE, wxT("Select Setup File"), wxT("Select xml file containing the processing parameters"));
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

	m_progressReporter.reset(new TextCtrlProgressReporter<std::ostream>(m_logText, true, this, &sci::nulloutch));
	m_progressReporter->setShouldStop(true);

	try
	{
		if (m_setupFileName.length() > 0)
			setupProcessingOptionsOnly(m_setupFileName, m_processingOptions);
	}
	catch (...)
	{
	}

	if (m_processingOptions.startImmediately)
		start();
	else
	{
		m_logText->SetValue("To run AMF Boundary Layer Suite either run with command line arguments - which "
			"provides a settings file, or modify the default settings file \"processingSettings.xml\". If"
			" the /processingSettings/runImmediately element of this file is set to true, then the processing "
			"will begin running straight away. Otherwise, the software will wait until you hit Run on the "
			"File menu.\n\n");
		if (wxTheApp->argc != 1 && wxTheApp->argc != 2)
		{
			wxTextAttr originalStyle = m_logText->GetDefaultStyle();
			m_logText->SetDefaultStyle(wxTextAttr(*wxRED));
			m_logText->AppendText("You ran Lidar Quicklook Plotter with the wrong number of arguments. Check above for "
				"usage instructions..\n\n");
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
	start();
}

void mainFrame::OnStop(wxCommandEvent& event)
{
	stop();
}

void mainFrame::OnSelectSetupFile(wxCommandEvent& event)
{
	if (m_plotting)
	{
		wxMessageBox("Please stop processing before attempting to change the setup file.");
		return;
	}
	wxString defaultDir = wxGetCwd();
	wxString defaultFileName = wxEmptyString;
	if (m_setupFileName.length() > 0)
	{
		wxFileName currentFile(sci::nativeUnicode(m_setupFileName));
		defaultDir = currentFile.GetPath();
		defaultFileName = currentFile.GetFullName();
	}
	sci::string filename = sci::fromWxString(wxFileSelector("Select the xml setup file.", defaultDir, defaultFileName, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_FILE_MUST_EXIST | wxFD_OPEN));
	if (filename.length() == 0)
		return;
	setSetupFile(filename);
	(*m_progressReporter) << sU("Setup file changed to ") << m_setupFileName << sU("\n");
}

void mainFrame::setSetupFile(sci::string setupFileName)
{
	m_setupFileName = setupFileName;
	m_isSetup = false;
}

void mainFrame::readSetupFile()
{
	(*m_progressReporter) << "Reading setup information. For a moving platform this can take some time while the position data is read.\n";
	try
	{
		setup(m_setupFileName, sU("info.xml"), *m_progressReporter, m_processingOptions, m_author, m_projectInfo, m_platform, m_instrumentProcessors);
		if (m_progressReporter->shouldStop())
			(*m_progressReporter) << sU("Reading setup data halted at the users request. Processing will stop.\n\n");
		m_isSetup = true;
	}
	catch (sci::err err)
	{
		ErrorSetter setter(m_progressReporter.get());
		(*m_progressReporter) << err.getErrorCategory() << ":" << err.getErrorCode() << " " << err.getErrorMessage() << "\n";
	}
	catch (std::exception err)
	{
		ErrorSetter setter(m_progressReporter.get());
		(*m_progressReporter) << err.what() << "\n";
	}
	if (m_isSetup)
		(*m_progressReporter) << sU("Setup data read successfully\n\n");
	else
		(*m_progressReporter) << sU("Reading setup data failed. Correct the setup file or info.xml file, or select a different setup file, then rerun. \n\n");
}

void mainFrame::start()
{
	if (m_progressReporter->shouldStop() && m_plotting)
	{
		//The user has requested to stop but before the stop has happened they requested to start again
		//Tell them to be patient
		wxMessageBox("Although you have requested processing and plotting to stop, the software is just waiting for code to finish execution before it can do so. Please wait for the current processing to halt before trying to restart.");
		return;
	}
	if (m_plotting)
	{
		wxMessageBox("The code is already processing or plotting data. If you wish to restart, then select Stop to stop the current operation, then select Start to restart processing.");
		return;
	}
	m_progressReporter->setShouldStop(false);
	m_logText->AppendText("Starting\n\n");
	m_checkForNewDataTimer->Start(m_processingOptions.waitTime.value<sci::Second<1, -3>>());//Use this timer to check for new data as needed
	m_instantCheckTimer->StartOnce(1);//Use this timer to check for new data now

}

void mainFrame::stop()
{
	if (m_progressReporter->shouldStop())
		return;
	m_checkForNewDataTimer->Stop();
	m_progressReporter->setShouldStop(true);
	if (m_plotting)
		m_logText->AppendText("Stopping...\n");
	else
		m_logText->AppendText("Stopped\n\n");
	if (m_processingOptions.closeOnCompletion)
		this->Close();
}

void mainFrame::OnCheckDataTimer(wxTimerEvent& event)
{
	try
	{
		//if this started during a yield then trigger a timer to try again after a second
		wxEventLoopBase* eventLoop = wxEventLoopBase::GetActive();

		if (eventLoop && eventLoop->IsYielding())
		{
			m_instantCheckTimer->StartOnce(1000);
			return;
		}
		if (m_plotting || m_progressReporter->shouldStop())
			return;
		process();


		//Tell the user we are done for now
		if (m_progressReporter->shouldStop())
			m_logText->AppendText("Stopped\n\n");
		else if (m_processingOptions.checkForNewFiles)
			(*m_progressReporter) << sU("Processed all files found. New data will be checked for every ") << m_processingOptions.waitTime << sU(".\n\n");
		else
		{
			(*m_progressReporter) << sU("Processed all files found. Processing complete.\n\n");
			stop();
		}
	}
	catch (sci::err err)
	{
		ErrorSetter setter(m_progressReporter.get());
		(*m_progressReporter) << err.getErrorCategory() << ":" << err.getErrorCode() << " " << err.getErrorMessage() << "\n";
	}
	catch (std::exception err)
	{
		ErrorSetter setter(m_progressReporter.get());
		(*m_progressReporter) << err.what() << "\n";
	}
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

	//check if we have the setup data - if not then read it
	if (!m_isSetup)
		readSetupFile();
	//if this did not work for some reason then return
	if (!m_isSetup)
		return;

	//set the setup file - it will automatically be unset once this object goes out of scope
	std::fstream logOut;
	std::fstream* logOutPtr = nullptr;
	if (m_processingOptions.logFileName.length() > 0)
	{
		logOut.open(sci::nativeUnicode(m_processingOptions.logFileName), std::ios::app);
		bool open = logOut.is_open();
		wxString logDirectory = wxPathOnly(sci::nativeUnicode(m_processingOptions.logFileName));
		if(!wxDirExists(logDirectory))
			wxMkDir(logDirectory);
		sci::assertThrow(logOut.is_open(), sci::err(sci::SERR_USER, 0, sU("Could not open log file ")+ m_processingOptions.logFileName));
		logOutPtr = &logOut;
		(*m_progressReporter) << sU("Log file set to ") << m_processingOptions.logFileName << sU("\n");
		(*m_progressReporter) << sU("Search directory set to ") << m_processingOptions.inputDirectory << sU("\n");
		(*m_progressReporter) << sU("Time range set to ") << sci::fromCodepage(m_processingOptions.startTime.getIso8601String()) << " - "
			<< sci::fromCodepage(m_processingOptions.endTime.getIso8601String()) << sU("\n");
		if(m_processingOptions.onlyProcessNewFiles)
			(*m_progressReporter) << sU("Only previously unprocessed files will be processed\n");
		else
			(*m_progressReporter) << sU("Previously processed files will be reprocessed\n");

		(*m_progressReporter) << sU("Beginning processing at") << sci::fromCodepage(sci::UtcTime::now().getIso8601String()) << sU("\n\n");
	}
	ProgressReporterStreamSetter<std::ostream> logFileSetter(m_progressReporter.get(), logOutPtr);

	for (size_t i = 0; i < m_instrumentProcessors.size(); ++i)
		process(*m_instrumentProcessors[i]);


}

void mainFrame::process(InstrumentProcessor &processor)
{
	try
	{
		(*m_progressReporter) << sU("Starting ") << processor.getName() <<  sU(".\n\n");
		//Check that the input/output diectories are actually there
		checkDirectoryStructue();

		//These list changes that have occured since the last time its method
		//updateSnapshotFile() was called. 
		std::unique_ptr<FolderChangesLister> plotChangesLister;
		std::unique_ptr<FolderChangesLister> ncChangesLister;

		if (m_processingOptions.onlyProcessNewFiles)
		{
			//This class in particular assumes that when
			//it performs a search of previously existing files, the last one alphabetically
			//will have changed, but the rest will not.
			plotChangesLister.reset(new FolderChangesLister(m_processingOptions.inputDirectory, m_processingOptions.outputDirectory + sU("previouslyPlottedFilesByTime.txt")));
			ncChangesLister.reset(new FolderChangesLister(m_processingOptions.inputDirectory, m_processingOptions.outputDirectory + sU("previouslyProcessedFilesByTime.txt")));
		}
		else
		{
			//This class just assumes all files have changed so we process all files even if they existed previously and are unmodified
			plotChangesLister.reset(new AssumeAllChangedChangesLister(m_processingOptions.inputDirectory, m_processingOptions.outputDirectory + sU("previouslyPlottedFilesByTime.txt")));
			ncChangesLister.reset(new AssumeAllChangedChangesLister(m_processingOptions.inputDirectory, m_processingOptions.outputDirectory + sU("previouslyProcessedFilesByTime.txt")));
		}

		if (!m_progressReporter->shouldStop())
		{
			readDataThenPlotThenNc(*(plotChangesLister.get()), *(ncChangesLister.get()), m_author, m_processingSoftwareInfo, m_projectInfo, *m_platform, m_processingOptions, processor);
			if (!m_progressReporter->shouldStop())
				(*m_progressReporter) << sU("Processed all files for the current processor\n\n\n");
		}
	}
	catch (sci::err err)
	{
		ErrorSetter setter(m_progressReporter.get());
		(*m_progressReporter) << err.getErrorCategory() << ":" << err.getErrorCode() << " " << err.getErrorMessage() << "\n";
	}
	catch (std::exception err)
	{
		ErrorSetter setter(m_progressReporter.get());
		(*m_progressReporter) << err.what() << "\n";
	}
}

//Check that the directory names are correctly formatted (with trailing slash or blank) and
//that all input and output directories actually exist
void mainFrame::checkDirectoryStructue()
{
	//ensure that the input and output directories end with a slash or are empty
	if (m_processingOptions.inputDirectory.length() > 0 && m_processingOptions.inputDirectory.back() != sU('/') && m_processingOptions.inputDirectory.back() != sU('\\'))
		m_processingOptions.inputDirectory = m_processingOptions.inputDirectory + sU("/");
	if (m_processingOptions.outputDirectory.length() > 0 && m_processingOptions.outputDirectory.back() != sU('/') && m_processingOptions.outputDirectory.back() != sU('\\'))
		m_processingOptions.outputDirectory = m_processingOptions.outputDirectory + sU("/");

	//check the output directory exists
	if (!wxDirExists(sci::nativeUnicode(m_processingOptions.outputDirectory)))
		wxFileName::Mkdir(sci::nativeUnicode(m_processingOptions.outputDirectory), 770, wxPATH_MKDIR_FULL);
	if (!wxDirExists(sci::nativeUnicode(m_processingOptions.outputDirectory)))
	{
		sci::ostringstream message;
		message << sU("The output directory ") << m_processingOptions.outputDirectory << sU(" does not exist and could not be created.");
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, message.str()));
	}

	//check the input directory exists
	if (!wxDirExists(sci::nativeUnicode(m_processingOptions.inputDirectory)))
	{
		sci::ostringstream message;
		message << sU("The input directory ") << m_processingOptions.inputDirectory << sU(" does not exist.");
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, message.str()));
	}
}

void mainFrame::readDataThenPlotThenNc(const FolderChangesLister &plotChangesLister, const FolderChangesLister &ncChangesLister,
	const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, InstrumentProcessor &processor)
{
	if (processingOptions.generateQuicklooks)
	{
		//check for new files
		(*m_progressReporter) << sU("Looking for data files to plot.\n");
		sci::UtcTime checkedForChangesTime = sci::UtcTime::now();
		std::vector<sci::string> newPlotFiles = plotChangesLister.getChanges(processor, m_processingOptions.startTime, m_processingOptions.endTime);

		//Keep the user updated
		if (newPlotFiles.size() == 0)
		{
			(*m_progressReporter) << sU("Found no new files to process for the current processor\n");
			return;
		}
		else
		{
			(*m_progressReporter) << sU("Found the following new files to for the  current processor:\n");
			for (size_t i = 0; i < newPlotFiles.size(); ++i)
				(*m_progressReporter) << sU("\t") << newPlotFiles[i] << sU("\n");
		}

		//plot the quicklooks
		for (size_t i = 0; i < newPlotFiles.size(); ++i)
		{
			(*m_progressReporter) << sU("Plotting ") << newPlotFiles[i] << sU("\n");
			sci::string outputFile = m_processingOptions.outputDirectory + newPlotFiles[i].substr(m_processingOptions.inputDirectory.length(), sci::string::npos);
			try
			{
				//readData takes an array of files that will all be processed and plotted together
				//and put in a single netcdf. To process just one file we make an array with just one filename
				processor.readData({ newPlotFiles[i] }, platform, *m_progressReporter);

				if (m_progressReporter->shouldStop())
				{
					(*m_progressReporter) << sU("Operation halted at user request.\n");
					break;
				}

				if (processor.hasData())
				{
					processor.plotData(outputFile, { std::numeric_limits<metreF>::max(), metreF(2000.0), metreF(1000.0) }, *m_progressReporter, this);

					if (m_progressReporter->shouldStop())
					{
						(*m_progressReporter) << sU("Operation halted at user request.\n");
						break;
					}

					//remember which files have been plotted
					plotChangesLister.updateSnapshotFile(newPlotFiles[i], checkedForChangesTime);
				}
			}
			catch (sci::err err)
			{
				ErrorSetter setter(m_progressReporter.get());
				(*m_progressReporter) << err.getErrorCategory() << ":" << err.getErrorCode() << " " << err.getErrorMessage() << "\n";
			}
			catch (std::exception err)
			{
				ErrorSetter setter(m_progressReporter.get());
				(*m_progressReporter) << err.what() << "\n";
			}

			if (m_progressReporter->shouldStop())
			{
				(*m_progressReporter) << sU("Operation halted at user request.\n");
				break;
			}
		}
	}

	//generate the netcdfs
	if (processingOptions.generateNetCdf)
	{
		//check for new files
		(*m_progressReporter) << sU("Looking for data files to process into netcdf format.\n");
		//std::vector<sci::string> newNcFiles = ncChangesLister.getChanges(processor, m_processingOptions.startTime, m_processingOptions.endTime);
		//std::vector<sci::string> allFiles = ncChangesLister.listFolderContents(processor, m_processingOptions.startTime, m_processingOptions.endTime);
		sci::UtcTime checkedForChangesTime = sci::UtcTime::now();
		std::vector<std::vector<sci::string>> dayFileSets = ncChangesLister.getChangesSeparatedByOutput(processor, m_processingOptions.startTime, m_processingOptions.endTime);
		for (size_t i = 0; i < dayFileSets.size(); ++i)
		{
			(*m_progressReporter) << sU("Day ") << i + 1 << sU(": Found the following files:\n");
			for (size_t j = 0; j < dayFileSets[i].size(); ++j)
				(*m_progressReporter) << dayFileSets[i][j] << sU("\n");
			(*m_progressReporter) << sU("\nReading...\n\n");
			try
			{
				processor.readData(dayFileSets[i], platform, *m_progressReporter);

				if (m_progressReporter->shouldStop())
				{
					(*m_progressReporter) << sU("Operation halted at user request.\n");
					break;
				}

				if (processor.hasData())
				{
					(*m_progressReporter) << sU("read one day of data - writing to netcdf\n\n");
					processor.writeToNc(m_processingOptions.outputDirectory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, *m_progressReporter);

					if (m_progressReporter->shouldStop())
					{
						(*m_progressReporter) << sU("Operation halted at user request.\n");
						break;
					}
				}
				else
				{
					WarningSetter setter(m_progressReporter.get());
					(*m_progressReporter) << sU("No valid data found for this day, not netcdf will be written\n\n");
				}

				//whether the data files contained data or not, remember which files have been processed
				for (size_t j = 0; j < dayFileSets[i].size(); ++j)
					ncChangesLister.updateSnapshotFile(dayFileSets[i][j], checkedForChangesTime);
			}
			catch (sci::err err)
			{
				ErrorSetter setter(m_progressReporter.get());
				(*m_progressReporter) << err.getErrorCategory() << ":" << err.getErrorCode() << " " << err.getErrorMessage() << "\n";
			}
			catch (std::exception err)
			{
				ErrorSetter setter(m_progressReporter.get());
				(*m_progressReporter) << err.what() << "\n";
			}

			if (m_progressReporter->shouldStop())
			{
				(*m_progressReporter) << sU("Operation halted at user request.\n");
				break;
			}
		}
	}
}

void mainFrame::OnAbout(wxCommandEvent& event)
{
	
	wxMessageBox(sci::nativeUnicode(sU("Lidar Quicklook Plotter Version ") + m_processingSoftwareInfo.version + sU("\nAvailable from ") + m_processingSoftwareInfo.url), "About Lidar Quicklook Plotter...");
}

mainFrame::~mainFrame()
{
}