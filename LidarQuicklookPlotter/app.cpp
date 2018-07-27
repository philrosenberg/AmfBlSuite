#include "app.h"
#include "mainFrame.h"

IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	wxString inputDirectory = "D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\doppler_lidar_backup_ds1\\Data\\Proc\\2018\\";
	wxString outputDirectory = "D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\quicklooks\\";
	mainFrame* frame = new mainFrame(0L, "Lidar quicklook plotter", inputDirectory, outputDirectory, true);
	frame->SetIcon(wxICON(amain));
	frame->Show();
	return true;
}
