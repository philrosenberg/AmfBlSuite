#include "app.h"
#include "mainFrame.h"

IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	mainFrame* frame = new mainFrame(0L, wxT("$projectname$"));
	frame->SetIcon(wxICON(amain));
	frame->Show();
	return true;
}
