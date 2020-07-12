#include"MetConversions.h"
#include"Sondes.h"
#include<list>
#include<map>

constexpr unsigned char firstBitsMask(size_t n)
{
	unsigned char mask = 0;
	for (size_t i = 0; i < n; ++i)
		mask = mask | (unsigned char(1) << (7 - i));
	return mask;
}

constexpr unsigned char lastBitsMask(size_t n)
{
	unsigned char mask = 0;
	for (size_t i = 0; i < n; ++i)
		mask = mask | (unsigned char(1) << i);
	return mask;
}

//This right shifts the bits in byte, by the number of bits given and the bits shifted in
//come from previous byte. 
unsigned char shiftAcrossBytes(unsigned char previousByte, unsigned char byte, size_t nBitsToShift)
{
	unsigned char result = byte >> nBitsToShift;
	result = result | (previousByte & lastBitsMask(nBitsToShift)) << (8-nBitsToShift);
	return result;
}

std::vector<unsigned char>readBits(const std::vector<unsigned char> &buffer, size_t &currentByte, size_t &currentBit, size_t nBitsToRead)
{
	//first the case when all the data is in one byte
	if (currentBit + nBitsToRead < 9)
	{
		//mask out the unwanted bits
		unsigned char masked = buffer[currentByte] & lastBitsMask(8 - currentBit) & firstBitsMask(currentBit + nBitsToRead);
		//shift the remaining bits to the end of the byte and put it in a 1 element vector
		std::vector<unsigned char> result(1, masked >> (8 - currentBit - nBitsToRead));
		//increment the position
		currentBit += nBitsToRead;
		//if we read all the current byte then increment the current byte too
		if (currentBit == 8)
		{
			currentBit = 0;
			++currentByte;
		}
		//return the result
		return result;
	}
	size_t bitsToUseFromFirstByte = 8-currentBit;
	size_t bitsToUseFromLastByte = ((currentBit + nBitsToRead) % 8);
	if (bitsToUseFromLastByte == 0)
		bitsToUseFromLastByte = 8;
	size_t bytesToUse = (currentBit + nBitsToRead) / 8;
	if ((currentBit + nBitsToRead) % 8 > 0)
		++bytesToUse;
	size_t shiftAmount = 8 - bitsToUseFromLastByte;

	//The number of bytes in the result can be one less than
	//the bytes used if we shift all the data off the first byte
	//so calculate this separately
	size_t nResultBytes = nBitsToRead / 8;
	if (nBitsToRead % 8 > 0)
		++nResultBytes;
	//create a vector for the result
	std::vector<unsigned char> result(nResultBytes);

	//For the first byte we need to clear any initial bits we dont want
	char firstByte = buffer[currentByte] & lastBitsMask(bitsToUseFromFirstByte);

	//The first byte written is special as we need to use the cleared first byte above

	//If we are not shifting all the data from the first
	//byte we use, then write this byte shifted.
	size_t bytesRead;
	if (shiftAmount < bitsToUseFromFirstByte)
	{
		result[0] = firstByte >> shiftAmount;
		bytesRead = 1;
	}
	//If we are then write the next byte with the first byte shifted onto it
	else
	{
		result[0] = shiftAcrossBytes(firstByte, buffer[currentByte + 1], shiftAmount);
		bytesRead = 2;
	}

	//loop through the rest of the bytes;
	for (size_t i = 1; i < nResultBytes; ++i) //start with i=1 as we have already written the first byte
	{
		size_t byteToRead = currentByte + bytesRead;
		result[i] = shiftAcrossBytes(buffer[byteToRead - 1], buffer[byteToRead], shiftAmount);
		++bytesRead;
	}

	//increment currentBit and currentByte;
	currentBit += nBitsToRead;
	currentByte += currentBit / 8;
	currentBit = currentBit % 8;

	return result;
}

size_t bytesToValue(const std::vector<unsigned char> &bytes)
{
	size_t result = 0;
	for (size_t i = 0; i < bytes.size(); ++i)
	{
		result *= 256;
		result += bytes[i];
	}
	return result;
}

#pragma warning(push)
#pragma warning (disable : 26495)
class Bufr
{
public:
	struct Descriptor
	{
		int F;
		int X;
		int Y;
		bool operator< (const Descriptor &other) const
		{
			return (F < other.F) || (F == other.F && (X < other.X || (X == other.X && Y < other.Y)));
		}
		bool operator== (const Descriptor &other) const
		{
			return (F == other.F) && (X == other.X) && (Y == other.Y);
		}
	};
	struct ElementDescriptor
	{
		double scale;
		double reference;
		size_t bits;
		bool numeric;
	};
	struct ExtractedData
	{
		Bufr::Descriptor descriptor;
		std::vector<double> numericData;
		std::vector<std::vector<unsigned char>> nonNumericData;
		unsigned char prefixDataMeaning;
		std::vector<std::vector<unsigned char>> prefixData;

	};
	void read(std::ifstream &fin);
	void readHeader(std::ifstream &fin);
	static void expandDescriptorList(std::list<Descriptor> &list);
	sci::UtcTime getTime() const { return m_time; }
	const std::list<ExtractedData> &getData() const
	{
		return m_data;
	}
private:
	int m_editionNumber;
	unsigned char m_bufrMasterTable;
	unsigned char m_centre[4];
	int m_updateSequenceNumber;
	bool m_hasOptionalSection;
	unsigned char m_dataCategory[3];
	int m_masterTableVersion;
	int m_localTableVersion;
	sci::UtcTime m_time;
	std::vector<unsigned char> m_centreData;
	std::vector<unsigned char> m_optionalData;
	size_t m_numberOfDataSubsets;
	bool m_observationalData;
	bool m_compressed;
	std::list<Bufr::Descriptor> m_descriptors;
	std::list<ExtractedData> m_data;
};
#pragma warning(pop)

//replaces a single descriptor in a list with its expanded version.
//returns an iterator to the first item added
std::list<Bufr::Descriptor>::iterator expandItem(std::list<Bufr::Descriptor> &originalList, std::list<Bufr::Descriptor>::iterator itemToExpand, const std::list<Bufr::Descriptor> expandedItem)
{
	sci::assertThrow(expandedItem.size() > 0, sci::err(sci::SERR_USER, 0, sU("Attempting to expand a BUFR Descriptor, but did not find it in the encoded extract of WMO Table D")));
	
	auto insertPosition = originalList.erase(itemToExpand);
	return originalList.insert(insertPosition, expandedItem.begin(), expandedItem.end());
	
}

