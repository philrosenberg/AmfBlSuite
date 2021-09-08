#define _USE_MATH_DEFINES
#include"AmfNc.h"
#include<sstream>
#include<iomanip>
#include<filesystem>
#include<cmath>
#include<locale>


const double OutputAmfNcFile::Fill<double>::value = -1e20;
const float OutputAmfNcFile::Fill<float>::value = -1e20f;
const uint8_t OutputAmfNcFile::Fill<uint8_t>::value = 255;
const int16_t OutputAmfNcFile::Fill<int16_t>::value = 10000;
const int32_t OutputAmfNcFile::Fill<int32_t>::value = 1000000000;
const int64_t OutputAmfNcFile::Fill<int64_t>::value = 1000000000000000000;

const sci::string OutputAmfNcFile::TypeName<double>::name = sU("float64");
const sci::string OutputAmfNcFile::TypeName<float>::name = sU("float32");
const sci::string OutputAmfNcFile::TypeName<int8_t>::name = sU("int8");
const sci::string OutputAmfNcFile::TypeName<uint8_t>::name = sU("byte");
const sci::string OutputAmfNcFile::TypeName<int16_t>::name = sU("int16");
const sci::string OutputAmfNcFile::TypeName<int32_t>::name = sU("int32");
const sci::string OutputAmfNcFile::TypeName<int64_t>::name = sU("int64");

void correctDirection(degreeF measuredElevation, degreeF measuredAzimuth, unitlessF sinInstrumentElevation, unitlessF sinInstrumentAzimuth, unitlessF sinInstrumentRoll, unitlessF cosInstrumentElevation, unitlessF cosInstrumentAzimuth, unitlessF cosInstrumentRoll, degreeF &correctedElevation, degreeF &correctedAzimuth)
{
	//create a unit vector in the measured direction
	unitlessF x = sci::sin(measuredAzimuth)*sci::cos(measuredElevation);
	unitlessF y = sci::cos(measuredAzimuth)*sci::cos(measuredElevation);
	unitlessF z = sci::sin(measuredElevation);

	//correct the vector
	unitlessF correctedX;
	unitlessF correctedY;
	unitlessF correctedZ;
	correctDirection(x, y, z, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedX, correctedY, correctedZ);

	//convert the unit vector back to spherical polar
	correctedElevation = sci::asin(correctedZ);
	correctedAzimuth = sci::atan2(correctedX, correctedY);
}

degreeF angleBetweenDirections(degreeF elevation1, degreeF azimuth1, degreeF elevation2, degreeF azimuth2)
{
	return sci::acos(sci::cos(degreeF(90) - elevation1)*sci::cos(degreeF(90) - elevation2) + sci::sin(degreeF(90) - elevation1)*sin(degreeF(90) - elevation2)*sci::cos(azimuth2 - azimuth1));
}

sci::string getFormattedDateTime(const sci::UtcTime &time, sci::string dateSeparator, sci::string timeSeparator, sci::string dateTimeSeparator)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << time.getYear() << dateSeparator
		<< std::setw(2) << time.getMonth() << dateSeparator
		<< std::setw(2) << time.getDayOfMonth() << dateTimeSeparator
		<< std::setw(2) << time.getHour() << timeSeparator
		<< std::setw(2) << time.getMinute() << timeSeparator
		<< std::setw(2) << std::floor(time.getSecond());

	return timeString.str();
}

sci::string getFormattedDateOnly(const sci::UtcTime &time, sci::string separator)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << time.getYear() << separator
		<< std::setw(2) << time.getMonth() << separator
		<< std::setw(2) << time.getDayOfMonth();

	return timeString.str();
}

sci::string getFormattedTimeOnly(const sci::UtcTime &time, sci::string separator)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << std::setw(2) << time.getHour() << separator
		<< std::setw(2) << time.getMinute() << separator
		<< std::setw(2) << std::floor(time.getSecond());

	return timeString.str();
}

//std::string getFormattedLatLon(double decimalDegrees)
//{
//	std::string result;
//	int degrees = int(std::floor(decimalDegrees));
//	double decimalMinutes = (decimalDegrees - (double)degrees)*60.0;
//	int minutes = int(std::floor(decimalMinutes));
//	double decimalSeconds = (decimalMinutes - (double)minutes)*60.0;
//	result << 
//}

sci::string getBoundsString(const std::vector<degreeF> & latitudes, const std::vector<degreeF> &longitudes)
{
	sci::stringstream result;
	if (latitudes.size() == 1)
	{
		result << latitudes[0].value<degreeF>() << sU("N ") << longitudes[0].value<degreeF>() << sU("E");
	}
	else
	{
		result << sci::min<degreeF>(latitudes).value<degreeF>() << "N " << sci::min<degreeF>(longitudes).value<degreeF>() << "E, "
			<< sci::max<degreeF>(latitudes).value<degreeF>() << "N " << sci::min<degreeF>(longitudes).value<degreeF>() << "E";
	}

	return result.str();
}

