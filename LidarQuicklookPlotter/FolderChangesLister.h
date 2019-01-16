#pragma once
#include<string>
#include<vector>
#include<wx/dir.h>
#include<svector/sstring.h>

class FolderChangesLister
{
public:
	FolderChangesLister(const sci::string &directory, const sci::string snapshotFile)
	{
		setDirectory(directory);
		setSnapshotFile(snapshotFile);
	}
	void setDirectory(const sci::string &directory) { m_directory = directory; }
	void setSnapshotFile(const sci::string &snapshotFile) { m_snapshotFile = snapshotFile; }
	const sci::string &getDirectory() const { return m_directory; }
	const sci::string &getSnapshotFile() const { return m_snapshotFile; }
	virtual std::vector<sci::string> getChanges(const sci::string &filespec) const = 0;
	virtual void updateSnapshotFile( const sci::string &changedFile) const = 0;
	virtual void clearSnapshotFile();
	std::vector<sci::string> listFolderContents(const sci::string &filespec) const
	{
		wxArrayString files;
		wxDir::GetAllFiles(sci::nativeUnicode(m_directory), &files, sci::nativeUnicode(filespec));
		std::vector<sci::string> result(files.size());
		for (size_t i = 0; i < files.size(); ++i)
			result[i] = sci::fromWxString(files[i]);
		return result;
	}
private:
	sci::string m_directory;
	sci::string m_snapshotFile;
};

class ExistedFolderChangesLister : public FolderChangesLister
{
public:
	ExistedFolderChangesLister(const sci::string &directory, const sci::string snapshotFile)
		:FolderChangesLister(directory, snapshotFile)
	{}
	std::vector<sci::string> getChanges(const sci::string &filespec) const override;
	void updateSnapshotFile(const sci::string &changedFile) const override;
};