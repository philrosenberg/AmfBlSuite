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
	virtual void getOrientation(degree &roll, degree &pitch, degree &heading, sci::UtcTime time) const = 0;
	virtual ~OrientationGrabber() {}
};

class StaticOrientationGrabber : public OrientationGrabber
{
public:
	StaticOrientationGrabber(degree roll, degree pitch, degree heading)
	{}
	virtual void getOrientation(degree &roll, degree &pitch, degree &heading, sci::UtcTime time) const override
	{
		roll = m_roll;
		pitch = m_pitch;
		heading = m_heading;
	}
	virtual ~StaticOrientationGrabber() {}
private:
	const degree m_pitch;
	const degree m_roll;
	const degree m_heading;
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
	PlotableLidar(sci::string fileSearchRegEx) : InstrumentProcessor(fileSearchRegEx) {}
	void setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent, HplHeader hplHeader);
};

class LidarBackscatterDopplerProcessor : public PlotableLidar
{
public:
	LidarBackscatterDopplerProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber, sci::string fileSearchRegEx) : PlotableLidar(fileSearchRegEx), m_hasData(false), m_instrumentInfo(instrumentInfo), m_calibrationInfo(calibrationInfo), m_orientationGrabber(orientationGrabber) {}
	virtual ~LidarBackscatterDopplerProcessor() {}
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter, wxWindow *parent) override;
	void readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter, wxWindow *parent, bool clear);
	virtual bool hasData() const override { return m_hasData; }
	virtual std::vector<sci::string> getProcessingOptions() const = 0;
	std::vector<second> getTimesSeconds() const;
	std::vector<sci::UtcTime> getTimesUtcTime() const;
	std::vector<std::vector<perSteradianPerMetre>> getBetas() const;
	std::vector<std::vector<metrePerSecond>> getDopplerVelocities() const;
	std::vector<std::vector<unitless>> getSignalToNoiseRatios() const;
	std::vector<std::vector<uint8_t>> getDopplerFlags() const { return m_dopplerFlags; }
	std::vector<std::vector<uint8_t>> getBetaFlags() const { return m_betaFlags; }
	std::vector<metre> getGateBoundariesForPlotting(size_t profileIndex) const;
	std::vector<metre> getGateLowerBoundaries(size_t profileIndex) const;
	std::vector<metre> getGateUpperBoundaries(size_t profileIndex) const;
	std::vector<metre> getGateCentres(size_t profileIndex) const;
	std::vector<degree> getAzimuths() const { return m_correctedAzimuths;}
	std::vector<degree> getElevations() const { return m_correctedElevations; }
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
	std::vector<std::vector<uint8_t>> m_dopplerFlags;
	std::vector<std::vector<uint8_t>> m_betaFlags;
	std::vector<size_t> m_headerIndex; //this links headers to profiles m_hplHeader[m_headerIndex[i]] is the header for the ith profile
	const InstrumentInfo m_instrumentInfo;
	const CalibrationInfo m_calibrationInfo;
	std::shared_ptr<OrientationGrabber> m_orientationGrabber;
	std::vector<degree> m_correctedAzimuths;
	std::vector<degree> m_correctedElevations;
};

class LidarScanningProcessor : public LidarBackscatterDopplerProcessor
{
public:
	LidarScanningProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber, sci::string fileSearchRegEx)
		:LidarBackscatterDopplerProcessor(instrumentInfo, calibrationInfo, orientationGrabber, fileSearchRegEx)
	{}
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) override;
};

class LidarStareProcessor : public LidarBackscatterDopplerProcessor
{
public:
	LidarStareProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber, sci::string fileSearchRegEx) : LidarBackscatterDopplerProcessor(instrumentInfo, calibrationInfo, orientationGrabber, fileSearchRegEx) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) override;
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
};

class LidarCopolarisedStareProcessor : public LidarStareProcessor
{
public:
	LidarCopolarisedStareProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarStareProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]Proc[/\\\\]....[/\\\\]......[/\\\\]........[/\\\\]Stare_.._........_..\\.hpl$")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("fixed"), sU("co") }; }
};

class LidarCrosspolarisedStareProcessor : public LidarStareProcessor
{
public:
	LidarCrosspolarisedStareProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarStareProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]Proc[/\\\\]cross[/\\\\]....[/\\\\]......[/\\\\]........[/\\\\]Stare_.._........_..\\.hpl$")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("fixed"), sU("cr") }; }
};

class LidarRhiProcessor : public LidarScanningProcessor
{
public:
	LidarRhiProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarScanningProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]RHI_.._........_......\\.hpl$")) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("rhi") }; }
};



class ConicalScanningProcessor : public LidarScanningProcessor
{
public:
	ConicalScanningProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber, sci::string fileSearchRegEx, size_t  nSegmentsMin = 10) : LidarScanningProcessor(instrumentInfo, calibrationInfo, orientationGrabber, fileSearchRegEx), m_nSegmentsMin(nSegmentsMin) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	void plotDataPlan(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent);
	void plotDataCone(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent);
	void plotDataCone(degree viewAzimuth, metre maxRange, splot2d * plot);
	void plotDataUnwrapped(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent);
