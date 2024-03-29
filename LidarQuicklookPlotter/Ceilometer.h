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
	const std::vector<perSteradianPerKilometreF> &getBetas() const { return m_profile.getData(); }
	std::vector<size_t> getGates() const { return sci::indexvector<size_t>(m_profile.getData().size()); }
	void setTime(const sci::UtcTime &time) { m_time = time; }
	const sci::UtcTime &getTime() const { return m_time; }
	metreF getCloudBase1() const { return m_profile.getHeight1(); }
	metreF getCloudBase2() const { return m_profile.getHeight2(); }
	metreF getCloudBase3() const { return m_profile.getHeight3(); }
	metreF getCloudBase4() const { return m_profile.getHeight4(); }
	metreF getVisibility() const { return m_profile.getVisibility(); }
	double getHighestSignal() const { return m_profile.getHighestSignal(); }
	percentF getWindowTransmission() const { return m_profile.getWindowTransmission(); }
	metreF getResolution() const { return m_profile.getResolution(); }
	percentF getLaserPulseEnergy() const { return m_profile.getLaserPulseEnergy(); }
	percentF getScale() const { return m_profile.getScale(); }
	kelvinF getLaserTemperature() const { return m_profile.getLaserTemperature(); }
	degreeF getTiltAngle() const { return m_profile.getTiltAngle(); }
	millivoltF getBackground() const { return m_profile.getBackground(); }
	size_t getPulseQuantity() const { return m_profile.getPulseQuantity(); }
	hertzF getSampleRate() const { return m_profile.getSampleRate(); }
	perSteradianF getSum() const { return m_profile.getSum(); }
	uint8_t getProfileFlag() const;
	std::vector<uint8_t> getGateFlags() const;
private:
	CampbellMessage2 m_profile;
	sci::UtcTime m_time;
};


inline HplHeader createCeilometerHeader(const sci::string &filename, const CampbellHeader &firstHeader, const CampbellCeilometerProfile &firstProfile)
{
	HplHeader result;
	result.dopplerResolution = std::numeric_limits<metrePerSecondF>::quiet_NaN();
	result.filename = filename;
	result.focusRange = std::numeric_limits<metreF>::quiet_NaN();
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
	CeilometerProcessor(const InstrumentInfo& instrumentInfo, const CalibrationInfo& calibrationInfo)
		:PlotableLidar(sci::string(sU("[/\\\\]........_ceilometer\\.csv$"))), m_hasData(false), m_instrumentInfo(instrumentInfo), m_calibrationInfo(calibrationInfo)
	{}
	virtual void writeToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles,
		sci::string directory, const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo,
		const ProjectInfo &projectInfo, const Platform &platform, const ProcessingOptions &processingOptions);

	void readDataAndPlot(const std::string &inputFilename, const std::string &outputFilename,
		const std::vector<double> maxRanges, ProgressReporter &progressReporter, wxWindow *parent);

	void plotCeilometerProfiles(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles,
		sci::string filename, metreF maxRange, ProgressReporter &progressReporter, wxWindow *parent);

	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) override;
	virtual void readData(const sci::string &inputFilename, ProgressReporter &progressReporter, bool clearPrevious);
	virtual void plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override;
	virtual void writeToNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter) override;
	void writeToNc_1_1_0(const HplHeader& header, const std::vector<CampbellCeilometerProfile>& profiles,
		sci::string directory, const PersonInfo& author, const ProcessingSoftwareInfo& processingSoftwareInfo,
		const ProjectInfo& projectInfo, const Platform& platform, const ProcessingOptions& processingOptions);
	void writeToNc_2_0_0(const HplHeader& header, const std::vector<CampbellCeilometerProfile>& profiles,
		sci::string directory, const PersonInfo& author, const ProcessingSoftwareInfo& processingSoftwareInfo,
		const ProjectInfo& projectInfo, const Platform& platform, const ProcessingOptions& processingOptions);
	virtual bool hasData() const override { return m_hasData; }
	std::vector<std::vector<sci::string>> groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
	sci::string getName() const override
	{
		return sU("Ceilometer Processor");
	}
private:
	std::vector<CampbellCeilometerProfile> m_allData;
	CampbellHeader m_firstHeaderCampbell;
	HplHeader m_firstHeaderHpl;
	bool m_hasData;
	std::vector<sci::string> m_inputFilenames;
	void formatDataForOutput(const HplHeader& header,
		const std::vector<CampbellCeilometerProfile>& profiles,
		InstrumentInfo& ceilometerInfo,
		CalibrationInfo& ceilometerCalibrationInfo,
		DataInfo& dataInfo, sci::GridData<sci::UtcTime, 1>& times,
		sci::GridData<metreF, 2>& altitudesAboveInstrument,
		sci::GridData<perSteradianPerMetreF, 2>& backscatter,
		sci::GridData<metreF, 1>& cloudBase1, sci::GridData<metreF, 1>& cloudBase2, sci::GridData<metreF, 1>& cloudBase3,
		sci::GridData<metreF, 1>& cloudBase4, sci::GridData<percentF, 1>& laserEnergies, sci::GridData<kelvinF, 1>& laserTemperatures,
		sci::GridData<unitlessF, 1>& pulseQuantities, sci::GridData<degreeF, 1>& tiltAngles, sci::GridData<percentF, 1>& scales,
		sci::GridData<percentF, 1>& windowTransmissions, sci::GridData<millivoltF, 1>& windowContaminations, sci::GridData<millivoltF, 1>& backgrounds, sci::GridData<perSteradianF, 1>& sums,
		sci::GridData<uint8_t, 1>& profileFlags, sci::GridData<uint8_t, 2>& gateFlags, sci::GridData<uint8_t, 1>& cloudBaseFlags);
	CalibrationInfo m_calibrationInfo;
	InstrumentInfo m_instrumentInfo;
	sci::string m_ceilometerOsVersion;
};