//This is an extract of WMO table D, which basically takes descriptors with F==3, and provides a list of
//descriptors that they represent. Note that some of the descriptors returned, may themseves have F==3, so
//will need further expanding
const std::map<Bufr::Descriptor, std::list<Bufr::Descriptor>> WmoTableD
{
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,1,  1}, {{0,1,  1},{0, 1,  2}}),//block and station numbers**
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,1, 11}, {{0,4,  1},{0, 4,  2},{0, 4,  3}}),//year, month, day**
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,1, 13}, {{0,4,  4},{0, 4,  5},{0, 4,  6}}),//hour, minute, second**
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,1, 21}, {{0,5,  1},{0, 6,  1}}),//lattitude/longitude (high accuracy)**
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,1,111}, {{3,1,  1},{0, 1, 11},{0, 2, 11},{0, 2,13},{0, 2,14},{0, 2, 3}}),//Identification of launch site and instrumentation for P, T, U and wind measurements**
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,1,113}, {{0,8, 21},{3, 1, 11},{3, 1, 13}}),//Date/time of launch**
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,1,114}, {{3,1, 21},{0, 7, 30},{0, 7, 31},{0, 7, 7},{0,33,24}}),//Horizontal and vertical coordinates of launch site**
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,2, 49}, {{0,8,  2},{0,20, 11},{0,20, 13},{0,20,12},{0,20,12},{0,20,12},{0, 8,  2}}),//Cloud information reported with vertical soundings **
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,3, 51}, {{0,4, 86},{0, 8, 42},{0, 7,  4},{0, 5,15},{0, 6,15},{0,11,61},{0,11, 62}}),//Wind shear data at a pressure level with radiosonde position
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,3, 54}, {{0,4, 86},{0, 8, 42},{0, 7,  4},{0,10, 9},{0, 5,15},{0, 6,15},{0,12,101},{0,12,103},{0,11,1},{0,11,2}}),//Temperature, dewpoint and wind data at a pressure level with radiosonde position
	std::pair< Bufr::Descriptor, std::list<Bufr::Descriptor>>({3,9, 52}, {{3,1,111},{3, 1,113},{3, 1,114},{3, 2,49},{0,22,43},{1, 1, 0},{0,31,  2},{3,3,54},{1,1,0},{0,31,1},{3,3,51}})//Sequence for representation of TEMP, TEMP SHIP and TEMP MOBIL observation type data
};

const std::map<Bufr::Descriptor, Bufr::ElementDescriptor> WmoTableB
{
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,1,1}, {0,0,7, false}),//WMO block number 
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,1,2}, {0,0,10, false}), //WMO station Number
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,1,11}, {0,0,72, false}), //Ship or mobile land station identifier
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,1,81}, {0,0,160, false}), //Radiosonde serial number
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,1,82}, {0,0,14, true}), //Radiosonde ascension number
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,3}, {0,0,4, false}), //Type of measuring equipment used 
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,11}, {0,0,8, false}), //Radiosonde type 
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,13}, {0,0,4, false}), //Solar and infrared radiation correction 
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,14}, {0,0,7, false}), //Tracking technique/status of system used
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,17}, {0,0,5, false}), //Correction algorithms for humidity measurements
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,67}, {-5,0,15, true}), //Radiosonde operating frequency (Hz)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,95}, {0,0,5, false}), //Type of Pressure Sensor
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,96}, {0,0,5, false}), //Type Of Temperature Sensor
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,97}, {0,0,5, false}), //Type of Humidity Sensor
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,2,191}, {0,0,4, false}), //Geopotential height calculatiob
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,4,1}, {0,0,12, true}), //Year
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,4,2}, {0,0,4, true}), //Month
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,4,3}, {0,0,6, true}), //Day
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,4,4}, {0,0,5, true}), //Hour
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,4,5}, {0,0,6, true}), //Minute
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,4,6}, {0,0,6, true}), //second
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,4,86}, {0,-8192,15, true}), //Long time period or displacement (s)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,5,1}, {5,-9000000,25, true}), //Latitude (high accuracy) degree
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,5,15}, {5,-9000000,25, true}), //Latitude displacement(high accuracy) degree
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,6,1}, {5,-18000000,26, true}), //Longitude (high accuracy) degree
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,6,15}, {5,-18000000,26, true}), //Longitude displacement (high accuracy) degree
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,7,4}, {-1,0,14, true}), //Pressure (Pa)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,7,7}, {0,-1000,17, true}), //height (m)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,7,30}, {1,-4000,17, true}), //Height of station ground above mean sea level (m)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,7,31}, {1,-4000,17, true}), //Height of barometer above mean sea leve (m)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,8,2}, {0,0,6, false}), //Vertical significance (surface observations) 
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,8,21}, {0,0,5, false}), //Time significance
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,8,42}, {0,0,18, false}), //Extended vertical sounding significance
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,10,9}, {0,-1000,17, true}), //Geopotential height (gpm)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,11,1}, {0,0,9, true}), //Wind direction (degree true)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,11,2}, {1,0,12, true}), //Wind speed (m s-1)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,11,61}, {1,0,12, true}), //Absolute wind shear in 1 km layer below (m s-1)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,11,62}, {1,0,12, true}),  //Absolute wind shear in 1 km layer above (m s-1)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,12,101}, {2,0,16, true}), //Temperature/air temperature (K)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,12,103}, {2,0,16, true}), //Dewpoint Temperature (K)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,20,11}, {0,0,4, false}), //Cloud amount 
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,20,12}, {0,0,6, false}), //Cloud type
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,20,13}, {-1,-40,11, true}), //Height of base of cloud (m)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,22,43}, {2,0,15, true}), //Sea/water temperature (K)
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,25,61}, {0,0,96, false}), //Software identification and version number
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,31,1}, {0,0,8, true}), //Delayed descriptor replication factor
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,31,2}, {0,0,16, true}), //Extended delayed descriptor replication factor
	std::pair< Bufr::Descriptor, Bufr::ElementDescriptor>({0,33,24}, {0,0,4, false}) //Station Elevation Quality Mark (for mobile stations)
};


