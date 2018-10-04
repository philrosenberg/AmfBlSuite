#define _USE_MATH_DEFINES //This ensures we get values for math constants when we include cmath
#include"Campbell.h"
#include<stdint.h>
#include<vector>
#include<string>
#include<sstream>
#include<cmath>


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
	m_bytes = std::vector<char>(commonHeader, commonHeader + 7);

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
		m_bytes.insert(m_bytes.end(), messageNumberText, messageNumberText + 3);
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
		m_bytes.insert(m_bytes.end(), messageNumberText, messageNumberText + 2);
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
		m_bytes.insert(m_bytes.end(), shouldBeZero);
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
	m_bytes.insert(m_bytes.end(), headerEnd, headerEnd + 3);
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

int hexCharToNumber(char hexChar)
{
	if (hexChar > char(47) && hexChar < char(58))
		return int(hexChar) - int(48);
	else if (hexChar > char(96) && hexChar < char(103))
		return int(hexChar) - int(87);

	else
		throw("Recieved a non hex number to parse.");
}

int hexTextToNumber(char* textHex)
{
	int result = (((hexCharToNumber(textHex[0])*16+ hexCharToNumber(textHex[1]))*16 + hexCharToNumber(textHex[2]))*16 + hexCharToNumber(textHex[3]))*16 + hexCharToNumber(textHex[4]);
	//two's compliment format means the first bit is actually negative - correct for the
	//fact that we have aded it on.
	if (result > (8 * 16 * 16 * 16 * 16))
		result -= 2 * 8 * 16 * 16 * 16 * 16;

	return result;
}

CampbellMessage2::CampbellMessage2(char endOfTextCharacter)
	:m_endOfTextCharacter(endOfTextCharacter)
{
}

campbellAlarmStatus getAlarmStatus(char text)
{
	if (text == '0')
		return cas_ok;
	if (text == 'W')
		return cas_warning;
	if (text == 'A')
		return cas_alarm;

	//If we haven't returned by this point we have an invalid character
	throw("Received an invalid character when determining the ceilometer alarm status.");
}

ceilometerMessageStatus getMessageStatus(char text)
{
	//because the status character uses consecutive characters for the
	//first 7 messages and because we use the same order in the enumeration
	//we can use this shortcut to avoid loads of if statements
	if (text >= '0' && text <= '6')
		return (ceilometerMessageStatus)(cms_noSignificantBackscatter + (text - '0'));
	if (text == '/')
		return cms_rawDataMissingOrSuspect;

	//If we haven't returned by this point we have an invalid character
	throw("Received an invalid character when determining the ceilometer message status.");
}

