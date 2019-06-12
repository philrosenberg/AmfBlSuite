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
	static const int ID_FILE_SELECT_SETUP_FILE;
	static const int ID_HELP_ABOUT;
	static const int ID_CHECK_DATA_TIMER;
	static const int ID_INSTANT_CHECK_DATA_TIMER;

	mainFrame(wxFrame *frame, const wxString& title, const wxString &settingsFile);
	~mainFrame();
private:
	void OnExit(wxCommandEvent& event);
	void OnRun(wxCommandEvent& event);
	void OnStop(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnSelectSetupFile(wxCommandEvent& event);
	void OnCheckDataTimer(wxTimerEvent& event);

	wxTextCtrl *m_logText;
	wxTimer *m_checkForNewDataTimer;
	wxTimer *m_instantCheckTimer;

	bool m_plotting;
	std::unique_ptr<TextCtrlProgressReporter> m_progressReporter;

	bool m_isSetup;
	sci::string m_setupFileName;
	PersonInfo m_author;
	ProcessingSoftwareInfo m_processingSoftwareInfo;
	ProjectInfo m_projectInfo;
	std::shared_ptr<Platform> m_platform;
	std::vector<std::shared_ptr<InstrumentProcessor>> m_instrumentProcessors;
	ProcessingOptions m_processingOptions;
	//int m_processingLevel;

	void setSetupFile(sci::string setupFileName);
	void readSetupFile();
	void start();
	void stop();
	void process();
	void process(InstrumentProcessor &processor);
	void readDataThenPlotThenNc(const FolderChangesLister &plotChangesLister, const FolderChangesLister &ncChangesLister,
		const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, InstrumentProcessor &processor);
	void checkDirectoryStructue();

	DECLARE_EVENT_TABLE();
};
#endif // APEX_MAINFRAME_H
