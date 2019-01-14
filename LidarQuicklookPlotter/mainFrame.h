#ifndef APEX_MAINFRAME_H
#define APEX_MAINFRAME_H

#include "app.h"
#include<svector/svector.h>
#include<svector/splot.h>
#include<svector/sreadwrite.h>
#include<svector/sstring.h>
#include"TextCtrlProgressReporter.h"
#include<memory>

//predeclaration of the class that records status of files when looking for new ones.
//we need to predeclare it because we have a reference to it in the method declarations
//below.
class ExistedFolderChangesLister;
class InstrumentPlotter;

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

	std::string m_inputDirectory;
	std::string m_outputDirectory;

	bool m_plotting;
	std::unique_ptr<TextCtrlProgressReporter> m_progressReporter;

	void start();
	void stop();
	void process();
	void process(const std::string &filter, InstrumentPlotter &plotter);
	void readDataAndPlot(std::vector<std::string> &filesToPlot, const ExistedFolderChangesLister &changesLister, InstrumentPlotter &plotter);
	std::vector<std::string> checkForNewFiles(const std::string &filter, const ExistedFolderChangesLister &changesLister);

	DECLARE_EVENT_TABLE();
};
#endif // APEX_MAINFRAME_H
