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
	second samplingInterval;
	second averagingPeriod;
	int processingLevel;
	FeatureType featureType;
	degree minLat;//for point measurements set min and max the same or set one to nan
	degree maxLat;
	degree minLon;
	degree maxLon;
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
const std::vector<sci::string> g_deploymentModeStrings{ sU("air"), sU("land"), sU("sea") };

struct PlatformInfo
{
	sci::string name;
	PlatformType platformType;
	DeploymentMode deploymentMode;
	std::vector <metre> altitudes; //Use nan for varying altitude airborne
	std::vector<sci::string> locationKeywords;
	std::vector<degree> latitudes; //would have just one element for a static platform
	std::vector<degree>longitudes; //would have just one element for a static platform
};

template< class T >
void correctDirection( T measuredX, T measuredY, T measuredZ, unitless sinInstrumentElevation, unitless sinInstrumentAzimuth, unitless sinInstrumentRoll, unitless cosInstrumentElevation, unitless cosInstrumentAzimuth, unitless cosInstrumentRoll, T &correctedX, T &correctedY, T &correctedZ)
{
	//make things a little easier with notation
	unitless cosE = cosInstrumentElevation;
	unitless sinE = sinInstrumentElevation;
	unitless cosR = cosInstrumentRoll;
	unitless sinR = sinInstrumentRoll;
	unitless cosA = cosInstrumentAzimuth;
	unitless sinA = sinInstrumentAzimuth;

	//transform the unit vector into the correct direction
	correctedX = measuredX * (cosR*cosA - sinR * sinE*sinA) + measuredY * (-cosE * sinA) + measuredZ * (sinR*cosA + cosR * sinE*sinA);
	correctedY = measuredX * (cosR*sinA + sinR * sinE*cosA) + measuredY * (cosE * cosA) + measuredZ * (sinR*sinA - cosR * sinE*cosA);
	correctedZ = measuredX * (-sinR * cosE) + measuredY * (sinE)+measuredZ * (cosR*cosE);
}

void correctDirection(degree measuredElevation, degree measuredAzimuth, unitless sinInstrumentElevation, unitless sinInstrumentAzimuth, unitless sinInstrumentRoll, unitless cosInstrumentElevation, unitless cosInstrumentAzimuth, unitless cosInstrumentRoll, degree &correctedElevation, degree &correctedAzimuth);
degree angleBetweenDirections(degree elevation1, degree azimuth1, degree elevation2, degree azimuth2);

class Platform
{
public:
	Platform(sci::string name, PlatformType platformType, DeploymentMode deploymentMode, std::vector<metre> altitudes, std::vector<degree> latitudes, std::vector<degree> longitudes, std::vector<sci::string> locationKeywords)
	{
		m_platformInfo.name = name;
		m_platformInfo.platformType = platformType;
		m_platformInfo.deploymentMode = deploymentMode;
		m_platformInfo.altitudes = altitudes;
		m_platformInfo.latitudes = latitudes;
		m_platformInfo.longitudes = longitudes;
		m_platformInfo.locationKeywords = locationKeywords;
	}

