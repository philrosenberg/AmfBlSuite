#pragma once
#include<svector/time.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include<svector/serr.h>
#include"InstrumentProcessor.h"
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
	PlotableLidar(const sci::string &fileSearchRegex) : InstrumentProcessor(fileSearchRegex) {}
	void setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent, HplHeader hplHeader);
};

class HplFileLidar : public PlotableLidar
{
public:
	HplFileLidar(const sci::string &filePrefix, bool inCrossFolder, bool twoDigitTime)
		//constructing the regex is a bit complicated we need to specify either the cross or not cross directory and
		//2 or 6 digit time for stare/non-stare
		//:PlotableLidar(sci::string(sU("Proc[/\\\\]")) +
		//(inCrossFolder ? sU("cross[/\\\\]") : sU("")) +
		//	sU("....[/\\\\]......[/\\\\]........[/\\\\]") +
		//	filePrefix + sU("_.{0,4}.{6,8}_") +
		//	(twoDigitTime ? sU("..") : sU("......")) +
		//	sU("\\.hpl$")),
		: PlotableLidar(sci::string(sU("Proc[/\\\\]")) +
		(inCrossFolder ? sU("cross[/\\\\]") : sU("")) +
			sU("....[/\\\\]......[/\\\\]........[/\\\\]") +
			filePrefix + sU("_.*\\.hpl$")),
		m_filePrefix(filePrefix)
	{}
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override
	{
		sci::UtcTime fileTime = extractDateFromLidarFilename(fileName);
		return fileTime < endTime && fileTime >= startTime;
	}
	std::vector<std::vector<sci::string>> groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	sci::string getName() const override
	{
		return sU("Lidar Processor");
	}
private:
	sci::UtcTime extractDateFromLidarFilename(const sci::string &filename) const;
	const sci::string m_filePrefix;
};

class LidarBackscatterDopplerProcessor : public HplFileLidar
{
public:
	LidarBackscatterDopplerProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, const sci::string &filePrefix, bool inCrossFolder, bool twoDigitTime) : HplFileLidar(filePrefix, inCrossFolder, twoDigitTime), m_hasData(false), m_instrumentInfo(instrumentInfo), m_calibrationInfo(calibrationInfo) {}
	virtual ~LidarBackscatterDopplerProcessor() {}
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) override;
	void readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter, bool clear);
	virtual bool hasData() const override { return m_hasData; }
	virtual std::vector<sci::string> getProcessingOptions() const = 0;
	std::vector<second> getTimesSeconds() const;
	std::vector<sci::UtcTime> getTimesUtcTime() const;
	std::vector<std::vector<perSteradianPerMetreF>> getBetas() const;
	std::vector<std::vector<metrePerSecondF>> getInstrumentRelativeDopplerVelocities() const;
	std::vector<std::vector<metrePerSecondF>> getMotionCorrectedDopplerVelocities() const { return m_correctedDopplerVelocities; }
	std::vector<std::vector<unitlessF>> getSignalToNoiseRatios() const;
	std::vector<std::vector<unitlessF>> getSignalToNoiseRatiosPlusOne() const;
	std::vector<std::vector<uint8_t>> getDopplerFlags() const { return m_dopplerFlags; }
	std::vector<std::vector<uint8_t>> getBetaFlags() const { return m_betaFlags; }
	std::vector<metreF> getGateBoundariesForPlotting(size_t profileIndex) const;
	std::vector<metreF> getGateLowerBoundaries(size_t profileIndex) const;
	std::vector<metreF> getGateUpperBoundaries(size_t profileIndex) const;
	std::vector<metreF> getGateCentres(size_t profileIndex) const;
	std::vector<degreeF> getAttitudeCorrectedAzimuths() const { return m_correctedAzimuths; }
	std::vector<degreeF> getInstrumentRelativeElevations() const;
	std::vector<degreeF> getInstrumentRelativeAzimuths() const;
	std::vector<degreeF> getAttitudeCorrectedElevations() const { return m_correctedElevations; }
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
	size_t getPulsesPerRay(size_t profile) { return m_hplHeaders[m_headerIndex[profile]].pulsesPerRay; }
	size_t getNRays(size_t profile) { return m_hplHeaders[m_headerIndex[profile]].nRays; }
	metreF getFocus(size_t profile) { return m_hplHeaders[m_headerIndex[profile]].focusRange; }
	metrePerSecondF getDopplerResolution(size_t profile) { return m_hplHeaders[m_headerIndex[profile]].dopplerResolution; }
	metreF getGateLength(size_t profile) { return m_hplHeaders[m_headerIndex[profile]].rangeGateLength; }
	size_t getNPointsPerGate(size_t profile) { return m_hplHeaders[m_headerIndex[profile]].pointsPerGate; }
