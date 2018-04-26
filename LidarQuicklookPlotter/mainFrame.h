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
	static const int ID_HELP_ABOUT;

	mainFrame(wxFrame *frame, const wxString& title);
	~mainFrame();
private:
	void OnExit(wxCommandEvent& event);
	void OnRun(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	DECLARE_EVENT_TABLE();
};
#endif // APEX_MAINFRAME_H
