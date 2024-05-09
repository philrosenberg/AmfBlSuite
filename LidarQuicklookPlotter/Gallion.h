#pragma once
#include"AmfNc.h"
#include"Lidar.h"

class GalionWindProfileProcessor : public WindProfileProcessor, public InstrumentProcessor
{
public:
	GalionWindProfileProcessor(sci::string fileSearchRegEx, const InstrumentInfo &instrumentInfo, const CalibrationInfo &calibrationInfo)
		:InstrumentProcessor(fileSearchRegEx), m_hasData(false), m_instrumentInfo(instrumentInfo), m_calibrationInfo(calibrationInfo)
	{
	}
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform & platform, ProgressReporter & progressReporter);
	virtual void plotData(const sci::string& baseOutputFilename, const std::vector<metreF> maxRanges, ProgressReporter& progressReporter, wxWindow* parent)
	{
	}
	virtual void writeToNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter)
	{
		WindProfileProcessor::writeToNc(directory, author, processingSoftwareInfo, projectInfo, platform, processingOptions, progressReporter, m_instrumentInfo, m_calibrationInfo);
	}
	virtual bool hasData() const
	{
		return m_hasData;
	}
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const = 0;
	virtual std::vector<std::vector<sci::string>> groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const = 0;
	virtual sci::string getName() const
	{
		return sU("Galion Wind Lidar Processor");
	}
	void readWindProfile(const sci::string& filename, std::vector<Profile>& profiles, sci::GridData<sci::UtcTime, 2>& times,
		sci::GridData<metrePerSecondF, 2>& horizontalWindSpeeds, sci::GridData<metrePerSecondF, 2>& verticalWindSpeeds, sci::GridData<degreeF, 2>& windDirections,
		sci::GridData<metreSquaredPerSecondSquaredF, 2>& goodnessOfFits, sci::GridData<metreSquaredPerSecondSquaredF, 2>& minimumPowerIntensities, sci::GridData<metreSquaredPerSecondSquaredF, 2>& meanPowerIntensities,
		sci::GridData<metrePerSecondF, 2>& neSpeeds, sci::GridData<metrePerSecondF, 2>& esSpeeds, sci::GridData<metrePerSecondF, 2>& swSpeeds, sci::GridData<metrePerSecondF, 2>& wnSpeeds, ProgressReporter& progressReporter);
private:
	InstrumentInfo m_instrumentInfo;
	CalibrationInfo m_calibrationInfo;
	bool m_hasData;
};