private:
	bool m_hasData;
	std::vector<HplHeader> m_hplHeaders;
	std::vector<HplProfile> m_profiles;
	std::vector<std::vector<uint8_t>> m_dopplerFlags;
	std::vector<std::vector<uint8_t>> m_betaFlags;
	std::vector<size_t> m_headerIndex; //this links headers to profiles m_hplHeaders[m_headerIndex[i]] is the header for the ith profile
	const InstrumentInfo m_instrumentInfo;
	const CalibrationInfo m_calibrationInfo;
	std::vector<degreeF> m_correctedAzimuths;
	std::vector<degreeF> m_correctedElevations;
	std::vector<std::vector<metrePerSecondF>> m_correctedDopplerVelocities;
};

class LidarScanningProcessor : public LidarBackscatterDopplerProcessor
{
public:
	LidarScanningProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, const sci::string &filePrefix, bool inCrossFolder, bool twoDigitTime)
		:LidarBackscatterDopplerProcessor(instrumentInfo, calibrationInfo, filePrefix, inCrossFolder, twoDigitTime)
	{}
	void formatDataForOutput(ProgressReporter& progressReporter,
		std::vector<std::vector<std::vector<metreF>>>& ranges,
		std::vector<std::vector<degreeF>>& instrumentRelativeAzimuthAngles,
		std::vector<std::vector<degreeF>>& attitudeCorrectedAzimuthAngles,
		std::vector<std::vector<degreeF>>& instrumentRelativeElevationAngles,
		std::vector<std::vector<degreeF>>& attitudeCorrectedElevationAngles,
		std::vector<std::vector<std::vector<metrePerSecondF>>>& instrumentRelativeDopplerVelocities,
		std::vector<std::vector<std::vector<metrePerSecondF>>>& motionCorrectedDopplerVelocities,
		std::vector<std::vector<std::vector<perSteradianPerMetreF>>>& backscatters,
		std::vector<std::vector<std::vector<unitlessF>>>& snrsPlusOne,
		std::vector < std::vector<std::vector<uint8_t>>>& dopplerVelocityFlags,
		std::vector < std::vector<std::vector<uint8_t>>>& backscatterFlags,
		std::vector<sci::UtcTime>& scanStartTimes,
		std::vector<sci::UtcTime>& scanEndTimes,
		size_t& maxProfilesPerScan,
		size_t& maxNGates,
		size_t& pulsesPerRay,
		size_t& raysPerPoint,
		metreF& focus,
		metrePerSecondF& dopplerResolution,
		metreF& gateLength,
		size_t& pointsPerGate);
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) override;
};

class LidarStareProcessor : public LidarScanningProcessor
{
public:
	LidarStareProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, bool inCrossFolder) : LidarScanningProcessor(instrumentInfo, calibrationInfo, sU("Stare"), inCrossFolder, true) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	/*virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) override;
	void getFormattedData(std::vector<sci::UtcTime> &times,
		std::vector<degreeF> &instrumentRelativeAzimuthAngles,
		std::vector<degreeF> &instrumentRelativeElevationAngles,
		std::vector<std::vector<metrePerSecondF>> &instrumentRelativeDopplerVelocities,
		std::vector<degreeF> &attitudeCorrectedAzimuthAngles,
		std::vector<degreeF> &attitudeCorrectedElevationAngles,
		std::vector<std::vector<metrePerSecondF>> &motionCorrectedDopplerVelocities,
		std::vector<std::vector<perSteradianPerMetreF>> &backscatters,
		std::vector<std::vector<unitlessF>> &snrPlusOne,
		std::vector<std::vector<uint8_t>> &dopplerVelocityFlags,
		std::vector<std::vector<uint8_t>> &backscatterFlags,
		std::vector<std::vector<metreF>> &ranges,
		secondF &averagingPeriod,
		secondF &samplingInterval);*/
};

