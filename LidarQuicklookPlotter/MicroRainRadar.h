#pragma once
#include "InstrumentProcessor.h"
#include"AmfNc.h"
#include<svector/array.h>
#include<strstream>

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
	const sci::GridData<metreF, 1> &getRanges() const { return m_ranges; }
	const sci::GridData<unitlessF, 1> &getPathIntegratedAttenuation() const { return m_pathIntegratedAttenuation; }
	const sci::GridData<reflectivityF, 1> &getReflectivity() const { return m_reflectivity; }
	const sci::GridData< reflectivityF, 1> &getReflectivityAttenuationCorrected() const { return m_reflectivityAttenuationCorrected; }
	const sci::GridData<millimetrePerHourF, 1> &getRainRate() const { return m_rainRate; }
	const sci::GridData<gramPerMetreCubedF, 1> &getLiquidWaterContent() const { return m_liquidWaterContent; }
	const sci::GridData<metrePerSecondF, 1> &getFallVeocity() const { return m_fallVelocity; }
	const sci::GridData<perMetreF, 2> &getSpectralReflectivities() { return m_spectralReflectivities; }
	const sci::GridData<millimetreF, 2> &getDropDiameters() { return m_dropDiameters; }
	const sci::GridData<perMetreCubedPerMillimetreF, 2> &getNumberDistribution() { return m_numberDistribution; }
private:

	template<class T>
	sci::GridData<T, 1> readDataLine(std::istream &stream, std::string expectedPrefix)
	{
		std::string line;
		std::getline(stream, line);
		//all lines should have 220 characters, but i have occasionally found crazy TF lines that don't follow the correct pattern
		sci::assertThrow(line.length() == 220 || expectedPrefix =="TF ", sci::err(sci::SERR_USER, 0, sU("Micro rain radar data line found with the wrong length.")));
		sci::assertThrow(line.substr(0, 3) == expectedPrefix, sci::err(sci::SERR_USER, 0, sU("Micro rain radar data line found with an unexpected prefix.")));

		sci::GridData<T, 1> result(31);
		if (expectedPrefix == "TF ")
		{
			//all lines should have fixed width of 7 characters per value.
			//TF values should all be between 1 and zero. They are actually output
			//with a delimiting space then 4 digits after the point. This should be
			//the same as 7 characters fixed width. However, occasionally something
			//goes a bit haywire and we get values outside the range 0-1 breaking
			//the fixed width. Hence for TF lines we use the delimiter the read in
			//using the << operator instead.
			std::istringstream tfLineStream(line.substr(3));
			for (size_t i = 0; i < result.size(); ++i)
				tfLineStream >> result[i];
		}
		else
		{
			//the lines should all be fixed width with 7 characters
			//per value and a 3 character identifier to start the line
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
		}
		return result;
	}
	template<class T>
	sci::GridData<T, 1> readAndUndbDataLine(std::istream &stream, std::string expectedPrefix)
	{
		std::string line;
		std::getline(stream, line);
		sci::assertThrow(line.length() == 220, sci::err(sci::SERR_USER, 0, sU("Micro rain radar data line found with the wrong length.")));
		sci::assertThrow(line.substr(0, 3) == expectedPrefix, sci::err(sci::SERR_USER, 0, sU("Micro rain radar data line found with an unexpected prefix.")));

		sci::GridData<T, 1> result(31);
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
	sci::GridData<metreF, 1> m_ranges;
	sci::GridData<unitlessF, 1> m_transferFunction;
	sci::GridData<perMetreF, 2> m_spectralReflectivities;
	sci::GridData<millimetreF, 2> m_dropDiameters;
	sci::GridData<perMetreCubedPerMillimetreF, 2> m_numberDistribution;
	sci::GridData<unitlessF, 1> m_pathIntegratedAttenuation;
	sci::GridData<reflectivityF, 1> m_reflectivity;
	sci::GridData< reflectivityF, 1> m_reflectivityAttenuationCorrected;
	sci::GridData<millimetrePerHourF, 1> m_rainRate;
	sci::GridData<gramPerMetreCubedF, 1> m_liquidWaterContent;
	sci::GridData<metrePerSecondF, 1> m_fallVelocity;
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