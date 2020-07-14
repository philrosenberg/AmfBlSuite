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
	const std::vector<perSteradianPerKilometre> &getData() const { return m_data; }
	std::vector<metre> getHeights() const { return std::vector<metre>{m_height1, m_height2, m_height3, m_height4, }; }
	metre getHeight1() const { return m_height1; }
	metre getHeight2() const { return m_height2; }
	metre getHeight3() const { return m_height3; }
	metre getHeight4() const { return m_height4; }
	metre getVisibility() const { return m_visibility; }
	double getHighestSignal() const { return m_highestSignal; }
	percent getWindowTransmission() const { return m_windowTransmission; }
	percent getScale() const { return m_scale; }
	metre getResolution() const { return m_resolution; }
	percent getLaserPulseEnergy() const { return m_laserPulseEnergy; }
	kelvin getLaserTemperature() const { return m_laserTemperature; }
	degree getTiltAngle() const { return m_tiltAngle; }
	millivolt getBackground() const { return m_background; }
	size_t getPulseQuantity() const { return m_pulseQuantity; }
	hertz getSampleRate() const { return m_sampleRate; }
	perSteradian getSum() const { return m_sum; }
	const bool getPassedChecksum() const { return m_passedChecksum; }
	campbellAlarmStatus getAlarmStatus() const { return m_alarmStatus; }
	ceilometerMessageStatus getMessageStatus() const { return m_messageStatus; }
private:
	char m_endOfTextCharacter;
	std::vector<perSteradianPerKilometre> m_data;
	metre m_height1;
	metre m_height2;
	metre m_height3;
	metre m_height4;
	metre m_visibility;
	double m_highestSignal;
	percent m_windowTransmission;
	percent m_scale;
	metre m_resolution;
	percent m_laserPulseEnergy;
	kelvin m_laserTemperature;
	degree m_tiltAngle;
	millivolt m_background;
	size_t m_pulseQuantity;
	megahertz m_sampleRate;
	perSteradian m_sum;
	bool m_passedChecksum;
	campbellAlarmStatus m_alarmStatus;
	ceilometerMessageStatus m_messageStatus;
};