#pragma once
#include "InstrumentProcessor.h"
#include"AmfNc.h"

enum MicroRainRadarProfileType
{
	MRRPT_averaged,
	MRRPT_processed,
	MRRPT_raw
};

class MicroRainRadarProfile
{
public:
	bool readProfile(std::istream &stream);
	sci::UtcTime getTime() const { return m_time; }
	second getAveragingTime() const { return m_averagingTime; }
	percent getValidFraction() const { return m_validFraction; }
	std::vector<metre> getRanges() const { return m_ranges; }
	std::vector<unitless> getPathIntegratedAttenuation() const { return m_pathIntegratedAttenuation; }
	std::vector<reflectivity> getReflectivity() const { return m_reflectivity; }
	std::vector< reflectivity> getReflectivityAttenuationCorrected() const { return m_reflectivityAttenuationCorrected; }
	std::vector<millimetrePerHour> getRainRate() const { return m_rainRate; }
	std::vector<gramPerMetreCubed> getLiquidWaterContent() const { return m_liquidWaterContent; }
	std::vector<metrePerSecond> getFallVeocity() const { return m_fallVelocity; }
	std::vector<std::vector<perMetre>> getSpectralReflectivities() { return m_spectralReflectivities; }
	std::vector<std::vector<millimetre>> getDropDiameters() { return m_dropDiameters; }
	std::vector<std::vector<perMetreCubedPerMillimetre>> getNumberDistribution() { return m_numberDistribution; }
private:

	template<class T>
	std::vector<T> readDataLine(std::istream &stream, std::string expectedPrefix)
	{
		std::string line;
		std::getline(stream, line);
		sci::assertThrow(line.length() == 220, sci::err(sci::SERR_USER, 0, sU("Micro rain radar data line found with the wrong length.")));
		sci::assertThrow(line.substr(0, 3) == expectedPrefix, sci::err(sci::SERR_USER, 0, sU("Micro rain radar data line found with an unexpected prefix.")));

		std::vector<T> result(31);
		for (size_t i = 0; i < result.size(); ++i)
		{
			bool empty = true;
			for (size_t j = 0; j < 7; ++j)
				empty = empty && line[3 + i * 7 + j] == ' ';
			if (empty)
				result[i] = std::numeric_limits<T>::quiet_NaN();
			else
				result[i] = T((T::valueType)atof(line.substr(3 + i * 7, 7).c_str()));
		}
		return result;
	}
	template<class T>
	std::vector<T> readAndUndbDataLine(std::istream &stream, std::string expectedPrefix)
	{
		std::string line;
		std::getline(stream, line);
		sci::assertThrow(line.length() == 220, sci::err(sci::SERR_USER, 0, sU("Micro rain radar data line found with the wrong length.")));
		sci::assertThrow(line.substr(0, 3) == expectedPrefix, sci::err(sci::SERR_USER, 0, sU("Micro rain radar data line found with an unexpected prefix.")));

		std::vector<T> result(31);
		for (size_t i = 0; i < result.size(); ++i)
		{
			bool empty = true;
			for (size_t j = 0; j < 7; ++j)
				empty = empty && line[3 + i * 7 + j] == ' ';
			if (empty)
				result[i] = std::numeric_limits<T>::quiet_NaN();
			else
				result[i] = T((T::valueType)std::pow(10, atof(line.substr(3 + i * 7, 7).c_str()) / 10.0));
		}
		return result;
	}

	//header parameters
	sci::UtcTime m_time;
	second m_averagingTime;
	metre m_heightResolution;
	metre m_instrumentHeight;
	hertz m_samplingRate;
	sci::string m_serviceVersionNumber;
	sci::string m_firmwareVersionNumber;
	sci::string m_serialNumber;
	unitless m_calibrationConstant;
	percent m_validFraction;
	MicroRainRadarProfileType m_profileType;

	//Height dependent parameters
	std::vector<metre> m_ranges;
	std::vector<unitless> m_transferFunction;
	std::vector <std::vector<perMetre>> m_spectralReflectivities;
	std::vector<std::vector<millimetre>> m_dropDiameters;
	std::vector<std::vector<perMetreCubedPerMillimetre>> m_numberDistribution;
	std::vector<unitless> m_pathIntegratedAttenuation;
	std::vector<reflectivity> m_reflectivity;
	std::vector< reflectivity> m_reflectivityAttenuationCorrected;
	std::vector<millimetrePerHour> m_rainRate;
	std::vector<gramPerMetreCubed> m_liquidWaterContent;
	std::vector<metrePerSecond> m_fallVelocity;
};

class MicroRainRadarProcessor : public InstrumentProcessor
{
public:
	MicroRainRadarProcessor(const InstrumentInfo &instrumentInfo, const CalibrationInfo &calibrationInfo);
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) override;
	void readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter, bool clear);
	virtual void plotData(const sci::string &baseOutputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override
	{
		throw(sci::err(sci::SERR_USER, 0, "Micro Rain Radar plotting is not yet supported."));
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
	std::vector<MicroRainRadarProfile> m_profiles;
	InstrumentInfo m_instrumentInfo;
	CalibrationInfo m_calibrationInfo;
	bool m_hasData;
};