void extractData(Bufr::ExtractedData &destination,
	const std::vector<unsigned char> &buffer, size_t &currentByte, size_t &currentBit,
	char extraBits, char extraScale, const std::vector<double> &alternateReferenceStack,
	const std::map< Bufr::Descriptor, double> &alternateReferenceMap,
	unsigned char nPrefixBits, unsigned char &alternateNumberOfBits, bool &useAlternateNumberOfBits)
{
	//grab the element descriptor from table B
	Bufr::ElementDescriptor elementDescriptor;
	try
	{
		elementDescriptor = WmoTableB.at(destination.descriptor);
	}
	catch (...)
	{
		sci::ostringstream message;
		message << sU("Could not find bufr descriptor ") << destination.descriptor.F << sU(" ") << destination.descriptor.X << sU(" ") << destination.descriptor.Y << sU(" in the WMO Table B data available.");
		throw sci::err(sci::SERR_USER, 0, message.str());
	}

	//operators don't apply to 0 31 descriptors
	if (destination.descriptor.X == 31)
	{
		destination.nonNumericData.push_back(readBits(buffer, currentByte, currentBit, elementDescriptor.bits));
		if(elementDescriptor.numeric)
			destination.numericData.push_back((bytesToValue(destination.nonNumericData.back()) + elementDescriptor.reference) * std::pow(10.0, -elementDescriptor.scale));
		return;
	}

	if (nPrefixBits > 0)
	{
		destination.prefixData.push_back(readBits(buffer, currentByte, currentBit, nPrefixBits));
	}

	//calculate the numebr of bits we expect adding extraBits if this is a numeric 
	size_t nBits = elementDescriptor.bits + (elementDescriptor.numeric ? extraBits : 0);
	if (useAlternateNumberOfBits)
	{
		nBits = alternateNumberOfBits;
		alternateNumberOfBits = 0;
		useAlternateNumberOfBits = false;
	}
	destination.nonNumericData.push_back(readBits(buffer, currentByte, currentBit, nBits));
	if (elementDescriptor.numeric)
	{
		double reference = elementDescriptor.reference; //get default value
		if (alternateReferenceMap.find(destination.descriptor) != alternateReferenceMap.end()) //check for override from begining of file operator
			reference = alternateReferenceMap.at(destination.descriptor);
		if (alternateReferenceStack.size() > 0) //check for override from local operator
			reference = alternateReferenceStack.back();
		destination.numericData.push_back((bytesToValue(destination.nonNumericData.back()) + reference) * std::pow(10.0, -(elementDescriptor.scale + extraScale)));
	}
}

bool isDelayedDescriptorRepetitionFactor(Bufr::Descriptor descriptor)
{
	return descriptor.F == 0 && descriptor.X == 31 && (descriptor.Y == 0 || descriptor.Y == 1 || descriptor.Y == 2 || descriptor.Y == 11 || descriptor.Y == 12);
}

//This expansion is based on WMO Table D
//Any Descriptor with F==3 needs expanding as it represents a group
//of descriptors.
void Bufr::expandDescriptorList(std::list<Bufr::Descriptor> &list)
{
	if (list.size() == 0)
		return;
	
	bool expandedPreviousItem = false;
	std::list<Bufr::Descriptor>::iterator previousReplicator;
	std::list<Bufr::Descriptor>::iterator replicatorCompleteDescriptor;
	bool needToCheckReplicator = false;
	for (auto iter = list.begin(); iter != list.end(); ++iter)
	{
		//if we expanded the previous item then we need to go back one
		//item to check if any of the inserted items need expanding
		if (expandedPreviousItem)
			--iter;
		expandedPreviousItem = false;

		//check if we have reached the iterator that means we have expanded everything that needs replicating
		if (needToCheckReplicator && iter== replicatorCompleteDescriptor)
		{
			//count how many items there are now and save them in the replicator
			size_t newCounts = 0;
			for (auto replicatorIter = previousReplicator; replicatorIter != iter; ++replicatorIter)
				if (!isDelayedDescriptorRepetitionFactor(*replicatorIter))
					++newCounts;
			sci::assertThrow(newCounts < std::numeric_limits<int>::max(), sci::err(sci::SERR_USER, 0, sU("Too many replicators in the sonde data file to store in an int")));
			previousReplicator->X = (int)(newCounts-1);//the -1 is because we counted the replicator itself
			needToCheckReplicator = false; //mark replicator as satisfied
		}

		if (iter->F == 3)
		{
			expandedPreviousItem = true;
			iter = expandItem(list, iter, WmoTableD.at(*iter));
		}
		else if (iter->F == 1) //this is a replicator. We need to find the iterator to the descriptor after the last repicated item
		{
			sci::assertThrow(!needToCheckReplicator, sci::err(sci::SERR_USER, 0, sU("This code cannot deal with nested replicator descriptors (i.e. data with 2 or more dimensions")));
			previousReplicator = iter;
			replicatorCompleteDescriptor = iter;
			size_t count = 0;
			while (count < previousReplicator->X)
			{
				++replicatorCompleteDescriptor;
				if (!isDelayedDescriptorRepetitionFactor(*replicatorCompleteDescriptor))
					++count;
			}
			++replicatorCompleteDescriptor; //move one past the last item replicated, so it can be an end iterator
			needToCheckReplicator = true;
		}
	}
}

