#include "FolderChangesLister.h"
#include<fstream>
#include<algorithm>

void FolderChangesLister::clearSnapshotFile()
{
	std::fstream fin;
	fin.open(m_snapshotFile, std::ios::out);
	if (!fin.is_open())
		throw("Could not clear the snapshot file " + m_snapshotFile + ".");
	fin.close();
}

std::vector<std::string> ExistedFolderChangesLister::getChanges(const std::string &filespec) const
{
	std::vector<std::string> previouslyExistingFiles;
	std::fstream fin;
	fin.open(getSnapshotFile().c_str(), std::ios::in);
	std::string filename;
	std::getline(fin, filename);
	while (filename.length() > 0)
	{
		previouslyExistingFiles.push_back(filename);
		std::getline(fin, filename);
	}
	fin.close();

	//Find all the files in the input directory that match the filespec
	std::vector<std::string> allFiles = listFolderContents(filespec);

	//Sort the files in alphabetical order
	if (allFiles.size() > 0)
		std::sort(allFiles.begin(), allFiles.end());

	//find the new files
	std::vector<std::string> newFiles;
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

void ExistedFolderChangesLister::updateSnapshotFile(const std::string &changedFile) const
{
	std::fstream fout;
	fout.open(getSnapshotFile().c_str(), std::ios::app);
	fout << changedFile << "\n";
	fout.close();
}