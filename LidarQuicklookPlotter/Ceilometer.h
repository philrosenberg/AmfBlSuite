#pragma once
#include"Campbell.h"
#include<svector/time.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include"HplHeader.h"
#include"Units.h"
#include"AmfNc.h"
#include"Plotting.h"
#include"Lidar.h"

const uint8_t ceilometerUnusedFlag = 0;
const uint8_t ceilometerGoodFlag = 1;
const uint8_t ceilometerFilteredNoiseFlag = 2;
const uint8_t ceilometerLowSignalFlag = 3;
const uint8_t ceilometerPaddingFlag = 4;
const uint8_t ceilometerRawDataMissingOrSuspectFlag = 5;
const uint8_t ceilometerSomeObscurationTransparentFlag = 6;
const uint8_t ceilometerFullObscurationNoCloudBaseFlag = 7;
const uint8_t ceilometerNoSignificantBackscatterFlag = 8;


const std::vector<std::pair<uint8_t, sci::string>> ceilometerFlags
{
{ ceilometerUnusedFlag, sU("not_used") },
{ceilometerGoodFlag, sU("good_data")},
{ceilometerFilteredNoiseFlag, sU("Data filtered by instrument as below noise level") },
{ceilometerLowSignalFlag, sU("Data is below the level that it would be filtered by the instrument if filtering was on") },
{ceilometerPaddingFlag, sU("Data is padded because the number of gates changed during this file") },
{ceilometerRawDataMissingOrSuspectFlag, sU("Data is missing or suspect")},
{ceilometerSomeObscurationTransparentFlag, sU("Some obscuration detected but determined to be transparent")},
{ceilometerFullObscurationNoCloudBaseFlag, sU("Full obscuration determined but no cloud base detected")},
{ceilometerNoSignificantBackscatterFlag, sU("No significant back-scatter")}
};

class CampbellCeilometerProfile
{
public:
	CampbellCeilometerProfile(const sci::UtcTime &time, const CampbellMessage2 &profile)
		:m_time(time), m_profile(profile)
	{}
	const std::vector<perSteradianPerKilometre> &getBetas() const { return m_profile.getData(); }
	std::vector<size_t> getGates() const { return sci::indexvector<size_t>(m_profile.getData().size()); }
	void setTime(const sci::UtcTime &time) { m_time = time; }
	const sci::UtcTime &getTime() const { return m_time; }
	metre getCloudBase1() const { return m_profile.getHeight1(); }
	metre getCloudBase2() const { return m_profile.getHeight2(); }
	metre getCloudBase3() const { return m_profile.getHeight3(); }
	metre getCloudBase4() const { return m_profile.getHeight4(); }
	metre getVisibility() const { return m_profile.getVisibility(); }
	double getHighestSignal() const { return m_profile.getHighestSignal(); }
	percent getWindowTransmission() const { return m_profile.getWindowTransmission(); }
	metre getResolution() const { return m_profile.getResolution(); }
	percent getLaserPulseEnergy() const { return m_profile.getLaserPulseEnergy(); }
	percent getScale() const { return m_profile.getScale(); }
	kelvin getLaserTemperature() const { return m_profile.getLaserTemperature(); }
	degree getTiltAngle() const { return m_profile.getTiltAngle(); }
	millivolt getBackground() const { return m_profile.getBackground(); }
	size_t getPulseQuantity() const { return m_profile.getPulseQuantity(); }
	hertz getSampleRate() const { return m_profile.getSampleRate(); }
	perSteradian getSum() const { return m_profile.getSum(); }
	uint8_t getProfileFlag() const;
	std::vector<uint8_t> getGateFlags() const;
private:
	CampbellMessage2 m_profile;
	sci::UtcTime m_time;
};


inline HplHeader createCeilometerHeader(const sci::string &filename, const CampbellHeader &firstHeader, const CampbellCeilometerProfile &firstProfile)
{
	HplHeader result;
	result.dopplerResolution = std::numeric_limits<double>::quiet_NaN();
	result.filename = filename;
	result.focusRange = 0;
	result.nGates = 2046;
	result.nRays = 1;
	result.pointsPerGate = 1;
	result.pulsesPerRay = firstProfile.getPulseQuantity();
	result.rangeGateLength = firstProfile.getResolution();
	result.scanType = ScanType::stare;
	result.startTime = firstProfile.getTime();
	result.systemId = firstHeader.getId();

	return result;
}

class ProgressReporter;
class wxWindow;

class CeilometerProcessor : public PlotableLidar
{
public:
	CeilometerProcessor::CeilometerProcessor()
		:PlotableLidar(sU("[/\\\\]........_ceilometer\\.csv$")), m_hasData(false)
	{}
	static void writeToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles,
		sci::string directory, const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo,
		const ProjectInfo &projectInfo, const Platform &platform, const ProcessingOptions &processingOptions);

	void readDataAndPlot(const std::string &inputFilename, const std::string &outputFilename,
		const std::vector<double> maxRanges, ProgressReporter &progressReporter, wxWindow *parent);

	void plotCeilometerProfiles(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles,
		sci::string filename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent);

	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) override;
	virtual void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, bool clearPrevious);
	virtual void plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter) override;
	static void writeToNc_1_1_0(const HplHeader& header, const std::vector<CampbellCeilometerProfile>& profiles,
		sci::string directory, const PersonInfo& author, const ProcessingSoftwareInfo& processingSoftwareInfo,
		const ProjectInfo& projectInfo, const Platform& platform, const ProcessingOptions& processingOptions);
	static void writeToNc_2_0_0(const HplHeader& header, const std::vector<CampbellCeilometerProfile>& profiles,
		sci::string directory, const PersonInfo& author, const ProcessingSoftwareInfo& processingSoftwareInfo,
		const ProjectInfo& projectInfo, const Platform& platform, const ProcessingOptions& processingOptions);
	virtual bool hasData() const override { return m_hasData; }
	std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
private:
	std::vector<CampbellCeilometerProfile> m_allData;
	CampbellHeader m_firstHeaderCampbell;
	HplHeader m_firstHeaderHpl;
	bool m_hasData;
	std::vector<sci::string> m_inputFilenames;
	static void formatDataForOutput(const HplHeader& header, const std::vector<CampbellCeilometerProfile>& profiles,
		InstrumentInfo& ceilometerInfo, CalibrationInfo& ceilometerCalibrationInfo,
		DataInfo& dataInfo, std::vector<sci::UtcTime>& times, std::vector<std::vector<metre>>& altitudesAboveInstrument,
		std::vector<std::vector<perSteradianPerMetre>> &backscatter,
		std::vector<metre> &cloudBase1, std::vector<metre> &cloudBase2, std::vector<metre> &cloudBase3,
		std::vector<metre> &cloudBase4, std::vector<percent> &laserEnergies, std::vector<kelvin> &laserTemperatures,
		std::vector<unitless> &pulseQuantities, std::vector<degree> &tiltAngles, std::vector<percent> &scales,
		std::vector<percent> &windowTransmissions, std::vector<millivolt> &backgrounds, std::vector<perSteradian> &sums,
		std::vector<uint8_t> &profileFlags, std::vector<std::vector<uint8_t>> &gateFlags, std::vector<uint8_t>& cloudBaseFlags);
};
