#define NOMINMAX
#include "HplHeader.h"
#include<sstream>
#include<svector/serr.h>

//Used for setting colon to be treated like a space 
struct ColonIsSpace : std::ctype<char>
{
	ColonIsSpace() : std::ctype<char>(get_table()) {}
	static mask const* get_table()
	{
		static mask rc[table_size];
		rc[':'] = std::ctype_base::space;
		rc['\n'] = std::ctype_base::space;
		rc['\t'] = std::ctype_base::space;
		rc[' '] = std::ctype_base::space;
		return &rc[0];
	}
};

std::istream & operator>> (std::istream & stream, ScanType &scanType)
{
	std::string tempString;
	std::getline(stream, tempString);
	if (tempString == "Stare")
		scanType = st_stare;
	else if (tempString == "RHI")
		scanType = st_rhi;
	else if (tempString == "VAD")
		scanType = st_vad;
	else if (tempString == "Wind profile")
		scanType = st_wind;
	else if (tempString == "User file 1")
		scanType = st_user1;
	else if (tempString == "User file 2")
		scanType = st_user2;
	else if (tempString == "User file 3")
		scanType = st_user3;
	else if (tempString == "User file 4")
		scanType = st_user4;

	return stream;
}

template <class T>
void readHeaderVariable(std::istream &stream, sci::Physical<T> &variable)
{
	double dVar;
	stream >> dVar;
	variable = sci::Physical <T>(dVar);
}

template <class T>
void readHeaderVariable(std::istream &stream, T &variable)
{
	//I know this file uses only ascii so I can just use a std::istring
	stream >> variable;
}

template <>
void readHeaderVariable<sci::string>(std::istream &stream, sci::string &variable)
{
	//I know this file uses only ascii so I can just use a std::istring
	//However we have requested read into a unicode string so we need
	//to read into a std::string and convert
	std::string temp;
	stream >> temp;
	variable = sci::utf8ToUtf16(temp);
}

template <>
void readHeaderVariable(std::istream &stream, std::string &variable)
{
	std::getline(stream, variable);
}

template <>
void readHeaderVariable(std::istream &stream, sci::UtcTime &variable)
{
	std::string tempString;
	std::getline(stream, tempString);

	std::istringstream stringStream;
	stringStream.str(tempString);
	int date;
	int year;
	unsigned int month;
	unsigned int dayOfMonth;
	unsigned int hour;
	unsigned int minute;
	double second;

	//this changes the locale to a custom one where colon is a delimiter
	stringStream.imbue(std::locale(stringStream.getloc(), new ColonIsSpace));
	stringStream >> date >> hour >> minute >> second;
	year = date / 10000;
	month = (date % 10000) / 100;
	dayOfMonth = date % 100;

	variable = sci::UtcTime(year, month, dayOfMonth, hour, minute, second);
}

template <class T>
void readHeaderLine(std::istream &stream, T &variable, const std::string &name)
{
	std::string tempString;
	std::getline(stream, tempString);
	sci::assertThrow(stream.good(), sci::err(sci::SERR_USER, 0, sU("Read of file failed - it may be locked or inaccessible.")));
	size_t colonPosition = std::numeric_limits<size_t>::max();
	size_t valueStartPosition = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < tempString.size(); ++i)
	{
		if (tempString[i] == ':')
		{
			colonPosition = i;
			valueStartPosition = i + 2;
			break;
		}
	}
	std::string foundName = tempString.substr(0, colonPosition);
	std::string foundVariable = tempString.substr(valueStartPosition);
	sci::assertThrow(foundName == name, sci::err(sci::SERR_USER, 1, "Could not find the variable " + name + " in the file header at the expected location"));

	std::istringstream varStream;
	varStream.str(foundVariable);
	readHeaderVariable(varStream, variable);
}

std::istream & operator>> (std::istream & stream, HplHeader &hplHeader)
{
	try
	{
		readHeaderLine(stream, hplHeader.filename, "Filename");
	}
	catch (sci::err err)
	{
		//throw a sightly different error if we did't find the first variable
		if (err.getErrorCode() == 1) //missing variable uses error code 1
			sci::assertThrow(false, sci::err(sci::SERR_USER, 0, sU("The file is not a lidar profile. Lidar profile files should start with a \"Filename:\" parameter.")));
		else
			throw;
	}

	readHeaderLine(stream, hplHeader.systemId, "System ID");
	readHeaderLine(stream, hplHeader.nGates, "Number of gates");
	readHeaderLine(stream, hplHeader.rangeGateLength, "Range gate length (m)");
	readHeaderLine(stream, hplHeader.pointsPerGate, "Gate length (pts)");
	readHeaderLine(stream, hplHeader.pulsesPerRay, "Pulses/ray");
	readHeaderLine(stream, hplHeader.nRays, "No. of rays in file");
	readHeaderLine(stream, hplHeader.scanType, "Scan type");
	readHeaderLine(stream, hplHeader.focusRange, "Focus range");
	readHeaderLine(stream, hplHeader.startTime, "Start time");
	readHeaderLine(stream, hplHeader.dopplerResolution, "Resolution (m/s)");

	std::string tempString;

	std::getline(stream, tempString);
	sci::assertThrow(tempString == "Altitude of measurement (center of gate) = (range gate + 0.5) * Gate length",
		sci::err(sci::SERR_USER, 0, sU("Did not find the expected altitude of measurement equation. Incorrect file format.")));

	std::getline(stream, tempString);
	sci::assertThrow(tempString == "Data line 1: Decimal time (hours)  Azimuth (degrees)  Elevation (degrees) Pitch (degrees) Roll (degrees)",
		sci::err(sci::SERR_USER, 0, sU("Did not find the expected data line 1 descriptor. Incorrect file format.")));

	std::getline(stream, tempString);
	sci::assertThrow(tempString == "f9.6,1x,f6.2,1x,f6.2",
		sci::err(sci::SERR_USER, 0, sU("Did not find the expected data line 1 format. Incorrect file format.")));

	std::getline(stream, tempString);
	sci::assertThrow(tempString == "Data line 2: Range Gate  Doppler (m/s)  Intensity (SNR + 1)  Beta (m-1 sr-1)",
		sci::err(sci::SERR_USER, 0, sU("Did not find the expected data line 2 descriptor. Incorrect file format.")));

	std::getline(stream, tempString);
	sci::assertThrow(tempString == "i3,1x,f6.4,1x,f8.6,1x,e12.6 - repeat for no. gates",
		sci::err(sci::SERR_USER, 0, sU("Did not find the expected data line 2 format. Incorrect file format.")));

	std::getline(stream, tempString);
	sci::assertThrow(tempString == "****",
		sci::err(sci::SERR_USER, 0, sU("Did not find the expected **** header ending. Incorrect file format.")));

	return stream;
}