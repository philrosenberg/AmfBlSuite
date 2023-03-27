#include "app.h"
#include "mainFrame.h"

IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	wxString settingsFile = "processingSettingsExample.xml";
	if (argc == 2)
	{
		settingsFile = wxString(argv[1]);
	}

	mainFrame* frame = new mainFrame(0L, "Lidar quicklook plotter", settingsFile);
	frame->SetIcon(wxICON(amain));
	frame->Show();
	return true;
}