void Bufr::readHeader(std::ifstream &fin)
{
	//read section 1 - this should start with the BUFR 4CC, then the number of bytes in the file,
	//then the version number
	unsigned char indicatorSection[8];
	fin.read((char*)indicatorSection, 8);
	sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file indicator section.")));

	sci::assertThrow(indicatorSection[0] == 'B' && indicatorSection[1] == 'U' && indicatorSection[2] == 'F' && indicatorSection[3] == 'R',
		sci::err(sci::SERR_USER, 0, sU("The file is not a BUFR file - it doues not begin with the BUFR four character code.")));

	size_t expectedLength = size_t(indicatorSection[4]) * 256 * 256 + size_t(indicatorSection[5]) * 256 + size_t(indicatorSection[6]);
	m_editionNumber = indicatorSection[7];

	//the next section is the identification section. It has a size of at least 22 bytes, but can be more.
	//if the section includes data reserved for ADP centre use (stored in m_centreData)
	std::vector<unsigned char> identificationSection(22);
	fin.read((char*)&identificationSection[0], 22);
	sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file identification section.")));
	size_t expectedLengthIndentificationSection = (size_t)identificationSection[0] * 256 * 256 + (size_t)identificationSection[1] * 256 + (size_t)identificationSection[2];
	m_bufrMasterTable = identificationSection[3];
	m_centre[0] = identificationSection[4];
	m_centre[1] = identificationSection[5];
	m_centre[2] = identificationSection[6];
	m_centre[3] = identificationSection[7];
	m_updateSequenceNumber = identificationSection[8];
	m_hasOptionalSection = identificationSection[9] != 0;
	m_dataCategory[0] = identificationSection[10];
	m_dataCategory[1] = identificationSection[11];
	m_dataCategory[2] = identificationSection[12];
	m_masterTableVersion = identificationSection[13];
	m_localTableVersion = identificationSection[14];
	m_time = sci::UtcTime(identificationSection[15] * 256 + identificationSection[16],
		identificationSection[17],
		identificationSection[18],
		identificationSection[19],
		identificationSection[20],
		identificationSection[21]);
	if (expectedLengthIndentificationSection > 22)
	{
		m_centreData.resize(expectedLengthIndentificationSection - 22);
		fin.read((char*)(&m_centreData[0]), m_centreData.size());
		sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file identification section - additional data.")));
	}

	//read the optional section if it exists
	if (m_hasOptionalSection)
	{
		unsigned char optionalSectionHeader[4];
		fin.read((char*)optionalSectionHeader, 4);
		sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file optional section.")));
		size_t optionalSectionLength = (size_t)optionalSectionHeader[0] * 256 * 256 + (size_t)optionalSectionHeader[1] * 256 + (size_t)optionalSectionHeader[2];
		//note optionalSectionHeader[3] is set to zero always.
		if (optionalSectionLength > 4)
		{
			m_optionalData.resize(optionalSectionLength - 4);
			fin.read((char*)(&m_optionalData[0]), m_optionalData.size());
			sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file optional section - additional data.")));
		}
	}

	//read the data description section
	unsigned char dataDescriptionHeader[7];
	fin.read((char*)dataDescriptionHeader, 7);
	sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file data description section - header bytes.")));
	size_t dataDescriptionLength = (size_t)dataDescriptionHeader[0] * 256 * 256 + (size_t)dataDescriptionHeader[1] * 256 + (size_t)dataDescriptionHeader[2];
	//dataDescriptionHeader[3] is not used and should be zero
	m_numberOfDataSubsets = (size_t)dataDescriptionHeader[4] * 256 + (size_t)dataDescriptionHeader[5];
	m_observationalData = (dataDescriptionHeader[6] & 0x80) != 0;
	m_compressed = (dataDescriptionHeader[6] & 0x40) != 0;
	if (dataDescriptionLength > 7)
	{
		std::vector<unsigned char> binaryDescriptors;
		binaryDescriptors.resize(dataDescriptionLength - 7);
		fin.read((char*)(&binaryDescriptors[0]), binaryDescriptors.size());
		sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file data description section - descriptors.")));

		for (size_t i = 0; i < binaryDescriptors.size() / 2; ++i)
		{
			Descriptor descriptor;
			descriptor.F = (binaryDescriptors[i * 2] & 0xc0) >> 6;
			descriptor.X = binaryDescriptors[i * 2] & 0x3f;
			descriptor.Y = binaryDescriptors[i * 2 + 1];
			m_descriptors.push_back(descriptor);
		}
	}
}