//this function takes an array of angles and starting from
//the second element it adds or subtracts mutiples of 360 degrees
//so that the absolute differnce between two elements is never
//more than 180 degrees
void makeContinuous(std::vector<degreeF> &angles)
{
	if (angles.size() < 2)
		return;

	//find the first valid angle
	auto iter = angles.begin();
	for (; iter != angles.end(); ++iter)
	{
		if (*iter == *iter)
			break;
	}
	if (iter == angles.end())
		return;

	//if we get to here then we have found the first valid angle.
	//move to the next angle which is the first one we may need
	//to modify
	++iter;

	//Now move throught the array incrementing the points as needed
	for (; iter != angles.end(); ++iter)
	{
		degreeF jumpAmount = sci::ceil<unitlessF::unit>((*(iter - 1) - *iter - degreeF(180)) / degreeF(360))*degreeF(360);
		*iter += jumpAmount;
	}
}
OutputAmfNcFile::OutputAmfNcFile(AmfVersion amfVersion,
	const sci::string &directory,
	const InstrumentInfo &instrumentInfo,
	const PersonInfo &author,
	const ProcessingSoftwareInfo &processingsoftwareInfo,
	const CalibrationInfo &calibrationInfo,
	const DataInfo &dataInfo,
	const ProjectInfo &projectInfo,
	const Platform &platform,
	const sci::string &title,
	const std::vector<sci::UtcTime> &times,
	const std::vector<sci::NcDimension *> &nonTimeDimensions,
	const std::vector< sci::NcAttribute*>& globalAttributes,
	bool incrementMajorVersion)
	:OutputNcFile(), m_timeDimension(sU("time"), times.size()), m_times(times)
{
	initialise(amfVersion, directory, instrumentInfo, author, processingsoftwareInfo, calibrationInfo, dataInfo,
		projectInfo, platform, title, times, std::vector<degreeF>(0), std::vector<degreeF>(0), nonTimeDimensions,
		globalAttributes, incrementMajorVersion);
}

OutputAmfNcFile::OutputAmfNcFile(AmfVersion amfVersion,
	const sci::string &directory,
	const InstrumentInfo &instrumentInfo,
	const PersonInfo &author,
	const ProcessingSoftwareInfo &processingsoftwareInfo,
	const CalibrationInfo &calibrationInfo,
	const DataInfo &dataInfo,
	const ProjectInfo &projectInfo,
	const Platform &platform,
	const sci::string &title,
	const std::vector<sci::UtcTime> &times,
	const std::vector<degreeF> &latitudes,
	const std::vector<degreeF> &longitudes,
	const std::vector<sci::NcDimension *> &nonTimeDimensions,
	const std::vector< sci::NcAttribute*>& globalAttributes,
	bool incrementMajorVersion)
	:OutputNcFile(), m_timeDimension(sU("time"), times.size()), m_times(times)
{
	initialise(amfVersion, directory, instrumentInfo, author, processingsoftwareInfo, calibrationInfo, dataInfo,
		projectInfo, platform, title, times, latitudes, longitudes, nonTimeDimensions, globalAttributes,
		incrementMajorVersion);
}

