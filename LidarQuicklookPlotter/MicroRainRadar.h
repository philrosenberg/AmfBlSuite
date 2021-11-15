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
	percentF getValidFraction() const { return m_validFraction; }
	std::vector<metreF> getRanges() const { return m_ranges; }
	std::vector<unitlessF> getPathIntegratedAttenuation() const { return m_pathIntegratedAttenuation; }
	std::vector<reflectivityF> getReflectivity() const { return m_reflectivity; }
	std::vector< reflectivityF> getReflectivityAttenuationCorrected() const { return m_reflectivityAttenuationCorrected; }
	std::vector<millimetrePerHourF> getRainRate() const { return m_rainRate; }
	std::vector<gramPerMetreCubedF> getLiquidWaterContent() const { return m_liquidWaterContent; }
	std::vector<metrePerSecondF> getFallVeocity() const { return m_fallVelocity; }
	std::vector<std::vector<perMetreF>> getSpectralReflectivities() { return m_spectralReflectivities; }
	std::vector<std::vector<millimetreF>> getDropDiameters() { return m_dropDiameters; }
	std::vector<std::vector<perMetreCubedPerMillimetreF>> getNumberDistribution() { return m_numberDistribution; }
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
				result[i] = T((typename T::valueType)atof(line.substr(3 + i * 7, 7).c_str()));
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
				result[i] = T((typename T::valueType)std::pow(10, atof(line.substr(3 + i * 7, 7).c_str()) / 10.0));
		}
		return result;
	}

	//header parameters
	sci::UtcTime m_time;
	second m_averagingTime;
	metreF m_heightResolution;
	metreF m_instrumentHeight;
	hertzF m_samplingRate;
	sci::string m_serviceVersionNumber;
	sci::string m_firmwareVersionNumber;
	sci::string m_serialNumber;
	unitlessF m_calibrationConstant;
	percentF m_validFraction;
	MicroRainRadarProfileType m_profileType;

	//Height dependent parameters
	std::vector<metreF> m_ranges;
	std::vector<unitlessF> m_transferFunction;
	std::vector <std::vector<perMetreF>> m_spectralReflectivities;
	std::vector<std::vector<millimetreF>> m_dropDiameters;
	std::vector<std::vector<perMetreCubedPerMillimetreF>> m_numberDistribution;
	std::vector<unitlessF> m_pathIntegratedAttenuation;
	std::vector<reflectivityF> m_reflectivity;
	std::vector< reflectivityF> m_reflectivityAttenuationCorrected;
	std::vector<millimetrePerHourF> m_rainRate;
	std::vector<gramPerMetreCubedF> m_liquidWaterContent;
	std::vector<metrePerSecondF> m_fallVelocity;
};

class MicroRainRadarProcessor : public InstrumentProcessor
{
public:
	MicroRainRadarProcessor(const InstrumentInfo &instrumentInfo, const CalibrationInfo &calibrationInfo);
	virtual void readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter) override;
	void readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter, bool clear);
	virtual void plotData(const sci::string &baseOutputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) override
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
	virtual std::vector<std::vector<sci::string>> groupInputFilesbyOutputFiles(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const override;
	sci::string getName() const override
	{
		return sU("Micro Rain Radar Processor");
	}
private:
	std::vector<MicroRainRadarProfile> m_profiles;
	InstrumentInfo m_instrumentInfo;
	CalibrationInfo m_calibrationInfo;
	bool m_hasData;
};