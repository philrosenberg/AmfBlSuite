#ifndef APEX_MAINFRAME_H
#define APEX_MAINFRAME_H

#include "app.h"
#include<svector/svector.h>
#include<svector/splot.h>
#include<svector/sreadwrite.h>
#include<svector/sstring.h>
#include<faam/faam.h>

class mainFrame : public wxFrame
{
public:
	static const int ID_FILE_EXIT;
	static const int ID_FILE_RUN;
	static const int ID_FILE_STOP;
	static const int ID_HELP_ABOUT;
	static const int ID_CHECK_DATA_TIMER;

	mainFrame(wxFrame *frame, const wxString& title);
	~mainFrame();
private:
	void OnExit(wxCommandEvent& event);
	void OnRun(wxCommandEvent& event);
	void OnStop(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnCheckDataTimer(wxTimerEvent& event);

	wxTextCtrl *m_logText;
	wxTimer *m_checkForNewDataTimer;

	std::string m_inputDirectory;
	std::string m_outputDirectory;

	bool m_started;
	bool m_plotting;
	bool m_shouldStop;

	void start();
	void stop();
	void plot();

	DECLARE_EVENT_TABLE();
};
#endif // APEX_MAINFRAME_H
