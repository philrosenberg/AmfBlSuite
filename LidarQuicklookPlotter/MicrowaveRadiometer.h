#pragma once

#include<svector/sstring.h>
#include<vector>
#include"Units.h"
#include<svector/time.h>
#include<fstream>
#include<svector/serr.h>
#include<svector/array.h>
#include<map>
#include<memory>

//The purpose of this function is to always return false at compile time
//It can be used in combination with static_assert to throw a compile time error
//when an invalid template is instantiated. The reason it is needed is that
//static_assert(false, "some errorr message") will always generate an error even
//if the template is never instantiated. We need to actually use the template type
//to generate the bool argument of static_assert
template <uint32_t N>
constexpr bool makeFalse()
{
	return N == 0 && N == 1; //N can never be both 0 and 1, always returns false.
}

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
	if (hatproQuality == mwrMissingDataFlag) //this is what we use for padding if we need to pad the quality variable
		return mwrMissingDataFlag;
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
const uint32_t hatproAtnId = 7757564;
const uint32_t hatproBrtId = 666666;
const uint32_t hatproMetId = 599658943;
const uint32_t hatproMetNewId = 599658944;
const uint32_t hatproOlcId = 955874342;
const uint32_t hatproTpcId = 780798065;
const uint32_t hatproTpbId = 459769847;
const uint32_t hatproWvlId = 456783953;
const uint32_t hatproHpcNoRhId = 117343672;
const uint32_t hatproHpcWithRhId = 117343673;
const uint32_t hatproLprId = 4567;
const uint32_t hatproIrtOldId = 671112495;
const uint32_t hatproIrtNewId = 671112496;
const uint32_t hatproBlbOldId = 567845847;
const uint32_t hatproBlbNewId = 567845848;
const uint32_t hatproStaId = 454532;
const uint32_t hatproCbhId = 67777499;
const uint32_t hatproVltOldId = 362118746;
const uint32_t hatproVltNewId = 362118747;
const uint32_t hatproHkdId = 837854832;

const std::map<uint32_t, sci::string> hatproFileTypes
{
	{hatproLwpId, sU("LWP")},
	{hatproIwvId, sU("IWV")},
	{hatproAtnId, sU("ATN")},
	{hatproBrtId, sU("BRT")},
	{hatproMetId, sU("MET")},
	{hatproOlcId, sU("OLC")},
	{hatproTpcId, sU("TPC")},
	{hatproTpbId, sU("TPB")},
	{hatproWvlId, sU("WVL")},
	{hatproHpcNoRhId, sU("HPC (excluding RH)")},
	{hatproHpcWithRhId, sU("HPC (with RH)")},
	{hatproLprId, sU("LPR")},
	{hatproIrtOldId, sU("IRT (old)")},
	{hatproIrtNewId, sU("IRT (new)")},
	{hatproBlbOldId, sU("BLB (old)")},
	{hatproBlbNewId, sU("BLB (new)")},
	{hatproStaId, sU("STA")},
	{hatproCbhId, sU("CBH")},
	{hatproVltOldId, sU("VLT (old)")},
	{hatproVltNewId, sU("VLT (new)")},
	{hatproHkdId, sU("HKD")},
};



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
	sci::GridData<hectoPascalF, 1> &pressure,  sci::GridData<bool,1> rainFlag, uint32_t& fileTypeId)
{

	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO LWP file ") + filename));

		uint32_t nSamples;
		uint32_t timeType;

		fin.read((char*)&fileTypeId, 4);
		sci::assertThrow(fileTypeId == hatproMetId || fileTypeId== hatproMetNewId, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" does not start with the MET code.")));
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&nSamples, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		uint8_t additionalSensorCode = 0;
		float minPressure;
		float maxPressure;
		float minTemperature;
		float maxTemperature;
		float minRelativeHumidity;
		float maxRelativeHumidity;

		if (fileTypeId == hatproMetNewId)
			fin.read((char*)&additionalSensorCode, 1);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
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

		int nAdditionalSensors = 0;
		if (additionalSensorCode & 0x1)
			++nAdditionalSensors;
		if (additionalSensorCode & 0x2)
			++nAdditionalSensors;
		if (additionalSensorCode & 0x4)
			++nAdditionalSensors;

		//read the data

		time.resize(nSamples);
		rainFlag.resize(nSamples);
		temperature.resize(nSamples, std::numeric_limits<kelvinF>::quiet_NaN());
		relativeHumidity.resize(nSamples, std::numeric_limits<unitlessF>::quiet_NaN());
		pressure.resize(nSamples, std::numeric_limits<hectoPascalF>::quiet_NaN());
		
		uint32_t timeTemp;
		percentF relativeHumidityTemp;
		uint8_t rainFlagTemp;
		float extraTemp;
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
			for(size_t i=0; i<nAdditionalSensors; ++i)
				fin.read((char*)&extraTemp, 4);
		}
	}
	catch (...)
	{
		throw;
	}
}

