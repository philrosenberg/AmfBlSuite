#pragma once

#include<svector/sstring.h>
#include<vector>
#include"Units.h"
#include<svector/time.h>
#include<fstream>
#include<svector/serr.h>

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproLwpData(sci::string filename, std::vector<sci::UtcTime>& time, std::vector<LWP_TYPE>& lwp, std::vector<ANGLE_TYPE>& elevation, std::vector<ANGLE_TYPE>& azimuth,
	std::vector<bool>& rainFlag, std::vector<unsigned char>& quality, std::vector<unsigned char>& qualityExplanation)
{
	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO LWP file ") + filename));

		uint32_t code;
		uint32_t nSamples;
		float minLwp;
		float maxLwp;
		uint32_t timeType;
		uint32_t retrievalType;
		uint32_t timeTemp;
		float lwpTemp;
		float angleTemp;
		uint8_t rainFlagTemp;

		fin.read((char*)&code, 4);
		sci::assertThrow(code == 934501978, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" does not start with the LWP code.")));
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&nSamples, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&minLwp, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&maxLwp, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&timeType, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		sci::assertThrow(timeType == 1, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" is in local time, not UTC time. This software can only process UTC time data.")));

		fin.read((char*)&retrievalType, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));


		time.resize(nSamples);
		lwp.resize(nSamples);
		elevation.resize(nSamples);
		azimuth.resize(nSamples);
		rainFlag.resize(nSamples);
		quality.resize(nSamples);
		qualityExplanation.resize(nSamples);

		for (size_t i = 0; i < nSamples; ++i)
		{
			fin.read((char*)&timeTemp, 4);
			fin.read((char*)&rainFlagTemp, 4);
			fin.read((char*)&lwpTemp, 4);
			fin.read((char*)&angleTemp, 4);

			time[i] = sci::UtcTime(2001, 1, 1, 0, 0, 0) + secondF(float(timeTemp));
			rainFlag[i] = (rainFlagTemp | 0x00000001) > 0;
			quality[i] = (rainFlagTemp | 0x00000006) >> 1;
			qualityExplanation[i] = (rainFlagTemp | 0x00000018) >> 3;

			lwp[i] = gramPerMetreSquaredF(lwpTemp);
			float azimuthTemp = std::floor(std::abs(angleTemp) / 1000.0f);
			azimuth[i] = degreeF(azimuthTemp);
			if (angleTemp < 0.0f)
				elevation[i] = degreeF(-(std::abs(angleTemp) - 1000.0f * azimuthTemp));
			else
				elevation[i] = degreeF((std::abs(angleTemp) - 1000.0f * azimuthTemp));
		}
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
	}
	catch (...)
	{

		time.resize(0);
		lwp.resize(0);
		elevation.resize(0);
		azimuth.resize(0);
		rainFlag.resize(0);
		quality.resize(0);
		qualityExplanation.resize(0);
		throw;
	}
}



#include "InstrumentProcessor.h"
#include"AmfNc.h"
class MicrowaveRadiometerProcessor : public InstrumentProcessor
{
public:
	MicrowaveRadiometerProcessor(const InstrumentInfo& instrumentInfo, const CalibrationInfo& calibrationInfo);
	virtual void readData(const std::vector<sci::string>& inputFilenames, const Platform& platform, ProgressReporter& progressReporter) override;
	void readData(const sci::string& inputFilename, const Platform& platform, ProgressReporter& progressReporter, bool clear);
	virtual void plotData(const sci::string& baseOutputFilename, const std::vector<metreF> maxRanges, ProgressReporter& progressReporter, wxWindow* parent) override
	{
		throw(sci::err(sci::SERR_USER, 0, "Micro Rain Radar plotting is not yet supported."));
	}
	virtual void writeToNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter) override;
	virtual bool hasData() const override
	{
		return m_hasData;
	}
	virtual bool fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const override;
	virtual std::vector<std::vector<sci::string>> groupInputFilesbyOutputFiles(const std::vector<sci::string>& newFiles, const std::vector<sci::string>& allFiles) const override;
	sci::string getName() const override
	{
		return sU("Microwave Radiometer Processor");
	}
private:
	InstrumentInfo m_instrumentInfo;
	CalibrationInfo m_calibrationInfo;
	bool m_hasData;

	std::vector<sci::UtcTime> m_lwpTime;
	std::vector<gramPerMetreSquaredF> m_lwp;
	std::vector<degreeF> m_lwpElevation;
	std::vector<degreeF> m_lwpAzimuth;
	std::vector<bool> m_lwpRainFlag;
	std::vector<unsigned char> m_lwpQuality;
	std::vector<unsigned char> m_lwpQualityExplanation;
};