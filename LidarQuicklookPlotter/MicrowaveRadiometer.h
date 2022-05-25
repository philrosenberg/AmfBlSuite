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

const std::vector<std::pair<uint8_t, sci::string>> mwrMetFlags
{
	{ mwrUnusedFlag, sU("not_used") },
	{mwrGoodDataFlag, sU("good_data")}
};

const uint8_t mwrRainUnusedFlag = 0;
const uint8_t mwrRainGoodDataFlag = 1;
const uint8_t mwrRainRainingFlag = 2;
const uint8_t mwrRainMissingDataFlag = 3;

const std::vector<std::pair<uint8_t, sci::string>> mwrRainFlags
{
	{ mwrRainUnusedFlag, sU("not_used") },
	{mwrRainGoodDataFlag, sU("good_data")},
	{mwrRainRainingFlag, sU("raining")},
	{mwrRainMissingDataFlag, sU("missing_data")}
};

const uint8_t mwrChannelUnusedFlag = 0;
const uint8_t mwrChannelGoodDataFlag = 1;
const uint8_t mwrChannelFailFlag = 2;
const uint8_t mwrChannelMissingDataFlag = 3;

const std::vector<std::pair<uint8_t, sci::string>> mwrChannelFlags
{
	{ mwrRainUnusedFlag, sU("not_used") },
	{mwrRainGoodDataFlag, sU("good_data")},
	{mwrRainRainingFlag, sU("failed")},
	{mwrChannelMissingDataFlag, sU("missing_data")}
};

const uint8_t mwrStabilityUnusedFlag = 0;
const uint8_t mwrStabilityGoodDataFlag = 1;
const uint8_t mwrStabilityPoorFlag = 2;
const uint8_t mwrStabilityVeryPoorFlag = 3;
const uint8_t mwrStabilityVeryVeryPoorFlag = 4;
const uint8_t mwrStabilityMissingDataFlag = 5;

const std::vector<std::pair<uint8_t, sci::string>> mwrStabilityFlags
{
	{ mwrStabilityUnusedFlag, sU("not_used") },
	{mwrStabilityGoodDataFlag, sU("good_data")},
	{mwrStabilityPoorFlag, sU("suspect_data_temperature_stability_in_the_range_5_to_10mK")},
	{mwrStabilityVeryPoorFlag, sU("suspect_data_temperature_stability_in_the_range_10_to_50mK")},
	{mwrStabilityVeryVeryPoorFlag, sU("suspect_data_temperature_stability_over_50mK")},
	{mwrStabilityMissingDataFlag, sU("missing_data")}
};

const std::vector<std::pair<uint8_t, sci::string>> mwrFlags
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
const uint32_t hatproMetId = 599658943;

inline void readHatproHkdFile(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<degreeF, 1>& latitude, sci::GridData<degreeF, 1>& longitude,
	sci::GridData<kelvinF, 1>& ambientTarget1Temperature, sci::GridData<kelvinF, 1>& ambientTarget2Temperature, sci::GridData<kelvinF, 1>& humidityProfilerTemperature,
	sci::GridData<kelvinF, 1>& temperatureProfilerTemperature, sci::GridData<kelvinF, 1>& temperatureStabilityReceiver1, sci::GridData<kelvinF, 1>& temperatureStabilityReceiver2,
	sci::GridData<uint32_t, 1> &status, sci::GridData<uint32_t, 1>& remainingMemory, uint32_t& fileTypeId)
{
	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO LWP file ") + filename));

		uint32_t nSamples;
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
		status.resize(nSamples, -1);
		remainingMemory.resize(nSamples, -1);
		sci::GridData<uint8_t, 1> alarm(nSamples, -1);
		sci::GridData<uint32_t, 1> quality(nSamples, -1);
		uint32_t timeTemp;
		float angleRaw;
		float angleDegrees;
		float angleArcMinutes;
		for (size_t i = 0; i < nSamples; ++i)
		{
			fin.read((char*)&timeTemp, 4);
			fin.read((char*)&alarm[i], 1);

			time[i] = sci::UtcTime(2001, 1, 1, 0, 0, 0) + second(double(timeTemp));

			if (hkdSelect & 0x01)
			{
				fin.read((char*)&angleRaw, 4);
				angleDegrees = std::floor(angleRaw / 100.0f);
				angleArcMinutes = angleRaw - angleDegrees * 100.0f;
				longitude[i] = degreeF(angleDegrees) + arcMinuteF(angleArcMinutes);
				fin.read((char*)&angleRaw, 4);
				angleDegrees = std::floor(angleRaw / 100.0f);
				angleArcMinutes = angleRaw - angleDegrees * 100.0f;
				latitude[i] = degreeF(angleDegrees) + arcMinuteF(angleArcMinutes);
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

inline void readHatproMetData(const sci::string & filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<kelvinF, 1> &temperature, sci::GridData<unitlessF, 1> &relativeHumidity,
	sci::GridData<hectoPascalF, 1> &pressure, sci::GridData<bool,1> rainFlag, uint32_t& fileTypeId)
{

	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO LWP file ") + filename));

		uint32_t nSamples;
		uint32_t timeType;

		fin.read((char*)&fileTypeId, 4);
		sci::assertThrow(fileTypeId == hatproMetId, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" does not start with the MET code.")));
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&nSamples, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		float minPressure;
		float maxPressure;
		float minTemperature;
		float maxTemperature;
		float minRelativeHumidity;
		float maxRelativeHumidity;

		fin.read((char*)&minPressure, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&maxPressure, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&minTemperature, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&maxTemperature, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&minRelativeHumidity, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&maxRelativeHumidity, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));


		fin.read((char*)&timeType, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		sci::assertThrow(timeType == 1, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" is in local time, not UTC time. This software can only process UTC time data.")));


		//read the data

		time.resize(nSamples);
		temperature.resize(nSamples, std::numeric_limits<kelvinF>::quiet_NaN());
		relativeHumidity.resize(nSamples, std::numeric_limits<unitlessF>::quiet_NaN());
		pressure.resize(nSamples, std::numeric_limits<hectoPascalF>::quiet_NaN());
		
		uint32_t timeTemp;
		percentF relativeHumidityTemp;
		uint8_t rainFlagTemp;
		for (size_t i = 0; i < nSamples; ++i)
		{
			fin.read((char*)&timeTemp, 4);
			time[i] = sci::UtcTime(2001, 1, 1, 0, 0, 0) + second(double(timeTemp));
			fin.read((char*)&rainFlagTemp, 1);
			rainFlag[i] = rainFlagTemp == 0 ? false : true;
			fin.read((char*)&pressure[i], 4);
			fin.read((char*)&temperature[i], 4);
			fin.read((char*)&relativeHumidityTemp, 4);
			relativeHumidity[i] = relativeHumidityTemp;
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
			rainFlag[i] = (rainFlagTemp & 0x01) > 0;
			quality[i] = (rainFlagTemp & 0x06) >> 1;
			qualityExplanation[i] = (rainFlagTemp & 0x18) >> 3;

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
	void writeIwvLwpNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter);
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
	sci::GridData<uint32_t, 1> m_status;
	sci::GridData<uint32_t, 1> m_remainingMemory;

	sci::GridData<sci::UtcTime, 1> m_metTime;
	sci::GridData<kelvinF, 1> m_enviromentTemperature;
	sci::GridData<hectoPascalF, 1> m_enviromentPressure;
	sci::GridData<unitlessF, 1> m_enviromentRelativeHumidity;
};