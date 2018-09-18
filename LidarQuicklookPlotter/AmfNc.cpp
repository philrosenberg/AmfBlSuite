#include"AmfNc.h"
#include<sstream>
#include<iomanip>

std::string getFormattedTime(const sci::UtcTime &time)
{
	std::stringstream timeString;
	timeString << std::setfill('0');
	timeString << time.getYear() << "-"
		<< std::setw(2) << time.getMonth() << "-"
		<< std::setw(2) << time.getDayOfMonth() << "T"
		<< std::setw(2) << time.getHour() << ":"
		<< std::setw(2) << time.getMinute() << ":"
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

std::string getBoundsString(const DataInfo &dataInfo)
{
	bool hasLatNan = std::isnan(dataInfo.minLatDecimalDegrees) || std::isnan(dataInfo.maxLatDecimalDegrees);
	bool hasLonNan = std::isnan(dataInfo.minLonDecimalDegrees) || std::isnan(dataInfo.maxLonDecimalDegrees);
	bool hasLatValue = !std::isnan(dataInfo.minLatDecimalDegrees) || !std::isnan(dataInfo.maxLatDecimalDegrees);
	bool hasLonValue = !std::isnan(dataInfo.minLonDecimalDegrees) || !std::isnan(dataInfo.maxLonDecimalDegrees);
	bool valid = hasLatValue && hasLonValue //must have at least one value for lat/lon
		&& (hasLatNan == hasLonNan); //either both are flagged as points or neither are

	bool isPoint = hasLatNan || (dataInfo.minLatDecimalDegrees == dataInfo.maxLatDecimalDegrees && dataInfo.minLonDecimalDegrees == dataInfo.maxLonDecimalDegrees);

	std::stringstream result;
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

OutputAmfNcFile::OutputAmfNcFile(const std::wstring &filename,
	const InstrumentInfo &instrumentInfo,
	const PersonInfo &author,
	const ProcessingSoftwareInfo &processingsoftwareInfo,
	const CalibrationInfo &calibrationInfo,
	const DataInfo &dataInfo,
	const ProjectInfo &projectInfo,
	const PlatformInfo &platformInfo,
	const std::string &comment)
	:OutputNcFile(filename)
{
	write(sci::NcAttribute("Conventions", "CF-1.6, NCAS-AMF-0.2.3"));
	write(sci::NcAttribute("source", instrumentInfo.description));
	write(sci::NcAttribute("instrument_manufacturer", instrumentInfo.manufacturer));
	write(sci::NcAttribute("instrument_model", instrumentInfo.model));
	write(sci::NcAttribute("instrument_serial_number", instrumentInfo.serial));
	write(sci::NcAttribute("instrument_software", instrumentInfo.operatingSoftware));
	write(sci::NcAttribute("instrument_software_version", instrumentInfo.operatingSoftwareVersion));
	write(sci::NcAttribute("creator_name", author.name));
	write(sci::NcAttribute("creator_email", author.email));
	write(sci::NcAttribute("creator_url", author.orcidUrl));
	write(sci::NcAttribute("institution", author.institution));
	write(sci::NcAttribute("processing_software_url", processingsoftwareInfo.url));
	write(sci::NcAttribute("processing_software_version", processingsoftwareInfo.version));
	write(sci::NcAttribute("calibration_sensitivity", getFormattedTime(calibrationInfo.sensitivityCalibrationDate)));
	write(sci::NcAttribute("calibration_certification_date", getFormattedTime(calibrationInfo.certificationDate)));
	write(sci::NcAttribute("calibration_certification_url", calibrationInfo.certificateUrl));
	std::stringstream samplingIntervalString;
	samplingIntervalString << dataInfo.sampingInterval << " " << dataInfo.samplingIntervalUnit;
	write(sci::NcAttribute("sampling_interval", samplingIntervalString.str()));
	std::stringstream averagingIntervalString;
	averagingIntervalString << dataInfo.averagingPeriod << " " << dataInfo.averagingPeriodUnit;
	write(sci::NcAttribute("averaging_interval", samplingIntervalString.str()));
	write(sci::NcAttribute("processing_level", dataInfo.processingLevel));
	write(sci::NcAttribute("project", projectInfo.name));
	write(sci::NcAttribute("project_principal_investigator", projectInfo.principalInvestigatorInfo.name));
	write(sci::NcAttribute("project_principal_investigator_email", projectInfo.principalInvestigatorInfo.email));
	write(sci::NcAttribute("project_principal_investigator_url", projectInfo.principalInvestigatorInfo.orcidUrl));
	write(sci::NcAttribute("licence", "Data usage licence - UK Government Open Licence agreement: http://www.nationalarchives.gov.uk/doc/open-government-licence"));
	write(sci::NcAttribute("acknowledgement", "Acknowledgement of NCAS as the data provider is required whenever and wherever these data are used"));
	write(sci::NcAttribute("platform", platformInfo.name));
	write(sci::NcAttribute("platform_type", g_platformTypeStrings[ platformInfo.platformType ]));
	write(sci::NcAttribute("deployment_mode", g_deploymentModeStrings[ platformInfo.deploymentMode ] ));
	write(sci::NcAttribute("title", dataInfo.title));
	write(sci::NcAttribute("feature_type", g_featureTypeStrings[ dataInfo.featureType ] ));
	write(sci::NcAttribute("time_coverage_start", getFormattedTime(dataInfo.startTime)));
	write(sci::NcAttribute("time_coverage_end", getFormattedTime(dataInfo.endTime)));
	write(sci::NcAttribute("geospacial_bounds", getBoundsString(dataInfo)));
	if (std::isnan(platformInfo.altitudeMetres))
	{
		write(sci::NcAttribute("platform_altitude", "Not Applicable"));
	}
	else
	{
		std::stringstream altitudeString;
		altitudeString << platformInfo.altitudeMetres << " m";
		write(sci::NcAttribute("platform_altitude", altitudeString.str()));
	}
	std::stringstream locationKeywords;
	if (platformInfo.locationKeywords.size() > 0)
		locationKeywords << platformInfo.locationKeywords[0];
	for (size_t i = 1; i < platformInfo.locationKeywords.size(); ++i)
		locationKeywords << ", " << platformInfo.locationKeywords[i];
	write(sci::NcAttribute("location_keywords", locationKeywords.str()));
	write(sci::NcAttribute("amf_vocabularies_release", "https://github.com/ncasuk/AMF_CVs/tree/v0.2.3"));
	write(sci::NcAttribute("comment", comment));

	//Sort out product_version and history and last_revised_date
	std::string existingHistory;
	//add some code to read history from an existing file

	std::string processingReason = dataInfo.reasonForProcessing;
	if (processingReason.length() == 0)
		processingReason = "Processed.";
	sci::UtcTime now = sci::UtcTime::now();
	if (existingHistory.length() == 0)
	{
		std::string history = getFormattedTime(dataInfo.endTime) + " - Data collected.\n" + getFormattedTime(now) + " - v0 " + processingReason;
		write(sci::NcAttribute("history", history));
		write(sci::NcAttribute("product_version", "v0"));
	}
	
	else
	{
		int prevVersion;
		if (existingHistory.find_last_of('\n') == std::string::npos)
			prevVersion = -1;
		else
		{
			std::string lastHistoryLine(existingHistory.substr(existingHistory.find_last_of('\n') + 1));
			if (lastHistoryLine.length() > 20 && lastHistoryLine[19] == 'v')
			{
				std::string trimmedLastHistroryLine = lastHistoryLine.substr(20);
				std::istringstream readVersionString;
				readVersionString.str(trimmedLastHistroryLine);
				readVersionString >> prevVersion;
			}
			else
				prevVersion = -1;
		}
		std::ostringstream versionString;
		versionString << "v" << prevVersion + 1;
		std::string history = getFormattedTime(dataInfo.endTime) + " - Data collected.\n" + getFormattedTime(now) + " - " + versionString.str() + processingReason;
		write(sci::NcAttribute("history", history));
		write(sci::NcAttribute("product_version", versionString.str()));
	}
	write(sci::NcAttribute("last_revised_date", getFormattedTime(now)));
}