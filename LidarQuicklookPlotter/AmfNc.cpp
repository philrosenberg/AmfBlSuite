#include"AmfNc.h"
#include<sstream>
#include<iomanip>
#include<filesystem>

sci::string getFormattedDateTime(const sci::UtcTime &time)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << time.getYear() << sU("-")
		<< std::setw(2) << time.getMonth() << sU("-")
		<< std::setw(2) << time.getDayOfMonth() << sU("T")
		<< std::setw(2) << time.getHour() << sU(":")
		<< std::setw(2) << time.getMinute() << sU(":")
		<< std::setw(2) << std::floor(time.getSecond());

	return timeString.str();
}

sci::string getFormattedDateOnly(const sci::UtcTime &time)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << time.getYear() << sU("-")
		<< std::setw(2) << time.getMonth() << sU("-")
		<< std::setw(2) << time.getDayOfMonth();

	return timeString.str();
}

sci::string getFormattedTimeOnly(const sci::UtcTime &time)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << std::setw(2) << time.getHour() << sU(":")
		<< std::setw(2) << time.getMinute() << sU(":")
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
	bool hasLatNan = std::isnan(dataInfo.minLatDecimalDegrees) || std::isnan(dataInfo.maxLatDecimalDegrees);
	bool hasLonNan = std::isnan(dataInfo.minLonDecimalDegrees) || std::isnan(dataInfo.maxLonDecimalDegrees);
	bool hasLatValue = !std::isnan(dataInfo.minLatDecimalDegrees) || !std::isnan(dataInfo.maxLatDecimalDegrees);
	bool hasLonValue = !std::isnan(dataInfo.minLonDecimalDegrees) || !std::isnan(dataInfo.maxLonDecimalDegrees);
	bool valid = hasLatValue && hasLonValue //must have at least one value for lat/lon
		&& (hasLatNan == hasLonNan); //either both are flagged as points or neither are

	bool isPoint = hasLatNan || (dataInfo.minLatDecimalDegrees == dataInfo.maxLatDecimalDegrees && dataInfo.minLonDecimalDegrees == dataInfo.maxLonDecimalDegrees);

	sci::stringstream result;
	if (isPoint)
	{
		if (!std::isnan(dataInfo.minLatDecimalDegrees))
			result << dataInfo.minLatDecimalDegrees << "N ";
		else
			result << dataInfo.maxLatDecimalDegrees << "N ";

		if (!std::isnan(dataInfo.minLonDecimalDegrees))
			result << dataInfo.minLonDecimalDegrees << "E";
		else
			result << dataInfo.maxLonDecimalDegrees << "E";
	}
	else
	{
		result << dataInfo.minLatDecimalDegrees << "N " << dataInfo.minLonDecimalDegrees << "E, "
			<< dataInfo.maxLatDecimalDegrees << "N " << dataInfo.maxLonDecimalDegrees << "E";
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
	const sci::string &comment)
	:OutputNcFile()
{
	//construct the title for the dataset
	//add the instument name
	sci::string title = instrumentInfo.name;
	//add the platform if there is one
	if (platformInfo.name.length() > 0)
		title = title + sU("_") + platformInfo.name;
	//add either the date or date and time depending upon whether the data is
	//continuous or a single measurement
	if (dataInfo.continuous)
		title = title + sU("_") + getFormattedDateOnly(dataInfo.startTime);
	else
		title = title + sU("_") + getFormattedDateTime(dataInfo.startTime);
	//add the data product name
	title = title + sU("_") + dataInfo.productName;
	//add any options
	for (size_t i = 0; i < dataInfo.options.size(); ++i)
		title = title + sU("_") + dataInfo.options[i];


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
	write(sci::NcAttribute(sU("calibration_sensitivity"), getFormattedDateTime(calibrationInfo.sensitivityCalibrationDate)));
	write(sci::NcAttribute(sU("calibration_certification_date"), getFormattedDateTime(calibrationInfo.certificationDate)));
	write(sci::NcAttribute(sU("calibration_certification_url"), calibrationInfo.certificateUrl));
	sci::stringstream samplingIntervalString;
	samplingIntervalString << dataInfo.sampingInterval << " " << dataInfo.samplingIntervalUnit;
	write(sci::NcAttribute(sU("sampling_interval"), samplingIntervalString.str()));
	sci::stringstream averagingIntervalString;
	averagingIntervalString << dataInfo.averagingPeriod << " " << dataInfo.averagingPeriodUnit;
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
	write(sci::NcAttribute(sU("time_coverage_start"), getFormattedDateTime(dataInfo.startTime)));
	write(sci::NcAttribute(sU("time_coverage_end"), getFormattedDateTime(dataInfo.endTime)));
	write(sci::NcAttribute(sU("geospacial_bounds"), getBoundsString(dataInfo)));
	if (std::isnan(platformInfo.altitudeMetres))
	{
		write(sci::NcAttribute(sU("platform_altitude"), sci::string(sU("Not Applicable"))));
	}
	else
	{
		sci::stringstream altitudeString;
		altitudeString << platformInfo.altitudeMetres << " m";
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
		existingHistory = getFormattedDateTime(dataInfo.endTime) + sU(" - Data collected.");
	}

	sci::UtcTime now = sci::UtcTime::now();
	sci::ostringstream history;
	history << existingHistory + sU("\n") + getFormattedDateTime(now) + sU(" - v") << prevVersion + 1 << sU(" ") + processingReason;
	write(sci::NcAttribute(sU("history"), history.str()));
	sci::ostringstream versionString;
	versionString << sU("v") << prevVersion + 1;
	write(sci::NcAttribute(sU("product_version"), versionString.str()));
	write(sci::NcAttribute(sU("last_revised_date"), getFormattedDateTime(now)));
}