void OutputAmfNcFile::initialise(AmfVersion amfVersion,
	const sci::string &directory,
	const InstrumentInfo &instrumentInfo,
	const PersonInfo &author,
	const ProcessingSoftwareInfo &processingsoftwareInfo,
	const CalibrationInfo &calibrationInfo,
	const DataInfo &dataInfo,
	const ProjectInfo &projectInfo,
	const Platform &platform,
	sci::string title,
	const std::vector<sci::UtcTime> &times,
	const std::vector<degreeF> &latitudes,
	const std::vector<degreeF> &longitudes,
	const std::vector<sci::NcDimension *> &nonTimeDimensions,
	const std::vector< sci::NcAttribute*>& globalAttributes,
	bool incrementMajorVersion)
{
	sci::assertThrow(latitudes.size() == longitudes.size(), sci::err(sci::SERR_USER, 0, sU("Cannot create a netcdf file with latitude data a different length to longitude data.")));
	sci::assertThrow(latitudes.size() == 0 || latitudes.size() == times.size(), sci::err(sci::SERR_USER, 0, sU("Cannot create a netcdf with latitude and longitude data a different length to time data.")));
	//construct the position and time variables;
	m_years.resize(m_times.size());
	m_months.resize(m_times.size());
	m_dayOfMonths.resize(m_times.size());
	m_dayOfYears.resize(m_times.size());
	m_hours.resize(m_times.size());
	m_minutes.resize(m_times.size());
	m_seconds.resize(m_times.size());
	for (size_t i = 0; i < times.size(); ++i)
	{
		m_years[i] = times[i].getYear();
		m_months[i] = times[i].getMonth();
		m_dayOfMonths[i] = times[i].getDayOfMonth();
		m_dayOfYears[i] = (float)std::floor(((sci::UtcTime((int)m_years[i], (unsigned int)m_months[i], (unsigned int)m_dayOfMonths[i], 0, 0, 0) - sci::UtcTime((int)m_years[i], 1, 1, 0, 0, 0)) / secondF(60.0 * 60.0 * 24.0)).value<unitlessF>());
		m_hours[i] = times[i].getHour();
		m_minutes[i] = times[i].getMinute();
		m_seconds[i] = (float)times[i].getSecond();
	}
	//sort out the position and attitude data
	bool overriddenPlatformPosition = false;
	if (latitudes.size() > 0)
	{
		//This means we are overriding the platform position data with instrument position data
		//This would be the case for e.g. sondes
		m_longitudes = longitudes;
		m_latitudes = latitudes;
		overriddenPlatformPosition = true;
	}
	else
	{
		//This means we didn't override the platform position
		if (platform.getPlatformInfo().platformType == PlatformType::stationary)
		{
			degreeF longitude;
			degreeF latitude;
			metreF altitude;
			platform.getLocation(times[times.size() / 2], times[times.size() / 2], latitude, longitude, altitude);
			m_longitudes = { longitude };
			m_latitudes = { latitude };
		}
		else
		{
			m_longitudes.resize(m_times.size(), degreeF(0));
			m_latitudes.resize(m_times.size(), degreeF(0));
			m_elevations.resize(m_times.size(), degreeF(0));
			m_elevationStdevs.resize(m_times.size(), degreeF(0));
			m_elevationMins.resize(m_times.size(), degreeF(0));
			m_elevationMaxs.resize(m_times.size(), degreeF(0));
			m_elevationRates.resize(m_times.size(), degreePerSecondF(0));
			m_azimuths.resize(m_times.size(), degreeF(0));
			m_azimuthStdevs.resize(m_times.size(), degreeF(0));
			m_azimuthMins.resize(m_times.size(), degreeF(0));
			m_azimuthMaxs.resize(m_times.size(), degreeF(0));
			m_azimuthRates.resize(m_times.size(), degreePerSecondF(0));
			m_rolls.resize(m_times.size(), degreeF(0));
			m_rollStdevs.resize(m_times.size(), degreeF(0));
			m_rollMins.resize(m_times.size(), degreeF(0));
			m_rollMaxs.resize(m_times.size(), degreeF(0));
			m_rollRates.resize(m_times.size(), degreePerSecondF(0));
			m_courses.resize(m_times.size(), degreeF(0));
			m_speeds.resize(m_times.size(), metrePerSecondF(0));
			m_headings.resize(m_times.size(), degreeF(0));
			metreF altitude;
			AttitudeAverage elevation;
			AttitudeAverage azimuth;
			AttitudeAverage roll;
			for (size_t i = 0; i < m_times.size(); ++i)
			{
				platform.getLocation(m_times[i], m_times[i] + dataInfo.averagingPeriod, m_latitudes[i], m_longitudes[i], altitude);
				platform.getInstrumentAttitudes(m_times[i], m_times[i] + dataInfo.averagingPeriod, elevation, azimuth, roll);
				m_elevations[i] = elevation.m_mean;
				m_elevationStdevs[i] = elevation.m_stdev;
				m_elevationMins[i] = elevation.m_min;
				m_elevationMaxs[i] = elevation.m_max;
				m_elevationRates[i] = elevation.m_rate;
				m_azimuths[i] = azimuth.m_mean;
				m_azimuthStdevs[i] = azimuth.m_stdev;
				m_azimuthMins[i] = azimuth.m_min;
				m_azimuthMaxs[i] = azimuth.m_max;
				m_azimuthRates[i] = azimuth.m_rate;
				m_rolls[i] = roll.m_mean;
				m_rollStdevs[i] = roll.m_stdev;
				m_rollMins[i] = roll.m_min;
				m_rollMaxs[i] = roll.m_max;
				m_rollRates[i] = roll.m_rate;
				platform.getInstrumentVelocity(m_times[i], m_times[i] + dataInfo.averagingPeriod, m_speeds[i], m_courses[i]);
				platform.getPlatformAttitudes(m_times[i], m_times[i] + dataInfo.averagingPeriod, elevation, azimuth, roll);
				m_headings[i] = azimuth.m_mean;
			}
		}
	}

	//construct the file path, without the version number of suffix
	sci::string baseFilename = directory;
	//append a directory separator if needed
	if (baseFilename.length() > 0 && baseFilename.back() != sU('\\') && baseFilename.back() != sU('/'))
	{
		baseFilename = baseFilename + sU('/');
	}

	//add the instument name
	baseFilename = baseFilename + instrumentInfo.name;
	//add the platform if there is one
	if (platform.getPlatformInfo().name.length() > 0)
		baseFilename = baseFilename + sU("_") + platform.getPlatformInfo().name;
	//add either the date or date and time depending upon whether the data is
	//continuous or a single measurement
	if (dataInfo.continuous)
		baseFilename = baseFilename + sU("_") + getFormattedDateOnly(dataInfo.startTime, sU(""));
	else
		baseFilename = baseFilename + sU("_") + getFormattedDateTime(dataInfo.startTime, sU(""), sU(""), sU("-"));
	//add the data product name
	baseFilename = baseFilename + sU("_") + dataInfo.productName;
	//add any options
	for (size_t i = 0; i < dataInfo.options.size(); ++i)
		if(dataInfo.options[i].length() > 0)
			baseFilename = baseFilename + sU("_") + dataInfo.options[i];

	//Swap spaces for hyphens and make lower case
	std::locale locale;
	size_t replaceStart = baseFilename.find_last_of(sU("\\/"));
	if (replaceStart == sci::string::npos)
		replaceStart = 0;
	else
		++replaceStart;
	for (size_t i = replaceStart; i < baseFilename.length(); ++i)
	{
		if (baseFilename[i] == sU(' '))
			baseFilename[i] = sU('-');
		else
			baseFilename[i] = std::tolower(baseFilename[i], locale);
	}

	//construct the title for the dataset if one was not provided
	if (title.length() == 0)
	{
		if (baseFilename.find_last_of(sU("\\/")) != sci::string::npos)
			title = baseFilename.substr(baseFilename.find_last_of(sU("\\/"))+1);
		else
			title = baseFilename;
	}

#ifdef _WIN32
	//It is worth checking that the ansi version of the filename doesn't get corrupted
	bool usedReplacement;
	std::string ansiBaseFilename = sci::nativeCodepage(baseFilename, '?', usedReplacement);
	sci::ostringstream message;
	message << sU("The filename ") << baseFilename << sU(" contains Unicode characters that cannot be represented with an ANSI string. Please complain to NetCDF that their Windows library does not support Unicode filenames. The file cannot be created.");
	sci::assertThrow(!usedReplacement, sci::err(sci::SERR_USER, 0, message.str()));
#endif

	//Check if there is an existing file and get its history
	sci::string existingHistory;
	size_t prevMajorVersion = 0;
	size_t prevMinorVersion = 0;
	std::vector<sci::string> existingFiles = sci::getAllFiles(directory, false, true);
	for (size_t i = 0; i < existingFiles.size(); ++i)
	{
		if (existingFiles[i].substr(0, baseFilename.length()) == baseFilename)
		{
			try
			{
				sci::InputNcFile previous(existingFiles[i]);
				sci::string thisExistingHistory = previous.getGlobalAttribute<sci::string>(sU("history"));
				sci::string previousVersionString = previous.getGlobalAttribute<sci::string>(sU("product_version"));
				sci::assertThrow(previousVersionString.length() > 1 && previousVersionString[0] == 'v', sci::err(sci::SERR_USER, 0, sU("Previous version of the file has an ill formatted product version.")));

				int thisPrevMajorVersion;
				int thisPrevMinorVersion;
				//remove the v from the beginning of the version string
				previousVersionString = previousVersionString.substr(1);
				//replace the . with a space
				if (previousVersionString.find_first_of(sU('.')) != sci::string::npos)
				{
					previousVersionString[previousVersionString.find_first_of(sU('.'))] = sU(' ');

					//replace the b in beta (if it is there) with a space
					if (previousVersionString.find_first_of(sU('b')) != sci::string::npos)
					{
						previousVersionString[previousVersionString.find_first_of(sU('b'))] = sU(' ');
					}
				}
				else
					previousVersionString = previousVersionString + sU(" 0"); //if the minor version isn't there, then add it

				//put the string into a stream and read out the major and minor versions
				sci::istringstream versionStream(previousVersionString);
				versionStream >> thisPrevMajorVersion;
				versionStream >> thisPrevMinorVersion;

				if (thisPrevMajorVersion > prevMajorVersion || (thisPrevMajorVersion == prevMajorVersion && thisPrevMinorVersion > prevMinorVersion))
				{
					prevMajorVersion = thisPrevMajorVersion;
					prevMinorVersion = thisPrevMinorVersion;
					existingHistory = thisExistingHistory;
				}
			}
			catch (...)
			{
				//we end up here if the file is not a valid netcdf or some other read problem
				//This isn't an issue, just move on to the next file.
			}
		}
	}

	size_t majorVersion;
	size_t minorVersion;
	if (incrementMajorVersion || prevMajorVersion == 0)
	{
		majorVersion = prevMajorVersion+1;
		minorVersion = 0;
	}
	else
	{
		majorVersion = prevMajorVersion;
		minorVersion = prevMinorVersion + 1;
	}

	sci::ostringstream versionString;
	versionString << sU("v") << majorVersion << sU(".") << minorVersion << (dataInfo.processingOptions.beta ? sU("beta") : sU(""));

	//Now construct the final filename
	sci::ostringstream filenameStream;
	filenameStream << baseFilename << sU("_") << versionString.str() << sU(".nc");
	sci::string filename = filenameStream.str();

	//create the new file
	openWritable(filename, '_');
	sci::string amfVersionString;
	if (amfVersion == AmfVersion::v1_1_0)
		amfVersionString = sU("1.1.0");
	else if (amfVersion == AmfVersion::v2_0_0)
		amfVersionString = sU("2.0.0");
	else
		throw(sci::err(sci::SERR_USER, 0, sU("Attempting to write a netcdf file with an unknown standard version.")));

	write(sci::NcAttribute(sU("Conventions"), sci::string(sU("CF-1.6, NCAS-AMF-")) + amfVersionString));
	write(sci::NcAttribute(sU("source"), instrumentInfo.description.length() > 0 ? instrumentInfo.description  : sU("not available")));
	write(sci::NcAttribute(sU("instrument_manufacturer"), instrumentInfo.manufacturer.length() > 0 ? instrumentInfo.manufacturer : sU("not available")));
	write(sci::NcAttribute(sU("instrument_model"), instrumentInfo.model.length() > 0 ? instrumentInfo.model : sU("not available")));
	write(sci::NcAttribute(sU("instrument_serial_number"), instrumentInfo.serial.length() > 0 ? instrumentInfo.serial : sU("not available")));
	write(sci::NcAttribute(sU("instrument_software"), instrumentInfo.operatingSoftware.length() > 0 ? instrumentInfo.operatingSoftware : sU("not available")));
	write(sci::NcAttribute(sU("instrument_software_version"), instrumentInfo.operatingSoftwareVersion.length() > 0 ? instrumentInfo.operatingSoftwareVersion : sU("not available")));
	write(sci::NcAttribute(sU("creator_name"), author.name.length() > 0 ? author.name : sU("not available")));
	write(sci::NcAttribute(sU("creator_email"), author.email.length() > 0 ? author.email : sU("not available")));
	write(sci::NcAttribute(sU("creator_url"), author.orcidUrl.length() > 0 ? author.orcidUrl : sU("not available")));
	write(sci::NcAttribute(sU("institution"), author.institution.length() > 0 ? author.institution : sU("not available")));
	write(sci::NcAttribute(sU("processing_software_url"), processingsoftwareInfo.url.length() > 0 ? processingsoftwareInfo.url : sU("not available")));
	write(sci::NcAttribute(sU("processing_software_version"), processingsoftwareInfo.version.length() > 0 ? processingsoftwareInfo.version : sU("not available")));
	write(sci::NcAttribute(sU("calibration_sensitivity"), calibrationInfo.calibrationDescription.length() > 0 ? calibrationInfo.calibrationDescription : sU("not available")));
	if (calibrationInfo.certificationDate == sci::UtcTime(0, 1, 1, 0, 0, 0))
		write(sci::NcAttribute(sU("calibration_certification_date"), sU("not available")));
	else
		write(sci::NcAttribute(sU("calibration_certification_date"), getFormattedDateTime(calibrationInfo.certificationDate, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("calibration_certification_url"), calibrationInfo.certificateUrl.length() > 0 ? calibrationInfo.certificateUrl : sU("not available")));
	sci::stringstream samplingIntervalString;
	samplingIntervalString << dataInfo.samplingInterval;
	write(sci::NcAttribute(sU("sampling_interval"), samplingIntervalString.str()));
	sci::stringstream averagingIntervalString;
	averagingIntervalString << dataInfo.averagingPeriod;
	write(sci::NcAttribute(sU("averaging_interval"), averagingIntervalString.str()));
	write(sci::NcAttribute(sU("processing_level"), dataInfo.processingLevel));
	write(sci::NcAttribute(sU("project"), projectInfo.name.length() > 0 ? projectInfo.name : sU("not available")));
	write(sci::NcAttribute(sU("project_principal_investigator"), projectInfo.principalInvestigatorInfo.name.length() > 0 ? projectInfo.principalInvestigatorInfo.name : sU("not available")));
	write(sci::NcAttribute(sU("project_principal_investigator_email"), projectInfo.principalInvestigatorInfo.email.length() > 0 ? projectInfo.principalInvestigatorInfo.email : sU("not available")));
	write(sci::NcAttribute(sU("project_principal_investigator_url"), projectInfo.principalInvestigatorInfo.orcidUrl.length() > 0 ? projectInfo.principalInvestigatorInfo.orcidUrl : sU("not available")));
	write(sci::NcAttribute(sU("licence"), sci::string(sU("Data usage licence - UK Government Open Licence agreement: http://www.nationalarchives.gov.uk/doc/open-government-licence"))));
	write(sci::NcAttribute(sU("acknowledgement"), sci::string(sU("Acknowledgement of NCAS as the data provider is required whenever and wherever these data are used"))));
	write(sci::NcAttribute(sU("platform"), platform.getPlatformInfo().name.length() > 0 ? platform.getPlatformInfo().name : sU("not available")));
	write(sci::NcAttribute(sU("platform_type"), g_platformTypeStrings.find(platform.getPlatformInfo().platformType)->second));
	write(sci::NcAttribute(sU("deployment_mode"), g_deploymentModeStrings.find(platform.getPlatformInfo().deploymentMode)->second));
	write(sci::NcAttribute(sU("title"), title));
	write(sci::NcAttribute(sU("feature_type"), g_featureTypeStrings.find(dataInfo.featureType)->second));
	write(sci::NcAttribute(sU("time_coverage_start"), getFormattedDateTime(dataInfo.startTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("time_coverage_end"), getFormattedDateTime(dataInfo.endTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("geospacial_bounds"), getBoundsString(m_latitudes, m_longitudes)));
	if (platform.getFixedAltitude())
	{
		degreeF latitude;
		degreeF longitude;
		metreF altitude;
		platform.getLocation(m_times[m_times.size() / 2], m_times[m_times.size() / 2], latitude, longitude, altitude);
		sci::stringstream altitudeString;
		altitudeString << altitude;
		write(sci::NcAttribute(sU("platform_altitude"), altitudeString.str()));
	}
	else
	{
		write(sci::NcAttribute(sU("platform_altitude"), sci::string(sU("Not Applicable"))));
	}
	sci::stringstream locationKeywords;
	if (platform.getPlatformInfo().locationKeywords.size() > 0)
		locationKeywords << platform.getPlatformInfo().locationKeywords[0];
	for (size_t i = 1; i < platform.getPlatformInfo().locationKeywords.size(); ++i)
		locationKeywords << ", " << platform.getPlatformInfo().locationKeywords[i];
	write(sci::NcAttribute(sU("location_keywords"), locationKeywords.str()));
	write(sci::NcAttribute(sU("amf_vocabularies_release"), sci::string(sU("https://github.com/ncasuk/AMF_CVs/releases/tag/v"))+ amfVersionString));
	write(sci::NcAttribute(sU("comment"), dataInfo.processingOptions.comment + sci::string(dataInfo.processingOptions.beta ? sU("\nBeta release - not to be used in published science.") : sU(""))));
	
	for (size_t i = 0; i < globalAttributes.size(); ++i)
		write(*globalAttributes[i]);

	sci::string processingReason = dataInfo.reasonForProcessing;
	if (processingReason.length() == 0)
		processingReason = sU("Processed.");

	if (existingHistory.length() == 0)
	{
		existingHistory = getFormattedDateTime(dataInfo.endTime, sU("-"), sU(":"), sU("T")) + sU(" - Data collected.");
	}

	sci::UtcTime now = sci::UtcTime::now();
	sci::ostringstream history;
	history << existingHistory + sU("\n") + getFormattedDateTime(now, sU("-"), sU(":"), sU("T")) + sU(" ") + versionString.str() << sU(" ") + processingReason;
	write(sci::NcAttribute(sU("history"), history.str()));
	write(sci::NcAttribute(sU("product_version"), versionString.str()));
	write(sci::NcAttribute(sU("last_revised_date"), getFormattedDateTime(now, sU("-"), sU(":"), sU("T"))));

	//Now add the time dimension
	write(m_timeDimension);
	//we also need to add a lat and lon dimension, although we don't actually use them
	//this helps indexing. They have size 1 for stationary platforms or same as time
	//for moving platforms.
	//Note for trajectory data we do not include the lat and lon dimensions
	sci::NcDimension longitudeDimension(sU("longitude"), m_longitudes.size());
	sci::NcDimension latitudeDimension(sU("latitude"), m_latitudes.size());
	if (dataInfo.featureType != FeatureType::trajectory)
	{
		write(longitudeDimension);
		write(latitudeDimension);
	}
	//and any other dimensions
	for (size_t i = 0; i < nonTimeDimensions.size(); ++i)
		write(*nonTimeDimensions[i]);

	//create the time variables, but do not write the data as we need to stay in define mode
	//so the user can add other variables
	m_timeVariable.reset(new AmfNcTimeVariable(*this, getTimeDimension(), m_times));
	m_dayOfYearVariable.reset(new AmfNcVariable<float, std::vector<float>>(sU("day_of_year"), *this, getTimeDimension(), sU("Day of Year"), sU(""), sU("1"), m_dayOfYears, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), 1));
	m_yearVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("year"), *this, getTimeDimension(), sU("Year"), sU(""), sU("1"), m_years, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), 1));
	m_monthVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("month"), *this, getTimeDimension(), sU("Month"), sU(""), sU("1"), m_months, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), 1));
	m_dayVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("day"), *this, getTimeDimension(), sU("Day"), sU(""), sU("1"), m_dayOfMonths, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), 1));
	m_hourVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("hour"), *this, getTimeDimension(), sU("Hour"), sU(""), sU("1"), m_hours, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), 1));
	m_minuteVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("minute"), *this, getTimeDimension(), sU("Minute"), sU(""), sU("1"), m_minutes, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), 1));
	m_secondVariable.reset(new AmfNcVariable<float, std::vector<float>>(sU("second"), *this, getTimeDimension(), sU("Second"), sU(""), sU("1"), m_seconds, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), 1));

	//for trajectories, the lat/lon depend on the time dimension, not the lat/lon dimensions (which aren't created)
	if (dataInfo.featureType == FeatureType::trajectory)
	{
		m_longitudeVariable.reset(new AmfNcLongitudeVariable(*this, m_timeDimension, m_longitudes, dataInfo.featureType, platform.getPlatformInfo().deploymentMode));
		m_latitudeVariable.reset(new AmfNcLatitudeVariable(*this, m_timeDimension, m_latitudes, dataInfo.featureType, platform.getPlatformInfo().deploymentMode));
	}
	else
	{
		m_longitudeVariable.reset(new AmfNcLongitudeVariable(*this, longitudeDimension, m_longitudes, dataInfo.featureType, platform.getPlatformInfo().deploymentMode));
		m_latitudeVariable.reset(new AmfNcLatitudeVariable(*this, latitudeDimension, m_latitudes, dataInfo.featureType, platform.getPlatformInfo().deploymentMode));
	}

	if (platform.getPlatformInfo().platformType == PlatformType::moving && !overriddenPlatformPosition)
	{
		std::vector<std::pair<sci::string, CellMethod>> motionCellMethods{ {sU("time"), CellMethod::mean} };
		std::vector<std::pair<sci::string, CellMethod>> motionStdevCellMethods{ {sU("time"), CellMethod::standardDeviation} };
		std::vector<std::pair<sci::string, CellMethod>> motionMinCellMethods{ {sU("time"), CellMethod::min} };
		std::vector<std::pair<sci::string, CellMethod>> motionMaxCellMethods{ {sU("time"), CellMethod::max} };
		std::vector<sci::string> motionCoordinates{ sU("longitude"), sU("latitude") };

		if(sci::anyTrue(sci::isEq(m_courses, m_courses)))
			m_courseVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("platform_course"), *this, getTimeDimension(), sU("Direction in which the platform is travelling"), sU("platform_course"), m_courses, true, motionCoordinates, motionCellMethods, 1));
		if (sci::anyTrue(sci::isEq(m_speeds, m_speeds)))
			m_speedVariable.reset(new AmfNcVariable<metrePerSecondF, std::vector<metrePerSecondF>>(sU("platform_speed_wrt_ground"), *this, getTimeDimension(), sU("Platform speed with respect to ground"), sU("platform_speed_wrt_ground"), m_speeds, true, motionCoordinates, motionCellMethods, 1));
		if (sci::anyTrue(sci::isEq(m_headings, m_headings)))
			m_orientationVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("platform_orientation"), *this, getTimeDimension(), sU("Direction in which \"front\" of platform is pointing"), sU("platform_orientation"), m_headings, true, motionCoordinates, motionCellMethods, 1));

		if (sci::anyTrue(sci::isEq(m_elevations, m_elevations)))
		{
			m_instrumentPitchVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_pitch_angle"), *this, getTimeDimension(), sU("Instrument Pitch Angle"), sU(""), m_elevations, true, motionCoordinates, motionCellMethods, 1));
			m_instrumentPitchStdevVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_pitch_standard_deviation"), *this, getTimeDimension(), sU("Instrument Pitch Angle Standard Deviation"), sU(""), m_elevationStdevs, true, motionCoordinates, motionStdevCellMethods, 1));
			m_instrumentPitchMinVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_pitch_minimum"), *this, getTimeDimension(), sU("Instrument Pitch Angle Minimum"), sU(""), m_elevationMins, true, motionCoordinates, motionMinCellMethods, 1));
			m_instrumentPitchMaxVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_pitch_maximum"), *this, getTimeDimension(), sU("Instrument Pitch Angle Maximum"), sU(""), m_elevationMaxs, true, motionCoordinates, motionMaxCellMethods, 1));
			m_instrumentPitchRateVariable.reset(new AmfNcVariable<degreePerSecondF, std::vector<degreePerSecondF>>(sU("instrument_pitch_rate"), *this, getTimeDimension(), sU("Instrument Pitch Angle Rate of Change"), sU(""), m_elevationRates, true, motionCoordinates, motionCellMethods, 1));
		}

		if (sci::anyTrue(sci::isEq(m_azimuths, m_azimuths)))
		{
			m_instrumentYawVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_yaw_angle"), *this, getTimeDimension(), sU("Instrument Yaw Angle"), sU(""), m_azimuths, true, motionCoordinates, motionCellMethods, 1));
			m_instrumentYawStdevVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_yaw_standard_deviation"), *this, getTimeDimension(), sU("Instrument Yaw Angle Standard Deviation"), sU(""), m_azimuthStdevs, true, motionCoordinates, motionStdevCellMethods, 1));
			m_instrumentYawMinVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_yaw_minimum"), *this, getTimeDimension(), sU("Instrument Yaw Angle Minimum"), sU(""), m_azimuthMins, true, motionCoordinates, motionMinCellMethods, 1));
			m_instrumentYawMaxVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_yaw_maximum"), *this, getTimeDimension(), sU("Instrument Yaw Angle Maximum"), sU(""), m_azimuthMaxs, true, motionCoordinates, motionMaxCellMethods, 1));
			m_instrumentYawRateVariable.reset(new AmfNcVariable<degreePerSecondF, std::vector<degreePerSecondF>>(sU("instrument_yaw_rate"), *this, getTimeDimension(), sU("Instrument Yaw Angle Rate of Change"), sU(""), m_azimuthRates, true, motionCoordinates, motionCellMethods, 1));
		}

		if (sci::anyTrue(sci::isEq(m_rolls, m_rolls)))
		{
			m_instrumentRollVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_roll_angle"), *this, getTimeDimension(), sU("Instrument Roll Angle"), sU(""), m_rolls, true, motionCoordinates, motionCellMethods, 1));
			m_instrumentRollStdevVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_roll_standard_deviation"), *this, getTimeDimension(), sU("Instrument Roll Angle Standard Deviation"), sU(""), m_rollStdevs, true, motionCoordinates, motionStdevCellMethods, 1));
			m_instrumentRollMinVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_roll_minimum"), *this, getTimeDimension(), sU("Instrument Roll Angle Minimum"), sU(""), m_rollMins, true, motionCoordinates, motionMinCellMethods, 1));
			m_instrumentRollMaxVariable.reset(new AmfNcVariable<degreeF, std::vector<degreeF>>(sU("instrument_roll_maximum"), *this, getTimeDimension(), sU("Instrument Roll Angle Maximum"), sU(""), m_rollMaxs, true, motionCoordinates, motionMaxCellMethods, 1));
			m_instrumentRollRateVariable.reset(new AmfNcVariable<degreePerSecondF, std::vector<degreePerSecondF>>(sU("instrument_roll_rate"), *this, getTimeDimension(), sU("Instrument Roll Angle Rate of Change"), sU(""), m_rollRates, true, motionCoordinates, motionCellMethods, 1));
		}

	}
	//and the time variable
	/*std::vector<double> secondsAfterEpoch(times.size());
	sci::UtcTime epoch(1970, 1, 1, 0, 0, 0.0);
	for (size_t i = 0; i < times.size(); ++i)
		secondsAfterEpoch[i] = times[i] - epoch;
	//this will end define mode!
	write(AmfNcTimeVariable(*this, m_timeDimension), secondsAfterEpoch);*/

}

