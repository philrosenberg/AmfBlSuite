#pragma once

#include<iostream>
#include<string>
#include<vector>
#include"Units.h"
#include<svector/serr.h>

enum class campbellMessageType
{
	cs,
	cl,
	ct
};

enum class campbellAlarmStatus
{
	ok,
	warning,
	alarm
};

enum class ceilometerMessageStatus
{
	noSignificantBackscatter,
	oneCloudBase,
	twoCloudBases,
	threeCloudBases,
	fourCloudBases,
	fullObscurationNoCloudBase,
	someObscurationTransparent,
	rawDataMissingOrSuspect
};

class CampbellHeader
{
public:
	CampbellHeader(char startOfHeaderCharacter = '\01', char startOfTextCharacter = '\02');
	void readHeader(std::istream &stream);
	char getStartOfHeaderCharacter() const { return m_startOfHeaderCharacter; }
	char getStartOfTextCharacter() const { return m_startOfTextCharacter; }
	campbellMessageType getMessageType() const { sci::assertThrow(m_initialised, sci::err(sci::SERR_USER, 0, sU("Attempted to get the message type for an uninitialised CampbellHeader object."))); return m_messageType; }
	std::string getOs() const { sci::assertThrow(m_initialised, sci::err(sci::SERR_USER, 0, sU("Attempted to get the OS for an uninitialised CampbellHeader object."))); return m_os; }
	short getMessageNumber() const { sci::assertThrow(m_initialised, sci::err(sci::SERR_USER, 0, sU("Attempted to get the message number for an uninitialised CampbellHeader object."))); return m_messageNumber; }
	char getId() const { sci::assertThrow(m_initialised, sci::err(sci::SERR_USER, 0, sU("Attempted to get the ID for an uninitialised CampbellHeader object."))); return m_id; }
	const std::vector<char>& getBytes() const { sci::assertThrow(m_initialised, sci::err(sci::SERR_USER, 0, sU("Attempted to get the bytes for an uninitialised CampbellHeader object."))); return m_bytes; }
	bool isInitialised() const { return m_initialised; }
	CampbellHeader(const CampbellHeader&) = default;
private:
	char m_startOfHeaderCharacter;
	char m_startOfTextCharacter;
	campbellMessageType m_messageType;
	std::string m_os;
	short m_messageNumber;
	char m_id;
	bool m_initialised;
	std::vector<char> m_bytes;
};



class DefaultMessage
{
public:
	//on windows the passed stream must be binary
	void readMessage(std::istream &stream);
};

class CampbellMessage2
{
public:
	CampbellMessage2(char endOfTextCharacter = '\3');
	void read(std::istream &stream, const CampbellHeader &header);
	const std::vector<perSteradianPerKilometreF> &getData() const { return m_data; }
	std::vector<metreF> getHeights() const { return std::vector<metreF>{m_height1, m_height2, m_height3, m_height4, }; }
	metreF getHeight1() const { return m_height1; }
	metreF getHeight2() const { return m_height2; }
	metreF getHeight3() const { return m_height3; }
	metreF getHeight4() const { return m_height4; }
	metreF getVisibility() const { return m_visibility; }
	double getHighestSignal() const { return m_highestSignal; }
	percentF getWindowTransmission() const { return m_windowTransmission; }
	percentF getScale() const { return m_scale; }
	metreF getResolution() const { return m_resolution; }
	percentF getLaserPulseEnergy() const { return m_laserPulseEnergy; }
	kelvinF getLaserTemperature() const { return m_laserTemperature; }
	degreeF getTiltAngle() const { return m_tiltAngle; }
	millivoltF getBackground() const { return m_background; }
	size_t getPulseQuantity() const { return m_pulseQuantity; }
	hertzF getSampleRate() const { return m_sampleRate; }
	perSteradianF getSum() const { return m_sum; }
	const bool getPassedChecksum() const { return m_passedChecksum; }
	campbellAlarmStatus getAlarmStatus() const { return m_alarmStatus; }
	ceilometerMessageStatus getMessageStatus() const { return m_messageStatus; }
private:
	char m_endOfTextCharacter;
	std::vector<perSteradianPerKilometreF> m_data;
	metreF m_height1;
	metreF m_height2;
	metreF m_height3;
	metreF m_height4;
	metreF m_visibility;
	double m_highestSignal;
	percentF m_windowTransmission;
	percentF m_scale;
	metreF m_resolution;
	percentF m_laserPulseEnergy;
	kelvinF m_laserTemperature;
	degreeF m_tiltAngle;
	millivoltF m_background;
	size_t m_pulseQuantity;
	megahertzF m_sampleRate;
	perSteradianF m_sum;
	bool m_passedChecksum;
	campbellAlarmStatus m_alarmStatus;
	ceilometerMessageStatus m_messageStatus;
};