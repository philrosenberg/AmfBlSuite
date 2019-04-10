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

sci::string getBoundsString(const std::vector<degree> & latitudes, const std::vector<degree> &longitudes)
{
	sci::stringstream result;
	if (latitudes.size() == 1)
	{
		result << latitudes[0].value<degree>() << sU(" N ") << longitudes[0].value<degree>() << sU(" E");
	}
	else
	{
		result << sci::min<degree>(latitudes).value<degree>() << " N " << sci::min<degree>(longitudes).value<degree>() << " E, "
			<< sci::max<degree>(latitudes).value<degree>() << " N " << sci::min<degree>(longitudes).value<degree>() << " E";
	}

	return result.str();
}

//this function takes an array of angles and starting from
//the second element it adds or subtracts mutiples of 360 degrees
//so that the absolute differnce between two elements is never
//more than 180 degrees
void makeContinuous(std::vector<degree> &angles)
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
		degree jumpAmount = sci::ceil((*(iter - 1) - *iter - degree(180)) / degree(360))*degree(360);
		*iter += jumpAmount;
	}
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
		m_dayOfYears[i] = std::floor(((sci::UtcTime((int)m_years[i], (unsigned int)m_months[i], (unsigned int)m_dayOfMonths[i], 0, 0, 0) - sci::UtcTime((int)m_years[i], 1, 1, 0, 0, 0)) / second(60.0 * 60.0 * 24.0)).value<unitless>());
		m_hours[i] = times[i].getHour();
		m_minutes[i] = times[i].getMinute();
		m_seconds[i] = times[i].getSecond();
	}
	//sort out the position and attitude data
	if (platform.getPlatformInfo().platformType == pt_stationary)
	{
		degree longitude;
		degree latitude;
		metre altitude;
		platform.getLocation(times[times.size() / 2], times[times.size() / 2], latitude, longitude, altitude);
		m_longitudes = { longitude };
		m_latitudes = { latitude };
	}
	else
	{
		m_longitudes.resize(m_times.size(), degree(0));
		m_latitudes.resize(m_times.size(), degree(0));
		m_elevations.resize(m_times.size(), degree(0));
		m_elevationStdevs.resize(m_times.size(), degree(0));
		m_elevationMins.resize(m_times.size(), degree(0));
		m_elevationMaxs.resize(m_times.size(), degree(0));
		m_elevationRates.resize(m_times.size(), degreePerSecond(0));
		m_azimuths.resize(m_times.size(), degree(0));
		m_azimuthStdevs.resize(m_times.size(), degree(0));
		m_azimuthMins.resize(m_times.size(), degree(0));
		m_azimuthMaxs.resize(m_times.size(), degree(0));
		m_azimuthRates.resize(m_times.size(), degreePerSecond(0));
		m_rolls.resize(m_times.size(), degree(0));
		m_rollStdevs.resize(m_times.size(), degree(0));
		m_rollMins.resize(m_times.size(), degree(0));
		m_rollMaxs.resize(m_times.size(), degree(0));
		m_rollRates.resize(m_times.size(), degreePerSecond(0));
		m_courses.resize(m_times.size(), degree(0));
		m_speeds.resize(m_times.size(), metrePerSecond(0));
		m_headings.resize(m_times.size(), degree(0));
		metre altitude;
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

	//construct the title for the dataset
	//add the instument name
	sci::string title = instrumentInfo.name;
	//add the platform if there is one
	if (platform.getPlatformInfo().name.length() > 0)
		title = title + sU("_") + platform.getPlatformInfo().name;
	//add either the date or date and time depending upon whether the data is
	//continuous or a single measurement
	if (dataInfo.continuous)
		title = title + sU("_") + getFormattedDateOnly(dataInfo.startTime, sU(""));
	else
		title = title + sU("_") + getFormattedDateTime(dataInfo.startTime, sU(""), sU(""), sU("T"));
	//add the data product name
	title = title + sU("_") + dataInfo.productName;
	//add any options
	for (size_t i = 0; i < dataInfo.options.size(); ++i)
		title = title + sU("_") + dataInfo.options[i];

	//Swap spaces for hyphens and make lower case
	std::locale locale;
	for (size_t i = 0; i < title.length(); ++i)
	{
		if (title[i] == sU(' '))
			title[i] = sU('-');
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
	if (calibrationInfo.certificationDate == sci::UtcTime(0, 1, 1, 0, 0, 0))
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
	write(sci::NcAttribute(sU("deployment_mode"), g_deploymentModeStrings[platform.getPlatformInfo().deploymentMode]));
	write(sci::NcAttribute(sU("title"), title));
	write(sci::NcAttribute(sU("feature_type"), g_featureTypeStrings[dataInfo.featureType]));
	write(sci::NcAttribute(sU("time_coverage_start"), getFormattedDateTime(dataInfo.startTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("time_coverage_end"), getFormattedDateTime(dataInfo.endTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("geospacial_bounds"), getBoundsString(m_latitudes, m_longitudes)));
	if (platform.getFixedAltitude())
	{
		degree latitude;
		degree longitude;
		metre altitude;
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
	write(sci::NcAttribute(sU("amf_vocabularies_release"), sci::string(sU("https://github.com/ncasuk/AMF_CVs/tree/v0.2.3"))));
	write(sci::NcAttribute(sU("comment"), dataInfo.processingOptions.comment + sci::string(dataInfo.processingOptions.beta ? sU("\nBeta release - not to be used in published science.") : sU(""))));


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
	sci::NcDimension longitudeDimension(sU("longitude"), platform.getPlatformInfo().platformType == pt_stationary ? 1 : times.size());
	sci::NcDimension latitudeDimension(sU("latitude"), platform.getPlatformInfo().platformType == pt_stationary ? 1 : times.size());
	write(longitudeDimension);
	write(latitudeDimension);
	//and any other dimensions
	for (size_t i = 0; i < nonTimeDimensions.size(); ++i)
		write(*nonTimeDimensions[i]);

	//create the time variables, but do not write the data as we need to stay in define mode
	//so the user can add other variables
	m_timeVariable.reset(new AmfNcTimeVariable(*this, getTimeDimension(), m_times));
	m_dayOfYearVariable.reset(new AmfNcVariable<double, std::vector<double>>(sU("day_of_year"), *this, getTimeDimension(), sU("Day of Year"), sU(""), sU("1"), m_dayOfYears));
	m_yearVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("year"), *this, getTimeDimension(), sU("Year"), sU(""), sU("1"), m_years));
	m_monthVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("month"), *this, getTimeDimension(), sU("Month"), sU(""), sU("1"), m_months));
	m_dayVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("day"), *this, getTimeDimension(), sU("Day"), sU(""), sU("1"), m_dayOfMonths));
	m_hourVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("hour"), *this, getTimeDimension(), sU("Hour"), sU(""), sU("1"), m_hours));
	m_minuteVariable.reset(new AmfNcVariable<int, std::vector<int>>(sU("minute"), *this, getTimeDimension(), sU("Minute"), sU(""), sU("1"), m_minutes));
	m_secondVariable.reset(new AmfNcVariable<double, std::vector<double>>(sU("second"), *this, getTimeDimension(), sU("Second"), sU(""), sU("1"), m_seconds));

	m_longitudeVariable.reset(new AmfNcLongitudeVariable(*this, longitudeDimension, m_longitudes));
	m_latitudeVariable.reset(new AmfNcLatitudeVariable(*this, latitudeDimension, m_latitudes));

	if (platform.getPlatformInfo().platformType == pt_moving)
	{
		m_courseVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("platform_course"), *this, getTimeDimension(), sU("Direction in which the platform is travelling"), sU("platform_course"), m_courses));
		m_speedVariable.reset(new AmfNcVariable<metrePerSecond, std::vector<metrePerSecond>>(sU("platform_speed_wrt_ground"), *this, getTimeDimension(), sU("Platform speed with respect to ground"), sU("platform_speed_wrt_ground"), m_speeds));
		m_orientationVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("platform_orientation"), *this, getTimeDimension(), sU("Direction in which \"front\" of platform is pointing"), sU("platform_orientation"), m_headings));

		m_instrumentPitchVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_pitch_angle"), *this, getTimeDimension(), sU("Instrument Pitch Angle"), sU(""), m_elevations));
		m_instrumentPitchStdevVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_pitch_standard_deviation"), *this, getTimeDimension(), sU("Instrument Pitch Angle Standard Deviation"), sU(""), m_elevationStdevs));
		m_instrumentPitchMinVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_pitch_minimum"), *this, getTimeDimension(), sU("Instrument Pitch Angle Minimum"), sU(""), m_elevationMins));
		m_instrumentPitchMaxVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_pitch_maximum"), *this, getTimeDimension(), sU("Instrument Pitch Angle Maximum"), sU(""), m_elevationMaxs));
		m_instrumentPitchRateVariable.reset(new AmfNcVariable<degreePerSecond, std::vector<degreePerSecond>>(sU("instrument_pitch_rate"), *this, getTimeDimension(), sU("Instrument Pitch Angle Rate of Change"), sU(""), m_elevationRates));

		m_instrumentYawVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_yaw_angle"), *this, getTimeDimension(), sU("Instrument Yaw Angle"), sU(""), m_azimuths));
		m_instrumentYawStdevVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_yaw_standard_deviation"), *this, getTimeDimension(), sU("Instrument Yaw Angle Standard Deviation"), sU(""), m_azimuthStdevs));
		m_instrumentYawMinVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_yaw_minimum"), *this, getTimeDimension(), sU("Instrument Yaw Angle Minimum"), sU(""), m_azimuthMins));
		m_instrumentYawMaxVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_yaw_maximum"), *this, getTimeDimension(), sU("Instrument Yaw Angle Maximum"), sU(""), m_azimuthMaxs));
		m_instrumentYawRateVariable.reset(new AmfNcVariable<degreePerSecond, std::vector<degreePerSecond>>(sU("instrument_yaw_rate"), *this, getTimeDimension(), sU("Instrument Yaw Angle Rate of Change"), sU(""), m_azimuthRates));

		m_instrumentRollVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_roll_angle"), *this, getTimeDimension(), sU("Instrument Roll Angle"), sU(""), m_rolls));
		m_instrumentRollStdevVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_roll_standard_deviation"), *this, getTimeDimension(), sU("Instrument Roll Angle Standard Deviation"), sU(""), m_rollStdevs));
		m_instrumentRollMinVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_roll_minimum"), *this, getTimeDimension(), sU("Instrument Roll Angle Minimum"), sU(""), m_rollMins));
		m_instrumentRollMaxVariable.reset(new AmfNcVariable<degree, std::vector<degree>>(sU("instrument_roll_maximum"), *this, getTimeDimension(), sU("Instrument Roll Angle Maximum"), sU(""), m_rollMaxs));
		m_instrumentRollRateVariable.reset(new AmfNcVariable<degreePerSecond, std::vector<degreePerSecond>>(sU("instrument_roll_rate"), *this, getTimeDimension(), sU("Instrument Roll Angle Rate of Change"), sU(""), m_rollRates));

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
		secondsAfterEpoch[i] = (m_times[i] - epoch).value<second>();
	}

	write(*m_timeVariable, secondsAfterEpoch);
	write(*m_dayOfYearVariable, m_dayOfYears);
	write(*m_yearVariable, m_years);
	write(*m_monthVariable, m_months);
	write(*m_dayVariable, m_dayOfMonths);
	write(*m_hourVariable, m_hours);
	write(*m_minuteVariable, m_minutes);
	write(*m_secondVariable, m_seconds);

	write(*m_latitudeVariable, sci::physicalsToValues<degree>(m_latitudes));
	write(*m_longitudeVariable, sci::physicalsToValues<degree>(m_longitudes));

	if (platform.getPlatformInfo().platformType == pt_moving)
	{
		write(*m_instrumentPitchVariable, m_elevations);
		write(*m_instrumentPitchStdevVariable, m_elevationStdevs);
		write(*m_instrumentPitchMinVariable, m_elevationMins);
		write(*m_instrumentPitchMaxVariable, m_elevationMaxs);
		write(*m_instrumentPitchRateVariable, m_elevationRates);

		write(*m_instrumentYawVariable, m_azimuths);
		write(*m_instrumentYawStdevVariable, m_azimuthStdevs);
		write(*m_instrumentYawMinVariable, m_azimuthMins);
		write(*m_instrumentYawMaxVariable, m_azimuthMaxs);
		write(*m_instrumentYawRateVariable, m_azimuthRates);

		write(*m_instrumentRollVariable, m_rolls);
		write(*m_instrumentRollStdevVariable, m_rollStdevs);
		write(*m_instrumentRollMinVariable, m_rollMins);
		write(*m_instrumentRollMaxVariable, m_rollMaxs);
		write(*m_instrumentRollRateVariable, m_rollRates);

		write(*m_courseVariable, m_courses);
		write(*m_speedVariable, m_speeds);
		write(*m_orientationVariable, m_headings);
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