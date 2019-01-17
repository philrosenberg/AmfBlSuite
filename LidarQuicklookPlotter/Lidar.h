#pragma once
#include<svector/time.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include"HplHeader.h"
#include"Units.h"
#include"AmfNc.h"
#include"Plotting.h"
#include"HplProfile.h"
#include"HplHeader.h"

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

class LidarBackscatterDopplerProcessor : public InstrumentProcessor
{
public:
	LidarBackscatterDopplerProcessor() :m_hasData(false) {}
	virtual void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual bool hasData() const override { return m_hasData; }
	std::vector<second> getTimes() const;
	std::vector<std::vector<perSteradianPerMetre>> getBetas() const;
	std::vector<std::vector<metrePerSecond>> getDopplerVelocities() const;
	std::vector<metre> getGateBoundariesForPlotting() const;
	std::vector<metre> getGateLowerBoundaries() const;
	std::vector<metre> getGateUpperBoundaries() const;
	std::vector<metre> getGateCentres() const;
	std::vector<radian> getAzimuths() const;
	std::vector<radian> getElevations() const;
	void setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent);
private:
	bool m_hasData;
	HplHeader m_hplHeader;
	std::vector<HplProfile> m_profiles;
};

class LidarStareProcessor : public LidarBackscatterDopplerProcessor
{
public:
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;

};

class LidarRhiProcessor : public LidarBackscatterDopplerProcessor
{
public:
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;

};

class LidarVadProcessor : public LidarBackscatterDopplerProcessor
{
	LidarVadProcessor(size_t nSegmentsMin = 10) : m_nSegmentsMin(nSegmentsMin) {}
public:
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
private:
	size_t m_nSegmentsMin;
};

class LidarUserProcessor : public LidarBackscatterDopplerProcessor
{
public:
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;

};