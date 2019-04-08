#include "FolderChangesLister.h"
#include<fstream>
#include<algorithm>
#include"InstrumentProcessor.h"

void FolderChangesLister::clearSnapshotFile()
{
	std::fstream fin;
	fin.open(sci::nativeUnicode(m_snapshotFile), std::ios::out);
	sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Could not clear the snapshot file ") + m_snapshotFile + sU(".")));
	fin.close();
}


std::vector<sci::string> FolderChangesLister::listFolderContents(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	wxArrayString files;
	wxDir::GetAllFiles(sci::nativeUnicode(m_directory), &files);
	std::vector<sci::string> result(files.size());
	for (size_t i = 0; i < files.size(); ++i)
		result[i] = sci::fromWxString(files[i]);
	result = processor.selectRelevantFiles(result, startTime, endTime);
	return result;
}

std::vector<sci::string> ExistedFolderChangesLister::getChanges(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const
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

void ExistedFolderChangesLister::updateSnapshotFile(const sci::string &changedFile) const
{
	std::fstream fout;
	fout.open(sci::nativeUnicode(getSnapshotFile()), std::ios::app);
	fout << sci::toUtf8(changedFile) << "\n";
	fout.close();
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
}

std::vector<sci::string> AssumeAllChangedChangesLister::getChanges(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	//Find all the files in the input directory that match the filespec
	std::vector<sci::string> allFiles = listFolderContents(processor, startTime, endTime);

	//Sort the files in alphabetical order
	if (allFiles.size() > 0)
		std::sort(allFiles.begin(), allFiles.end());

	return allFiles;
}