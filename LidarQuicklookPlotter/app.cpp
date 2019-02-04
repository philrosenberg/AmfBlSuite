#include "app.h"
#include "mainFrame.h"

IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	wxString inputDirectory;
	wxString outputDirectory;
#ifdef _DEBUG
	//inputDirectory = "D:\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\doppler_lidar_backup_ds1\\Data\\Proc\\2018\\";
	inputDirectory = "C:\\Users\\pdros\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\doppler_lidar_backup_ds1\\Data\\Proc\\2018\\201807\\20180711\\";
	outputDirectory = "C:\\Users\\pdros\\OneDrive\\Documents\\Work\\Leeds\\MOCCHA\\Mob data\\nctest\\doppler_lidar_backup_ds1\\Data\\Proc\\2018\\201807\\20180711\\";
	outputDirectory.utf8_str();
#endif
	if (argc == 3)
	{
		inputDirectory = wxString(argv[1]);
		outputDirectory = wxString(argv[2]);
	}

	mainFrame* frame = new mainFrame(0L, "Lidar quicklook plotter", inputDirectory, outputDirectory, inputDirectory.length()>0 && outputDirectory.length()>0);
	frame->SetIcon(wxICON(amain));
	frame->Show();
	return true;
}