void Bufr::read(std::ifstream &fin)
{
	readHeader(fin);

	//read the data section
	std::vector<unsigned char> binaryData;
	unsigned char dataHeader[4];
	fin.read((char*)dataHeader, 4);
	size_t dataLength = (size_t)dataHeader[0] * 256 * 256 + (size_t)dataHeader[1] * 256 + (size_t)dataHeader[2];
	sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file data section - header.")));
	//dataheader[3] is not used and should be zero
	if (dataLength > 4)
	{
		binaryData.resize(dataLength - 4);
		errno = 0;
		int e = errno;
		size_t pos1 = fin.tellg();
		fin.read((char*)(&binaryData[0]), binaryData.size());
		size_t nRead = fin.gcount();
		size_t pos2 = fin.tellg();
		sci::assertThrow(!fin.eof(), sci::err(sci::SERR_USER, 0, sU("Reached end of bufr file before finished reading data section.")));
		sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file data section - data bytes.")));
	}

	//read the end section
	unsigned char endSection[4];
	fin.read((char*)endSection, 4);
	sci::assertThrow(!fin.fail(), sci::err(sci::SERR_USER, 0, sU("Read fail when reading bufr file end section.")));
	sci::assertThrow(endSection[0] == '7' && endSection[1] == '7' && endSection[2] == '7' && endSection[3] == '7',
		sci::err(sci::SERR_USER, 0, sU("Did not find the \"7777\" end sequence in the BUFR file. The file is incomplete or corrupted")));




	//Now we need to Parse the data

	//initialise operators no no effect
	char extraBits = 0;
	char extraScale = 0;
	std::vector<double> alternateReferenceStack;
	unsigned char prefixBits = 0;
	unsigned char alternateNumberOfBits = 0;
	bool useAlternateNumberOfBits = false;
	bool readNonReferenceOperator = false;
	std::map<Bufr::Descriptor, double> referenceMap;
	unsigned char prefixDataMeaning = 0;


	//Expand the descriptors
	expandDescriptorList(m_descriptors);

	size_t bufferByte = 0;
	size_t bufferBit = 0;
	//now work through the descriptors
	for (auto iter = m_descriptors.begin(); iter != m_descriptors.end(); ++iter)
	{
		if (iter->F == 1)
		{
			size_t nVariables = iter->X;
			size_t nRepeats = iter->Y;
			//this is a replication descriptor
			if (nRepeats == 0)
			{
				//this is means we read the number of repeats from the stream
				++iter;
				sci::assertThrow(iter->X == 31 && (iter->Y == 1 || iter->Y == 2 || iter->Y == 3), sci::err(sci::SERR_USER, 0, sU("Found a delayed replication descriptor in a BUFR file, but the following descriptor with none of 0 31 000, 0 31 001 nor 0 31 002.")));
				m_data.push_back(Bufr::ExtractedData{ *iter, std::vector<double>(0), std::vector<std::vector<unsigned char>>(0), prefixDataMeaning, std::vector<std::vector<unsigned char>>(0) });
				extractData(m_data.back(), binaryData, bufferByte, bufferBit, extraBits, extraScale, alternateReferenceStack,
					referenceMap, prefixBits, alternateNumberOfBits, useAlternateNumberOfBits);
				nRepeats = (size_t)m_data.back().numericData[0];
			}
			//move on to the first variable descriptor
			++iter;

			//We want to go through each of the descriptors being repeated and add a data element to our data list
			//also grab an iterator to the last element of our data so we can use that when adding our repeats;
			auto repeatedIter = iter;
			auto variableStartIter = std::prev(m_data.end());//initially point this to the last existing element
			for (size_t i = 0; i < nVariables; ++i)
			{
				//check that the descriptor is an element descriptor.
				//this code cannot deal with 2D data, so if it is a nested repeat descriptor then we can't deal with it
				//this code also can't deal with operator descriptors(F=2)
				//We should have expanded any sequence descriptors already
				sci::assertThrow(repeatedIter->F == 0, sci::err(sci::SERR_USER, 0, sU("Found a BUFR descriptor that this code cannot deal with. Either this is a nested repeat descriptor or an operator descriptor.")));
				//add a new element to our data with the 
				m_data.push_back(Bufr::ExtractedData{ *repeatedIter, std::vector<double>(0), std::vector<std::vector<unsigned char>>(0), prefixDataMeaning, std::vector<std::vector<unsigned char>>(0) });
				//reserve space for the data to be added
				m_data.back().nonNumericData.reserve(nRepeats);
				if(WmoTableB.at(*repeatedIter).numeric)
					m_data.back().numericData.reserve(nRepeats);
				++repeatedIter;
			}
			//now move the variableStartIter on to the first new data element
			++variableStartIter;

			//work through the binary data adding it to the data array
			for (size_t i = 0; i < nRepeats; ++i)
			{
				for (auto variableIter = variableStartIter; variableIter != m_data.end(); ++variableIter)
				{
					extractData(*variableIter, binaryData, bufferByte, bufferBit, extraBits, extraScale, alternateReferenceStack,
						referenceMap, prefixBits, alternateNumberOfBits, useAlternateNumberOfBits);
				}
			}

			//move the iterator to the last descriptor of the repeat section. The -1 is because
			//we already moved to the first item
			std::advance(iter, nVariables - 1);
		}
		else if(iter->F == 0)
		{
			if (!readNonReferenceOperator && alternateReferenceStack.size() > 0)
			{
				//this means we have a change reference operator prefixing the
				//data descriptors. This indicates we want to change the reference for
				//this value for the whole file. Record this.
				referenceMap[*iter] = alternateReferenceStack.back();
			}
			readNonReferenceOperator = true;
			//check that the descriptor is an element descriptor.
			//this code can't deal with operator descriptors(F=2)
			//We should have expanded any sequence descriptors already
			//We should have dealt with repeater descriptors above
			sci::assertThrow(iter->F == 0, sci::err(sci::SERR_USER, 0, sU("Found a BUFR operator descriptor that this code cannot deal with.")));
			m_data.push_back(Bufr::ExtractedData{ *iter, std::vector<double>(0), std::vector<std::vector<unsigned char>>(0), prefixDataMeaning, std::vector<std::vector<unsigned char>>(0) });
			extractData(m_data.back(), binaryData, bufferByte, bufferBit, extraBits, extraScale, alternateReferenceStack,
				referenceMap, prefixBits, alternateNumberOfBits, useAlternateNumberOfBits);
			if (m_data.back().descriptor.X == 31 && m_data.back().descriptor.Y==21)
			{
				sci::assertThrow(m_data.back().nonNumericData[0].size() > 0, sci::err(sci::SERR_USER, 0, sU("Found a bufr descriptor 0 31 21 with no data.")));
				sci::assertThrow(m_data.back().nonNumericData[0].size() == 1, sci::err(sci::SERR_USER, 0, sU("Found a bufr descriptor 0 31 21 with more than one byte of data.")));
				prefixDataMeaning = m_data.back().nonNumericData[0][0];
			}
		}
		else if (iter->F == 2)
		{
			if (iter->X == 1)
			{
				readNonReferenceOperator = true;
				extraBits = char(int(iter->Y)-128);
			}
			else if (iter->X == 2)
			{
				readNonReferenceOperator = true;
				extraScale = char(int(iter->Y) - 128);
			}
			else if (iter->X == 3)
			{
				if (iter->Y == 255)
				{
					sci::assertThrow(alternateReferenceStack.size() > 0, sci::err(sci::SERR_USER, 0, sU("When reading a BUFR file, found a terminate reference definition when no reference was defined")));
					alternateReferenceStack.pop_back();
				}
				else if (iter->Y == 0)
				{
					alternateReferenceStack.resize(0);
				}
				else
				{
					bool negative = readBits(binaryData, bufferByte, bufferBit, 1)[0] == 1;
					alternateReferenceStack.push_back((double)bytesToValue(readBits(binaryData, bufferByte, bufferBit, iter->Y-1)));
				}
			}
			else if (iter->X == 4)
			{
				extraBits = iter->Y;
				prefixDataMeaning = 0;
			}
			else if (iter->X == 5)
			{
				readNonReferenceOperator = true;
				m_data.push_back(Bufr::ExtractedData{ *iter, std::vector<double>(0), std::vector<std::vector<unsigned char>>(0), prefixDataMeaning, std::vector<std::vector<unsigned char>>(0) });
				m_data.back().nonNumericData.push_back(readBits(binaryData, bufferByte, bufferBit, size_t(iter->Y) * 8));
			}
			else if (iter->X == 6)
			{
				readNonReferenceOperator = true;
				useAlternateNumberOfBits = true;
				alternateNumberOfBits = iter->Y;
			}
		}
	}

}













const uint8_t sondeMotionUnusedFlag = 0;
const uint8_t sondeMotionGoodDataFlag = 1;
const uint8_t sondeMotionFirstPointNoAscentRateFlag = 2;
const uint8_t sondeMotionZeroAscentRateFlag = 3;
const uint8_t sondeMotionZeroWindSpeedFlag = 4;
const uint8_t sondeMotionRepeatedTimeFlag = 5;
const uint8_t sondeMotionNoWindFlag = 6;
const uint8_t sondeMotionNoPositionFlag = 7;
const uint8_t sondeMotionNoWindOrPositionFlag = 8;

