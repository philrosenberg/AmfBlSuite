#include"Campbell.h"
#include<stdint.h>
#include<vector>
#include<string>
#include<sstream>


// ----------------------------------------------------
// Calculate CRC16 checksum
// buf is a pointer to the input string
// len is the length of the input string
// copied directly from the cs135 manual, but with types
// changed to fixed size types and the function name and
// result variable name changed
// ----------------------------------------------------
uint16_t generateChecksum(char *buffer, size_t length)
{
	uint16_t checksum;
	uint16_t m;
	int32_t i, j;
	checksum = 0xFFFF;
	for (i = 0; i < length; ++i)
	{
		checksum ^= buffer[i] << 8;
		for (j = 0; j<8; ++j) {
			m = (checksum & 0x8000) ? 0x1021 : 0;
			checksum <<= 1;
			checksum ^= m;
		}
	}
	checksum ^= 0xFFFF;
	return checksum;
}

CampbellHeader::CampbellHeader(char startOfHeaderCharacter, char startOfTextCharacter)
	:m_startOfHeaderCharacter(startOfHeaderCharacter), m_startOfTextCharacter(startOfTextCharacter)
{
	m_initialised = false;
}

void CampbellHeader::readHeader(std::istream &stream)
{
	char commonHeader[8];
	stream.read(commonHeader, 7);

	//check the start header character
	if(commonHeader[0] != m_startOfHeaderCharacter)
	{
		std::ostringstream message;
		message << "Expected Campbell header to start with character '" << m_startOfHeaderCharacter << "', but attempting to read a header beginning with '" << commonHeader[0] << "'.";
		throw(message.str());
	}

	//check what type of message we have
	if (commonHeader[1] == 'C' && commonHeader[2] == 'S')
		m_messageType = cmt_cs;
	else if (commonHeader[1] == 'C' && commonHeader[2] == 'L')
		m_messageType = cmt_cl;
	else if (commonHeader[1] == 'C' && commonHeader[2] == 'T')
		m_messageType = cmt_ct;
	else
	{
		std::ostringstream message;
		message << "Found a Campbell message with an unknown type: " << commonHeader[1] << ". Only CS, CL and CT are supported.";
		throw(message.str());
	}

	//grab the id and the OS
	m_id = commonHeader[3];
	m_os = std::string(&commonHeader[4], &commonHeader[7]);

	//if this is a CS message then get the message number
	if (m_messageType == cmt_cs)
	{
		char messageNumberText[4];
		stream.read(messageNumberText, 3);
		messageNumberText[3] = '\0';
		if (messageNumberText[0] < '0' || messageNumberText[0] > '9'
			|| messageNumberText[1] < '0' || messageNumberText[1] > '9'
			|| messageNumberText[2] < '0' || messageNumberText[2] > '9')
		{

			std::ostringstream message;
			message << "Found an invalid Campbell message number: " << messageNumberText << ". This should be a number from 000-999.";
			throw(message.str());
		}
		m_messageNumber = std::atoi(messageNumberText);
	}

	//if this is a CL message then get the Samples number, which defines the message number
	else if(m_messageType == cmt_cl)
	{

		char messageNumberText[3];
		stream.read(messageNumberText, 2);
		messageNumberText[2] = '\0';
		if (messageNumberText[0] == '1')
		{
			if (messageNumberText[1] == '1')
				m_messageNumber = 101;
			else if (messageNumberText[1] == '2')
				m_messageNumber = 102;
			else if (messageNumberText[1] == '3')
				m_messageNumber = 103;
			else if (messageNumberText[1] == '4')
				m_messageNumber = 104;
			else if (messageNumberText[1] == '5')
				m_messageNumber = 105;
			else if (messageNumberText[1] == '0')//This IS 0 in the manual, but is a bit suspicous - maybe should be 6
				m_messageNumber = 106;
			else
			{
				std::ostringstream message;
				message << "Found an invalid Campbell CL Samples value. The value shoud be a number form 0-5, but we found the character '" << messageNumberText[1] << "'.";
				throw(message.str());
			}
		}
		else if (messageNumberText[0] == '2')
		{
			if (messageNumberText[1] == '1')
				m_messageNumber = 107;
			else if (messageNumberText[1] == '2')
				m_messageNumber = 108;
			else if (messageNumberText[1] == '3')
				m_messageNumber = 109;
			else if (messageNumberText[1] == '4')
				m_messageNumber = 110;
			else if (messageNumberText[1] == '5')
				m_messageNumber = 111;
			else if (messageNumberText[1] == '0')//This IS 0 in the manual, but is a bit suspicous - maybe should be 6
				m_messageNumber = 112;
			else
			{
				std::ostringstream message;
				message << "Found an invalid Campbell CL Samples value. The value shoud be a number form 0-5, but we found the character '" << messageNumberText[1] << "'.";
				throw(message.str());
			}
		}
		else
		{
			std::ostringstream message;
			message << "Found an invalid Campbell CL header. The character after the OS shoud always be '1' or '2'. Here we found '" << messageNumberText[0] << "'.";
			throw(message.str());
		}
		m_messageNumber = std::atoi(messageNumberText);
	}
	//for ct type we don't have the message number in the header
	//the OS was only 2 characters, not 3 as in the other types
	//and should be followed by "10"
	else if (m_messageType == cmt_ct)
	{
		char shouldBeZero;
		stream.read(&shouldBeZero, 1);
		char shouldBeOne = m_os[2];
		m_os = m_os.substr(0, 2);
		m_messageNumber = 0;
		if (shouldBeZero != '0' || shouldBeOne != '1')
		{
			std::ostringstream message;
			message << "Found an invalid Campbell CT header. The characters after the OS should be \"10\". Here we found \"" << shouldBeOne << shouldBeZero << "\".";
			throw(message.str());
		}
	}


	//Check we end with the text start character and crlf
	char headerEnd[3];
	stream.read(headerEnd, 3);
	if(headerEnd[0] != m_startOfTextCharacter)
	{
		std::ostringstream message;
		message << "Found an invalid Campbell header. Expected to find the start of text character '" << m_startOfTextCharacter << "', but found '" << headerEnd[0] << "'.";
		throw(message.str());
	}
	if (headerEnd[1] != char(13) || headerEnd[2] != char(10))
	{
		std::ostringstream message;
		message << "Found an invalid Campbell header. Expected the header to end with the carriage return and line feed characters, but it did not.";
		throw(message.str());
	}
	m_initialised = true;
}

