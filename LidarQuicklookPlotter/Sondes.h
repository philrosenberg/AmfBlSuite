#pragma once
#include"InstrumentProcessor.h"
#include"AmfNc.h"

class SondeProcessor: public InstrumentProcessor
{
public:
	SondeProcessor(const InstrumentInfo &instrumentInfo, const CalibrationInfo &calibrationInfo);
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) override;
	void readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter);
	virtual void plotData(const sci::string &baseOutputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override
	{
		throw(sci::err(sci::SERR_USER, 0, "Sonde plotting is not yet supported."));
	}
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override
	{
		return m_hasData;
	}
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
	virtual std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
private:
	InstrumentInfo m_instrumentInfo;
	CalibrationInfo m_calibrationInfo;
	bool m_hasData;
	std::vector<metre> m_altitude;
	std::vector<degree>m_latitude;
	std::vector<degree> m_longitude;
	std::vector<hectoPascal> m_airPressure;
	std::vector<kelvin> m_temperature;
	std::vector<kelvin> m_dewpoint;
	std::vector<percent> m_relativeHumidity;
	std::vector<metrePerSecond> m_windSpeed;
	std::vector<degree> m_windFromDirection;
	std::vector<metrePerSecond> m_balloonUpwardVelocity;
	std::vector<secondf> m_elapsedTime;
	std::vector<sci::UtcTime> m_time;
	std::vector<unsigned char> m_motionFlags;
	std::vector<unsigned char> m_temperatureFlags;
	std::vector<unsigned char> m_humidityFlags;
	std::vector<unsigned char> m_pressureFlags;
	sci::string m_sondeSerialNumber;
};