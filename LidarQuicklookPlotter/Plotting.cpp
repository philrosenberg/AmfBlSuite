#include"plotting.h"
#include<svector\splot.h>
#include"HplHeader.h"
#include"HplProfile.h"
#include<fstream>
#include<wx/filename.h>
#include"ProgressReporter.h"
#include"Ceilometer.h"
#include<wx/dir.h>
#include<wx/regex.h>



WindowCleaner::WindowCleaner(wxWindow *window)
	: m_window(window)
{}
WindowCleaner::~WindowCleaner()
{
	m_window->Destroy();
}



sci::string getScanTypeString (const HplHeader &header)
{
	if (header.scanType == st_stare)
		return sU("Stare");
	if (header.scanType == st_rhi)
		return sU("RHI");
	if (header.scanType == st_vad)
		return sU("VAD");
	if (header.scanType == st_wind)
		return sU("Wind Profile");
	if (header.scanType == st_user1)
		return sU("User Scan 1");
	if (header.scanType == st_user2)
		return sU("User Scan 2");
	if (header.scanType == st_user3)
		return sU("User Scan 3");
	if (header.scanType == st_user4)
		return sU("User Scan 4");

	return sU("Unknown Scan Type");
}


void createDirectoryAndWritePlot(splotframe *plotCanvas, sci::string filename, size_t width, size_t height, ProgressReporter &progressReporter)
{
	size_t lastSlashPosition = filename.find_last_of(sU("/\\"));
	sci::string directory;
	if (lastSlashPosition == sci::string::npos)
		directory = sU(".");
	else
		directory = filename.substr(0, lastSlashPosition + 1);
	
	if (!wxDirExists(sci::nativeUnicode(directory)))
		wxFileName::Mkdir(sci::nativeUnicode(directory), 770, wxPATH_MKDIR_FULL);
	if (!wxDirExists(sci::nativeUnicode(directory)))
	{
		sci::ostringstream message;
		message << sU("The output directory ") << directory << sU("does not exist and could not be created.");
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, message.str()));
	}

	sci::string lastFourChars = filename.length() < 4 ? sU("") : filename.substr(filename.length() - 4, sci::string::npos);
	if (lastFourChars != sU(".png") && lastFourChars != sU(".PNG"))
		filename = filename + sU(".png");

	progressReporter << sU("Rendering plot to file - the application may be unresponsive for a minute.\n");
	if (progressReporter.shouldStop())
	{
		return;
	}
	if( !plotCanvas->writetofile(filename, width, height, 1.0))
	{
		sci::ostringstream message;
		message << sU("The output file ") << filename << sU(" could not be created.");
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, message.str()));
	}
	progressReporter << sU("Plot rendered to ") << filename << sU("\n");
}

bool InstrumentProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime, size_t dateStartCharacter, size_t hourStartCharacter, size_t minuteStartCharacter, size_t secondStartCharacter, second fileDuration)
{
	sci::string fileNameToSplit = sci::fromWxString(wxFileName(sci::nativeUnicode(fileName)).GetFullName());
	int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour = 0;
	unsigned int minute = 0;
	double second = 0;
	sci::istringstream stream;
	stream.str(fileNameToSplit.substr(dateStartCharacter, 4));
	stream >> year;
	stream.clear();
	stream.str(fileNameToSplit.substr(dateStartCharacter + 4, 2));
	stream >> month;
	stream.clear();
	stream.str(fileNameToSplit.substr(dateStartCharacter+6, 2));
	stream >> day;
	if (hourStartCharacter != size_t(-1))
	{
		stream.clear();
		stream.str(fileNameToSplit.substr(hourStartCharacter, 2));
		stream >> hour;
	}
	if (minuteStartCharacter != size_t(-1))
	{
		stream.clear();
		stream.str(fileNameToSplit.substr(minuteStartCharacter, 2));
		stream >> minute;
	}
	if (secondStartCharacter != size_t(-1))
	{
		stream.clear();
		stream.str(fileNameToSplit.substr(secondStartCharacter, 2));
		stream >> second;
	}
	sci::UtcTime fileStartTime(year, month, day, hour, minute, second);
	return fileStartTime < endTime && (fileStartTime + fileDuration.value<::second>()) >= startTime;
}

std::vector<std::vector<sci::string>> InstrumentProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles, size_t dateStartCharacter)
{
	//Here we grab the date from each file based on it starting at the given character in the filename (path removed)
	//and assuming it is formatted yyyymmdd and use this to detemine which files belong to which day

	std::vector<std::vector<sci::string>> result;
	std::vector<sci::string> unassignedNewFiles = newFiles;

	//extract the dates from all the existing files
	std::vector<sci::string> datesAllFiles(allFiles.size());
	for (size_t i = 0; i < datesAllFiles.size(); ++i)
	{
		wxFileName fileNameToSplit(sci::nativeUnicode(allFiles[i]));
		datesAllFiles[i] = sci::fromWxString(fileNameToSplit.GetFullName()).substr(dateStartCharacter, 8);
	}
	std::vector<sci::string> datesNewFiles(newFiles.size());
	for (size_t i = 0; i < datesNewFiles.size(); ++i)
	{
		wxFileName fileNameToSplit(sci::nativeUnicode(newFiles[i]));
		datesNewFiles[i] = sci::fromWxString(fileNameToSplit.GetFullName()).substr(dateStartCharacter, 8);
	}


	for(size_t i=0; i<datesNewFiles.size(); ++i)
	{
		//check we have a valid date - we blank out duplicate dates.
		if (datesNewFiles[i] != sU(""))
		{
			//expand our result
			result.resize(result.size() + 1);

			//go through all the files and grab those that have a date the same as the current
			//unassigned new file
			for (size_t j = 0; j < allFiles.size(); ++j)
			{
				if (datesAllFiles[j] == datesNewFiles[i])
				{
					result.back().push_back(allFiles[j]);
				}
			}

			//now go through all the remaining unassigned new files and blank out the date if it matches
			//Note we strt at i+1 otherwise we overwrite the date we just used and then no other dates
			//will match it
			for (size_t j = i+1; j < datesNewFiles.size(); ++j)
			{
				if (datesNewFiles[j] == datesNewFiles[i])
					datesNewFiles[j] = sU("");
			}
		}
	}

	return result;
}

std::vector<sci::string> InstrumentProcessor::selectRelevantFilesUsingRegEx(const std::vector<sci::string> &allFiles, sci::string regEx)
{
	std::vector<sci::string> result;
	result.reserve(allFiles.size());
	wxRegEx regularExpression = sci::nativeUnicode(regEx);
	sci::assertThrow(regularExpression.IsValid(), sci::err(sci::SERR_USER, 0, sci::nativeCodepage(regEx)));
	for(size_t i=0; i<allFiles.size(); ++i)
	{
		if (regularExpression.Matches(sci::nativeUnicode(allFiles[i])))
		{
			result.push_back(allFiles[i]);
		}
	}
	return result;
}