	PlatformInfo getPlatformInfo() const 
	{
		return m_platformInfo;
	};
	virtual ~Platform() {}
	void correctDirection(sci::UtcTime time, degree instrumentRelativeAzimuth, degree instrumentRelativeElevation,  degree &correctedAzimuth, degree &correctedElevation) const
	{
		unitless sinInstrumentElevation;
		unitless sinInstrumentAzimuth;
		unitless sinInstrumentRoll;
		unitless cosInstrumentElevation;
		unitless cosInstrumentAzimuth;
		unitless cosInstrumentRoll;
		getInstrumentTrigAttitudes(time, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll);
		
		::correctDirection(instrumentRelativeElevation, instrumentRelativeAzimuth, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedElevation, correctedAzimuth);
	}
	template<class T>
	void correctVector(sci::UtcTime time, T measuredX, T measuredY, T measuredZ, T &correctedX, T &correctedY, T &correctedZ) const
	{
		unitless sinInstrumentElevation;
		unitless sinInstrumentAzimuth;
		unitless sinInstrumentRoll;
		unitless cosInstrumentElevation;
		unitless cosInstrumentAzimuth;
		unitless cosInstrumentRoll;
		getInstrumentTrigAttitudes(time, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll);

		::correctDirection(measuredX, measuredY, measuredZ, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedX, correctedY, correctedZ);
	}
	virtual void getInstrumentVelocity(sci::UtcTime time, metrePerSecond &eastwardVelocity, metrePerSecond &northwardVelocity, metrePerSecond &upwardVelocity) const = 0;
	virtual void getInstrumentTrigAttitudes(sci::UtcTime time, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const = 0;
private:
	PlatformInfo m_platformInfo;
};

class StationaryPlatform : public Platform
{
public:
	StationaryPlatform(sci::string name, metre altitude, degree latitude, degree longitude, std::vector<sci::string> locationKeywords, degree instrumentElevation, degree instrumentAzimuth, degree instrumentRoll)
		:Platform(name, pt_stationary, dm_land, { altitude }, { latitude }, { longitude }, locationKeywords)
	{
		m_sinInstrumentElevation = sci::sin(instrumentElevation);
		m_sinInstrumentAzimuth = sci::sin(instrumentAzimuth);
		m_sinInstrumentRoll = sci::sin(instrumentRoll);
		m_cosInstrumentElevation = sci::cos(instrumentElevation);
		m_cosInstrumentAzimuth = sci::cos(instrumentAzimuth);
		m_cosInstrumentRoll = sci::cos(instrumentRoll);
	}
	virtual void getInstrumentVelocity(sci::UtcTime time, metrePerSecond &eastwardVelocity, metrePerSecond &northwardVelocity, metrePerSecond &upwardVelocity) const override
	{
		eastwardVelocity = metrePerSecond(0.0);
		northwardVelocity = metrePerSecond(0.0);
		upwardVelocity = metrePerSecond(0.0);
	}
	virtual void getInstrumentTrigAttitudes(sci::UtcTime time, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const override
	{
		sinInstrumentElevation = m_sinInstrumentElevation;
		sinInstrumentAzimuth = m_sinInstrumentAzimuth;
		sinInstrumentRoll = m_sinInstrumentRoll;
		cosInstrumentElevation = m_cosInstrumentElevation;
		cosInstrumentAzimuth = m_cosInstrumentAzimuth;
		cosInstrumentRoll = m_cosInstrumentRoll;
	}
private:
	unitless m_sinInstrumentElevation;
	unitless m_sinInstrumentAzimuth;
	unitless m_sinInstrumentRoll;
	unitless m_cosInstrumentElevation;
	unitless m_cosInstrumentAzimuth;
	unitless m_cosInstrumentRoll;
};

class MovingLevelPlatform : public Platform
{
	MovingLevelPlatform(sci::string name, DeploymentMode deploymentMode, std::vector<metre> altitudes, std::vector<degree> latitudes, std::vector<degree> longitudes, std::vector<sci::string> locationKeywords, degree instrumentElevation, degree instrumentAzimuth, degree instrumentRoll)
		:Platform(name, pt_stationary, deploymentMode, altitudes, latitudes, longitudes, locationKeywords)
	{

	}
};

class ShipRelativePlatform : public Platform
{
public:
	ShipRelativePlatform(sci::string name, metre altitude, const std::vector<sci::UtcTime> &times, const std::vector<degree> &latitudes, const std::vector<degree> &longitudes, const std::vector<sci::string> &locationKeywords, degree instrumentElevationsShipRelative, degree instrumentAzimuthsShipRelative, degree instrumentRollsShipRelative, const std::vector<degree> &shipCourses, const std::vector<metrePerSecond> &shipSpeeds, const std::vector<degree> &shipElevations, const std::vector<degree> &shipAzimuths, const std::vector<degree> &shipRolls)
		:Platform(name, pt_moving, dm_sea, { altitude }, latitudes, longitudes, locationKeywords)
	{
		m_times = times;
		m_instrumentAzimuthsShipRelative = std::vector<degree>(times.size(), instrumentAzimuthsShipRelative);
		m_instrumentElevationsShipRelative = std::vector<degree>(times.size(), instrumentElevationsShipRelative);
		m_instrumentRollsShipRelative = std::vector<degree>(times.size(), instrumentRollsShipRelative);
		m_shipAzimuths = shipAzimuths;
		m_shipCourses = shipCourses;
		m_shipElevations = shipElevations;
		m_shipRolls = shipRolls;
		m_shipSpeeds = shipSpeeds;
	}
private:
	std::vector<sci::UtcTime> m_times;
	std::vector<degree> m_instrumentElevationsAbsolute;
	std::vector<degree> m_instrumentAzimuthsAbsolute;
	std::vector<degree> m_instrumentRollsAbsolute;
	std::vector<degree> m_instrumentElevationsShipRelative;
	std::vector<degree> m_instrumentAzimuthsShipRelative;
	std::vector<degree> m_instrumentRollsShipRelative;
	std::vector<degree> m_shipCourses;
	std::vector<degree> m_shipElevations;
	std::vector<degree> m_shipAzimuths;
	std::vector<degree> m_shipRolls;
	std::vector<metrePerSecond> m_shipSpeeds;
};


class ShipPlatform : public Platform
{
public:
	ShipPlatform(sci::string name, metre altitude, const std::vector<sci::UtcTime> &times, const std::vector<degree> &latitudes, const std::vector<degree> &longitudes, const std::vector<sci::string> &locationKeywords, const std::vector<degree> &instrumentElevationsShipRelative, const std::vector<degree> &instrumentAzimuthsShipRelative, const std::vector<degree> &instrumentRollsShipRelative, const std::vector<degree> &shipCourses, const std::vector<metrePerSecond> &shipSpeeds, const std::vector<degree> &shipElevations, const std::vector<degree> &shipAzimuths, const std::vector<degree> &shipRolls)
		:Platform(name, pt_moving, dm_sea, { altitude }, latitudes, longitudes, locationKeywords)
	{
		m_times = times;
		m_shipAzimuths = shipAzimuths;
		m_shipCourses = shipCourses;
		m_shipSpeeds = shipSpeeds;


		m_instrumentElevationsAbsolute = std::vector<degree>(times.size(), degree(0.0));
		m_instrumentAzimuthsAbsolute = std::vector<degree>(times.size(), degree(0.0));
		m_instrumentRollsAbsolute = std::vector<degree>(times.size(), degree(0.0));

		for (size_t i = 0; i < m_times.size(); ++i)
		{
			//We can calculate the elevation and azimuth of the instrument
			//in absolute terms by taking the elevation and azimuth in ship terms
			//and correcting that for the ship attitude
			::correctDirection(instrumentElevationsShipRelative[i],
				instrumentAzimuthsShipRelative[i],
				sci::sin(shipElevations[i]),
				sci::sin(shipAzimuths[i]),
				sci::sin(shipRolls[i]),
				sci::cos(shipElevations[i]),
				sci::cos(shipAzimuths[i]),
				sci::cos(shipRolls[i]),
				m_instrumentElevationsAbsolute[i],
				m_instrumentAzimuthsAbsolute[i]);
			//roll is a bit more tricky. To calculate roll we find the absolute
			//direction that is up relative to the instrument and convert it to
			//ship coordinates. Then we convert it to absolute coordinates.
			//We then take the absolute elevation and azimuth of the instrument
			//and add 90 degrees to the elevation. This gives the direction of
			//up relative to the instrument if there was no absolute roll.
			//The absolute roll is the angle between these two directions which
			//we find useing the spherical trig cosine rule.
			degree upElevation;
			degree upAzimuth;
			::correctDirection(degree(90.0),
				degree(0.0),
				sci::sin(instrumentElevationsShipRelative[i]),
				sci::sin(instrumentAzimuthsShipRelative[i]),
				sci::sin(instrumentRollsShipRelative[i]),
				sci::cos(instrumentElevationsShipRelative[i]),
				sci::cos(instrumentAzimuthsShipRelative[i]),
				sci::cos(instrumentRollsShipRelative[i]),
				upElevation,
				upAzimuth);
			::correctDirection(upElevation,
				upAzimuth,
				sci::sin(shipElevations[i]),
				sci::sin(shipAzimuths[i]),
				sci::sin(shipRolls[i]),
				sci::cos(shipElevations[i]),
				sci::cos(shipAzimuths[i]),
				sci::cos(shipRolls[i]),
				upElevation,
				upAzimuth);
			degree upElevationNoRoll = m_instrumentElevationsAbsolute[i] + degree(90.0);
			degree upAzimuthNoRoll = m_instrumentAzimuthsAbsolute[i];
			m_instrumentRollsAbsolute[i] = angleBetweenDirections(upElevationNoRoll, upAzimuthNoRoll, upElevation, upAzimuth);
		}
	}
	ShipPlatform(sci::string name, metre altitude, const std::vector<sci::UtcTime> &times, const std::vector<degree> &latitudes, const std::vector<degree> &longitudes, const std::vector<sci::string> &locationKeywords, degree instrumentElevationShipRelative, degree instrumentAzimuthShipRelative, degree instrumentRollShipRelative, const std::vector<degree> &shipCourses, const std::vector<metrePerSecond> &shipSpeeds, const std::vector<degree> &shipElevations, const std::vector<degree> &shipAzimuths, const std::vector<degree> &shipRolls)
		:Platform(name, pt_moving, dm_sea, { altitude }, latitudes, longitudes, locationKeywords)
	{
		m_times = times;
		m_shipAzimuths = shipAzimuths;
		m_shipCourses = shipCourses;
		m_shipSpeeds = shipSpeeds;

		//We can save some calculations by doing these outside of the loop
		unitless sinE = sci::sin(instrumentElevationShipRelative);
		unitless sinA = sci::sin(instrumentAzimuthShipRelative);
		unitless sinR = sci::sin(instrumentRollShipRelative);
		unitless cosE = sci::cos(instrumentElevationShipRelative);
		unitless cosA = sci::cos(instrumentAzimuthShipRelative);
		unitless cosR = sci::cos(instrumentRollShipRelative);

		for (size_t i = 0; i < m_times.size(); ++i)
		{
			//We can calculate the elevation and azimuth of the instrument
			//in absolute terms by taking the elevation and azimuth in ship terms
			//and correcting that for the ship attitude
			::correctDirection(instrumentElevationShipRelative,
				instrumentAzimuthShipRelative,
				sci::sin(shipElevations[i]),
				sci::sin(shipAzimuths[i]),
				sci::sin(shipRolls[i]),
				sci::cos(shipElevations[i]),
				sci::cos(shipAzimuths[i]),
				sci::cos(shipRolls[i]),
				m_instrumentElevationsAbsolute[i],
				m_instrumentAzimuthsAbsolute[i]);
			//roll is a bit more tricky. To calculate roll we find the absolute
			//direction that is up relative to the instrument and convert it to
			//ship coordinates. Then we convert it to absolute coordinates.
			//We then take the absolute elevation and azimuth of the instrument
			//and add 90 degrees to the elevation. This gives the direction of
			//up relative to the instrument if there was no absolute roll.
			//The absolute roll is the angle between these two directions which
			//we find useing the spherical trig cosine rule.
			degree upElevation;
			degree upAzimuth;
			::correctDirection(degree(90.0),
				degree(0.0),
				sinE,
				sinA,
				sinR,
				cosE,
				cosA,
				cosR,
				upElevation,
				upAzimuth);
			::correctDirection(upElevation,
				upAzimuth,
				sci::sin(shipElevations[i]),
				sci::sin(shipAzimuths[i]),
				sci::sin(shipRolls[i]),
				sci::cos(shipElevations[i]),
				sci::cos(shipAzimuths[i]),
				sci::cos(shipRolls[i]),
				upElevation,
				upAzimuth);
			degree upElevationNoRoll = m_instrumentElevationsAbsolute[i] + degree(90.0);
			degree upAzimuthNoRoll = m_instrumentAzimuthsAbsolute[i];
			m_instrumentRollsAbsolute[i] = angleBetweenDirections(upElevationNoRoll, upAzimuthNoRoll, upElevation, upAzimuth);
		}
	}
private:
	std::vector<sci::UtcTime> m_times;
	std::vector<degree> m_instrumentElevationsAbsolute;
	std::vector<degree> m_instrumentAzimuthsAbsolute;
	std::vector<degree> m_instrumentRollsAbsolute;
	std::vector<degree> m_shipCourses;
	std::vector<degree> m_shipAzimuths;
	std::vector<metrePerSecond> m_shipSpeeds;
};

template <class T>
class AmfNcVariable : public sci::NcVariable<T>
{
public:
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &units, T validMin, T validMax, const sci::string &comment = sU(""))
		:NcVariable<T>(name, ncFile, dimension)
	{
		setAttributes(ncFile, longName, units, validMin, validMax, comment);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &units, T validMin, T validMax, const sci::string &comment = sU(""))
		:NcVariable<T>(name, ncFile, dimensions)
	{
		setAttributes(ncFile, longName, units, validMin, validMax, comment);
	}
private:
	void setAttributes(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &units, T validMin, T validMax, const sci::string &comment)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute unitsAttribute(sU("units"), units);
		sci::NcAttribute validMinAttribute(sU("valid_min"), validMin);
		sci::NcAttribute validMaxAttribute(sU("valid_max"), validMax);
		sci::NcAttribute commentAttribute(sU("comment"), comment);
		addAttribute(longNameAttribute, ncFile);
		addAttribute(unitsAttribute, ncFile);
		addAttribute(validMinAttribute, ncFile);
		addAttribute(validMaxAttribute, ncFile);
		if (comment.length() > 0)
			addAttribute(commentAttribute, ncFile);
	}
};