class LidarCopolarisedStareProcessor : public LidarStareProcessor
{
public:
	LidarCopolarisedStareProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : LidarStareProcessor(instrumentInfo, calibrationInfo, false) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("fixed"), sU("co") }; }
};

class LidarCrosspolarisedStareProcessor : public LidarStareProcessor
{
public:
	LidarCrosspolarisedStareProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : LidarStareProcessor(instrumentInfo, calibrationInfo, true) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("fixed"), sU("cr") }; }
};

class LidarRhiProcessor : public LidarScanningProcessor
{
public:
	LidarRhiProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : LidarScanningProcessor(instrumentInfo, calibrationInfo, sU("RHI"), false, false) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("rhi") }; }
};



class ConicalScanningProcessor : public LidarScanningProcessor
{
public:
	ConicalScanningProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, const sci::string &filePrefix, size_t  nSegmentsMin = 10) : LidarScanningProcessor(instrumentInfo, calibrationInfo, filePrefix, false, false), m_nSegmentsMin(nSegmentsMin) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	void plotDataPlan(const sci::string &outputFilename, metreF maxRange, ProgressReporter &progressReporter, wxWindow *parent);
	void plotDataCone(const sci::string &outputFilename, metreF maxRange, ProgressReporter &progressReporter, wxWindow *parent);
	void plotDataCone(degreeF viewAzimuth, metreF maxRange, splot2d * plot);
	void plotDataUnwrapped(const sci::string &outputFilename, metreF maxRange, ProgressReporter &progressReporter, wxWindow *parent);
private:
	size_t m_nSegmentsMin;
	void getDataSortedByAzimuth(std::vector<std::vector<perSteradianPerMetreF>> &sortedBetas, std::vector<degreeF> &sortedElevations, std::vector<degreeF> &sortedMidAzimuths, std::vector<degreeF> &azimuthBoundaries);
};

class LidarVadProcessor : public ConicalScanningProcessor
{
public:
	LidarVadProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, size_t  nSegmentsMin = 10) : ConicalScanningProcessor(instrumentInfo, calibrationInfo, sU("VAD"), nSegmentsMin) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("ppi") }; }
private:
};

class LidarWindVadProcessor : public ConicalScanningProcessor
{
public:
	LidarWindVadProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, size_t  nSegmentsMin = 10) : ConicalScanningProcessor(instrumentInfo, calibrationInfo, sU("Wind_Profile"), nSegmentsMin) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("wind ppi") }; }
private:
};

class LidarUserProcessor : public LidarScanningProcessor
{
public:
	LidarUserProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo, const sci::string &filePrefix) : LidarScanningProcessor(instrumentInfo, calibrationInfo, filePrefix, false, false) {}
	virtual void plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
private:
	sci::string m_userNumber;
};

