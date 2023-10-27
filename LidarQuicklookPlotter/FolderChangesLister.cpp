#include "FolderChangesLister.h"
#include<fstream>
#include<algorithm>
#include"InstrumentProcessor.h"
#include<map>

std::vector<sci::string> FolderChangesLister::getChanges(const InstrumentProcessor& processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	return getChanges(listFolderContents(processor, startTime, endTime), processor, startTime, endTime);
}

std::vector<std::vector<sci::string>> FolderChangesLister::getChangesSeparatedByOutput(const InstrumentProcessor& processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	std::vector<std::pair<sci::string, sci::UtcTime>> folderContents = listFolderContents(processor, startTime, endTime);
	std::vector<sci::string> changes = getChanges(folderContents, processor, startTime, endTime);
	std::vector<sci::string> folderContentsNoTime(folderContents.size());
	for (size_t i = 0; i < folderContentsNoTime.size(); ++i)
		folderContentsNoTime[i] = folderContents[i].first;

	return processor.groupInputFilesbyOutputFiles(changes, folderContentsNoTime);
}

void FolderChangesLister::updateSnapshotFile(const sci::string& changedFile, sci::UtcTime checkedTime) const
{
	std::fstream fout;
	fout.open(sci::nativeUnicode(getSnapshotFile()), std::ios::app);
	fout << sci::toUtf8(changedFile) << ":" << checkedTime.getIso8601String(0, false, false, true) << "\n";
	fout.close();
}

void FolderChangesLister::clearSnapshotFile()
{
	std::fstream fin;
	fin.open(sci::nativeUnicode(m_snapshotFile), std::ios::out);
	sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Could not clear the snapshot file ") + m_snapshotFile + sU(".")));
	fin.close();
}


std::vector<std::pair<sci::string, sci::UtcTime>> FolderChangesLister::listFolderContents(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	//get all the contents of the folder
	wxArrayString files;
	wxDir::GetAllFiles(sci::nativeUnicode(m_directory), &files);
	std::vector<sci::string> folderContents(files.size());
	for (size_t i = 0; i < files.size(); ++i)
		folderContents[i] = sci::fromWxString(files[i]);

	//filter just the ones relevant to the processor
	folderContents = processor.selectRelevantFiles(folderContents, startTime, endTime);

	//get the modified date of the relevant files
	std::vector<std::pair<sci::string, sci::UtcTime>> result(folderContents.size());
	for (size_t i = 0; i < folderContents.size(); ++i)
	{
		wxStructStat strucStat;
		wxStat(sci::nativeUnicode(folderContents[i]), &strucStat);
		result[i].first = folderContents[i];
		result[i].second = sci::UtcTime::getPosixEpoch()+sci::TimeInterval(strucStat.st_mtime);
	}

	return result;
}

std::vector<sci::string> FolderChangesLister::getChanges(const std::vector<std::pair<sci::string, sci::UtcTime>>& folderContents, const InstrumentProcessor& processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	std::map<sci::string, sci::UtcTime> lastProcessedTimeMap;

	//read in the previously known about files and the last time they were processed
	//we put them in a map to make searching fast and easy
	std::fstream fin;
	fin.open(sci::nativeUnicode(getSnapshotFile()), std::ios::in);
	if (fin.is_open())
	{
		std::string line; //utf8
		std::getline(fin, line);
		size_t lineNumber = 1; //for debugging purposes, it will be optimised out in release builds
		while (!fin.eof() && !fin.bad() && !fin.fail())
		{
			if (line.length() > 19)
			{
				//split the line into a filename and a date/time
				sci::string filename = sci::fromUtf8(line.substr(0, line.length() - 20));
				std::string timeString = line.substr(line.length() - 19);
				//replace the date/time separators with spaces
				for (size_t i = 0; i < timeString.length(); ++i)
					if (timeString[i] == '-' || timeString[i] == ':' || timeString[i] == 'T')
						timeString[i] = ' ';
				//read in time from the time string
				int hour;
				int minute;
				int second;
				int year;
				int month;
				int dayOfMonth;
				std::istringstream strm(timeString);
				strm >> year >> month >> dayOfMonth >> hour >> minute >> second;
				sci::UtcTime time(year, month, dayOfMonth, hour, minute, second);

				//record the filename and time
				lastProcessedTimeMap[filename] = time;

				//get the next line
				//line.clear(); //clear the line in case getline fails, then we have a zero length line and exit the loop
				std::getline(fin, line);
				++lineNumber;
			}
			else
			{
				std::getline(fin, line);
			}
		}
		fin.close();
	}

	//check the map to see if the files found exist in the map and if they do, if their
	//processed date is before their modified date. 
	std::vector<sci::string> changedFiles;
	for (size_t i = 0; i < folderContents.size(); ++i)
	{
		auto processed = lastProcessedTimeMap.find(folderContents[i].first);
		if (processed == lastProcessedTimeMap.end())
			changedFiles.push_back(folderContents[i].first);
		else if (processed->second < folderContents[i].second)
			changedFiles.push_back(folderContents[i].first);
	}

	//check that the files we actually found are files the processor actually wants
	return processor.selectRelevantFiles(changedFiles, startTime, endTime);
}

