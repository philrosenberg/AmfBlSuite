#include "FolderChangesLister.h"
#include<fstream>
#include<algorithm>

void FolderChangesLister::clearSnapshotFile()
{
	std::fstream fin;
	fin.open(sci::nativeUnicode(m_snapshotFile), std::ios::out);
	if (!fin.is_open())
		throw(sU("Could not clear the snapshot file ") + m_snapshotFile + sU("."));
	fin.close();
}

std::vector<sci::string> ExistedFolderChangesLister::getChanges(const sci::string &filespec) const
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

	//Find all the files in the input directory that match the filespec
	std::vector<sci::string> allFiles = listFolderContents(filespec);

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

void ExistedFolderChangesLister::updateSnapshotFile(const sci::string &changedFile) const
{
	std::fstream fout;
	fout.open(sci::nativeUnicode(getSnapshotFile()), std::ios::app);
	fout << sci::toUtf8(changedFile) << "\n";
	fout.close();
}