void OutputAmfNcFile::writeTimeAndLocationData(const Platform &platform)
{
	std::vector<double> secondsAfterEpoch(m_times.size());

	sci::UtcTime epoch(1970, 1, 1, 0, 0, 0.0);
	for (size_t i = 0; i < m_times.size(); ++i)
	{
		secondsAfterEpoch[i] = (m_times[i] - epoch).value<secondF>();
	}

	write(*m_timeVariable, secondsAfterEpoch);
	write(*m_dayOfYearVariable, m_dayOfYears);
	write(*m_yearVariable, m_years);
	write(*m_monthVariable, m_months);
	write(*m_dayVariable, m_dayOfMonths);
	write(*m_hourVariable, m_hours);
	write(*m_minuteVariable, m_minutes);
	write(*m_secondVariable, m_seconds);

	write(*m_latitudeVariable, sci::physicalsToValues<degreeF>(m_latitudes));
	write(*m_longitudeVariable, sci::physicalsToValues<degreeF>(m_longitudes));

	if (m_speeds.size() > 0)
	{
		if (m_instrumentPitchVariable)
		{
			write(*m_instrumentPitchVariable, m_elevations);
			write(*m_instrumentPitchStdevVariable, m_elevationStdevs);
			write(*m_instrumentPitchMinVariable, m_elevationMins);
			write(*m_instrumentPitchMaxVariable, m_elevationMaxs);
			write(*m_instrumentPitchRateVariable, m_elevationRates);
		}

		if (m_instrumentYawVariable)
		{
			write(*m_instrumentYawVariable, m_azimuths);
			write(*m_instrumentYawStdevVariable, m_azimuthStdevs);
			write(*m_instrumentYawMinVariable, m_azimuthMins);
			write(*m_instrumentYawMaxVariable, m_azimuthMaxs);
			write(*m_instrumentYawRateVariable, m_azimuthRates);
		}

		if (m_instrumentRollVariable)
		{
			write(*m_instrumentRollVariable, m_rolls);
			write(*m_instrumentRollStdevVariable, m_rollStdevs);
			write(*m_instrumentRollMinVariable, m_rollMins);
			write(*m_instrumentRollMaxVariable, m_rollMaxs);
			write(*m_instrumentRollRateVariable, m_rollRates);
		}

		if(m_courseVariable)
			write(*m_courseVariable, m_courses);
		if(m_speedVariable)
			write(*m_speedVariable, m_speeds);
		if(m_orientationVariable)
			write(*m_orientationVariable, m_headings);
	}


}