class AmfNcFlagVariable : public sci::NcVariable<uint8_t>
{
public:
	AmfNcFlagVariable(sci::string name, const std::vector<std::pair<uint8_t, sci::string>> &flagDefinitions, const sci::OutputNcFile &ncFile, std::vector<sci::NcDimension *> &dimensions)
		: sci::NcVariable<uint8_t>(name, ncFile, dimensions)
	{
		std::vector<uint8_t>  flagValues(flagDefinitions.size());
		std::vector<sci::string>  flagDescriptions(flagDefinitions.size());
		for (size_t i = 0; i < flagDefinitions.size(); ++i)
		{
			flagValues[i] = flagDefinitions[i].first;
			flagDescriptions[i] = flagDefinitions[i].second;
		}
		addAttribute(sci::NcAttribute(sU("long name"), sU("Data Quality Flag")), ncFile);
		addAttribute(sci::NcAttribute(sU("flag_values"), flagValues), ncFile);
		addAttribute(sci::NcAttribute(sU("flag_meanings"), flagDescriptions), ncFile);
	}
};

//create a partial specialization for any AmfNcVariable based on a sci::Physical value
template <class T>
class AmfNcVariable<sci::Physical<T>> : public sci::NcVariable<sci::Physical<T>>
{
public:
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, sci::Physical<T> validMin, sci::Physical<T> validMax)
		:NcVariable<sci::Physical<T>>(name, ncFile, dimension)
	{
		setAttributes(ncFile, longName, validMin, validMax);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, sci::Physical<T> validMin, sci::Physical<T> validMax)
		:NcVariable<sci::Physical<T>>(name, ncFile, dimensions)
	{
		setAttributes(ncFile, longName, validMin, validMax);
	}
	template <class U>
	static auto convertValues(const std::vector<U> &physicals) -> decltype(sci::physicalsToValues<T>(physicals))
	{
		return sci::physicalsToValues<sci::Physical<T>>(physicals);
	}