const std::vector<std::pair<uint8_t, sci::string>> sondeMotionFlags
{
{sondeMotionUnusedFlag, sU("not_used") },
{sondeMotionGoodDataFlag, sU("good_data")},
{sondeMotionFirstPointNoAscentRateFlag, sU("first point - cannot calculate ascent rate") },
{sondeMotionZeroAscentRateFlag, sU("suspect data - ascent rate of zero")},
{sondeMotionZeroWindSpeedFlag, sU("suspect data - zero wind speed") },
{sondeMotionRepeatedTimeFlag, sU("time interval less than time resolution - the time change was not captured by the 1s time resolution. Ascent rate cannot be caltulated") },
{sondeMotionNoWindFlag, sU("wind data missing")},
{sondeMotionNoPositionFlag, sU("position data missing")},
{sondeMotionNoWindOrPositionFlag, sU("wind and position data missing")}
};

const uint8_t sondeTemperatureUnusedFlag = 0;
const uint8_t sondeTemperatureGoodDataFlag = 1;

const std::vector<std::pair<uint8_t, sci::string>> sondeTemperatureFlags
{
{sondeTemperatureUnusedFlag, sU("not_used") },
{sondeTemperatureGoodDataFlag, sU("good_data")}
};

const uint8_t sondeHumidityUnusedFlag = 0;
const uint8_t sondeHumidityGoodDataFlag = 1;

const std::vector<std::pair<uint8_t, sci::string>> sondeHumidityFlags
{
{sondeHumidityUnusedFlag, sU("not_used") },
{sondeHumidityGoodDataFlag, sU("good_data")}
};

const uint8_t sondePressureUnusedFlag = 0;
const uint8_t sondePressureGoodDataFlag = 1;

const std::vector<std::pair<uint8_t, sci::string>> sondePressureFlags
{
{sondePressureUnusedFlag, sU("not_used") },
{sondePressureGoodDataFlag, sU("good_data")}
};

SondeProcessor::SondeProcessor(const InstrumentInfo &instrumentInfo, const CalibrationInfo &calibrationInfo)
	: InstrumentProcessor(sU("[/\\\\]bufr309052_all_........_...._1.bfr"))
{
	m_hasData = false;
	m_instrumentInfo = instrumentInfo;
	m_calibrationInfo = calibrationInfo;
}

void SondeProcessor::readData(const std::vector<sci::string> &inputFilenames, const Platform &platform, ProgressReporter &progressReporter)
{
	sci::assertThrow(inputFilenames.size() == 1, sci::err(sci::SERR_USER, 0, sU("The sonde processor has been asked to read multiple files, but as a point processor it only accepts single files.")));
	readData(inputFilenames[0], platform, progressReporter);
}

