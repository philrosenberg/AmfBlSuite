#pragma once
#include<string>
#include<vector>
#include<wx/dir.h>
#include<svector/sstring.h>
#include<svector/time.h>

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
	std::vector<sci::string> getChanges(const InstrumentProcessor& processor, sci::UtcTime startTime, sci::UtcTime endTime) const;
	virtual std::vector<std::vector<sci::string>> getChangesSeparatedByOutput(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const;
	virtual void updateSnapshotFile(const sci::string &changedFile, sci::UtcTime checkedTime) const;
	virtual void clearSnapshotFile();
	std::vector<std::pair<sci::string, sci::UtcTime>> listFolderContents(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const;
	virtual ~FolderChangesLister() {}
private:
	virtual std::vector<sci::string> getChanges(const std::vector<std::pair<sci::string, sci::UtcTime>>& folderContents, const InstrumentProcessor& processor, sci::UtcTime startTime, sci::UtcTime endTime) const;
	sci::string m_directory;
	sci::string m_snapshotFile;
};

//This class lists folder changes simply assuming that if a file existed during the last snapshot
//then it hasn't changed
/*class ExistedFolderChangesLister : public FolderChangesLister
{
public:
	ExistedFolderChangesLister(const sci::string &directory, const sci::string snapshotFile)
		:FolderChangesLister(directory, snapshotFile)
	{}
	std::vector<sci::string> getChanges(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime, sci::UtcTime& checkTime) const override;
	virtual ~ExistedFolderChangesLister() {}
	std::vector<sci::string> getPreviouslyExistingFiles(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const;
	std::vector<sci::string> getAllPreviouslyExistingFiles() const;
};

//This class lists folder changes simply assuming that if a file existed during the last snapshot
//then it hasn't changed
class AlphabeticallyLastCouldHaveChangedChangesLister : public FolderChangesLister
{
public:
	AlphabeticallyLastCouldHaveChangedChangesLister(const sci::string &directory, const sci::string snapshotFile)
		:ExistedFolderChangesLister(directory, snapshotFile)
	{}
	std::vector<sci::string> getChanges(const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime, sci::UtcTime& checkTime) const override;
	void updateSnapshotFile(const sci::string &changedFile) const override;
	virtual ~AlphabeticallyLastCouldHaveChangedChangesLister() {}
};*/

class AssumeAllChangedChangesLister : public FolderChangesLister
{
public:
	AssumeAllChangedChangesLister(const sci::string &directory, const sci::string snapshotFile)
		:FolderChangesLister(directory, snapshotFile)
	{}
	std::vector<sci::string> getChanges(const std::vector<std::pair<sci::string, sci::UtcTime>>& folderContents, const InstrumentProcessor &processor, sci::UtcTime startTime, sci::UtcTime endTime) const override;
};