#pragma once

#include<iostream>
#include<string>

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
	char getStartOfHeaderCharacter() { return m_startOfHeaderCharacter; }
	char getStartOfTextCharacter() { return m_startOfTextCharacter; }
	campbellMessageType getMessageType() { if (!m_initialised) throw("Attempted to get the message type for an uninitialised CampbellHeader object."); return m_messageType; }
	std::string getOs() { if (!m_initialised) throw("Attempted to get the OS for an uninitialised CampbellHeader object."); return m_os; }
	short getMessageNumber() { if (!m_initialised) throw("Attempted to get the message number for an uninitialised CampbellHeader object."); return m_messageNumber; }
	char getId() { if (!m_initialised) throw("Attempted to get the ID for an uninitialised CampbellHeader object."); return m_id; }
	bool isInitialised() { return m_initialised; }
private:
	const char m_startOfHeaderCharacter;
	const char m_startOfTextCharacter;
	campbellMessageType m_messageType;
	std::string m_os;
	short m_messageNumber;
	char m_id;
	bool m_initialised;
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
	void read(std::istream &stream);
private:
	const char m_endOfTextCharacter;
};