void SondeProcessor::readData(const sci::string &inputFilename, const Platform &platform, ProgressReporter &progressReporter)
{
	//create a new Bufr
	Bufr bufr;
	m_hasData = false;
	m_altitude.clear();
	m_latitude.clear();
	m_longitude.clear();
	m_airPressure.clear();
	m_temperature.clear();
	m_dewpoint.clear();
	m_relativeHumidity.clear();
	m_windSpeed.clear();
	m_windFromDirection.clear();
	m_balloonUpwardVelocity.clear();
	m_elapsedTime.clear();
	m_time.clear();
	m_motionFlags.clear();
	m_temperatureFlags.clear();
	m_humidityFlags.clear();
	m_pressureFlags.clear();

	std::ifstream fin;
	fin.open(sci::nativeUnicode(inputFilename), std::ios::in | std::ios::binary);
	sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Could not open file ") + inputFilename + sU(".")));
	bufr.read(fin);

	auto data = bufr.getData();
	degree startingLongitude;
	degree startingLatitude;
	int year;
	int month;
	int day;
	int hour;
	int minute;
	double second;
	std::vector<unsigned char> sondeSerialNumber;
	std::vector<unsigned char> softwareVersion;
	for (auto iter = data.begin(); iter != data.end(); ++iter)
	{
		if (iter->descriptor == Bufr::Descriptor{ 0,7,4 } && iter->numericData.size() > m_airPressure.size())
			sci::convert(m_airPressure, iter->numericData / 100.0);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 10, 9 } && iter->numericData.size() > m_altitude.size())
			sci::convert(m_altitude, iter->numericData);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 6, 15 } && iter->numericData.size() > m_longitude.size())
			sci::convert(m_longitude, iter->numericData);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 5, 15 } && iter->numericData.size() > m_latitude.size())
			sci::convert(m_latitude, iter->numericData);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 6, 1 })
			startingLongitude = degree((degree::valueType)iter->numericData[0]);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 5, 1 })
			startingLatitude = degree((degree::valueType)iter->numericData[0]);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 12, 101 } && iter->numericData.size() > m_temperature.size())
			sci::convert(m_temperature, iter->numericData);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 12, 103 } && iter->numericData.size() > m_dewpoint.size())
			sci::convert(m_dewpoint, iter->numericData);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 11, 1 } && iter->numericData.size() > m_windFromDirection.size())
			sci::convert(m_windFromDirection, iter->numericData);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 11, 2 } && iter->numericData.size() > m_windSpeed.size())
			sci::convert(m_windSpeed, iter->numericData);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 4, 86 } && iter->numericData.size() > m_elapsedTime.size())
			sci::convert(m_elapsedTime, iter->numericData);

		else if (iter->descriptor == Bufr::Descriptor{ 0, 1, 81 })
			sondeSerialNumber = iter->nonNumericData[0];

		else if (iter->descriptor == Bufr::Descriptor{ 0, 25, 61 })
			softwareVersion = iter->nonNumericData[0];

		else if (iter->descriptor == Bufr::Descriptor{ 0, 4, 1 })
			year = (int)iter->numericData[0];

		else if (iter->descriptor == Bufr::Descriptor{ 0, 4, 2 })
			month = (int)iter->numericData[0];

		else if (iter->descriptor == Bufr::Descriptor{ 0, 4, 3 })
			day = (int)iter->numericData[0];

		else if (iter->descriptor == Bufr::Descriptor{ 0, 4, 4 })
			hour = (int)iter->numericData[0];

		else if (iter->descriptor == Bufr::Descriptor{ 0, 4, 5 })
			minute = (int)iter->numericData[0];

		else if (iter->descriptor == Bufr::Descriptor{ 0, 4, 6 })
			second = iter->numericData[0];
	}

	//The above data has put lat/lon displacemnt into the lat/lon arrays. Add the satrting point;
	m_latitude += startingLatitude;
	m_longitude += startingLongitude;
	//calculate relative humidity
	m_relativeHumidity.resize(m_dewpoint.size());
	for (size_t i = 0; i < m_relativeHumidity.size(); ++i)
	{
		m_relativeHumidity[i] = relativeHumidityBolton(m_dewpoint[i], m_temperature[i]);
	}

	m_time = sci::UtcTime(year,month, day, hour, minute, second) + m_elapsedTime;
	m_balloonUpwardVelocity.resize(m_altitude.size(), std::numeric_limits<metrePerSecond>::quiet_NaN());
	for (size_t i = 1; i < m_balloonUpwardVelocity.size(); ++i)
		m_balloonUpwardVelocity[i] = (m_altitude[i] - m_altitude[i - 1]) / (m_elapsedTime[i] - m_elapsedTime[i - 1]);

	//add an end of string character to the end of the serial numbers and software version
	sondeSerialNumber.push_back(0);
	softwareVersion.push_back(0);

	//put the serial number in the apropriate member variable and trim spaces and check that the software version matches
	m_sondeSerialNumber = sci::fromCodepage((char*)&sondeSerialNumber[0]);
	m_sondeSerialNumber.resize(std::min(m_sondeSerialNumber.find_first_of(sU(' ')), m_sondeSerialNumber.length()));
	
	sci::string operatingSoftware = sci::fromCodepage((char*)&softwareVersion[0]);
	sci::string operatingSoftwareVersion = operatingSoftware.substr(operatingSoftware.find_first_of(sU(' ')) + 1);
	operatingSoftware.resize(std::min(operatingSoftware.find_first_of(sU(' ')), operatingSoftware.length()));
	operatingSoftwareVersion.resize(std::min(operatingSoftwareVersion.find_first_of(sU(' ')), operatingSoftwareVersion.length()));
	operatingSoftwareVersion = sU("v") + operatingSoftwareVersion;

	sci::assertThrow(m_instrumentInfo.operatingSoftware.substr(0, operatingSoftware.length()) == operatingSoftware, sci::err(sci::SERR_USER, 0, sU("Found inconsistent software names in sonde file and setup file. The software name in the setup file must begin ") + operatingSoftware));
	sci::assertThrow(m_instrumentInfo.operatingSoftwareVersion.substr(0, operatingSoftwareVersion.length()) == operatingSoftwareVersion, sci::err(sci::SERR_USER, 0, sU("Found inconsistent software version in sonde file and setup file. The software version in the setup file must begin ") + operatingSoftwareVersion));

	//generate the flag variable.
	m_motionFlags.assign(m_time.size(), sondeMotionGoodDataFlag);
	m_temperatureFlags.assign(m_time.size(), sondeTemperatureGoodDataFlag);
	m_humidityFlags.assign(m_time.size(), sondeHumidityGoodDataFlag);
	m_pressureFlags.assign(m_time.size(), sondePressureGoodDataFlag);
	sci::assign(m_motionFlags, sci::isEq(m_windSpeed, metrePerSecond(0)), sondeMotionZeroWindSpeedFlag);
	sci::assign(m_motionFlags, sci::isEq(m_balloonUpwardVelocity, metrePerSecond(0)), sondeMotionZeroAscentRateFlag);
	if (m_motionFlags.size() > 0)
		m_motionFlags[0] = sondeMotionFirstPointNoAscentRateFlag;

	//because the time is only 1 second resolution we can get repeat times - not ideal
	//also means that at these times the ascent rate is calculated as infinity
	for (size_t i = 1; i < m_time.size(); ++i)
	{
		if (m_time[i] == m_time[i - 1])
		{
			m_balloonUpwardVelocity[i] = std::numeric_limits<metrePerSecond>::quiet_NaN();
			m_motionFlags[i] = sondeMotionRepeatedTimeFlag;
		}
	}

	//If the GPS data is missing then the wind direction, wind speed, lat and lon
	//are set to out of sensible range (for angles) or all 1s (speed). I'm not sure
	//if we can have missing winds with good positions or visa-versa, but allow the
	//flags to permit this situation just in case.
	for (size_t i = 0; i < m_time.size(); ++i)
	{
		if (m_windFromDirection[i] > degree(360) && m_latitude[i] > degree(90.0))
		{
			m_windFromDirection[i] = std::numeric_limits<degree>::quiet_NaN();
			m_windSpeed[i] = std::numeric_limits<metrePerSecond>::quiet_NaN();
			m_latitude[i] = std::numeric_limits<degree>::quiet_NaN();
			m_longitude[i] = std::numeric_limits<degree>::quiet_NaN();
			m_motionFlags[i] = sondeMotionNoWindOrPositionFlag;
		}
		else if (m_windFromDirection[i] > degree(360))
		{
			m_windFromDirection[i] = std::numeric_limits<degree>::quiet_NaN();
			m_windSpeed[i] = std::numeric_limits<metrePerSecond>::quiet_NaN();
			m_motionFlags[i] = sondeMotionNoWindFlag;
		}
		else if (m_latitude[i] > degree(90.0))
		{
			m_latitude[i] = std::numeric_limits<degree>::quiet_NaN();
			m_longitude[i] = std::numeric_limits<degree>::quiet_NaN();
			m_motionFlags[i] = sondeMotionNoPositionFlag;
		}
	}

	m_hasData = true;
}

bool SondeProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	std::ifstream fin;
	fin.open(sci::nativeUnicode(fileName), std::ios::in || std::ios::binary);
	sci::assertThrow(fin.is_open(), sci::err(sci::SERR_USER, 0, sU("Could not open file ") + fileName + sU(".")));
	Bufr bufr;
	try
	{
		bufr.readHeader(fin);
	}
	catch (sci::err err)
	{
		//if we get a sci error here then it means the file was invalid, return false
		//we get burf files with additional header info for emailing to GTS, so it was
		//probably one of these files.
		return false;
	}

	return bufr.getTime() >= startTime && bufr.getTime() <= endTime;
}