void CampbellMessage2::read(std::istream &istream, const CampbellHeader &header)
{

	//first read in the whole stream and tag it onto the header. We will use this
	//for checksum calculation
	std::vector<char> buffer = header.getBytes();
	size_t messageStartByte = buffer.size();
	buffer.resize(buffer.size()+10335);
	istream.read(&buffer[messageStartByte], 10335);

	//create a binary stream from the data that we will use to parse each of the values;
	std::istringstream bufferStream(std::string(&buffer[messageStartByte], buffer.size()- messageStartByte));

	//parse all the variables from the bufferStream

	char crlf[2];
	char space;

	char messageStatus;
	char alarmStatus;
	char transmission[4];
	char height1[6];
	char height2[6];
	char height3[6];
	char height4[6];
	char flags[12];
	bufferStream.read(&messageStatus, 1);
	bufferStream.read(&alarmStatus, 1);
	bufferStream.read(&space, 1);
	bufferStream.read(transmission, 3);
	bufferStream.read(&space, 1);
	bufferStream.read(height1, 5);
	bufferStream.read(&space, 1);
	bufferStream.read(height2, 5);
	bufferStream.read(&space, 1);
	bufferStream.read(height3, 5);
	bufferStream.read(&space, 1);
	bufferStream.read(height4, 5);
	bufferStream.read(&space, 1);
	bufferStream.read(flags, 12);
	bufferStream.read(crlf, 2);

	height1[5] = '\0';
	height2[5] = '\0';
	height3[5] = '\0';
	height4[5] = '\0';
	transmission[3] = '\0';
	m_height1 = metre(std::numeric_limits<double>::quiet_NaN());
	m_height2 = metre(std::numeric_limits<double>::quiet_NaN());
	m_height3 = metre(std::numeric_limits<double>::quiet_NaN());
	m_height4 = metre(std::numeric_limits<double>::quiet_NaN());
	m_visibility = metre(std::numeric_limits<double>::quiet_NaN());
	m_highestSignal = std::numeric_limits<double>::quiet_NaN();

	if (messageStatus == '5')
	{
		m_visibility = metre(std::atof(height1));
		m_highestSignal = std::atof(height2);
	}
	else if(messageStatus < '5')
	{
		m_height1 = metre(std::atof(height1));
		if (messageStatus > '1')
			m_height2 = metre(std::atof(height2));
		if (messageStatus > '2')
			m_height3 = metre(std::atof(height3));
		if (messageStatus > '3')
			m_height4 = metre(std::atof(height4));
	}
	m_windowTransmission = percent(std::atof(transmission));
	m_alarmStatus = getAlarmStatus(alarmStatus);
	m_messageStatus = getMessageStatus(messageStatus);

	char scale[6];
	char res[3];
	char n[5];
	char energy[4];
	char laserTemperature[4];
	char tiltAngle[3];
	char background[5];
	char pulseQuantity[5];
	char sampleRate[3];
	char sum[4];
	bufferStream.read(scale, 5);
	scale[5] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(res, 2);
	res[2] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(n, 4);
	n[4] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(energy, 3);
	energy[3] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(laserTemperature, 3);
	laserTemperature[3] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(tiltAngle, 2);
	tiltAngle[2] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(background, 4);
	background[4] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(pulseQuantity, 4);
	pulseQuantity[4] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(sampleRate, 2);
	sampleRate[2] = '\0';
	bufferStream.read(&space, 1);
	bufferStream.read(sum, 3);
	sum[3] = '\0';
	bufferStream.read(crlf, 2);

	m_scale = percent(std::atof(scale));
	m_resolution = metre(std::atof(res));
	m_laserPulseEnergy = percent(std::atof(energy));
	m_laserTemperature = kelvin(std::atof(laserTemperature)+273.15);
	m_tiltAngle = radian(std::atof(tiltAngle)/360.0*2.0*M_PI);
	m_background = millivolt(std::atof(background));
	m_pulseQuantity = std::atoi(pulseQuantity)*1000;
	m_sampleRate = megahertz(std::atof(sampleRate));
	m_sum = std::atof(sum);


	
	std::vector<char> data(10240);
	bufferStream.read(&data[0], 10240);
	bufferStream.read(crlf, 2);

	char endOfTextCharacter;
	char checksum[4];
	bufferStream.read(&endOfTextCharacter, 1);
	bufferStream.read(checksum, 4);

	m_data.resize(2046); //last two points are always zero so ignore them and use 2046 rather than 2048
	char* currentPoint = &data[0];
	for (size_t i = 0; i < m_data.size(); ++i)
	{
		m_data[i] = steradianPerKilometre(hexTextToNumber(currentPoint))*m_scale / unitless(100000.0);
		currentPoint += 5;
	}

	//we calculate the checksum based on all caharacters after the start of header
	//character, up to and including the end of text character. So we exclude the
	//1st character and the last 4 from the buffer.
	unsigned int calculatedChecksum = generateChecksum(&buffer[1], buffer.size() - 5);
	//now convert the read checksum into a number for easy comparison. We can use the
	//same function as used for converting the profile data, but this accepts 5
	//characters, so prepend a 0. Note we don't need to stress about 2s compliment 
	//format and signed/unsigned values because the first byte is 0, so the sign bit
	//will always be 0.
	char checksumToConvert[5];
	checksumToConvert[0] = '0';
	checksumToConvert[1] = checksum[0];
	checksumToConvert[2] = checksum[1];
	checksumToConvert[3] = checksum[2];
	checksumToConvert[4] = checksum[3];
	unsigned int readChecksum = hexTextToNumber(checksumToConvert);

	m_passedChecksum = readChecksum == calculatedChecksum;
}