template<uint32_t expectedFileTypeId, class DATA_TYPE, class ANGLE_TYPE>
void readHatproOneDimensionalData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<DATA_TYPE, 1>& data, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{
	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO ") + hatproFileTypes.at(expectedFileTypeId) + sU(" file ") + filename));

		uint32_t nSamples;
		float minData;
		float maxData;
		uint32_t timeType;
		uint32_t retrievalType;
		uint32_t timeTemp;
		float dataTemp;
		float angleTemp;
		uint8_t rainFlagTemp;

		fin.read((char*)&fileTypeId, 4);
		sci::assertThrow(fileTypeId == expectedFileTypeId, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" does not start with the ") + hatproFileTypes.at(expectedFileTypeId) + sU(" code.")));
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&nSamples, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&minData, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&maxData, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&timeType, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		sci::assertThrow(timeType == 1, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" is in local time, not UTC time. This software can only process UTC time data.")));

		fin.read((char*)&retrievalType, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));


		time.resize(nSamples);
		data.resize(nSamples);
		elevation.resize(nSamples);
		azimuth.resize(nSamples);
		rainFlag.resize(nSamples);
		quality.resize(nSamples);
		qualityExplanation.resize(nSamples);

		for (size_t i = 0; i < nSamples; ++i)
		{
			fin.read((char*)&timeTemp, 4);
			fin.read((char*)&rainFlagTemp, 1);
			fin.read((char*)&dataTemp, 4);
			fin.read((char*)&angleTemp, 4);

			time[i] = sci::UtcTime(2001, 1, 1, 0, 0, 0) + second(double(timeTemp));
			rainFlag[i] = (rainFlagTemp & 0x01) > 0;
			quality[i] = (rainFlagTemp & 0x06) >> 1;
			qualityExplanation[i] = (rainFlagTemp & 0x18) >> 3;

			if constexpr (expectedFileTypeId == hatproLwpId)
				data[i] = gramPerMetreSquaredF(dataTemp);
			else if constexpr (expectedFileTypeId == hatproIwvId)
				data[i] = kilogramPerMetreSquaredF(dataTemp);
			else
				static_assert(makeFalse<expectedFileTypeId>(), "Attempting to read a hatpro file with an unknown expected file type id.");
			azimuth[i] = hatproDecodeAzimuth(angleTemp);
			elevation[i] = hatproDecodeElevation(angleTemp);
		}
		sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
	}
	catch (...)
	{

		time.resize(0);
		data.resize(0);
		elevation.resize(0);
		azimuth.resize(0);
		rainFlag.resize(0);
		quality.resize(0);
		qualityExplanation.resize(0);
		fileTypeId = 0;
		throw;
	}
}

template<class LI_TYPE, class KO_TYPE, class TTI_TYPE, class KI_TYPE, class SI_TYPE, class CAPE_TYPE>
void readHatproStaData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LI_TYPE, 1>& liftedIndex, sci::GridData<KO_TYPE, 1>& kModifiedIndex,
	sci::GridData<TTI_TYPE, 1>& totalTotalsIndex, sci::GridData<KI_TYPE, 1>& kIndex, sci::GridData<SI_TYPE, 1>& showalterIndex, sci::GridData<CAPE_TYPE, 1>& cape,
	sci::GridData<bool, 1>& rainFlag, uint32_t& fileTypeId)
{
	const uint32_t expectedFileTypeId = hatproStaId;
	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO ") + hatproFileTypes.at(expectedFileTypeId) + sU(" file ") + filename));

		uint32_t nSamples;
		float minData;
		float maxData;
		uint32_t indexList[6];
		uint32_t timeType;
		uint32_t timeTemp;
		float indexTemp[6];
		uint8_t rainFlagTemp;

		fin.read((char*)&fileTypeId, 4);
		sci::assertThrow(fileTypeId == expectedFileTypeId, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" does not start with the ") + hatproFileTypes.at(expectedFileTypeId) + sU(" code.")));
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&nSamples, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&minData, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&maxData, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&indexList[0], 24);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		for (size_t i = 0; i < 6; ++i)
			sci::assertThrow(indexList[i] == 0 || indexList[i] == 1, sci::err(sci::SERR_USER, 0, sU("Found corrupt STAIndexList (value neither 0 nor 1) in file ") + filename));

		fin.read((char*)&timeType, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		sci::assertThrow(timeType == 1, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" is in local time, not UTC time. This software can only process UTC time data.")));


		time.resize(nSamples);
		if(indexList[0] == 1)
			liftedIndex.resize(nSamples);
		if (indexList[1] == 1)
			kModifiedIndex.resize(nSamples);
		if (indexList[2] == 1)
			totalTotalsIndex.resize(nSamples);
		if (indexList[3] == 1)
			kIndex.resize(nSamples);
		if (indexList[4] == 1)
			showalterIndex.resize(nSamples);
		if (indexList[5] == 1)
			cape.resize(nSamples);
		rainFlag.resize(nSamples);

		for (size_t i = 0; i < nSamples; ++i)
		{
			fin.read((char*)&timeTemp, 4);
			fin.read((char*)&rainFlagTemp, 1);
			for (size_t j = 0; j < 6; ++j)
				if (indexList[j])
					fin.read((char*)&indexTemp[j], 4);

			time[i] = sci::UtcTime(2001, 1, 1, 0, 0, 0) + second(double(timeTemp));
			rainFlag[i] = (rainFlagTemp & 0x01) > 0;

			if (indexList[0])
				liftedIndex[i] = kelvinF(indexTemp[0]);
			if (indexList[1])
				kModifiedIndex[i] = kelvinF(indexTemp[1]);
			if (indexList[2])
				totalTotalsIndex[i] = kelvinF(indexTemp[2]);
			if (indexList[3])
				kIndex[i] = kelvinF(indexTemp[3]);
			if (indexList[4])
				showalterIndex[i] = kelvinF(indexTemp[4]);
			if (indexList[5])
				cape[i] = joulePerKilogramF(indexTemp[5]);
		}
		sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
	}
	catch (...)
	{

		time.resize(0);
		rainFlag.resize(0);
		liftedIndex.resize(0);
		kModifiedIndex.resize(0);
		totalTotalsIndex.resize(0);
		kIndex.resize(0);
		showalterIndex.resize(0);
		cape.resize(0);
		fileTypeId = 0;
		throw;
	}
}