class LidarUser1Processor : public LidarUserProcessor
{
public:
	LidarUser1Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : LidarUserProcessor(instrumentInfo, calibrationInfo, sU("User1")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 1") }; }
};

class LidarUser2Processor : public LidarUserProcessor
{
public:
	LidarUser2Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : LidarUserProcessor(instrumentInfo, calibrationInfo, sU("User2")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 2") }; }
};

class LidarUser3Processor : public LidarUserProcessor
{
public:
	LidarUser3Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : LidarUserProcessor(instrumentInfo, calibrationInfo, sU("User3$")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 3") }; }
};

class LidarUser4Processor : public LidarUserProcessor
{
public:
	LidarUser4Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : LidarUserProcessor(instrumentInfo, calibrationInfo, sU("User4")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 4") }; }
};

class LidarUser5Processor : public LidarUserProcessor
{
public:
	LidarUser5Processor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : LidarUserProcessor(instrumentInfo, calibrationInfo, sU("User5")) {}
	virtual std::vector<sci::string> getProcessingOptions() const override { return { sU("user 5") }; }
};

class LidarWindProfileProcessor : public HplFileLidar
{
public:
	LidarWindProfileProcessor(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) :HplFileLidar(sU("Processed_Wind_Profile"), false, false), m_hasData(false), m_instrumentInfo(instrumentInfo), m_calibrationInfo(calibrationInfo) {}
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) override;
	virtual void plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override { return m_hasData; }
	InstrumentInfo getInstrumentInfo() const { return m_instrumentInfo; }
	CalibrationInfo getCalibrationInfo() const { return m_calibrationInfo; }
private:
	bool m_hasData;
	struct Profile
	{
		Profile(InstrumentInfo instrumentInfo, CalibrationInfo calibrationInfo) : m_VadProcessor(instrumentInfo, calibrationInfo) {}
		std::vector<metreF> m_heights;
		std::vector<degreeF> m_instrumentRelativeWindDirections;
		std::vector<metrePerSecondF> m_instrumentRelativeWindSpeeds;
		std::vector<degreeF> m_motionCorrectedWindDirections;
		std::vector<metrePerSecondF> m_motionCorrectedWindSpeeds;
		InstrumentInfo m_instrumentInfo;
		//sci::UtcTime m_time;
		LidarWindVadProcessor m_VadProcessor; // have a separate Wind VAD processor for each profile
		std::vector<uint8_t> m_windFlags;
	};
	std::vector<Profile> m_profiles;
	const InstrumentInfo m_instrumentInfo;
	const CalibrationInfo m_calibrationInfo;
};

class LidarDepolProcessor : public PlotableLidar
{
public:
	LidarDepolProcessor(const LidarCopolarisedStareProcessor &copolarisedProcessor, LidarCrosspolarisedStareProcessor crosspolarisedProcessor) : PlotableLidar(copolarisedProcessor.getFileSearchRegex() + sU("|") + crosspolarisedProcessor.getFileSearchRegex()), m_copolarisedProcessor(copolarisedProcessor), m_crosspolarisedProcessor(crosspolarisedProcessor) {}
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) override;
	virtual void plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter) override;
	virtual bool hasData() const override { return m_copolarisedProcessor.hasData() &&m_crosspolarisedProcessor.hasData(); }
	virtual bool InstrumentProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override
	{
		if(isCrossFile(fileName))
			return m_crosspolarisedProcessor.fileCoversTimePeriod(fileName, startTime, endTime);
		else
			return m_copolarisedProcessor.fileCoversTimePeriod(fileName, startTime, endTime);
	}
	virtual std::vector<std::vector<sci::string>> groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	sci::string getName() const override
	{
		return sU("Lidar Depolarisation Processor");
	}
private:
	LidarCopolarisedStareProcessor m_copolarisedProcessor;
	LidarCrosspolarisedStareProcessor m_crosspolarisedProcessor;
	bool isCrossFile(const sci::string &fileName) const;
};


const uint8_t lidarUnusedFlag = 0;
const uint8_t lidarGoodDataFlag = 1;
const uint8_t lidarSnrBelow3Flag = 2;
const uint8_t lidarSnrBelow2Flag = 3;
const uint8_t lidarSnrBelow1Flag = 4;
const uint8_t lidarDopplerOutOfRangeFlag = 5;
const uint8_t lidarUserChangedGatesFlag = 6;
const uint8_t lidarClippedWindProfileFlag = 7;
const uint8_t lidarPaddedBackscatter = 8;
const uint8_t lidarNonMatchingRanges = 9;

const std::vector<std::pair<uint8_t, sci::string>> lidarDopplerFlags
{
{ lidarUnusedFlag, sU("not_used") },
{lidarGoodDataFlag, sU("good_data")},
{lidarSnrBelow3Flag, sU("SNR less than 3") },
{lidarSnrBelow2Flag, sU("SNR less_than 2") },
{lidarSnrBelow1Flag, sU("SNR less_than 1") },
{lidarDopplerOutOfRangeFlag, sU("doppler velocity out of +- 19 m s-1 range") },
{lidarUserChangedGatesFlag, sU("user changed number of gates during the day so padding with fill value")},
{lidarClippedWindProfileFlag, sU("wind profiles are clipped by manufacturer software so padding wih fill value")},
{lidarPaddedBackscatter, sU("padded crosspolarised or copolarised data to match the other in dimension size")},
{lidarNonMatchingRanges, sU("crosspolarised and copolarised data do not have matching ranges or directions")}
};