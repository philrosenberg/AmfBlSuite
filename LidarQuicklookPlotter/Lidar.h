#pragma once
#include<svector/time.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include<svector/serr.h>
#include"HplHeader.h"
#include"Units.h"
#include"AmfNc.h"
#include"Plotting.h"
#include"HplProfile.h"
#include"HplHeader.h"

class ProgressReporter;
class wxWindow;

class OrientationGrabber
{
public:
	virtual void getOrientation(radian &roll, radian &pitch, radian &heading, sci::UtcTime time) const = 0;
	virtual ~OrientationGrabber() {}
};

class StaticOrientationGrabber : public OrientationGrabber
{
public:
	StaticOrientationGrabber(radian roll, radian pitch, radian heading)
	{}
	virtual void getOrientation(radian &roll, radian &pitch, radian &heading, sci::UtcTime time) const override
	{
		roll = m_roll;
		pitch = m_pitch;
		heading = m_heading;
	}
	virtual ~StaticOrientationGrabber() {}
private:
	const radian m_pitch;
	const radian m_roll;
	const radian m_heading;
};

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
	LidarBackscatterDopplerProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) :m_hasData(false), m_calibrationInfo(calibrationInfo) {}
	virtual void readData(const std::vector<sci::string> &inputFilenames, ProgressReporter &progressReporter, wxWindow *parent) override;
	void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent, bool clear);
	virtual bool hasData() const override { return m_hasData; }
	std::vector<second> getTimesSeconds() const;
	std::vector<sci::UtcTime> getTimesUtcTime() const;
	std::vector<std::vector<perSteradianPerMetre>> getBetas() const;
	std::vector<std::vector<metrePerSecond>> getDopplerVelocities() const;
	std::vector<metre> getGateBoundariesForPlotting(size_t profileIndex) const;
	std::vector<metre> getGateLowerBoundaries(size_t profileIndex) const;
	std::vector<metre> getGateUpperBoundaries(size_t profileIndex) const;
	std::vector<metre> getGateCentres(size_t profileIndex) const;
	std::vector<radian> getAzimuths() const;
	std::vector<radian> getElevations() const;
	CalibrationInfo getCalibrationInfo() const { return m_calibrationInfo; }
	InstrumentInfo getInstrumentInfo() const { return m_instrumentInfo; }
	void setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent)
	{
		sci::assertThrow(m_hplHeaders.size() == 1, sci::err(sci::SERR_USER, 0, "Attempt to set up a lidar plot for data with either 0 or >1 header. Need exactly one header (i.e. one file) to set up a quicklook plot."));
		PlotableLidar::setupCanvas(window, plot, extraDescriptor, parent, m_hplHeaders[0]);
	}
	const HplHeader &getHeaderForProfile(size_t profileIndex) { return m_hplHeaders[m_headerIndex[profileIndex]]; }
	size_t getNGates(size_t profile) { return m_profiles[profile].nGates(); }
	size_t getNFilesRead() { return m_hplHeaders.size(); }
private:
	bool m_hasData;
	std::vector<HplHeader> m_hplHeaders;
	std::vector<HplProfile> m_profiles;
	std::vector<size_t> m_headerIndex; //this links headers to profiles m_hplHeader[m_headerIndex[i]] is the header for the ith profile
	const InstrumentInfo m_instrumentInfo;
	const CalibrationInfo m_calibrationInfo;
	std::shared_ptr<OrientationGrabber> m_orientationGrabber;
};

class LidarStareProcessor : public LidarBackscatterDopplerProcessor
{
public:
	LidarStareProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarBackscatterDopplerProcessor(instrumentInfo, calibrationInfo, orientationGrabber) {}
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
	LidarRhiProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarBackscatterDopplerProcessor(instrumentInfo, calibrationInfo, orientationGrabber) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) override;
	sci::string getFilenameFilter() const override;
};

class LidarVadProcessor : public LidarBackscatterDopplerProcessor
{
public:
	LidarVadProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber, size_t  nSegmentsMin = 10) : LidarBackscatterDopplerProcessor(instrumentInfo, calibrationInfo, orientationGrabber), m_nSegmentsMin(nSegmentsMin) {}
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
	LidarUserProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarBackscatterDopplerProcessor(instrumentInfo, calibrationInfo, orientationGrabber) {}
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
	LidarWindProfileProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) :m_hasData(false), m_instrumentInfo(instrumentInfo), m_calibrationInfo(calibrationInfo), m_orientationGrabber(orientationGrabber) {}
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
		Profile(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : m_VadProcessor(instrumentInfo, calibrationInfo, orientationGrabber) {}
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
	std::shared_ptr<OrientationGrabber> m_orientationGrabber;
};