template<uint32_t expectedFileTypeId, class FREQUENCY_TYPE, class DATA_TYPE, class ANGLE_TYPE>
void readHatproOneDimensionPerFrequencyData(sci::string filename, bool hasRetrievalVariable, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<FREQUENCY_TYPE, 1> &frequencies, sci::GridData<DATA_TYPE, 2>& data, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, uint32_t& fileTypeId)
{
	try
	{
		std::fstream fin;
		fin.open(sci::nativeUnicode(filename), std::ios::in | std::ios::binary);
		sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Failed to open HATPRO ") + hatproFileTypes.at(expectedFileTypeId) + sU(" file ") + filename));

		uint32_t nSamples;
		uint32_t nFrequencies;
		sci::GridData<gigaHertzF, 1> frequenciesTemp;
		sci::GridData<float, 1> minData;
		sci::GridData<float, 1> maxData;
		uint32_t timeType;
		uint32_t retrievalType;

		uint32_t timeTemp;
		sci::GridData<float, 1> dataTemp;
		float angleTemp;
		uint8_t rainFlagTemp;

		fin.read((char*)&fileTypeId, 4);
		sci::assertThrow(fileTypeId == expectedFileTypeId, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" does not start with the ") + hatproFileTypes.at(expectedFileTypeId) + sU(" code.")));
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&nSamples, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		fin.read((char*)&timeType, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		sci::assertThrow(timeType == 1, sci::err(sci::SERR_USER, 0, sU("File ") + filename + sU(" is in local time, not UTC time. This software can only process UTC time data.")));

		if (hasRetrievalVariable)
		{
			fin.read((char*)&retrievalType, 4);
			sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		}

		fin.read((char*)&nFrequencies, 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));

		frequencies.resize(nFrequencies);
		frequenciesTemp.resize(nFrequencies);
		minData.resize(nFrequencies);
		maxData.resize(nFrequencies);

		fin.read((char*)&frequenciesTemp[0], nFrequencies * 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&minData[0], nFrequencies * 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		fin.read((char*)&maxData[0], nFrequencies * 4);
		sci::assertThrow(fin.good() && !fin.eof(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
		for (size_t i = 0; i < frequencies.size(); ++i)
			frequencies[i] = frequenciesTemp[i];

		time.resize(nSamples);
		data.reshape({ nSamples, nFrequencies });
		elevation.resize(nSamples);
		azimuth.resize(nSamples);
		rainFlag.resize(nSamples);
		dataTemp.resize(nFrequencies);

		for (size_t i = 0; i < nSamples; ++i)
		{
			fin.read((char*)&timeTemp, 4);
			fin.read((char*)&rainFlagTemp, 1);
			for (size_t j = 0; j < nFrequencies; ++j)
				fin.read((char*)&dataTemp[j], 4);
			fin.read((char*)&angleTemp, 4);

			time[i] = sci::UtcTime(2001, 1, 1, 0, 0, 0) + second(double(timeTemp));
			rainFlag[i] = (rainFlagTemp & 0x01) > 0;

			if constexpr (expectedFileTypeId == hatproBrtId || expectedFileTypeId == hatproOlcId)
				for(size_t j=0; j<nFrequencies; ++j)
					data[i][j] = kelvinF(dataTemp[j]);
			else if constexpr (expectedFileTypeId == hatproAtnId)
				for (size_t j = 0; j < nFrequencies; ++j)
					data[i][j] = unitlessF(dataTemp[j]);
			else if constexpr (expectedFileTypeId == hatproIrtNewId)
				for (size_t j = 0; j < nFrequencies; ++j)
					data[i][j] = kelvinF(dataTemp[j])+kelvinF(273.15);
			else
				static_assert(makeFalse<expectedFileTypeId>(), "Attempting to read a hatpro file with an unknown expected file type id.");
			azimuth[i] = hatproDecodeAzimuth(angleTemp);
			elevation[i] = hatproDecodeElevation(angleTemp);
		}
		sci::assertThrow(fin.good(), sci::err(sci::SERR_USER, 0, sU("Found unexpected end of file or bad read in file ") + filename));
	}
	catch (...)
	{

		time.resize(0);
		data.reshape({ 0,0 });
		elevation.resize(0);
		azimuth.resize(0);
		rainFlag.resize(0);
		fileTypeId = 0;
		throw;
	}
}

template<uint32_t expectedFileTypeId, class DATA_TYPE, class ANGLE_TYPE>
void readHatproProfile(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<DATA_TYPE, 2>& data, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{

}

//might need old and new for this
template<uint32_t expectedFileTypeId, class DATA_TYPE, class ANGLE_TYPE>
void readHatproOneProfilePerFrequency(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<DATA_TYPE, 3>& data, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{

}

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproIwvData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 1>& iwv, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t &fileTypeId)
{
	readHatproOneDimensionalData<hatproIwvId>(filename, time, iwv, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
}

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproLwpData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 1>& lwp, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{
	readHatproOneDimensionalData<hatproLwpId>(filename, time, lwp, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
}

template<class FREQUENCY_TYPE, class LWP_TYPE, class ANGLE_TYPE>
void readHatproAtnData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<FREQUENCY_TYPE, 1>& frequencies, sci::GridData<LWP_TYPE, 2>& atn, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, uint32_t& fileTypeId)
{
	readHatproOneDimensionPerFrequencyData<hatproAtnId>(filename, true, time, frequencies, atn, elevation, azimuth, rainFlag, fileTypeId);
}

template<class FREQUENCY_TYPE, class LWP_TYPE, class ANGLE_TYPE>
void readHatproBrtData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<FREQUENCY_TYPE, 1>& frequencies, sci::GridData<LWP_TYPE, 2>& brt, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, uint32_t& fileTypeId)
{
	readHatproOneDimensionPerFrequencyData<hatproBrtId>(filename, false, time, frequencies, brt, elevation, azimuth, rainFlag, fileTypeId);
}

template<class FREQUENCY_TYPE, class LWP_TYPE, class ANGLE_TYPE>
void readHatproOlcData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<FREQUENCY_TYPE, 1>& frequencies, sci::GridData<LWP_TYPE, 2>& olc, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, uint32_t& fileTypeId)
{
	readHatproOneDimensionPerFrequencyData<hatproOlcId>(filename, false, time, frequencies, olc, elevation, azimuth, rainFlag, fileTypeId);
}

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproTpcData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 2>& tpc, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{
	readHatproProfile<hatproTpcId>(filename, time, tpc, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
}

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproTpbData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 2>& tpb, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{
	readHatproProfile<hatproTpbId>(filename, time, tpb, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
}

//need to deal with with and without RH
//template<class LWP_TYPE, class ANGLE_TYPE>
//void readHatproHpcData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 2>& hpc, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
//	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
//{
//	readHatproProfile<hatproHpcId>(filename, time, hpc, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
//}

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproLprData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 2>& lpr, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{
	readHatproProfile<hatproLprId>(filename, time, lpr, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
}

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproIrtOldData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 1>& irt, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{
	readHatproOneDimensionalData<hatproIrtOldId>(filename, time, irt, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
}

template<class FREQUENCY_TYPE, class LWP_TYPE, class ANGLE_TYPE>
void readHatproIrtNewData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<FREQUENCY_TYPE, 1>& frequencies, sci::GridData<LWP_TYPE, 2>& irt, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, uint32_t& fileTypeId)
{
	readHatproOneDimensionPerFrequencyData<hatproIrtNewId>(filename, false, time, frequencies, irt, elevation, azimuth, rainFlag, fileTypeId);
}

//BLB

template<class LWP_TYPE, class ANGLE_TYPE>
void readHatproCbhData(sci::string filename, sci::GridData<sci::UtcTime, 1>& time, sci::GridData<LWP_TYPE, 1>& cbh, sci::GridData<ANGLE_TYPE, 1>& elevation, sci::GridData<ANGLE_TYPE, 1>& azimuth,
	sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality, sci::GridData<unsigned char, 1>& qualityExplanation, uint32_t& fileTypeId)
{
	readHatproOneDimensionalData<hatproCbhId>(filename, time, cbh, elevation, azimuth, rainFlag, quality, qualityExplanation, fileTypeId);
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
		throw(sci::err(sci::SERR_USER, 0, "Microwave Radiometer plotting is not yet supported."));
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
	template<class DATA_UNIT, size_t NDIMS>
	static void padDataToMatchHkp(const sci::GridData<sci::UtcTime, 1>& hkdTime, sci::GridData<sci::UtcTime, 1>& dataTime, sci::GridData<DATA_UNIT, NDIMS>& data,
		sci::GridData<degreeF, 1> &elevation, sci::GridData<degreeF, 1> &azimuth, sci::GridData<bool, 1> &rainFlag, sci::GridData<unsigned char, 1> &quality,
		sci::GridData<unsigned char, 1> &qualityExplanation, sci::GridData<uint8_t, 1> &flag);
	template<class DATA_UNIT, size_t NDIMS>
	static void padDataToMatchHkp(const sci::GridData<sci::UtcTime, 1>& hkdTime, sci::GridData<sci::UtcTime, 1> dataTime, sci::GridData<DATA_UNIT, NDIMS>& data, DATA_UNIT padValue = std::numeric_limits<DATA_UNIT>::quiet_NaN());

	template<class DATA_UNIT, size_t NDIMS>
	static sci::GridData<uint8_t, 1> buildAmfFlagFromHatproQualityAndNanBadData(const sci::GridData<unsigned char, 1>& quality, const sci::GridData<unsigned char, 1>& qualityExplanation, sci::GridData<DATA_UNIT, NDIMS>& data)
	{
		sci::GridData<uint8_t, 1> amfFlag(quality.size());
		for (size_t i = 0; i < amfFlag.size(); ++i)
			amfFlag[i] = hatproToAmofFlag(quality[i], qualityExplanation[i]);

		for (size_t i = 0; i < amfFlag.size(); ++i)
			if (amfFlag[i] == mwrLowQualityUnknownReasonFlag
				|| amfFlag[i] == mwrLowQualityInterferenceFailureFlag
				|| amfFlag[i] == mwrLowQualityLwpTooHighFlag
				|| amfFlag[i] == mwrMissingDataFlag)
				data[i] = std::numeric_limits<DATA_UNIT>::quiet_NaN();
		return amfFlag;
	}

	class MetFlags
	{
	public:
		MetFlags(size_t nPoints)
		{
			surfaceTemperatureFlag.clear();
			surfacePressureFlag.clear();
			surfaceRelativeHumidityFlag.clear();

			surfaceTemperatureFlag.resize(nPoints, mwrGoodDataFlag);
			surfacePressureFlag.resize(nPoints, mwrGoodDataFlag);
			surfaceRelativeHumidityFlag.resize(nPoints, mwrGoodDataFlag);
		}
		void createNcFlags(OutputAmfNcFile& file)
		{
			temperatureFlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_surface_temperature"), mwrMetFlags, file, file.getTimeDimension()));
			relativeHumidityFlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_surface_relative_humidity"), mwrMetFlags, file, file.getTimeDimension()));
			pressureFlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_surface_pressure"), mwrMetFlags, file, file.getTimeDimension()));
		}
		void writeToNetCdf(OutputAmfNcFile& file)
		{
			file.write(*temperatureFlagVariable, surfaceTemperatureFlag);
			file.write(*relativeHumidityFlagVariable, surfaceRelativeHumidityFlag);
			file.write(*pressureFlagVariable, surfacePressureFlag);
		}
		sci::GridData<uint8_t, 1> getFilter() const
		{
			sci::GridData<uint8_t, 1> filter(surfaceTemperatureFlag.size());
			for (size_t i = 0; i < filter.size(); ++i)
			{
				filter[i] = anyBad(i) ? 2 : 1;
			}
			return filter;
		}
		bool anyBad(size_t index) const
		{
			return surfaceTemperatureFlag[index] > 1 ||
				surfacePressureFlag[index] > 1 ||
				surfaceRelativeHumidityFlag[index] > 1;
		}

	private:
		sci::GridData<uint8_t, 1> surfaceTemperatureFlag;
		sci::GridData<uint8_t, 1> surfacePressureFlag;
		sci::GridData<uint8_t, 1> surfaceRelativeHumidityFlag;

		std::unique_ptr<AmfNcFlagVariable> temperatureFlagVariable;
		std::unique_ptr<AmfNcFlagVariable> relativeHumidityFlagVariable;
		std::unique_ptr<AmfNcFlagVariable> pressureFlagVariable;
	};

	class MetAndStabilityFlags
	{
	public:
		MetAndStabilityFlags(const sci::GridData<bool, 1>& rawRainFlag,
			const sci::GridData<uint32_t, 1>& status,
			const sci::GridData<kelvinF, 1>& temperatureStabilityReceiver1,
			const sci::GridData<kelvinF, 1>& temperatureStabilityReceiver2)
			: metFlags(status.size())
		{
			rainFlag.clear();
			channelFailureFlags.clear();
			temperatureReceiverStabilityFlag.clear();
			relativeHumidityReceiverStabilityFlag.clear();


			const size_t nPoints = status.size();


			//set the rest of the flags
			rainFlag.resize(nPoints, mwrRainGoodDataFlag);
			for (size_t i = 0; i < rainFlag.size(); ++i)
				if (rawRainFlag[i])
					rainFlag[i] = mwrRainRainingFlag;

			channelFailureFlags.resize(14, sci::GridData<uint8_t, 1>(nPoints));
			sci::GridData<uint32_t, 1> statusFilters{ 0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000100, 0x00000200,
				0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000 }; //note 0x0080 and 0x8000 are missing deliberately
			for (size_t i = 0; i < 14; ++i)
				for (size_t j = 0; j < nPoints; ++j)
					channelFailureFlags[i][j] = (status[j] & statusFilters[i]) == 0 ? mwrChannelFailFlag : mwrChannelGoodDataFlag;

			temperatureReceiverStabilityFlag.resize(nPoints, mwrStabilityGoodDataFlag);
			for (size_t i = 0; i < temperatureReceiverStabilityFlag.size(); ++i)
			{
				if (temperatureStabilityReceiver1[i] > millikelvinF(50))
					temperatureReceiverStabilityFlag[i] = mwrStabilityVeryVeryPoorFlag;
				else if (temperatureStabilityReceiver1[i] > millikelvinF(10))
					temperatureReceiverStabilityFlag[i] = mwrStabilityVeryPoorFlag;
				else if (temperatureStabilityReceiver1[i] > millikelvinF(5))
					temperatureReceiverStabilityFlag[i] = mwrStabilityPoorFlag;
			}

			relativeHumidityReceiverStabilityFlag.resize(nPoints, mwrStabilityGoodDataFlag);
			for (size_t i = 0; i < relativeHumidityReceiverStabilityFlag.size(); ++i)
			{
				if (temperatureStabilityReceiver2[i] > millikelvinF(50))
					relativeHumidityReceiverStabilityFlag[i] = mwrStabilityVeryVeryPoorFlag;
				else if (temperatureStabilityReceiver2[i] > millikelvinF(10))
					relativeHumidityReceiverStabilityFlag[i] = mwrStabilityVeryPoorFlag;
				else if (temperatureStabilityReceiver2[i] > millikelvinF(5))
					relativeHumidityReceiverStabilityFlag[i] = mwrStabilityPoorFlag;
			}
		}

		void createNcFlags(OutputAmfNcFile& file)
		{
			metFlags.createNcFlags(file);
			rainFlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_surface_precipitation"), mwrRainFlags, file, file.getTimeDimension()));
			ch1FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_1_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch2FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_2_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch3FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_3_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch4FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_4_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch5FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_5_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch6FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_6_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch7FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_7_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch8FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_8_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch9FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_9_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch10FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_10_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch11FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_11_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch12FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_12_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch13FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_13_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			ch14FlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_channel_14_failure"), mwrChannelFlags, file, file.getTimeDimension()));
			temperatureStabilityFlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_t_receiver_temperature_stability"), mwrStabilityFlags, file, file.getTimeDimension()));
			relativeHumidityStabilityFlagVariable.reset(new AmfNcFlagVariable(sU("qc_flag_rh_receiver_temperature_stability"), mwrStabilityFlags, file, file.getTimeDimension()));

			createdStabilityFlags = true;
		}

		void writeToNetCdf(OutputAmfNcFile& file)
		{
			metFlags.writeToNetCdf(file);
			if (createdStabilityFlags)
			{
				file.write(*rainFlagVariable, rainFlag);
				file.write(*ch1FlagVariable, channelFailureFlags[0]);
				file.write(*ch2FlagVariable, channelFailureFlags[1]);
				file.write(*ch3FlagVariable, channelFailureFlags[2]);
				file.write(*ch4FlagVariable, channelFailureFlags[3]);
				file.write(*ch5FlagVariable, channelFailureFlags[4]);
				file.write(*ch6FlagVariable, channelFailureFlags[5]);
				file.write(*ch7FlagVariable, channelFailureFlags[6]);
				file.write(*ch8FlagVariable, channelFailureFlags[7]);
				file.write(*ch9FlagVariable, channelFailureFlags[8]);
				file.write(*ch10FlagVariable, channelFailureFlags[9]);
				file.write(*ch11FlagVariable, channelFailureFlags[10]);
				file.write(*ch12FlagVariable, channelFailureFlags[11]);
				file.write(*ch13FlagVariable, channelFailureFlags[12]);
				file.write(*ch14FlagVariable, channelFailureFlags[13]);
				file.write(*temperatureStabilityFlagVariable, temperatureReceiverStabilityFlag);
				file.write(*relativeHumidityStabilityFlagVariable, relativeHumidityReceiverStabilityFlag);
			}
		}

		sci::GridData<uint8_t, 1> getFilter() const
		{
			sci::GridData<uint8_t, 1> filter(rainFlag.size());
			for (size_t i = 0; i < filter.size(); ++i)
			{
				filter[i] = anyBad(i) ? 2 : 1;
			}
			return filter;
		}

		bool anyBad(size_t index) const
		{
			return metFlags.anyBad(index) ||
				rainFlag[index] > 1 ||
				channelFailureFlags[0][index] > 1 ||
				channelFailureFlags[1][index] > 1 ||
				channelFailureFlags[2][index] > 1 ||
				channelFailureFlags[3][index] > 1 ||
				channelFailureFlags[4][index] > 1 ||
				channelFailureFlags[5][index] > 1 ||
				channelFailureFlags[6][index] > 1 ||
				channelFailureFlags[7][index] > 1 ||
				channelFailureFlags[8][index] > 1 ||
				channelFailureFlags[9][index] > 1 ||
				channelFailureFlags[10][index] > 1 ||
				channelFailureFlags[11][index] > 1 ||
				channelFailureFlags[12][index] > 1 ||
				channelFailureFlags[13][index] > 1 ||
				temperatureReceiverStabilityFlag[index] > 1 ||
				relativeHumidityReceiverStabilityFlag[index] > 1;
		}

	private:
		MetFlags metFlags;

		sci::GridData<uint8_t, 1> rainFlag;
		sci::GridData<sci::GridData<uint8_t, 1>, 1> channelFailureFlags;
		sci::GridData<uint8_t, 1> temperatureReceiverStabilityFlag;
		sci::GridData<uint8_t, 1> relativeHumidityReceiverStabilityFlag;

		std::unique_ptr<AmfNcFlagVariable> rainFlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch1FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch2FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch3FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch4FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch5FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch6FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch7FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch8FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch9FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch10FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch11FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch12FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch13FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> ch14FlagVariable;
		std::unique_ptr<AmfNcFlagVariable> temperatureStabilityFlagVariable;
		std::unique_ptr<AmfNcFlagVariable> relativeHumidityStabilityFlagVariable;

		bool createdStabilityFlags;
	};

	void static buildMetAndStabilityFlags(sci::GridData<uint8_t, 1>& surfaceTemperatureFlag,
		sci::GridData<uint8_t, 1>& surfacePressureFlag,
		sci::GridData<uint8_t, 1>& surfaceRelativeHumidityFlag,
		sci::GridData<uint8_t, 1>& rainFlag,
		sci::GridData<sci::GridData<uint8_t, 1>, 1>& channelFailureFlags,
		sci::GridData<uint8_t, 1>& temperatureReceiverStabilityFlag,
		sci::GridData<uint8_t, 1>& relativeHumidityReceiverStabilityFlag,
		const sci::GridData<bool, 1> &rawRainFlag,
		const sci::GridData<uint32_t, 1> &status,
		const sci::GridData<kelvinF, 1> &temperatureStabilityReceiver1,
		const sci::GridData<kelvinF, 1> &temperatureStabilityReceiver2
		)
	{
		surfaceTemperatureFlag.clear();
		surfacePressureFlag.clear();
		surfaceRelativeHumidityFlag.clear();
		rainFlag.clear();
		channelFailureFlags.clear();
		temperatureReceiverStabilityFlag.clear();
		relativeHumidityReceiverStabilityFlag.clear();


		const size_t nPoints = status.size();

		surfaceTemperatureFlag.resize(nPoints, mwrGoodDataFlag);
		surfacePressureFlag.resize(nPoints, mwrGoodDataFlag);
		surfaceRelativeHumidityFlag.resize(nPoints, mwrGoodDataFlag);

		//set the rest of the flags
		rainFlag.resize(nPoints, mwrRainGoodDataFlag);
		for (size_t i = 0; i < rainFlag.size(); ++i)
			if (rawRainFlag[i])
				rainFlag[i] = mwrRainRainingFlag;

		channelFailureFlags.resize(14, sci::GridData<uint8_t, 1>(nPoints));
		sci::GridData<uint32_t, 1> statusFilters{ 0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000100, 0x00000200,
			0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000 }; //note 0x0080 and 0x8000 are missing deliberately
		for (size_t i = 0; i < 14; ++i)
			for (size_t j = 0; j < nPoints; ++j)
				channelFailureFlags[i][j] = (status[j] & statusFilters[i]) == 0 ? mwrChannelFailFlag : mwrChannelGoodDataFlag;

		temperatureReceiverStabilityFlag.resize(nPoints, mwrStabilityGoodDataFlag);
		for (size_t i = 0; i < temperatureReceiverStabilityFlag.size(); ++i)
		{
			if (temperatureStabilityReceiver1[i] > millikelvinF(50))
				temperatureReceiverStabilityFlag[i] = mwrStabilityVeryVeryPoorFlag;
			else if (temperatureStabilityReceiver1[i] > millikelvinF(10))
				temperatureReceiverStabilityFlag[i] = mwrStabilityVeryPoorFlag;
			else if (temperatureStabilityReceiver1[i] > millikelvinF(5))
				temperatureReceiverStabilityFlag[i] = mwrStabilityPoorFlag;
		}

		relativeHumidityReceiverStabilityFlag.resize(nPoints, mwrStabilityGoodDataFlag);
		for (size_t i = 0; i < relativeHumidityReceiverStabilityFlag.size(); ++i)
		{
			if (temperatureStabilityReceiver2[i] > millikelvinF(50))
				relativeHumidityReceiverStabilityFlag[i] = mwrStabilityVeryVeryPoorFlag;
			else if (temperatureStabilityReceiver2[i] > millikelvinF(10))
				relativeHumidityReceiverStabilityFlag[i] = mwrStabilityVeryPoorFlag;
			else if (temperatureStabilityReceiver2[i] > millikelvinF(5))
				relativeHumidityReceiverStabilityFlag[i] = mwrStabilityPoorFlag;
		}
	}

	static DataInfo buildDataInfo(sci::string productName, const sci::GridData<sci::UtcTime, 1> &dataTime, const ProcessingOptions& processingOptions)
	{
		sci::assertThrow(dataTime.size() > 1, sci::err(sci::SERR_USER, 0, sU("Tried to process Hatpro data with fewer than 2 data points.")));
		DataInfo dataInfo;
		dataInfo.averagingPeriod = std::numeric_limits<secondF>::quiet_NaN();
		dataInfo.samplingInterval = std::numeric_limits<secondF>::quiet_NaN();
		dataInfo.continuous = true;
		dataInfo.startTime = dataTime[0];
		dataInfo.endTime = dataTime.back();

		dataInfo.featureType = FeatureType::timeSeriesPoint;
		dataInfo.options = std::vector<sci::string>(0);
		dataInfo.processingLevel = 1;
		dataInfo.productName = sU("iwv lwp");
		dataInfo.processingOptions = processingOptions;

		sci::GridData<secondF, 1> intervals(dataTime.size() - 1);
		for (size_t i = 0; i < intervals.size(); ++i)
			intervals[i] = secondF(dataTime[i + 1] - dataTime[i]);
		dataInfo.averagingPeriod = sci::median(intervals);
		dataInfo.samplingInterval = dataInfo.averagingPeriod;

		return dataInfo;
	}

	void writeIwvLwpNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter);

	void writeSurfaceMetNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter);

	void writeStabilityNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter);

	void writeBrightnessTemperatureNc(const sci::string& directory, const PersonInfo& author,
		const ProcessingSoftwareInfo& processingSoftwareInfo, const ProjectInfo& projectInfo,
		const Platform& platform, const ProcessingOptions& processingOptions, ProgressReporter& progressReporter);
private:
	InstrumentInfo m_instrumentInfo;
	CalibrationInfo m_calibrationInfo;
	bool m_hasData;

	//liquid water path data
	sci::GridData<sci::UtcTime, 1> m_lwpTime;
	sci::GridData<gramPerMetreSquaredF, 1> m_lwp;
	sci::GridData<degreeF, 1> m_lwpElevation;
	sci::GridData<degreeF, 1> m_lwpAzimuth;
	sci::GridData<bool, 1> m_lwpRainFlag;
	sci::GridData<unsigned char, 1> m_lwpQuality;
	sci::GridData<unsigned char, 1> m_lwpQualityExplanation;

	//integrated water vapour data
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

	//met data
	sci::GridData<sci::UtcTime, 1> m_metTime;
	sci::GridData<kelvinF, 1> m_enviromentTemperature;
	sci::GridData<hectoPascalF, 1> m_enviromentPressure;
	sci::GridData<unitlessF, 1> m_enviromentRelativeHumidity;

	//stability data
	sci::GridData<sci::UtcTime, 1> m_staTime;
	sci::GridData<bool, 1> m_staRainFlag;
	sci::GridData<kelvinF, 1> m_liftedIndex;
	sci::GridData<kelvinF, 1> m_kModifiedIndex;
	sci::GridData<kelvinF, 1> m_totalTotalsIndex;
	sci::GridData<kelvinF, 1> m_kIndex;
	sci::GridData<kelvinF, 1> m_showalterIndex;
	sci::GridData<joulePerKilogramF, 1> m_cape;

	//boundary layer depth data
	sci::GridData<sci::UtcTime, 1> m_bldTime;
	sci::GridData<metreF, 1> m_boundaryLayerDepth;

	//Brightness temperature data
	sci::GridData<sci::UtcTime, 1> m_brtTime;
	sci::GridData<perSecondF, 1> m_brtFrequencies;
	sci::GridData<kelvinF, 2> m_brightnessTemperature;
	sci::GridData<degreeF, 1> m_brtElevation;
	sci::GridData<degreeF, 1> m_brtAzimuth;
	sci::GridData<bool, 1> m_brtRainFlag;

	//Attenuation data
	sci::GridData<sci::UtcTime, 1> m_atnTime;
	sci::GridData<perSecondF, 1> m_atnFrequencies;
	sci::GridData<unitlessF, 2> m_attenuation;
	sci::GridData<degreeF, 1> m_atnElevation;
	sci::GridData<degreeF, 1> m_atnAzimuth;
	sci::GridData<bool, 1> m_atnRainFlag;
};

template<class DATA_UNIT, size_t NDIMS>
static void MicrowaveRadiometerProcessor::padDataToMatchHkp(const sci::GridData<sci::UtcTime, 1>& hkdTime, sci::GridData<sci::UtcTime, 1>& dataTime, sci::GridData<DATA_UNIT, NDIMS>& data,
	sci::GridData<degreeF, 1>& elevation, sci::GridData<degreeF, 1>& azimuth, sci::GridData<bool, 1>& rainFlag, sci::GridData<unsigned char, 1>& quality,
	sci::GridData<unsigned char, 1>& qualityExplanation, sci::GridData<uint8_t, 1> &flag)
{
	for (size_t i = 0; i < std::min(dataTime.size(), hkdTime.size()); ++i)
	{
		if (dataTime[i] > hkdTime[i])
		{
			size_t j;
			for (j = i; j < hkdTime.size(); ++j)
				if (dataTime[i] <= hkdTime[j])
					break;
			if (j == hkdTime.size())
			{
				break;
			}
			else
			{
				//if we found an exact match for our time, then pad with nans
				//if we went past our time, then also nan out the curent point - there is no matching hkd time
				size_t nToPad = j - i;
				if (dataTime[i] < hkdTime[j])
				{
					--nToPad;
					dataTime[i] = hkdTime[j];
					data[i] = std::numeric_limits<DATA_UNIT>::quiet_NaN();
					elevation[i] = std::numeric_limits<degreeF>::quiet_NaN();
					azimuth[i] = std::numeric_limits<degreeF>::quiet_NaN();
					rainFlag[i] = mwrRainMissingDataFlag;
					quality[i] = mwrMissingDataFlag;
					qualityExplanation[i] = 0;
					flag[i] = mwrMissingDataFlag;
				}
				//now pad
				if (nToPad > 0) //this can actually be zero if there was just one bad time and it is decremented above
				{
					dataTime.insert(i, hkdTime.subGrid(i, nToPad));
					data.insert(i, nToPad, std::numeric_limits<DATA_UNIT>::quiet_NaN());
					elevation.insert(i, nToPad, std::numeric_limits<degreeF>::quiet_NaN());
					azimuth.insert(i, nToPad, std::numeric_limits<degreeF>::quiet_NaN());
					rainFlag.insert(i, nToPad, mwrRainMissingDataFlag);
					quality.insert(i, nToPad, mwrMissingDataFlag);
					qualityExplanation.insert(i, nToPad, 0);
					flag.insert(i, nToPad, mwrMissingDataFlag);
				}
			}
		}
	}
	if (dataTime.size() < hkdTime.size())
	{
		dataTime.insert(dataTime.size(), hkdTime.subGrid(dataTime.size(), hkdTime.size() - dataTime.size()));
		data.resize(hkdTime.size(), std::numeric_limits<DATA_UNIT>::quiet_NaN());
		elevation.resize(hkdTime.size(), std::numeric_limits<degreeF>::quiet_NaN());
		azimuth.resize(hkdTime.size(), std::numeric_limits<degreeF>::quiet_NaN());
		rainFlag.resize(hkdTime.size(), mwrRainMissingDataFlag);
		quality.resize(hkdTime.size(), mwrMissingDataFlag);
		qualityExplanation.resize(hkdTime.size(), 0);
		flag.resize(hkdTime.size(), mwrMissingDataFlag);
	}
}

template<class DATA_UNIT, size_t NDIMS>
static void MicrowaveRadiometerProcessor::padDataToMatchHkp(const sci::GridData<sci::UtcTime, 1>& hkdTime, sci::GridData<sci::UtcTime, 1> dataTime, sci::GridData<DATA_UNIT, NDIMS>& data, DATA_UNIT padValue)
{
	for (size_t i = 0; i < std::min(dataTime.size(), hkdTime.size()); ++i)
	{
		if (dataTime[i] > hkdTime[i])
		{
			size_t j;
			for (j = i; j < hkdTime.size(); ++j)
				if (dataTime[i] <= hkdTime[j])
					break;
			if (j == hkdTime.size())
			{
				break;
			}
			else
			{
				//if we found an exact match for our time, then pad with nans
				//if we went past our time, then also nan out the curent point - there is no matching hkd time
				size_t nToPad = j - i;
				if (dataTime[i] < hkdTime[j])
				{
					--nToPad;
					dataTime[i] = hkdTime[j];
					data[i] = padValue;
				}
				//now pad
				if (nToPad > 0) //this can actually be zero if there was just one bad time and it is decremented above
				{
					dataTime.insert(i, hkdTime.subGrid(i, nToPad));
					data.insert(i, nToPad, padValue);
				}
			}
		}
	}
	if (dataTime.size() < hkdTime.size())
	{
		dataTime.insert(dataTime.size(), hkdTime.subGrid(dataTime.size(), hkdTime.size() - dataTime.size()));
		auto shape = data.shape();
		shape[0] = hkdTime.size();
		data.reshape(shape, padValue);
	}
}