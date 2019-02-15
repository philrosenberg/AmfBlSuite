#define _USE_MATH_DEFINES
#include"AmfNc.h"
#include<sstream>
#include<iomanip>
#include<filesystem>
#include<cmath>
#include<locale>

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
	const PlatformInfo &platformInfo,
	const sci::string &comment,
	const std::vector<sci::UtcTime> &times,
	degree longitude,
	degree latitude,
	const std::vector<sci::NcDimension *> &nonTimeDimensions)
	:OutputNcFile(), m_timeDimension(sU("time"), times.size()), m_times(times), m_longitude(longitude), m_latitude(latitude)
{
	//construct the title for the dataset
	//add the instument name
	sci::string title = instrumentInfo.name;
	//add the platform if there is one
	if (platformInfo.name.length() > 0)
		title = title + sU("-") + platformInfo.name;
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
	if (usedReplacement)
		throw("The filename contains Unicode characters that cannot be represented with an ANSI string. Please complain to NetCDF that their Windows library does not support Unicode filenames. The file cannot be created.");
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
				if (previousVersionString.length() < 2 || previousVersionString[0] != 'v')
				{
					throw("Previous version of the file has an ill formatted product version.");
				}
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
	filenameStream << baseFilename << prevVersion + 1 << sU(".nc");
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
	write(sci::NcAttribute(sU("platform"), platformInfo.name));
	write(sci::NcAttribute(sU("platform_type"), g_platformTypeStrings[ platformInfo.platformType ]));
	write(sci::NcAttribute(sU("deployment_mode"), g_deploymentModeStrings[ platformInfo.deploymentMode ] ));
	write(sci::NcAttribute(sU("title"), title));
	write(sci::NcAttribute(sU("feature_type"), g_featureTypeStrings[ dataInfo.featureType ] ));
	write(sci::NcAttribute(sU("time_coverage_start"), getFormattedDateTime(dataInfo.startTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("time_coverage_end"), getFormattedDateTime(dataInfo.endTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("geospacial_bounds"), getBoundsString(dataInfo)));
	if (std::isnan(platformInfo.altitude))
	{
		write(sci::NcAttribute(sU("platform_altitude"), sci::string(sU("Not Applicable"))));
	}
	else
	{
		sci::stringstream altitudeString;
		altitudeString << platformInfo.altitude;
		write(sci::NcAttribute(sU("platform_altitude"), altitudeString.str()));
	}
	sci::stringstream locationKeywords;
	if (platformInfo.locationKeywords.size() > 0)
		locationKeywords << platformInfo.locationKeywords[0];
	for (size_t i = 1; i < platformInfo.locationKeywords.size(); ++i)
		locationKeywords << ", " << platformInfo.locationKeywords[i];
	write(sci::NcAttribute(sU("location_keywords"), locationKeywords.str()));
	write(sci::NcAttribute(sU("amf_vocabularies_release"), sci::string(sU("https://github.com/ncasuk/AMF_CVs/tree/v0.2.3"))));
	write(sci::NcAttribute(sU("comment"), comment));

	

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
	versionString << sU("v") << prevVersion + 1;
	write(sci::NcAttribute(sU("product_version"), versionString.str()));
	write(sci::NcAttribute(sU("last_revised_date"), getFormattedDateTime(now, sU("-"), sU(":"), sU("T"))));

	//Now add the time dimension
	write(m_timeDimension);
	//we also need to add a lat and lon dimension, although we don't actually use them
	//this helps indexing
	sci::NcDimension longitudeDimension(sU("longitude"), 1);
	sci::NcDimension latitudeDimension(sU("latitude"), 1);
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
	m_dayOfYearVariable.reset(new AmfNcVariable<double>(sU("day_of_year"), *this, getTimeDimension(),sU("Day of Year"), sU("1"), 0.0, 366.0));
	m_yearVariable.reset(new AmfNcVariable<int>(sU("year"), *this, getTimeDimension(), sU("Year"), sU("1"), std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
	m_monthVariable.reset(new AmfNcVariable<int>(sU("month"), *this, getTimeDimension(), sU("Month"), sU("1"), 1, 12));
	m_dayVariable.reset(new AmfNcVariable<int>(sU("day"), *this, getTimeDimension(), sU("Day"), sU("1"), 1, 31));
	m_hourVariable.reset(new AmfNcVariable<int>(sU("hour"), *this, getTimeDimension(), sU("Hour"), sU("1"), 0, 23));
	m_minuteVariable.reset(new AmfNcVariable<int>(sU("minute"), *this, getTimeDimension(), sU("Minute"), sU("1"), 0, 59));
	m_secondVariable.reset(new AmfNcVariable<double>(sU("second"), *this, getTimeDimension(), sU("Second"), sU("1"), 0.0, 60*(1.0-std::numeric_limits<double>::epsilon())));
	

	//and the time variable
	/*std::vector<double> secondsAfterEpoch(times.size());
	sci::UtcTime epoch(1970, 1, 1, 0, 0, 0.0);
	for (size_t i = 0; i < times.size(); ++i)
		secondsAfterEpoch[i] = times[i] - epoch;
	//this will end define mode!
	write(AmfNcTimeVariable(*this, m_timeDimension), secondsAfterEpoch);*/
	
}

void OutputAmfNcFile::writeTimeAndLocationData()
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

	write(*m_latitudeVariable, std::vector<double>(1, m_latitude.value<degree>()));
	write(*m_longitudeVariable, std::vector<double>(1, m_latitude.value<degree>()));
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