void readWindProfile(const sci::string& filename, sci::GridData<sci::UtcTime, 2>& times, sci::GridData<metreF, 1>& heights,
	sci::GridData<metrePerSecondF, 2>& horizontalWindSpeeds, sci::GridData<metrePerSecondF, 2>& verticalWindSpeeds, sci::GridData<degreeF, 2>& windDirections,
	sci::GridData<metreSquaredPerSecondSquaredF, 2>& goodnessOfFits, sci::GridData<metreSquaredPerSecondSquaredF, 2>& minimumPowerIntensities, sci::GridData<metreSquaredPerSecondSquaredF, 2>& meanPowerIntensities,
	sci::GridData<metrePerSecondF, 2>& neSpeeds, sci::GridData<metrePerSecondF, 2>& esSpeeds, sci::GridData<metrePerSecondF, 2>& swSpeeds, sci::GridData<metrePerSecondF, 2>& wnSpeeds)
{
	times.reshape({ 0,0 });
	heights.resize(0);
	horizontalWindSpeeds.reshape({ 0,0 });
	verticalWindSpeeds.reshape({ 0,0 });
	windDirections.reshape({ 0,0 });
	goodnessOfFits.reshape({ 0,0 });
	minimumPowerIntensities.reshape({ 0,0 });
	meanPowerIntensities.reshape({ 0,0 });
	neSpeeds.reshape({ 0,0 });
	esSpeeds.reshape({ 0,0 });
	swSpeeds.reshape({ 0,0 });
	wnSpeeds.reshape({ 0,0 });

	std::fstream fin;
	fin.open(sci::nativeUnicode(filename), std::ios::in);
	sci::stringstream message;
	message << "could not open file " << filename;
	sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, message.str()));

	std::string line;
	std::getline(fin, line); //read the header line, we don't need it.

	//we first want to count how many altitudes we are using by seeing when the altitudes repeat
	metreF height;
	std::getline(fin, line);
	assertThrow(line.length() > 27, sci::err(sci::SERR_USER, 0, sU("found a cropped line")));
	std::stringstream lineStream;
	lineStream.str(line.substr(24));
	lineStream >> height;
	heights.push_back(height);
	while(1)
	{
		std::getline(fin, line);
		assertThrow(line.length() > 27, sci::err(sci::SERR_USER, 0, sU("found a cropped line")));
		std::stringstream lineStream;
		lineStream.str(line.substr(24));
		lineStream >> height;
		if (height == heights[0])
			break;
		heights.push_back(height);
	}
	
	size_t nHeights = heights.size();
	//there's about 120 bytes per line. Work out ow many bytes we have and reserve space in the grids
	fin.seekg(0, std::ios::end);
	size_t fileLength = fin.tellg();
	size_t approxLines = fileLength / 110; //overestimate by a bit to give some slack
	times.reserve(approxLines);
	horizontalWindSpeeds.reserve(approxLines);
	verticalWindSpeeds.reserve(approxLines);
	windDirections.reserve(approxLines);
	goodnessOfFits.reserve(approxLines);
	minimumPowerIntensities.reserve(approxLines);
	meanPowerIntensities.reserve(approxLines);
	neSpeeds.reserve(approxLines);
	esSpeeds.reserve(approxLines);
	swSpeeds.reserve(approxLines);
	wnSpeeds.reserve(approxLines);

	//return to the beginning of the file;
	fin.seekg(0, std::ios::beg);
	std::getline(fin, line); //read the header line, we still don't need it.
	size_t nProfiles = 0;
	while (!fin.eof() && !fin.bad() && !fin.fail())
	{
		++nProfiles;
		times.reshape({ nProfiles,nHeights }, std::numeric_limits<sci::UtcTime>::quiet_NaN());
		horizontalWindSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		verticalWindSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		windDirections.reshape({ nProfiles,nHeights }, std::numeric_limits<degreeF>::quiet_NaN());
		goodnessOfFits.reshape({ nProfiles,nHeights }, std::numeric_limits<metreSquaredPerSecondSquaredF>::quiet_NaN());
		minimumPowerIntensities.reshape({ nProfiles,nHeights }, std::numeric_limits<metreSquaredPerSecondSquaredF>::quiet_NaN());
		meanPowerIntensities.reshape({ nProfiles,nHeights }, std::numeric_limits<metreSquaredPerSecondSquaredF>::quiet_NaN());
		neSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		esSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		swSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		wnSpeeds.reshape({ nProfiles,nHeights }, std::numeric_limits<metrePerSecondF>::quiet_NaN());
		for (size_t i = 0; i < nHeights; ++i)
		{
			std::getline(fin, line);
			if (line.length() == 0)
				break;
			assertThrow(line.length() > 113, sci::err(sci::SERR_USER, 0, sU("found a cropped line")));
			line[4] = ' ';
			line[7] = ' ';
			line[13] = ' ';
			line[16] = ' ';
			int year;
			int month;
			int day;
			int hour;
			int minute;
			double second;
			std::stringstream lineStream;
			lineStream.str(line);
			lineStream >> year >> month >> day >> hour >> minute >> second >> height
				>>horizontalWindSpeeds[nProfiles-1][i] >> verticalWindSpeeds[nProfiles - 1][i] >> windDirections[nProfiles - 1][i]
				>> goodnessOfFits[nProfiles - 1][i] >> minimumPowerIntensities[nProfiles - 1][i] >> meanPowerIntensities[nProfiles - 1][i]
				>> neSpeeds[nProfiles - 1][i] >> esSpeeds[nProfiles - 1][i]>> swSpeeds[nProfiles - 1][i]
				>> wnSpeeds[nProfiles - 1][i];
			times[nProfiles - 1][i] = sci::UtcTime(year, month, day, hour, minute, second);
		}
	}
}