std::vector<std::pair<sci::string, CellMethod>> getLatLonCellMethod(FeatureType featureType, DeploymentMode deploymentMode)
{
	if (featureType == FeatureType::trajectory)
		return std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}};
	if (deploymentMode == DeploymentMode::sea)
		return std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::mean}};
	if (deploymentMode == DeploymentMode::air)
		return std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}};

	//land has no cell method
	return
		std::vector<std::pair<sci::string, CellMethod>>(0);
}

/*OutputAmfSeaNcFile::OutputAmfSeaNcFile(const sci::string &directory,
	const InstrumentInfo &instrumentInfo,
	const PersonInfo &author,
	const ProcessingSoftwareInfo &processingsoftwareInfo,
	const CalibrationInfo &calibrationInfo,
	const DataInfo &dataInfo,
	const ProjectInfo &projectInfo,
	const PlatformInfo &platformInfo,
	const sci::string &comment,
	const std::vector<sci::UtcTime> &times,
	const std::vector<double> &latitudes,
	const std::vector<double> &longitudes,
	const std::vector<sci::NcDimension *> &nonTimeDimensions)
	:OutputAmfNcFile(directory, instrumentInfo, author, processingsoftwareInfo, calibrationInfo, dataInfo, projectInfo, platformInfo, comment, times)
{
	write(AmfNcLatitudeVariable(*this, getTimeDimension()), latitudes);
	write(AmfNcLongitudeVariable(*this, getTimeDimension()), longitudes);
}*/