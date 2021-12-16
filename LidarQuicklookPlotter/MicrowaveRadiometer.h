#pragma once

#include<svector/sstring.h>
#include<vector>
#include"Units.h"
#include<svector/time.h>
#include<fstream>
#include<svector/serr.h>
#include<svector/array.h>

const uint8_t mwrUnusedFlag = 0;
const uint8_t mwrGoodDataFlag = 1;
const uint8_t mwrReducedQualityUnknownReasonFlag = 2;
const uint8_t mwrReducedQualityInterferenceFailureFlag = 3;
const uint8_t mwrReducedQualityLwpTooHighFlag = 4;
const uint8_t mwrLowQualityUnknownReasonFlag = 5;
const uint8_t mwrLowQualityInterferenceFailureFlag = 6;
const uint8_t mwrLowQualityLwpTooHighFlag = 7;
const uint8_t mwrQualityNotEvaluatedFlag = 8;
const uint8_t mwrMissingDataFlag = 9;

const std::vector<std::pair<uint8_t, sci::string>> mwrDopplerFlags
{
	{mwrUnusedFlag, sU("not_used")},
	{mwrGoodDataFlag, sU("good_data")},
	{mwrReducedQualityUnknownReasonFlag, sU("reduced_quality_unknown_reason")},
	{mwrReducedQualityInterferenceFailureFlag, sU("reduced_quality_possible_external_interference_on_a_receiver_channel_or_failure_of_a_receiver_channel_that_is_used_in_the_retrieval_of_this_product")},
	{mwrReducedQualityLwpTooHighFlag, sU("reduced_quality_LWP_too_high")},
	{mwrLowQualityUnknownReasonFlag, sU("low_quality_do_not_use_unknown_reason")},
	{mwrLowQualityInterferenceFailureFlag, sU("low_quality_do_not_use_possible_external_interference_on_a_receiver_channel_or_failure_of_a_receiver_channel_that_is_used_in_the_retrieval_of_this_product")},
	{mwrLowQualityLwpTooHighFlag, sU("low_quality_do_not_use_LWP_too_high")},
	{mwrQualityNotEvaluatedFlag, sU("quality_not_evaluated")},
	{mwrMissingDataFlag, sU("missing_data")}
};

inline uint8_t hatproToAmofFlag(uint8_t hatproQuality, uint8_t hatproExplanation)
{
	if (hatproQuality == 0)
		return mwrQualityNotEvaluatedFlag;
	if (hatproQuality == 1)
		return mwrGoodDataFlag;
	return (hatproQuality - uint8_t(2)) * uint8_t(3) + hatproExplanation + uint8_t(2);
}

inline degreeF hatproDecodeAzimuth(float hatproAngle)
{
	return degreeF( std::floor(std::abs(hatproAngle) / 1000.0f) );
}

inline degreeF hatproDecodeElevation(float hatproAngle)
{
	if (hatproAngle < 0.0f)
		return degreeF(hatproAngle + 1000.0f * std::floor(hatproAngle / 1000.0f));
	else
		return degreeF(hatproAngle - 1000.0f * std::floor(hatproAngle / 1000.0f));
}

const uint32_t hatproLwpId = 934501978;
const uint32_t hatproIwvId = 594811068;
const uint32_t hatproHkdId = 837854832;

