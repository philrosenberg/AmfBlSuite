#pragma once
#include<string>
#include<vector>
#include<wx/dir.h>
#include<svector/sstring.h>

class InstrumentProcessor;

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
	virtual std::vector<sci::string> getChanges(const InstrumentProcessor &processor) const = 0;
	virtual void updateSnapshotFile( const sci::string &changedFile) const = 0;
	virtual void clearSnapshotFile();
	std::vector<sci::string> listFolderContents(const InstrumentProcessor &processor) const;
	virtual ~FolderChangesLister() {}
private:
	sci::string m_directory;
	sci::string m_snapshotFile;
};

//This class lists folder changes simply assuming that if a file existed during the last snapshot
//then it hasn't changed
class ExistedFolderChangesLister : public FolderChangesLister
{
public:
	ExistedFolderChangesLister(const sci::string &directory, const sci::string snapshotFile)
		:FolderChangesLister(directory, snapshotFile)
	{}
	std::vector<sci::string> getChanges(const InstrumentProcessor &processor) const override;
	void updateSnapshotFile(const sci::string &changedFile) const override;
	virtual ~ExistedFolderChangesLister() {}
	std::vector<sci::string> getPreviouslyExistingFiles(const InstrumentProcessor &processor) const;
	std::vector<sci::string> getAllPreviouslyExistingFiles() const;
};

//This class lists folder changes simply assuming that if a file existed during the last snapshot
//then it hasn't changed
class AlphabeticallyLastCouldHaveChangedChangesLister : public ExistedFolderChangesLister
{
public:
	AlphabeticallyLastCouldHaveChangedChangesLister(const sci::string &directory, const sci::string snapshotFile)
		:ExistedFolderChangesLister(directory, snapshotFile)
	{}
	std::vector<sci::string> getChanges(const InstrumentProcessor &processor) const override;
	void updateSnapshotFile(const sci::string &changedFile) const override;
	virtual ~AlphabeticallyLastCouldHaveChangedChangesLister() {}
};