private:
	void setAttributes(const sci::OutputNcFile &ncFile, const sci::string &longName, sci::Physical<T> validMin, sci::Physical<T> validMax)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute unitsAttribute(sU("units"), sci::Physical<T>::getShortUnitString());
		sci::NcAttribute validMinAttribute(sU("valid_min"), validMin.value<T>() );
		sci::NcAttribute validMaxAttribute(sU("valid_min"), validMax.value<T>() );
		addAttribute(longNameAttribute, ncFile);
		addAttribute(unitsAttribute, ncFile);
		addAttribute(validMinAttribute, ncFile);
		addAttribute(validMaxAttribute, ncFile);
	}
};

class AmfNcTimeVariable : public AmfNcVariable<double>
{
public:
	AmfNcTimeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension)
		:AmfNcVariable<double>(sU("time"), ncFile, dimension, sU("Time (seconds since 1970-01-01 00:00:00)"), sU("seconds since 1970-01-01 00:00:00"), 0, std::numeric_limits<double>::max())
	{}
	static std::vector<double> convertToSeconds(const std::vector<sci::UtcTime> &times)
	{
		std::vector<double> secondsAfterEpoch(times.size());
		sci::UtcTime epoch(1970, 1, 1, 0, 0, 0.0);
		for (size_t i = 0; i < times.size(); ++i)
			secondsAfterEpoch[i] = times[i] - epoch;
		return secondsAfterEpoch;
	}
};
class AmfNcLongitudeVariable : public AmfNcVariable<double>
{
public:
	AmfNcLongitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension)
		:AmfNcVariable<double>(sU("longitude"), ncFile, dimension, sU("Longitude"), sU("degrees_north"), -360.0, 360.0)
	{}
};
class AmfNcLatitudeVariable : public AmfNcVariable<double>
{
public:
	AmfNcLatitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension)
		:AmfNcVariable<double>(sU("latitude"), ncFile, dimension, sU("Latitude"), sU("degrees_east"), -90.0, 90.0)
	{}
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
		const Platform &platform,
		const sci::string &comment,
		const std::vector<sci::UtcTime> &times,
		degree longitude,
		degree latitude,
		const std::vector<sci::NcDimension *> &nonTimeDimensions= std::vector<sci::NcDimension *>(0));
	sci::NcDimension &getTimeDimension() { return m_timeDimension; }
	void writeTimeAndLocationData();
	static double getFillValue() { return -1e20; }
private:
	sci::NcDimension m_timeDimension;
	std::unique_ptr<AmfNcTimeVariable> m_timeVariable;
	std::unique_ptr<AmfNcLatitudeVariable> m_latitudeVariable;
	std::unique_ptr<AmfNcLongitudeVariable> m_longitudeVariable;
	std::unique_ptr<AmfNcVariable<double>> m_dayOfYearVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_yearVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_monthVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_dayVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_hourVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_minuteVariable;
	std::unique_ptr<AmfNcVariable<double>> m_secondVariable;

	std::vector<sci::UtcTime> m_times;
	degree m_longitude;
	degree m_latitude;
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
