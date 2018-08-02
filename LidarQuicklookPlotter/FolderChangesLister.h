#pragma once
#include<string>
#include<vector>
#include<wx/dir.h>

class FolderChangesLister
{
public:
	FolderChangesLister(const std::string &directory, const std::string snapshotFile)
	{
		setDirectory(directory);
		setSnapshotFile(snapshotFile);
	}
	void setDirectory(const std::string &directory) { m_directory = directory; }
	void setSnapshotFile(const std::string &snapshotFile) { m_snapshotFile = snapshotFile; }
	const std::string &getDirectory() const { return m_directory; }
	const std::string &getSnapshotFile() const { return m_snapshotFile; }
	virtual std::vector<std::string> getChanges(const std::string &filespec) const = 0;
	virtual void updateSnapshotFile( const std::string &changedFile) const = 0;
	virtual void clearSnapshotFile();
	std::vector<std::string> listFolderContents(const std::string &filespec) const
	{
		wxArrayString files;
		wxDir::GetAllFiles(m_directory, &files, filespec);
		std::vector<std::string> result(files.size());
		for (size_t i = 0; i < files.size(); ++i)
			result[i] = std::string(files[i]);
		return result;
	}
private:
	std::string m_directory;
	std::string m_snapshotFile;
};

class ExistedFolderChangesLister : public FolderChangesLister
{
public:
	ExistedFolderChangesLister(const std::string &directory, const std::string snapshotFile)
		:FolderChangesLister(directory, snapshotFile)
	{}
	std::vector<std::string> getChanges(const std::string &filespec) const override;
	void updateSnapshotFile(const std::string &changedFile) const override;
};