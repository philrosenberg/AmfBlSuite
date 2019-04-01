#include "InstrumentProcessor.h"
#include"AmfNc.h"
#include<wx/filename.h>
//#include<wx/dir.h>
#include<wx/regex.h>

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
	stream.str(fileNameToSplit.substr(dateStartCharacter + 6, 2));
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
	return fileStartTime < endTime && (fileStartTime + fileDuration) >= startTime;
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


	for (size_t i = 0; i < datesNewFiles.size(); ++i)
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
			for (size_t j = i + 1; j < datesNewFiles.size(); ++j)
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
	for (size_t i = 0; i < allFiles.size(); ++i)
	{
		if (regularExpression.Matches(sci::nativeUnicode(allFiles[i])))
		{
			result.push_back(allFiles[i]);
		}
	}
	return result;
}



#include"Lidar.h"
#include"Ceilometer.h"
#include"MicroRainRadar.h"
std::shared_ptr<InstrumentProcessor> getProcessorByName(sci::string name, InstrumentInfo &instrumentInfo, CalibrationInfo &calibrationInfo)
{
	if (name == sU("LidarCopolarisedStareProcessor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarCopolarisedStareProcessor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarCrosspolarisedStareProcessor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarCrosspolarisedStareProcessor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarWindVadProcessor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarWindVadProcessor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarVadProcessor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarVadProcessor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarWindProfileProcessor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarWindProfileProcessor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarRhiProcessor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarRhiProcessor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarUser1Processor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarUser1Processor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarUser2Processor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarUser2Processor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarUser3Processor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarUser3Processor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarUser4Processor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarUser4Processor(instrumentInfo, calibrationInfo));
	if (name == sU("LidarUser5Processor"))
		return std::shared_ptr<InstrumentProcessor>(new LidarUser5Processor(instrumentInfo, calibrationInfo));
	if (name == sU("CeilometerProcessor"))
		return std::shared_ptr<InstrumentProcessor>(new CeilometerProcessor());
	if (name == sU("MicroRainRadarProcessor"))
		return std::shared_ptr<InstrumentProcessor>(new MicroRainRadarProcessor(instrumentInfo, calibrationInfo));

	throw(sci::err(sci::SERR_USER, 0, sU("Invalid processor name: ") + name));
}