private:
	size_t m_nSegmentsMin;
	void getDataSortedByAzimuth(std::vector<std::vector<perSteradianPerMetre>> &sortedBetas, std::vector<degree> &sortedElevations, std::vector<degree> &sortedMidAzimuths, std::vector<degree> &azimuthBoundaries);
};

class LidarVadProcessor : public ConicalScanningProcessor
{
public:
	LidarVadProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber, size_t  nSegmentsMin = 10) : ConicalScanningProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]VAD_.._........_......\\.hpl$"), nSegmentsMin) {}
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("ppi") }; }
private:
	size_t m_nSegmentsMin;
};

class LidarWindVadProcessor : public ConicalScanningProcessor
{
public:
	LidarWindVadProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber, size_t  nSegmentsMin = 10) : ConicalScanningProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]Wind_Profile_.._........_......\\.hpl$"), nSegmentsMin) {}
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("wind ppi") }; }
private:
	size_t m_nSegmentsMin;
};

class LidarUserProcessor : public LidarScanningProcessor
{
public:
	LidarUserProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber, sci::string fileSearchRegEx) : LidarScanningProcessor(instrumentInfo, calibrationInfo, orientationGrabber, fileSearchRegEx) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
private:
	sci::string m_userNumber;
};

class LidarUser1Processor : public LidarUserProcessor
{
public:
	LidarUser1Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarUserProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]User1_.._........_......\\.hpl$")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 1") }; }
};

class LidarUser2Processor : public LidarUserProcessor
{
public:
	LidarUser2Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarUserProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]User2_.._........_......\\.hpl$")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 2") }; }
};

class LidarUser3Processor : public LidarUserProcessor
{
public:
	LidarUser3Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarUserProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]User3_.._........_......\\.hpl$")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 3") }; }
};

class LidarUser4Processor : public LidarUserProcessor
{
public:
	LidarUser4Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarUserProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]User4_.._........_......\\.hpl$")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 4") }; }
};

class LidarUser5Processor : public LidarUserProcessor
{
public:
	LidarUser5Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : LidarUserProcessor(instrumentInfo, calibrationInfo, orientationGrabber, sU("[/\\\\]User5_.._........_......\\.hpl$")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 5") }; }
};

class LidarWindProfileProcessor : public InstrumentProcessor
{
public:
	LidarWindProfileProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) :InstrumentProcessor(sU("[/\\\\]Processed_Wind_Profile_.._........_......\\.hpl$")), m_hasData(false), m_instrumentInfo(instrumentInfo), m_calibrationInfo(calibrationInfo), m_orientationGrabber(orientationGrabber) {}
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override { return m_hasData; }
	InstrumentInfo getInstrumentInfo() const { return m_instrumentInfo; }
	CalibrationInfo getCalibrationInfo() const { return m_calibrationInfo; }
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
private:
	bool m_hasData;
	struct Profile
	{
		Profile(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, std::shared_ptr<OrientationGrabber> orientationGrabber) : m_VadProcessor(instrumentInfo, calibrationInfo, orientationGrabber) {}
		std::vector<metre> m_heights;
		std::vector<degree> m_windDirections;
		std::vector<metrePerSecond> m_windSpeeds;
		InstrumentInfo m_instrumentInfo;
		//sci::UtcTime m_time;
		LidarWindVadProcessor m_VadProcessor; // have a separate Wind VAD processor for each profile
		std::vector<uint8_t> m_windFlags;
	};
	std::vector<Profile> m_profiles;
	const InstrumentInfo m_instrumentInfo;
	const CalibrationInfo m_calibrationInfo;
	std::shared_ptr<OrientationGrabber> m_orientationGrabber;
};



const uint8_t lidarUnusedFlag = 0;
const uint8_t lidarGoodDataFlag = 1;
const uint8_t lidarSnrBelow3Flag = 2;
const uint8_t lidarSnrBelow2Flag = 3;
const uint8_t lidarSnrBelow1Flag = 4;
const uint8_t lidarDopplerOutOfRangeFlag = 5;
const uint8_t lidarUserChangedGatesFlag = 6;
const uint8_t lidarClippedWindProfileFlag = 7;

const std::vector<std::pair<uint8_t,sci::string>> lidarDopplerFlags
{
{ lidarUnusedFlag, sU("not_used") },
{lidarGoodDataFlag, sU("good_data")},
{lidarSnrBelow3Flag, sU("SNR_less_than_3") },
{lidarSnrBelow2Flag, sU("SNR_less_than_2") },
{lidarSnrBelow1Flag, sU("SNR_less_than_1") },
{lidarDopplerOutOfRangeFlag, sU("doppler_velocity_out_of_+-_19_m_s-1_range") },
{lidarUserChangedGatesFlag, sU("User changed number of gates during the day so padding with fill value")},
{lidarClippedWindProfileFlag, sU("Wind profiles are clipped by manufacturer software so padding wih fill value")}
};