#define _USE_MATH_DEFINES
#include"AmfNc.h"
#include<sstream>
#include<iomanip>
#include<filesystem>
#include<cmath>
#include<locale>

void correctDirection(degree measuredElevation, degree measuredAzimuth, unitless sinInstrumentElevation, unitless sinInstrumentAzimuth, unitless sinInstrumentRoll, unitless cosInstrumentElevation, unitless cosInstrumentAzimuth, unitless cosInstrumentRoll, degree &correctedElevation, degree &correctedAzimuth)
{
	//create a unit vector in the measured direction
	unitless x = sci::sin(measuredAzimuth)*sci::cos(measuredElevation);
	unitless y = sci::cos(measuredAzimuth)*sci::cos(measuredElevation);
	unitless z = sci::sin(measuredElevation);

	//correct the vector
	unitless correctedX;
	unitless correctedY;
	unitless correctedZ;
	correctDirection(x, y, z, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedX, correctedY, correctedZ);

	//convert the unit vector back to spherical polar
	correctedElevation = sci::asin(correctedZ);
	correctedAzimuth = sci::atan2(correctedX, correctedY);
}

degree angleBetweenDirections(degree elevation1, degree azimuth1, degree elevation2, degree azimuth2)
{
	return sci::acos(sci::cos(degree(90) - elevation1)*sci::cos(degree(90) - elevation2) + sci::sin(degree(90) - elevation1)*sin(degree(90) - elevation2)*sci::cos(azimuth2 - azimuth1));
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

sci::string getBoundsString(const DataInfo &dataInfo)
{
	bool hasLatNan = std::isnan(dataInfo.minLat) || std::isnan(dataInfo.maxLat);
	bool hasLonNan = std::isnan(dataInfo.minLon) || std::isnan(dataInfo.maxLon);
	bool hasLatValue = !std::isnan(dataInfo.minLat) || !std::isnan(dataInfo.maxLat);
	bool hasLonValue = !std::isnan(dataInfo.minLon) || !std::isnan(dataInfo.maxLon);
	bool valid = hasLatValue && hasLonValue //must have at least one value for lat/lon
		&& (hasLatNan == hasLonNan); //either both are flagged as points or neither are

	bool isPoint = hasLatNan || (dataInfo.minLat == dataInfo.maxLat && dataInfo.minLon == dataInfo.maxLon);

	sci::stringstream result;
	if (isPoint)
	{
		if (!std::isnan(dataInfo.minLat))
			result << dataInfo.minLat << " N ";
		else
			result << dataInfo.maxLat << " N ";

		if (!std::isnan(dataInfo.minLon))
			result << dataInfo.minLon << " E";
		else
			result << dataInfo.maxLon << " E";
	}
	else
	{
		result << dataInfo.minLat << " N " << dataInfo.minLon << " E, "
			<< dataInfo.maxLat << " N " << dataInfo.maxLon << " E";
	}

	return result.str();
}

OutputAmfNcFile::OutputAmfNcFile(const sci::string &directory,
	const InstrumentInfo &instrumentInfo,
	const PersonInfo &author,
	const ProcessingSoftwareInfo &processingsoftwareInfo,
	const CalibrationInfo &calibrationInfo,
	const DataInfo &dataInfo,
	const ProjectInfo &projectInfo,
	const Platform &platform,
	const std::vector<sci::UtcTime> &times,
	const std::vector<sci::NcDimension *> &nonTimeDimensions)
	:OutputNcFile(), m_timeDimension(sU("time"), times.size()), m_times(times)
{
	//construct the title for the dataset
	//add the instument name
	sci::string title = instrumentInfo.name;
	//add the platform if there is one
	if (platform.getPlatformInfo().name.length() > 0)
		title = title + sU("-") + platform.getPlatformInfo().name;
	//add either the date or date and time depending upon whether the data is
	//continuous or a single measurement
	if (dataInfo.continuous)
		title = title + sU("-") + getFormattedDateOnly(dataInfo.startTime, sU(""));
	else
		title = title + sU("-") + getFormattedDateTime(dataInfo.startTime, sU(""), sU(""), sU("T"));
	//add the data product name
	title = title + sU("-") + dataInfo.productName;
	//add any options
	for (size_t i = 0; i < dataInfo.options.size(); ++i)
		title = title + sU("_") + dataInfo.options[i];

	//Swap spaces for underscores and make lower case
	std::locale locale;
	for (size_t i = 0; i < title.length(); ++i)
	{
		if (title[i] == sU(' '))
			title[i] = sU('_');
		else
			title[i] = std::tolower(title[i], locale);
	}


	//construct the file path, without the version number of suffix
	sci::string baseFilename = directory;
	//append a directory separator if needed
	if (baseFilename.length() > 0 && baseFilename.back() != sU('\\') && baseFilename.back() != sU('/'))
	{
		baseFilename = baseFilename + sU('/');
	}
	baseFilename = baseFilename + title + sU("_v");

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
	int prevVersion = -1;
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
				
				int thisPrevVersion;
				sci::istringstream versionStream(previousVersionString.substr(1));
				versionStream >> thisPrevVersion;

				if (thisPrevVersion > prevVersion)
				{
					prevVersion = thisPrevVersion;
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

	//Now construct the final filename
	sci::ostringstream filenameStream;
	filenameStream << baseFilename << prevVersion + 1 << (dataInfo.processingOptions.beta ? sU("beta") : sU("")) << sU(".nc");
	sci::string filename = filenameStream.str();

	//create the new file
	openWritable(filename, '_');

	write(sci::NcAttribute(sU("Conventions"), sci::string(sU("CF-1.6, NCAS-AMF-0.2.3"))));
	write(sci::NcAttribute(sU("source"), instrumentInfo.description));
	write(sci::NcAttribute(sU("instrument_manufacturer"), instrumentInfo.manufacturer));
	write(sci::NcAttribute(sU("instrument_model"), instrumentInfo.model));
	write(sci::NcAttribute(sU("instrument_serial_number"), instrumentInfo.serial));
	write(sci::NcAttribute(sU("instrument_software"), instrumentInfo.operatingSoftware));
	write(sci::NcAttribute(sU("instrument_software_version"), instrumentInfo.operatingSoftwareVersion));
	write(sci::NcAttribute(sU("creator_name"), author.name));
	write(sci::NcAttribute(sU("creator_email"), author.email));
	write(sci::NcAttribute(sU("creator_url"), author.orcidUrl));
	write(sci::NcAttribute(sU("institution"), author.institution));
	write(sci::NcAttribute(sU("processing_software_url"), processingsoftwareInfo.url));
	write(sci::NcAttribute(sU("processing_software_version"), processingsoftwareInfo.version));
	write(sci::NcAttribute(sU("calibration_sensitivity"), calibrationInfo.calibrationDescription));
	if(calibrationInfo.certificationDate == sci::UtcTime(0,1,1,0,0,0))
		write(sci::NcAttribute(sU("calibration_certification_date"), sU("Not available")));
	else
		write(sci::NcAttribute(sU("calibration_certification_date"), getFormattedDateTime(calibrationInfo.certificationDate, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("calibration_certification_url"), calibrationInfo.certificateUrl));
	sci::stringstream samplingIntervalString;
	samplingIntervalString << dataInfo.samplingInterval;
	write(sci::NcAttribute(sU("sampling_interval"), samplingIntervalString.str()));
	sci::stringstream averagingIntervalString;
	averagingIntervalString << dataInfo.averagingPeriod;
	write(sci::NcAttribute(sU("averaging_interval"), averagingIntervalString.str()));
	write(sci::NcAttribute(sU("processing_level"), dataInfo.processingLevel));
	write(sci::NcAttribute(sU("project"), projectInfo.name));
	write(sci::NcAttribute(sU("project_principal_investigator"), projectInfo.principalInvestigatorInfo.name));
	write(sci::NcAttribute(sU("project_principal_investigator_email"), projectInfo.principalInvestigatorInfo.email));
	write(sci::NcAttribute(sU("project_principal_investigator_url"), projectInfo.principalInvestigatorInfo.orcidUrl));
	write(sci::NcAttribute(sU("licence"), sci::string(sU("Data usage licence - UK Government Open Licence agreement: http://www.nationalarchives.gov.uk/doc/open-government-licence"))));
	write(sci::NcAttribute(sU("acknowledgement"), sci::string(sU("Acknowledgement of NCAS as the data provider is required whenever and wherever these data are used"))));
	write(sci::NcAttribute(sU("platform"), platform.getPlatformInfo().name));
	write(sci::NcAttribute(sU("platform_type"), g_platformTypeStrings[platform.getPlatformInfo().platformType]));
	write(sci::NcAttribute(sU("deployment_mode"), g_deploymentModeStrings[platform.getPlatformInfo().deploymentMode] ));
	write(sci::NcAttribute(sU("title"), title));
	write(sci::NcAttribute(sU("feature_type"), g_featureTypeStrings[ dataInfo.featureType ] ));
	write(sci::NcAttribute(sU("time_coverage_start"), getFormattedDateTime(dataInfo.startTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("time_coverage_end"), getFormattedDateTime(dataInfo.endTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("geospacial_bounds"), getBoundsString(dataInfo)));
	sci::assertThrow(platform.getPlatformInfo().altitudes.size() > 0, sci::err(sci::SERR_USER, 0, "Attempting to output platform info for a platform with no altitude information."));
	if (platform.getPlatformInfo().altitudes.size()>1)
	{
		write(sci::NcAttribute(sU("platform_altitude"), sci::string(sU("Not Applicable"))));
	}
	else
	{
		sci::stringstream altitudeString;
		altitudeString << platform.getPlatformInfo().altitudes[0];
		write(sci::NcAttribute(sU("platform_altitude"), altitudeString.str()));
	}
	sci::stringstream locationKeywords;
	if (platform.getPlatformInfo().locationKeywords.size() > 0)
		locationKeywords << platform.getPlatformInfo().locationKeywords[0];
	for (size_t i = 1; i < platform.getPlatformInfo().locationKeywords.size(); ++i)
		locationKeywords << ", " << platform.getPlatformInfo().locationKeywords[i];
	write(sci::NcAttribute(sU("location_keywords"), locationKeywords.str()));
	write(sci::NcAttribute(sU("amf_vocabularies_release"), sci::string(sU("https://github.com/ncasuk/AMF_CVs/tree/v0.2.3"))));
	write(sci::NcAttribute(sU("comment"), dataInfo.processingOptions.comment + sci::string(dataInfo.processingOptions.beta ? sU("\nBeta release - not to be used in published science."): sU(""))));

	

	sci::string processingReason = dataInfo.reasonForProcessing;
	if (processingReason.length() == 0)
		processingReason = sU("Processed.");

	if (existingHistory.length() == 0)
	{
		existingHistory = getFormattedDateTime(dataInfo.endTime, sU("-"), sU(":"), sU("T")) + sU(" - Data collected.");
	}

	sci::UtcTime now = sci::UtcTime::now();
	sci::ostringstream history;
	history << existingHistory + sU("\n") + getFormattedDateTime(now, sU("-"), sU(":"), sU("T")) + sU(" - v") << prevVersion + 1 << sU(" ") + processingReason;
	write(sci::NcAttribute(sU("history"), history.str()));
	sci::ostringstream versionString;
	versionString << sU("v") << prevVersion + 1 << (dataInfo.processingOptions.beta ? sU("beta") : sU(""));
	write(sci::NcAttribute(sU("product_version"), versionString.str()));
	write(sci::NcAttribute(sU("last_revised_date"), getFormattedDateTime(now, sU("-"), sU(":"), sU("T"))));

	//Now add the time dimension
	write(m_timeDimension);
	//we also need to add a lat and lon dimension, although we don't actually use them
	//this helps indexing. They have size 1 for stationary platforms or same as time
	//for moving platforms.
	sci::NcDimension longitudeDimension(sU("longitude"), platform.getPlatformInfo().platformType==pt_stationary ? 1 : times.size());
	sci::NcDimension latitudeDimension(sU("latitude"), platform.getPlatformInfo().platformType == pt_stationary ? 1 : times.size());
	write(longitudeDimension);
	write(latitudeDimension);
	//and any other dimensions
	for (size_t i = 0; i < nonTimeDimensions.size(); ++i)
		write(*nonTimeDimensions[i]);

	//create the time variables, but do not write the data as we need to stay in define mode
	//so the user can add other variables

	m_timeVariable.reset(new AmfNcTimeVariable(*this, getTimeDimension()));
	m_longitudeVariable.reset(new AmfNcLongitudeVariable(*this, longitudeDimension));
	m_latitudeVariable.reset(new AmfNcLatitudeVariable(*this, latitudeDimension));
	m_dayOfYearVariable.reset(new AmfNcVariable<double>(sU("day_of_year"), *this, getTimeDimension(),sU("Day of Year"), sU(""), sU("1"), 0.0, 366.0));
	m_yearVariable.reset(new AmfNcVariable<int>(sU("year"), *this, getTimeDimension(), sU("Year"), sU(""), sU("1"), std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
	m_monthVariable.reset(new AmfNcVariable<int>(sU("month"), *this, getTimeDimension(), sU("Month"), sU(""), sU("1"), 1, 12));
	m_dayVariable.reset(new AmfNcVariable<int>(sU("day"), *this, getTimeDimension(), sU("Day"), sU(""), sU("1"), 1, 31));
	m_hourVariable.reset(new AmfNcVariable<int>(sU("hour"), *this, getTimeDimension(), sU("Hour"), sU(""), sU("1"), 0, 23));
	m_minuteVariable.reset(new AmfNcVariable<int>(sU("minute"), *this, getTimeDimension(), sU("Minute"), sU(""), sU("1"), 0, 59));
	m_secondVariable.reset(new AmfNcVariable<double>(sU("second"), *this, getTimeDimension(), sU("Second"), sU(""), sU("1"), 0.0, 60*(1.0-std::numeric_limits<double>::epsilon())));

	m_courseVariable.reset(new AmfNcVariable<degree>(sU("platform_course"), *this, getTimeDimension(), sU("Direction in which the platform is travelling"), sU("platform_course"), degree(-180), degree(180)));
	m_speedVariable.reset(new AmfNcVariable<metrePerSecond>(sU("platform_speed_wrt_ground"), *this, getTimeDimension(), sU("Platform speed with respect to ground"), sU("platform_speed_wrt_ground"), std::numeric_limits<metrePerSecond>::min(), std::numeric_limits<metrePerSecond>::max()));
	m_orientationVariable.reset(new AmfNcVariable<degree>(sU("platform_orientation"), *this, getTimeDimension(), sU("Direction in which \"front\" of platform is pointing"), sU("platform_orientation"), degree(-180), degree(180)));
	m_instrumentPitchVariable.reset(new AmfNcVariable<degree>(sU("instrument_pitch_angle"), *this, getTimeDimension(), sU("Instrument Pitch Angle"), sU(""), degree(-90), degree(90)));
	m_instrumentYawVariable.reset(new AmfNcVariable<degree>(sU("instrument_yaw_angle"), *this, getTimeDimension(), sU("Instrument Yaw Angle"), sU(""), degree(-180), degree(180)));
	m_instrumentRollVariable.reset(new AmfNcVariable<degree>(sU("instrument_roll_angle"), *this, getTimeDimension(), sU("Instrument Roll Angle"), sU(""), degree(-180), degree(180)));

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
	std::vector<double> dayOfYear(m_times.size());
	std::vector<int> year(m_times.size());
	std::vector<int> month(m_times.size());
	std::vector<int> day(m_times.size());
	std::vector<int> hour(m_times.size());
	std::vector<int> minute(m_times.size());
	std::vector<double> second(m_times.size());

	sci::UtcTime epoch(1970, 1, 1, 0, 0, 0.0);
	for (size_t i = 0; i < m_times.size(); ++i)
	{
		secondsAfterEpoch[i] = m_times[i] - epoch;
		year[i] = m_times[i].getYear();
		month[i] = m_times[i].getMonth();
		day[i] = m_times[i].getDayOfMonth();
		hour[i] = m_times[i].getHour();
		minute[i] = m_times[i].getMinute();
		second[i] = m_times[i].getSecond();
		sci::UtcTime startOfYear(year[i], 1, 1, 0, 0, 0);
		dayOfYear[i] = (m_times[i] - startOfYear) / 60.0 / 60.0 / 24.0;
	}

	write(*m_timeVariable, secondsAfterEpoch);
	write(*m_dayOfYearVariable, dayOfYear);
	write(*m_yearVariable, year);
	write(*m_monthVariable, month);
	write(*m_dayVariable, day);
	write(*m_hourVariable, hour);
	write(*m_minuteVariable, minute);
	write(*m_secondVariable, second);

	if (platform.getPlatformInfo().platformType == pt_stationary)
	{
		std::vector<degree> latitudes(1, degree(0.0));
		std::vector<degree> longitudes(1, degree(0.0));
		metre altitude;
		//get the location for the middle time - it should be irrelevant anyway as it is stationary
		platform.getLocation(m_times[m_times.size() / 2], latitudes[0], longitudes[0], altitude);
		write(*m_latitudeVariable, sci::physicalsToValues<degree>(latitudes));
		write(*m_longitudeVariable, sci::physicalsToValues<degree>(longitudes));
	}
	else
	{
		//get the lat and lon and all the other movement variables at the appropriate time
		std::vector<degree> latitudes(m_times.size(), degree(0.0));
		std::vector<degree> longitudes(m_times.size(), degree(0.0));
		std::vector<degree> courses(m_times.size(), degree(0.0));
		std::vector<degree> orientations(m_times.size(), degree(0.0));
		std::vector<metrePerSecond> speeds(m_times.size(), metrePerSecond(0.0));
		std::vector<degree> pitches(m_times.size(), degree(0.0));
		std::vector<degree> rolls(m_times.size(), degree(0.0));
		std::vector<degree> yaws(m_times.size(), degree(0.0));

		for (size_t i = 0; i < m_times.size(); ++i)
		{
			metre altitude;
			platform.getLocation(m_times[i], latitudes[i], longitudes[i], altitude);
			platform.getMotion(m_times[i], speeds[i], courses[i], orientations[i]);
			platform.getInstrumentAttitudes(m_times[i], pitches[i], yaws[i], rolls[i]);
		}
		write(*m_latitudeVariable, sci::physicalsToValues<degree>(latitudes));
		write(*m_longitudeVariable, sci::physicalsToValues<degree>(longitudes));
		write(*m_courseVariable, courses);
		write(*m_speedVariable, speeds);
		write(*m_orientationVariable, orientations);
		write(*m_instrumentPitchVariable, pitches);
		write(*m_instrumentYawVariable, yaws);
		write(*m_instrumentRollVariable, rolls);
	}
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