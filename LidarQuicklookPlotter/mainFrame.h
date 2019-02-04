#ifndef APEX_MAINFRAME_H
#define APEX_MAINFRAME_H

#include "app.h"
#include<svector/svector.h>
#include<svector/splot.h>
#include<svector/sreadwrite.h>
#include<svector/sstring.h>
#include"TextCtrlProgressReporter.h"
#include<memory>
#include"AmfNc.h"

//predeclaration of the class that records status of files when looking for new ones.
//we need to predeclare it because we have a reference to it in the method declarations
//below.
class FolderChangesLister;
class InstrumentProcessor;

class mainFrame : public wxFrame
{
public:
	static const int ID_FILE_EXIT;
	static const int ID_FILE_RUN;
	static const int ID_FILE_STOP;
	static const int ID_FILE_SELECT_INPUT_DIR;
	static const int ID_FILE_SELECT_OUTPUT_DIR;
	static const int ID_HELP_ABOUT;
	static const int ID_CHECK_DATA_TIMER;
	static const int ID_INSTANT_CHECK_DATA_TIMER;

	mainFrame(wxFrame *frame, const wxString& title, const wxString &inputDirectory, const wxString &outputDirectory, bool runImmediately);
	~mainFrame();
private:
	void OnExit(wxCommandEvent& event);
	void OnRun(wxCommandEvent& event);
	void OnStop(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnSelectInputDir(wxCommandEvent& event);
	void OnSelectOutputDir(wxCommandEvent& event);
	void OnCheckDataTimer(wxTimerEvent& event);

	wxTextCtrl *m_logText;
	wxTimer *m_checkForNewDataTimer;
	wxTimer *m_instantCheckTimer;

	sci::string m_inputDirectory;
	sci::string m_outputDirectory;

	bool m_plotting;
	std::unique_ptr<TextCtrlProgressReporter> m_progressReporter;

	PersonInfo m_author;
	ProcessingSoftwareInfo m_processingSoftwareInfo;
	ProjectInfo m_projectInfo;
	PlatformInfo m_platformInfo;
	sci::string m_comment;

	void start();
	void stop();
	void process();
	void process(const sci::string &filter, InstrumentProcessor &processor);
	void readDataThenPlotThenNc(std::vector<sci::string> &filesToPlot, const FolderChangesLister &changesLister,
		const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, InstrumentProcessor &processor);
	std::vector<sci::string> checkForNewFiles(const sci::string &filter, const FolderChangesLister &changesLister);

	DECLARE_EVENT_TABLE();
};
#endif // APEX_MAINFRAME_H