/*std::vector<sci::string> ExistedFolderChangesLister::getChanges(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	//find all the previously existing files that match the filespec
	std::vector<sci::string> previouslyExistingFiles = getPreviouslyExistingFiles(processor, startTime, endTime);
	//Find all the files in the input directory that match the filespec
	std::vector<sci::string> allFiles = listFolderContents(processor, startTime, endTime);

	//Sort the files in alphabetical order
	if (allFiles.size() > 0)
		std::sort(allFiles.begin(), allFiles.end());

	//find the new files
	std::vector<sci::string> newFiles;
	newFiles.reserve(allFiles.size());

	for (size_t i = 0; i < allFiles.size(); ++i)
	{
		bool alreadyExisted = false;
		for (size_t j = 0; j < previouslyExistingFiles.size(); ++j)
		{
			if (allFiles[i] == previouslyExistingFiles[j])
			{
				alreadyExisted = true;
				break;
			}
		}
		if (!alreadyExisted)
			newFiles.push_back(allFiles[i]);
	}

	return newFiles;
}

std::vector<sci::string>  ExistedFolderChangesLister::getPreviouslyExistingFiles(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	std::vector<sci::string> previouslyExistingFiles;
	std::fstream fin;
	fin.open(sci::nativeUnicode(getSnapshotFile()), std::ios::in);
	std::string filename; //utf8
	std::getline(fin, filename);
	while (filename.length() > 0)
	{
		previouslyExistingFiles.push_back(sci::fromUtf8(filename));
		std::getline(fin, filename);
	}
	fin.close();

	return processor.selectRelevantFiles(previouslyExistingFiles, startTime, endTime);
}

std::vector<sci::string>  ExistedFolderChangesLister::getAllPreviouslyExistingFiles() const
{
	std::vector<sci::string> previouslyExistingFiles;
	std::fstream fin;
	fin.open(sci::nativeUnicode(getSnapshotFile()), std::ios::in);
	std::string filename; //utf8
	std::getline(fin, filename);
	while (filename.length() > 0)
	{
		previouslyExistingFiles.push_back(sci::fromUtf8(filename));
		std::getline(fin, filename);
	}
	fin.close();

	return previouslyExistingFiles;
}

std::vector<sci::string> AlphabeticallyLastCouldHaveChangedChangesLister::getChanges(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	//find all the previously existing files that match the filespec
	std::vector<sci::string> previouslyExistingFiles = getPreviouslyExistingFiles(processor, startTime, endTime);
	//Find all the files in the input directory that match the filespec
	std::vector<sci::string> allFiles = listFolderContents(processor, startTime, endTime);

	//Sort the files in alphabetical order
	if (allFiles.size() > 0)
		std::sort(allFiles.begin(), allFiles.end());
	if (previouslyExistingFiles.size() > 0)
		std::sort(previouslyExistingFiles.begin(), previouslyExistingFiles.end());
	else
		return allFiles; //this saves us including code below to deal with the case of previouslyExistingFiles.size() == 0 causing overruns

	//remove duplicates from previously existing files;
	for (size_t i = 1; i < previouslyExistingFiles.size(); ++i)
	{
		if (previouslyExistingFiles[i] == previouslyExistingFiles[i - 1])
		{
			previouslyExistingFiles.erase(previouslyExistingFiles.begin() + i);
			--i;
		}
	}

	//find the new files
	std::vector<sci::string> newFiles;
	newFiles.reserve(allFiles.size());

	for (size_t i = 0; i < allFiles.size(); ++i)
	{
		bool alreadyExisted = false;
		//note we don't check against the last previously existing file alphabetically
		//as we always assume that this file could have changed
		for (size_t j = 0; j < previouslyExistingFiles.size() - 1; ++j)
		{
			if (allFiles[i] == previouslyExistingFiles[j])
			{
				alreadyExisted = true;
				break;
			}
		}
		if (!alreadyExisted)
			newFiles.push_back(allFiles[i]);
	}

	return newFiles;
}

void AlphabeticallyLastCouldHaveChangedChangesLister::updateSnapshotFile(const sci::string &changedFile) const
{
	//Check to make sure the file hasn't been added already.
	std::vector<sci::string> previouslyExistingFiles = getAllPreviouslyExistingFiles();
	for (size_t i = 0; i < previouslyExistingFiles.size(); ++i)
		if (changedFile == previouslyExistingFiles[i])
			return;
	ExistedFolderChangesLister::updateSnapshotFile(changedFile);
}*/

std::vector<sci::string> AssumeAllChangedChangesLister::getChanges(const std::vector<std::pair<sci::string, sci::UtcTime>>& folderContents, const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	//simply return all the files
	std::vector<sci::string> allFiles(folderContents.size());
	for (size_t i = 0; i < allFiles.size(); ++i)
		allFiles[i] = folderContents[i].first;

	//Sort the files in alphabetical order
	if (allFiles.size() > 0)
		std::sort(allFiles.begin(), allFiles.end());

	return allFiles;
}