inline void readHatproHkdFile(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<degreeF, 1>& latitude, sci::GridData<degreeF, 1>& longitude,
	sci::GridData<kelvinF, 1>& ambientTarget1Temperature, sci::GridData<kelvinF, 1>& ambientTarget2Temperature, sci::GridData<kelvinF, 1>& humidityProfilerTemperature,
	sci::GridData<kelvinF, 1>& temperatureProfilerTemperature, sci::GridData<kelvinF, 1>& temperatureStabilityReceiver1, sci::GridData<kelvinF, 1>& temperatureStabilityReceiver2,
	sci::GridData<uint32_t, 1>& remainingMemory)
{
	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO LWP file ") + filename));

		uint32_t nSamples;
		uint32_t fileTypeId;
		uint32_t timeType;

		fin.read((char*)&fileTypeId, 4);
		sci::assertThrow(fileTypeId == hatproHkdId, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" does not start with the HKD code.")));
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&nSamples, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&timeType, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		sci::assertThrow(timeType == 1, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" is in local time, not UTC time. This software can only process UTC time data.")));

		uint32_t hkdSelect;
		fin.read((char*)&hkdSelect, 4);


		//read the data

		time.resize(nSamples);
		latitude.resize(nSamples, std::numeric_limits<degreeF>::quiet_NaN());
		longitude.resize(nSamples, std::numeric_limits<degreeF>::quiet_NaN());
		ambientTarget1Temperature.resize(nSamples, std::numeric_limits<kelvinF>::quiet_NaN());
		ambientTarget2Temperature.resize(nSamples, std::numeric_limits<kelvinF>::quiet_NaN());
		humidityProfilerTemperature.resize(nSamples, std::numeric_limits<kelvinF>::quiet_NaN());
		temperatureProfilerTemperature.resize(nSamples, std::numeric_limits<kelvinF>::quiet_NaN());
		temperatureStabilityReceiver1.resize(nSamples, std::numeric_limits<kelvinF>::quiet_NaN());
		temperatureStabilityReceiver2.resize(nSamples, std::numeric_limits<kelvinF>::quiet_NaN());
		remainingMemory.resize(nSamples, -1);
		sci::GridData<uint8_t, 1> alarm(nSamples, -1);
		sci::GridData<uint32_t, 1> quality(nSamples, -1);
		sci::GridData<uint32_t, 1> status(nSamples, -1);
		uint32_t timeTemp;
		for (size_t i = 0; i < nSamples; ++i)
		{
			fin.read((char*)&timeTemp, 4);
			fin.read((char*)&alarm[i], 1);

			time[i] = sci::UtcTime(2001, 1, 1, 0, 0, 0) + second(double(timeTemp));

			if (hkdSelect & 0x01)
			{
				fin.read((char*)&latitude[i], 4);
				fin.read((char*)&longitude[i], 4);
			}
			if (hkdSelect & 0x02)
			{
				fin.read((char*)&ambientTarget1Temperature[i], 4);
				fin.read((char*)&ambientTarget2Temperature[i], 4);
				fin.read((char*)&humidityProfilerTemperature[i], 4);
				fin.read((char*)&temperatureProfilerTemperature[i], 4);
			}
			if (hkdSelect & 0x04)
			{
				fin.read((char*)&temperatureStabilityReceiver1[i], 4);
				fin.read((char*)&temperatureStabilityReceiver2[i], 4);
			}
			if (hkdSelect & 0x08)
			{
				fin.read((char*)&remainingMemory[i], 4);
			}
			if (hkdSelect & 0x10)
			{
				fin.read((char*)&quality[i], 4);
				fin.read((char*)&status[i], 4);
			}
		}
	}
	catch (...)
	{
		throw;
	}
}

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproLwpOrIwvData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 1>& water, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t &fileTypeId)
{
	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO LWP file ") + filename));

		uint32_t nSamples;
		float minLwp;
		float maxLwp;
		uint32_t timeType;
		uint32_t retrievalType;
		uint32_t timeTemp;
		float waterTemp;
		float angleTemp;
		uint8_t rainFlagTemp;

		fin.read((char*)&fileTypeId, 4);
		sci::assertThrow(fileTypeId == hatproLwpId || fileTypeId == hatproIwvId, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" does not start with the LWP nor IWV code.")));
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
		water.resize(nSamples);
		elevation.resize(nSamples);
		azimuth.resize(nSamples);
		rainFlag.resize(nSamples);
		quality.resize(nSamples);
		qualityExplanation.resize(nSamples);

		for (size_t i = 0; i < nSamples; ++i)
		{
			fin.read((char*)&timeTemp, 4);
			fin.read((char*)&rainFlagTemp, 1);
			fin.read((char*)&waterTemp, 4);
			fin.read((char*)&angleTemp, 4);

			time[i] = sci::UtcTime(2001, 1, 1, 0, 0, 0) + second(double(timeTemp));
			rainFlag[i] = (rainFlagTemp | 0x00000001) > 0;
			quality[i] = (rainFlagTemp | 0x00000006) >> 1;
			qualityExplanation[i] = (rainFlagTemp | 0x00000018) >> 3;

			if(fileTypeId == hatproLwpId)
				water[i] = gramPerMetreSquaredF(waterTemp);
			else
				water[i] = kilogramPerMetreSquaredF(waterTemp);
			azimuth[i] = hatproDecodeAzimuth(angleTemp);
			elevation[i] = hatproDecodeElevation(angleTemp);
		}
		sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
	}
	catch (...)
	{

		time.resize(0);
		water.resize(0);
		elevation.resize(0);
		azimuth.resize(0);
		rainFlag.resize(0);
		quality.resize(0);
		qualityExplanation.resize(0);
		fileTypeId = 0;
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

	sci::GridData<sci::UtcTime, 1> m_lwpTime;
	sci::GridData<gramPerMetreSquaredF, 1> m_lwp;
	sci::GridData<degreeF, 1> m_lwpElevation;
	sci::GridData<degreeF, 1> m_lwpAzimuth;
	sci::GridData<bool, 1> m_lwpRainFlag;
	sci::GridData<unsigned char, 1> m_lwpQuality;
	sci::GridData<unsigned char, 1> m_lwpQualityExplanation;

	sci::GridData<sci::UtcTime, 1> m_iwvTime;
	sci::GridData<kilogramPerMetreSquaredF, 1> m_iwv;
	sci::GridData<degreeF, 1> m_iwvElevation;
	sci::GridData<degreeF, 1> m_iwvAzimuth;
	sci::GridData<bool, 1> m_iwvRainFlag;
	sci::GridData<unsigned char, 1> m_iwvQuality;
	sci::GridData<unsigned char, 1> m_iwvQualityExplanation;

	//housekeeping data
	sci::GridData<sci::UtcTime, 1> m_hkdTime;
	sci::GridData<degreeF, 1> m_latitude;
	sci::GridData<degreeF, 1> m_longitude;
	sci::GridData<kelvinF, 1> m_ambientTarget1Temperature;
	sci::GridData<kelvinF, 1> m_ambientTarget2Temperature;
	sci::GridData<kelvinF, 1> m_humidityProfilerTemperature;
	sci::GridData<kelvinF, 1> m_temperatureProfilerTemperature;
	sci::GridData<kelvinF, 1> m_temperatureStabilityReceiver1;
	sci::GridData<kelvinF, 1> m_temperatureStabilityReceiver2;
	sci::GridData<uint32_t, 1> m_remainingMemory;
};