void DefaultMessage::readMessage(std::istream &stream)
{

}

CampbellMessage2::CampbellMessage2(char endOfTextCharacter)
	:m_endOfTextCharacter(endOfTextCharacter)
{
}

void CampbellMessage2::read(std::istream &stream)
{
	char crlf[2];
	char space;

	char messageStatus;
	char alarmStatus;
	char transmission[3];
	char height1[5];
	char height2[5];
	char height3[5];
	char height4[5];
	char flags[12];
	stream.read(&messageStatus, 1);
	stream.read(&alarmStatus, 1);
	stream.read(&space, 1);
	stream.read(transmission, 3);
	stream.read(&space, 1);
	stream.read(height1, 5);
	stream.read(&space, 1);
	stream.read(height2, 5);
	stream.read(&space, 1);
	stream.read(height3, 5);
	stream.read(&space, 1);
	stream.read(height4, 5);
	stream.read(&space, 1);
	stream.read(flags, 12);
	stream.read(crlf, 2);

	char scale[5];
	char res[2];
	char n[4];
	char energy[3];
	char laserTemperature[3];
	char tiltAngle[2];
	char background[4];
	char pulseQuantity[4];
	char sampleRate[2];
	char sum[3];
	stream.read(scale, 5);
	stream.read(&space, 1);
	stream.read(res, 2);
	stream.read(&space, 1);
	stream.read(n, 4);
	stream.read(&space, 1);
	stream.read(energy, 3);
	stream.read(&space, 1);
	stream.read(laserTemperature, 3);
	stream.read(&space, 1);
	stream.read(tiltAngle, 2);
	stream.read(&space, 1);
	stream.read(background, 4);
	stream.read(&space, 1);
	stream.read(pulseQuantity, 4);
	stream.read(&space, 1);
	stream.read(sampleRate, 2);
	stream.read(&space, 1);
	stream.read(sum, 3);
	stream.read(crlf, 2);
	
	std::vector<char> data(10240);
	stream.read(&data[0], 10240);
	stream.read(crlf, 2);

	char endOfTextCharacter;
	char checksum[4];
	stream.read(&endOfTextCharacter, 1);
	stream.read(checksum, 4);
}