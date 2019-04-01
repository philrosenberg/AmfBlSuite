#pragma once

#include<vector>
#include"Units.h"
#include<svector/splot.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include<svector/time.h>
#include<memory>

class ProgressReporter;
class Platform;
struct PersonInfo;
struct ProjectInfo;
struct ProcessingSoftwareInfo;
struct ProcessingOptions;
struct InstrumentInfo;
struct CalibrationInfo;

class InstrumentProcessor
{
public:
	//a note on regexes for the constructor. \ is the escape character in the regex. It is also the escape character
	//in C++, so \\ creates an escape character in the regex (e.g. \\t for a tab) or if you actually want an escaped
	//forward slash you need to use 4 of them (e.g. \\\\).
	//[] means any character in the bracket. Combined with the escaping above this gives [/\\\\] meaning either a unix
	//or windows path delimiter.
	//. means any character
	//$ means the end of the string
	//^ means the beginning of the string
	//so the following might be a typical regex
	// [/\\\\^]........_......_myInstrument\\.ext
	//which is a file that has the following either fate a slash of any type or the start of the filename:
	//8 characters (maybe a date)
	//an undercore
	//6 characters (maybe a time)
	//_myInstrument
	//a . (escaped)
	//ext
	InstrumentProcessor(sci::string fileSearchRegEx) : m_fileSearchRegEx(fileSearchRegEx) {}
	virtual ~InstrumentProcessor() {}
	//virtual void readDataAndPlot(const std::string &inputFilename, const std::string &outputFilename, const std::vector<double> maxRanges, ProgressReporter &progressReporter, wxWindow *parent);
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) = 0;
	virtual void plotData(const sci::string &baseOutputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) = 0;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) = 0;
	virtual bool hasData() const = 0;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const = 0;
	static bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime, size_t dateStartCharacter, size_t hourStartCharacter, size_t minuteStartCharacter, size_t secondStartCharacter, second fileDuration);
	virtual std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const = 0;
	static std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles, size_t dateStartCharacter);
	virtual std::vector<sci::string> selectRelevantFiles(const std::vector<sci::string> &allFiles, sci::UtcTime startTime, sci::UtcTime endTime) const
	{
		std::vector<sci::string> regexMatchingFiles = selectRelevantFilesUsingRegEx(allFiles, m_fileSearchRegEx);
		std::vector<sci::string> finalFiles;
		if (regexMatchingFiles.size() > 0)
		{
			finalFiles.reserve(regexMatchingFiles.size());
			for (auto iter = regexMatchingFiles.begin(); iter != regexMatchingFiles.end(); ++iter)
			{
				if (fileCoversTimePeriod(*iter, startTime, endTime))
					finalFiles.push_back(*iter);
			}
		}
		return finalFiles;
	}
private:
	sci::string m_fileSearchRegEx;
	static std::vector<sci::string> selectRelevantFilesUsingRegEx(const std::vector<sci::string> &allFiles, sci::string regEx);
};

std::shared_ptr<InstrumentProcessor> getProcessorByName(sci::string name, InstrumentInfo &instrumentInfo, CalibrationInfo &calibrationInfo);