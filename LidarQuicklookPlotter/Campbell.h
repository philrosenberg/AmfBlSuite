#pragma once

#include<iostream>
#include<string>
#include<vector>

enum campbellMessageType
{
	cmt_cs,
	cmt_cl,
	cmt_ct
};

class CampbellHeader
{
public:
	CampbellHeader(char startOfHeaderCharacter='\01', char startOfTextCharacter='\02');
	void readHeader(std::istream &stream);
	char getStartOfHeaderCharacter() const { return m_startOfHeaderCharacter; }
	char getStartOfTextCharacter() const { return m_startOfTextCharacter; }
	campbellMessageType getMessageType() const { if (!m_initialised) throw("Attempted to get the message type for an uninitialised CampbellHeader object."); return m_messageType; }
	std::string getOs() const { if (!m_initialised) throw("Attempted to get the OS for an uninitialised CampbellHeader object."); return m_os; }
	short getMessageNumber() const { if (!m_initialised) throw("Attempted to get the message number for an uninitialised CampbellHeader object."); return m_messageNumber; }
	char getId() const { if (!m_initialised) throw("Attempted to get the ID for an uninitialised CampbellHeader object."); return m_id; }
	bool isInitialised() const { return m_initialised; }
	CampbellHeader(const CampbellHeader&) = default;
	//inline CampbellHeader &operator=(const CampbellHeader &);
private:
	char m_startOfHeaderCharacter;
	char m_startOfTextCharacter;
	campbellMessageType m_messageType;
	std::string m_os;
	short m_messageNumber;
	char m_id;
	bool m_initialised;
};


//inline CampbellHeader &CampbellHeader::operator=(const CampbellHeader &) = default;

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
	void read(std::istream &stream);
	const std::vector<double> &getData() const { return m_data; }
	std::vector<double> getHeights() const { return std::vector<double>{m_height1, m_height2, m_height3, m_height4, }; }
	double getHeight1() const { return m_height1; }
	double getHeight2() const { return m_height2; }
	double getHeight3() const { return m_height3; }
	double getHeight4() const { return m_height4; }
	double getResolution() const { return m_resolution; }
	double getPulseQuantity() const { return m_pulseQuantity; }
	double getSampleRate() const { return m_sampleRate; }
private:
	char m_endOfTextCharacter;
	std::vector<double> m_data;
	double m_height1;
	double m_height2;
	double m_height3;
	double m_height4;
	double m_visibility;
	double m_highestSignal;
	double m_windowTransmission;
	double m_scale;
	double m_resolution;
	double m_laserPulseEnergy;
	double m_laserTemperature;
	double m_tiltAngle;
	double m_background;
	double m_pulseQuantity;
	double m_sampleRate;
	double m_sum;
};