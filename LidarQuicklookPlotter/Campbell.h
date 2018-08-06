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