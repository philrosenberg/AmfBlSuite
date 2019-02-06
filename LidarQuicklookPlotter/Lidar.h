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

const InstrumentInfo g_dopplerLidar1Info
{
	sU("NCAS Doppler Aerosol Lidar unit 1"),
	sU(""),
	sU("HALO Photonics"),
	sU("StreamLine"),
	sU("1210-18"),
	sU("StreamLine"),
	sU("v9")
};

class PlotableLidar : public InstrumentProcessor
{
public:
	void setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent, HplHeader hplHeader);
};

class LidarBackscatterDopplerProcessor : public PlotableLidar
{
public:
	LidarBackscatterDopplerProcessor(CalibrationInfo calibrationInfo) :m_hasData(false), m_calibrationInfo(calibrationInfo) {}
	virtual void readData(const std::vector<sci::string> &inputFilenames, ProgressReporter &progressReporter, wxWindow *parent) override;
	void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent, bool clear);
	virtual bool hasData() const override { return m_hasData; }
	std::vector<second> getTimesSeconds() const;
	std::vector<sci::UtcTime> getTimesUtcTime() const;
	std::vector<std::vector<perSteradianPerMetre>> getBetas() const;
	std::vector<std::vector<metrePerSecond>> getDopplerVelocities() const;
	std::vector<metre> getGateBoundariesForPlotting() const;
	std::vector<metre> getGateLowerBoundaries() const;
	std::vector<metre> getGateUpperBoundaries() const;
	std::vector<metre> getGateCentres() const;
	std::vector<radian> getAzimuths() const;
	std::vector<radian> getElevations() const;
	CalibrationInfo getCalibrationInfo() const { return m_calibrationInfo; }
	void setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent)
	{
		PlotableLidar::setupCanvas(window, plot, extraDescriptor, parent, m_hplHeader);
	}
private:
	bool m_hasData;
	HplHeader m_hplHeader;
	std::vector<HplProfile> m_profiles;
	const CalibrationInfo m_calibrationInfo;
};

class LidarStareProcessor : public LidarBackscatterDopplerProcessor
{
public:
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	sci::string getFilenameFilter() const override { return sU("*Stare_??_????????_??.hpl"); };
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
};

class LidarRhiProcessor : public LidarBackscatterDopplerProcessor
{
public:
	LidarRhiProcessor(CalibrationInfo calibrationInfo) : LidarBackscatterDopplerProcessor(calibrationInfo) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	sci::string getFilenameFilter() const override;
};

class LidarVadProcessor : public LidarBackscatterDopplerProcessor
{
public:
	LidarVadProcessor(CalibrationInfo calibrationInfo, size_t  nSegmentsMin = 10) : LidarBackscatterDopplerProcessor(calibrationInfo), m_nSegmentsMin(nSegmentsMin) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	void plotDataPlan(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent);
	void plotDataCone(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent);
	void plotDataCone(radian viewAzimuth, metre maxRange, splot2d * plot);
	void plotDataUnwrapped(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent);
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	sci::string getFilenameFilter() const override { return sU("*VAD_??_????????_??????.hpl"); };
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
private:
	size_t m_nSegmentsMin;
	void getDataSortedByAzimuth(std::vector<std::vector<perSteradianPerMetre>> &sortedBetas, std::vector<radian> &sortedElevations, std::vector<radian> &sortedMidAzimuths, std::vector<radian> &azimuthBoundaries);
};

class LidarUserProcessor : public LidarBackscatterDopplerProcessor
{
public:
	LidarUserProcessor(CalibrationInfo calibrationInfo) : LidarBackscatterDopplerProcessor(calibrationInfo) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	sci::string getFilenameFilter() const override { return sU("*User*.hpl"); };
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
};

class LidarWindProfileProcessor : public InstrumentProcessor
{
public:
	LidarWindProfileProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) :m_hasData(false), m_instrumentInfo(instrumentInfo), m_calibrationInfo(calibrationInfo) {}
	virtual void readData(const std::vector<sci::string> &inputFilenames, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override { return m_hasData; }
	InstrumentInfo getInstrumentInfo() const { return m_instrumentInfo; }
	CalibrationInfo getCalibrationInfo() const { return m_calibrationInfo; }
	sci::string getFilenameFilter() const override { return sU("*Processed_Wind_Profile_??_????????_??????.hpl"); };
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
private:
	bool m_hasData;
	struct Profile
	{
		Profile(CalibrationInfo calibrationInfo) : m_VadProcessor(calibrationInfo) {}
		std::vector<metre> m_heights;
		std::vector<radian> m_windDirections;
		std::vector<metrePerSecond> m_windSpeeds;
		InstrumentInfo m_instrumentInfo;
		//sci::UtcTime m_time;
		LidarVadProcessor m_VadProcessor; // have a separate VAD processor for each profile
	};
	std::vector<Profile> m_profiles;
	const InstrumentInfo m_instrumentInfo;
	const CalibrationInfo m_calibrationInfo;
};