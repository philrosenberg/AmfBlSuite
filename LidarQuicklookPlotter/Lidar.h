#pragma once
#include<svector/time.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include"HplHeader.h"
#include"Units.h"
#include"AmfNc.h"
#include"Plotting.h"

class ProgressReporter;
class wxWindow;

class LidarWindProfileProcessor : public InstrumentProcessor
{
public:
	LidarWindProfileProcessor() :m_hasData(false) {}
	virtual void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override {return m_hasData;}
private:
	bool m_hasData;
	std::vector<metre> m_heights;
	std::vector<radian> m_windDirections;
	std::vector<metrePerSecond> m_windSpeeds;
};

class LidarStareProcessor : public InstrumentProcessor
{
public:
	LidarStareProcessor() :m_hasData(false) {}
	virtual void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override { return m_hasData; }
private:
	bool m_hasData;
};

class LidarVadProcessor : public InstrumentProcessor
{
public:
	LidarVadProcessor() :m_hasData(false) {}
	virtual void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override { return m_hasData; }
private:
	bool m_hasData;
};

class LidarUserProcessor : public InstrumentProcessor
{
public:
	LidarUserProcessor() :m_hasData(false) {}
	virtual void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override { return m_hasData; }
private:
	bool m_hasData;
};