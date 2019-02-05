#pragma once
#include<vector>
#include<svector/sreadwrite.h>
#include<string>
#include<svector/time.h>
#include<svector/sstring.h>
#include<svector/Units.h>
#include"Units.h"

struct HplHeader;
class HplProfile;
class wxWindow;
class ProgressReporter;
class CampbellMessage2;
class CampbellCeilometerProfile;


struct InstrumentInfo
{
	sci::string name;
	sci::string description;
	sci::string manufacturer;
	sci::string model;
	sci::string serial;
	sci::string operatingSoftware;
	sci::string operatingSoftwareVersion;
};

struct ProcessingSoftwareInfo
{
	sci::string url;
	sci::string version;
};

struct PersonInfo
{
	sci::string name;
	sci::string email;
	sci::string orcidUrl;
	sci::string institution;
};

struct CalibrationInfo
{
	sci::string calibrationDescription; //range, what was done, etc
	sci::UtcTime certificationDate;
	sci::string certificateUrl; //link to a cal certificate or cal file
};

enum FeatureType
{
	ft_timeSeriesPoint
};
const std::vector<sci::string> g_featureTypeStrings{ sU("timeSeriesPoint") };

struct DataInfo
{
	double sampingInterval;
	sci::string samplingIntervalUnit;
	double averagingPeriod;
	sci::string averagingPeriodUnit;
	int processingLevel;
	FeatureType featureType;
	double minLatDecimalDegrees;//for point measurements set min and max the same or set one to nan
	double maxLatDecimalDegrees;
	double minLonDecimalDegrees;
	double maxLonDecimalDegrees;
	sci::UtcTime startTime;
	sci::UtcTime endTime;
	sci::string reasonForProcessing; // This string is put in the history. Add any special reason for processing if there is any e.g. Reprocessing due to x error or initial processing to be updated later for y reason
	bool continuous; // set to true if the measurements are continuously made over a moderate period of false if it is a one-off (e.g. a sonde)
	sci::string productName;
	std::vector<sci::string> options;
};

struct ProjectInfo
{
	sci::string name;
	PersonInfo principalInvestigatorInfo;
};

enum PlatformType
{
	pt_stationary,
	pt_moving
};
const std::vector<sci::string> g_platformTypeStrings{ sU("stationary_platform"), sU("moving_platform") };

enum DeploymentMode
{
	dm_air,
	dm_land,
	dm_sea
};
const std::vector<sci::string> g_deploymentModeStrings{ sU("air"), sU("sea"), sU("land") };

struct PlatformInfo
{
	sci::string name;
	PlatformType platformType;
	DeploymentMode deploymentMode;
	double altitudeMetres; //Use nan for varying altitude airborne
	std::vector<sci::string> locationKeywords;
	std::vector<radian> latitudes; //would have just one element for a static platform
	std::vector<radian>longitudes; //would have just one element for a static platform
};

class OutputAmfNcFile : public sci::OutputNcFile
{
public:
	OutputAmfNcFile(const sci::string &directory,
		const InstrumentInfo &instrumentInfo,
		const PersonInfo &author,
		const ProcessingSoftwareInfo &processingsoftwareInfo,
		const CalibrationInfo &calibrationInfo,
		const DataInfo &dataInfo,
		const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo,
		const sci::string &comment,
		const std::vector<sci::UtcTime> &times,
		const std::vector<sci::NcDimension *> &nonTimeDimensions= std::vector<sci::NcDimension *>(0));
	sci::NcDimension &getTimeDimension() { return m_timeDimension; }
private:
	sci::NcDimension m_timeDimension;
};
/*template<>
void sci::OutputNcFile::write<metre>(const sci::NcVariable<metre> &variable, const std::vector<metre> &data)
{
	//static_assert(false, "Using this template");
}*/

template <class T>
class AmfNcVariable : public sci::NcVariable<T>
{
public:
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &units)
		:NcVariable<T>(name, ncFile, dimension)
	{
		setAttributes(longName, units);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &units)
		:NcVariable<T>(name, ncFile, dimensions)
	{
		setAttributes(longName, units);
	}
private:
	void setAttributes(const sci::string &longName, const sci::string &units)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute unitsAttribute(sU("units"), units);
		addAttribute(longNameAttribute);
		addAttribute(unitsAttribute);
	}
};

//create a partial specialization for any AmfNcVariable based on a sci::Physical value
template <class T>
class AmfNcVariable<sci::Physical<T>> : public sci::NcVariable<sci::Physical<T>>
{
public:
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName)
		:NcVariable<sci::Physical<T>>(name, ncFile, dimension)
	{
		setAttributes(longName);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName)
		:NcVariable<sci::Physical<T>>(name, ncFile, dimensions)
	{
		setAttributes(longName);
	}
	template <class U>
	static auto convertValues(const std::vector<U> &physicals) -> decltype(sci::physicalsToValues<T>(physicals))
	{
		return sci::physicalsToValues<sci::Physical<T>>(physicals);
	}

private:
	void setAttributes(const sci::string &longName)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute unitsAttribute(sU("units"), sci::Physical<T>::getShortUnitString());
		addAttribute(longNameAttribute);
		addAttribute(unitsAttribute);
	}
};




class AmfNcTimeVariable : public AmfNcVariable<double>
{
public:
	AmfNcTimeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension)
		:AmfNcVariable<double>(sU("time"), ncFile, dimension, sU("Time (seconds since 1970-01-01 00:00:00)"), sU("seconds since 1970-01-01 00:00:00"))
	{}
};
class AmfNcLongitudeVariable : public AmfNcVariable<double>
{
public:
	AmfNcLongitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension)
		:AmfNcVariable<double>(sU("longitude"), ncFile, dimension, sU("Longitude"), sU("degrees_north"))
	{}
};
class AmfNcLatitudeVariable : public AmfNcVariable<double>
{
public:
	AmfNcLatitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension)
		:AmfNcVariable<double>(sU("latitude"), ncFile, dimension, sU("Latitude"), sU("degrees_east"))
	{}
};

class OutputAmfSeaNcFile : public OutputAmfNcFile
{
public:
	OutputAmfSeaNcFile(const sci::string &directory,
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
		const std::vector<sci::NcDimension *> &nonTimeDimensions = std::vector<sci::NcDimension *>(0));
};