std::vector<std::vector<sci::string>> SondeProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	//Sondes are always processed singly so we don't need to group any of the files
	//just report them singly
	std::vector<std::vector<sci::string>> result(newFiles.size());
	for (size_t i = 0; i < newFiles.size(); ++i)
		result[i] = std::vector<sci::string>(1, newFiles[i]);
	return result;
}

void SondeProcessor::writeToNc(const sci::string &directory, const PersonInfo &author,
	const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const Platform &platform, const ProcessingOptions &processingOptions, ProgressReporter &progressReporter)
{
	sci::assertThrow(m_hasData, sci::err(sci::SERR_USER, 0, sU("The sonde processor has been asked to write data to a netcdf, when it has no data to write.")));

	DataInfo dataInfo;
	dataInfo.averagingPeriod = std::numeric_limits<second>::quiet_NaN();
	dataInfo.samplingInterval = std::numeric_limits<second>::quiet_NaN();
	dataInfo.continuous = false;
	dataInfo.startTime = m_time[0];;
	dataInfo.endTime = m_time.back();
	dataInfo.featureType = FeatureType::trajectory;
	dataInfo.options = std::vector<sci::string>(0);
	dataInfo.processingLevel = 2;
	dataInfo.productName = sU("sonde");
	dataInfo.processingOptions = processingOptions;

	//Put the sonde serial number at the beginning of the comment
	sci::string comment = sU("Radiosonde Serial Number:") + m_sondeSerialNumber;
	if (dataInfo.processingOptions.comment.length() > 0)
		comment = comment + sU("\n") + dataInfo.processingOptions.comment;
	dataInfo.processingOptions.comment = comment;

	//work out the median sampling interval
	std::vector<second> samplingIntervals(m_elapsedTime.size() - 1);
	for (size_t i = 0; i < samplingIntervals.size(); ++i)
	{
		samplingIntervals[i] = m_elapsedTime[i + 1] - m_elapsedTime[i];
	}
	std::sort(samplingIntervals.begin(), samplingIntervals.end());
	dataInfo.samplingInterval = samplingIntervals[samplingIntervals.size() / 2];
	dataInfo.averagingPeriod = dataInfo.samplingInterval;


	//Output the parameters

	OutputAmfNcFile file(AmfVersion::v1_1_0, directory, m_instrumentInfo, author, processingSoftwareInfo, m_calibrationInfo, dataInfo,
		projectInfo, platform, sU("radio sonde trajectory"), m_time, m_latitude, m_longitude);

	std::vector<std::pair<sci::string, CellMethod>>cellMethods{ {sU("time"), CellMethod::point} };
	std::vector<sci::string> coordinates{ sU("longitude"), sU("latitude"), sU("altitude") };

	AmfNcAltitudeVariable altitudesVariable(file, file.getTimeDimension(), m_altitude, dataInfo.featureType);
	AmfNcVariable<hectoPascal, std::vector<hectoPascal>> airPressureVariable(sU("air_pressure"), file, file.getTimeDimension(), sU("Air Pressure"), sU("air_pressure"), m_airPressure, true, coordinates, cellMethods);
	AmfNcVariable<kelvin, std::vector<kelvin>> airTemperatureVariable(sU("air_temperature"), file, file.getTimeDimension(), sU("Air Temperature"), sU("air_temperature"), m_temperature, true, coordinates, cellMethods);
	//AmfNcVariable<kelvin, std::vector<kelvin>> dewPointVariable(sU("dew_point_temperature"), file, file.getTimeDimension(), sU("Dew Point Temperature"), sU("dew_point_temperature"), m_dewpoint, true, coordinates, cellMethods);
	AmfNcVariable<percent, std::vector<percent>> dewPointVariable(sU("relative_humidity"), file, file.getTimeDimension(), sU("Relative Humidity"), sU("relative_humidity"), m_relativeHumidity, true, coordinates, cellMethods);
	AmfNcVariable<metrePerSecond, std::vector<metrePerSecond>> windSpeedVariable(sU("wind_speed"), file, file.getTimeDimension(), sU("Wind Speed"), sU("wind_speed"), m_windSpeed, true, coordinates, cellMethods);
	AmfNcVariable<degree, std::vector<degree>> windDirectionVariable(sU("wind_from_direction"), file, file.getTimeDimension(), sU("Wind From Direction"), sU("wind_from_direction"), m_windFromDirection, true, coordinates, cellMethods);
	AmfNcVariable<metrePerSecond, std::vector<metrePerSecond>> upwardBalloonVelocityVariable(sU("upward_balloon_velocity"), file, file.getTimeDimension(), sU("Balloon Ascent Rate"), sU(""), m_balloonUpwardVelocity, true, coordinates, cellMethods);
	AmfNcVariable<secondf, std::vector<secondf>> elapsedTimeVariable(sU("elapsed_time"), file, file.getTimeDimension(), sU("Elapsed Time"), sU(""), m_elapsedTime, true, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}});
	AmfNcFlagVariable motionFlagVariable(sU("qc_flag_motion"), sondeMotionFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension() });
	AmfNcFlagVariable temperatureFlagVariable(sU("qc_flag_temperature"), sondeTemperatureFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension() });
	AmfNcFlagVariable humidityFlagVariable(sU("qc_flag_humidity"), sondeHumidityFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension() });
	AmfNcFlagVariable pressureFlagVariable(sU("qc_flag_pressure"), sondePressureFlags, file, std::vector<sci::NcDimension*>{ &file.getTimeDimension() });

	file.writeTimeAndLocationData(platform);

	file.write(altitudesVariable);
	file.write(airPressureVariable);
	file.write(airTemperatureVariable);
	file.write(dewPointVariable);
	file.write(windSpeedVariable);
	file.write(windDirectionVariable);
	file.write(upwardBalloonVelocityVariable);
	file.write(elapsedTimeVariable);
	file.write(motionFlagVariable, m_motionFlags);
	file.write(temperatureFlagVariable, m_temperatureFlags);
	file.write(humidityFlagVariable, m_humidityFlags);
	file.write(pressureFlagVariable, m_pressureFlags);
}