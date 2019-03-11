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
struct ProcessingOptions
{
	sci::string inputDirectory;
	sci::string outputDirectory;
	sci::string comment;
	sci::string reasonForProcessing;
	bool beta;
	sci::UtcTime startTime;
	sci::UtcTime endTime;
	bool startImmediately;
	bool checkForNewFiles;
	bool onlyProcessNewFiles;
	bool generateQuicklooks;
	bool generateNetCdf;
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
	ProcessingOptions processingOptions;
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
	std::vector<sci::UtcTime>times;
	std::vector <metre> altitudes;
	std::vector<sci::string> locationKeywords;
	std::vector<degree> latitudes; //would have just one element for a static platform
	std::vector<degree>longitudes; //would have just one element for a static platform
	std::vector<degree> elevations;
	std::vector<degree> azimuths;
	std::vector<degree> rolls;
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
template<class U>
U interpolate(sci::UtcTime x, sci::UtcTime x1, sci::UtcTime x2, U y1, U y2)
{
	auto m = (y2 - y1) / second(x2 - x1);
	return m * second(x - x1) + y1;
}

class Platform
{
public:
	Platform(sci::string name, PlatformType platformType, DeploymentMode deploymentMode, std::vector<sci::UtcTime> times, std::vector<metre> altitudes, std::vector<degree> latitudes, std::vector<degree> longitudes, std::vector<sci::string> locationKeywords)
	{
		m_previousLowerIndex = 0;
		m_platformInfo.name = name;
		m_platformInfo.platformType = platformType;
		m_platformInfo.deploymentMode = deploymentMode;
		m_platformInfo.times = times;
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
		getInstrumentTrigAttitudesForDirectionCorrection(time, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll);
		
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
		getInstrumentTrigAttitudesForDirectionCorrection(time, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll);

		::correctDirection(measuredX, measuredY, measuredZ, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedX, correctedY, correctedZ);
	}
	virtual void getInstrumentVelocity(sci::UtcTime time, metrePerSecond &eastwardVelocity, metrePerSecond &northwardVelocity, metrePerSecond &upwardVelocity) const = 0;
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime time, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const = 0;
	virtual void getLocation(sci::UtcTime time, degree &latitude, degree &longitude, metre &altitude) const
	{
		size_t lowerIndex = findLowerIndex(time);
		if (lowerIndex > m_platformInfo.times.size() - 2)
		{
			latitude = std::numeric_limits<degree>::quiet_NaN();
			longitude = std::numeric_limits<degree>::quiet_NaN();
			altitude = std::numeric_limits<metre>::quiet_NaN();
			return;
		}
		
		latitude = interpolate(time, m_platformInfo.times[lowerIndex], m_platformInfo.times[lowerIndex + 1], m_platformInfo.latitudes[lowerIndex], m_platformInfo.latitudes[lowerIndex + 1]);
		longitude = interpolate(time, m_platformInfo.times[lowerIndex], m_platformInfo.times[lowerIndex + 1], m_platformInfo.longitudes[lowerIndex], m_platformInfo.longitudes[lowerIndex + 1]);
		altitude = interpolate(time, m_platformInfo.times[lowerIndex], m_platformInfo.times[lowerIndex + 1], m_platformInfo.altitudes[lowerIndex], m_platformInfo.altitudes[lowerIndex + 1]);
	}
	virtual void getMotion(sci::UtcTime time, metrePerSecond &speed, degree &course, degree & azimuth) const = 0;
	virtual void getInstrumentAttitudes(sci::UtcTime time, degree &elevation, degree &azimuth, degree &roll) const = 0;
	size_t findLowerIndex(sci::UtcTime time) const
	{
		if (time > m_platformInfo.times.back() || time < m_platformInfo.times[0])
			return -1;
		size_t result = std::max(size_t(1), m_previousLowerIndex); // we use this to keep track of the last place we found - usually we are iterating forward in time so it is a useful optimisation
		if (m_platformInfo.times[result] > time)
			result = 1;
		for (; result < m_platformInfo.times.size(); ++result)
		{
			if (m_platformInfo.times[result] >= time)
			{
				--result;
				break;
			}
		}
		m_previousLowerIndex = result;
		return result;
	}
private:
	mutable size_t m_previousLowerIndex;
	PlatformInfo m_platformInfo;
};

class StationaryPlatform : public Platform
{
public:
	StationaryPlatform(sci::string name, metre altitude, degree latitude, degree longitude, std::vector<sci::string> locationKeywords, degree instrumentElevation, degree instrumentAzimuth, degree instrumentRoll)
		:Platform(name, pt_stationary, dm_land, { sci::UtcTime(0,1,1,0,0,0) }, { altitude }, { latitude }, { longitude }, locationKeywords)
	{
		m_latitude = latitude;
		m_longitude = longitude;
		m_altitude = altitude;
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
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime time, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const override
	{
		sinInstrumentElevation = m_sinInstrumentElevation;
		sinInstrumentAzimuth = m_sinInstrumentAzimuth;
		sinInstrumentRoll = m_sinInstrumentRoll;
		cosInstrumentElevation = m_cosInstrumentElevation;
		cosInstrumentAzimuth = m_cosInstrumentAzimuth;
		cosInstrumentRoll = m_cosInstrumentRoll;
	}
	virtual void getLocation(sci::UtcTime time, degree &latitude, degree &longitude, metre &altitude) const override
	{
		latitude = m_latitude;
		longitude = m_longitude;
		altitude = m_altitude;
	}
	virtual void getMotion(sci::UtcTime time, metrePerSecond &speed, degree &course, degree & azimuth) const override
	{
		speed = metrePerSecond(0);
		course = std::numeric_limits<degree>::quiet_NaN();
		azimuth = std::numeric_limits<degree>::quiet_NaN();
	}
	virtual void getInstrumentAttitudes(sci::UtcTime time, degree &elevation, degree &azimuth, degree &roll) const override
	{
		elevation = m_elevation;
		azimuth = m_azimuth;
		roll = m_roll;
	}
private:
	degree m_latitude;
	degree m_longitude;
	metre m_altitude;
	degree m_elevation;
	degree m_azimuth;
	degree m_roll;
	unitless m_sinInstrumentElevation;
	unitless m_sinInstrumentAzimuth;
	unitless m_sinInstrumentRoll;
	unitless m_cosInstrumentElevation;
	unitless m_cosInstrumentAzimuth;
	unitless m_cosInstrumentRoll;
};

//class MovingLevelPlatform : public Platform
//{
//	MovingLevelPlatform(sci::string name, DeploymentMode deploymentMode, std::vector<metre> altitudes, std::vector<degree> latitudes, std::vector<degree> longitudes, std::vector<sci::string> locationKeywords, degree instrumentElevation, degree instrumentAzimuth, degree instrumentRoll)
//		:Platform(name, pt_stationary, deploymentMode, altitudes, latitudes, longitudes, locationKeywords)
//	{
//
//	}
//};

//class ShipRelativePlatform : public Platform
//{
//public:
//	ShipRelativePlatform(sci::string name, metre altitude, const std::vector<sci::UtcTime> &times, const std::vector<degree> &latitudes, const std::vector<degree> &longitudes, const std::vector<sci::string> &locationKeywords, degree instrumentElevationsShipRelative, degree instrumentAzimuthsShipRelative, degree instrumentRollsShipRelative, const std::vector<degree> &shipCourses, const std::vector<metrePerSecond> &shipSpeeds, const std::vector<degree> &shipElevations, const std::vector<degree> &shipAzimuths, const std::vector<degree> &shipRolls)
//		:Platform(name, pt_moving, dm_sea, { altitude }, latitudes, longitudes, locationKeywords)
//	{
//		m_times = times;
//		m_instrumentAzimuthsShipRelative = std::vector<degree>(times.size(), instrumentAzimuthsShipRelative);
//		m_instrumentElevationsShipRelative = std::vector<degree>(times.size(), instrumentElevationsShipRelative);
//		m_instrumentRollsShipRelative = std::vector<degree>(times.size(), instrumentRollsShipRelative);
//		m_shipAzimuths = shipAzimuths;
//		m_shipCourses = shipCourses;
//		m_shipElevations = shipElevations;
//		m_shipRolls = shipRolls;
//		m_shipSpeeds = shipSpeeds;
//	}
//private:
//	std::vector<sci::UtcTime> m_times;
//	std::vector<degree> m_instrumentElevationsAbsolute;
//	std::vector<degree> m_instrumentAzimuthsAbsolute;
//	std::vector<degree> m_instrumentRollsAbsolute;
//	std::vector<degree> m_instrumentElevationsShipRelative;
//	std::vector<degree> m_instrumentAzimuthsShipRelative;
//	std::vector<degree> m_instrumentRollsShipRelative;
//	std::vector<degree> m_shipCourses;
//	std::vector<degree> m_shipElevations;
//	std::vector<degree> m_shipAzimuths;
//	std::vector<degree> m_shipRolls;
//	std::vector<metrePerSecond> m_shipSpeeds;
//};


class ShipPlatform : public Platform
{
public:
	ShipPlatform(sci::string name, metre altitude, const std::vector<sci::UtcTime> &times, const std::vector<degree> &latitudes, const std::vector<degree> &longitudes, const std::vector<sci::string> &locationKeywords, const std::vector<degree> &instrumentElevationsShipRelative, const std::vector<degree> &instrumentAzimuthsShipRelative, const std::vector<degree> &instrumentRollsShipRelative, const std::vector<degree> &shipCourses, const std::vector<metrePerSecond> &shipSpeeds, const std::vector<degree> &shipElevations, const std::vector<degree> &shipAzimuths, const std::vector<degree> &shipRolls)
		:Platform(name, pt_moving, dm_sea, times, { altitude }, latitudes, longitudes, locationKeywords)
	{
		m_times = times;
		m_shipAzimuths = shipAzimuths;
		m_shipCourses = shipCourses;
		m_shipSpeeds = shipSpeeds;


		m_instrumentElevationsAbsolute = std::vector<degree>(times.size(), degree(0.0));
		m_instrumentAzimuthsAbsolute = std::vector<degree>(times.size(), degree(0.0));
		m_instrumentRollsAbsolute = std::vector<degree>(times.size(), degree(0.0));
		m_sinInstrumentElevationsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_sinInstrumentAzimuthsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_sinInstrumentRollsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_cosInstrumentElevationsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_cosInstrumentAzimuthsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_cosInstrumentRollsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));

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

			m_sinInstrumentElevationsAbsolute[i] = sci::sin(m_instrumentElevationsAbsolute[i]);
			m_sinInstrumentAzimuthsAbsolute[i] = sci::sin(m_instrumentAzimuthsAbsolute[i]);
			m_sinInstrumentRollsAbsolute[i] = sci::sin(m_instrumentRollsAbsolute[i]);
			m_cosInstrumentElevationsAbsolute[i] = sci::cos(m_instrumentElevationsAbsolute[i]);
			m_cosInstrumentAzimuthsAbsolute[i] = sci::cos(m_instrumentAzimuthsAbsolute[i]);
			m_cosInstrumentRollsAbsolute[i] = sci::cos(m_instrumentRollsAbsolute[i]);
		}
	}
	ShipPlatform(sci::string name, metre altitude, const std::vector<sci::UtcTime> &times, const std::vector<degree> &latitudes, const std::vector<degree> &longitudes, const std::vector<sci::string> &locationKeywords, degree instrumentElevationShipRelative, degree instrumentAzimuthShipRelative, degree instrumentRollShipRelative, const std::vector<degree> &shipCourses, const std::vector<metrePerSecond> &shipSpeeds, const std::vector<degree> &shipElevations, const std::vector<degree> &shipAzimuths, const std::vector<degree> &shipRolls)
		:Platform(name, pt_moving, dm_sea, times, { altitude }, latitudes, longitudes, locationKeywords)
	{
		m_times = times;
		m_shipAzimuths = shipAzimuths;
		m_shipCourses = shipCourses;
		m_shipSpeeds = shipSpeeds;

		m_instrumentElevationsAbsolute = std::vector<degree>(times.size(), degree(0.0));
		m_instrumentAzimuthsAbsolute = std::vector<degree>(times.size(), degree(0.0));
		m_instrumentRollsAbsolute = std::vector<degree>(times.size(), degree(0.0));
		m_sinInstrumentElevationsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_sinInstrumentAzimuthsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_sinInstrumentRollsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_cosInstrumentElevationsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_cosInstrumentAzimuthsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));
		m_cosInstrumentRollsAbsolute = std::vector<unitless>(times.size(), unitless(0.0));

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

			m_sinInstrumentElevationsAbsolute[i] = sci::sin(m_instrumentElevationsAbsolute[i]);
			m_sinInstrumentAzimuthsAbsolute[i] = sci::sin(m_instrumentAzimuthsAbsolute[i]);
			m_sinInstrumentRollsAbsolute[i] = sci::sin(m_instrumentRollsAbsolute[i]);
			m_cosInstrumentElevationsAbsolute[i] = sci::cos(m_instrumentElevationsAbsolute[i]);
			m_cosInstrumentAzimuthsAbsolute[i] = sci::cos(m_instrumentAzimuthsAbsolute[i]);
			m_cosInstrumentRollsAbsolute[i] = sci::cos(m_instrumentRollsAbsolute[i]);
		}
	}
	virtual void getLocation(sci::UtcTime time, degree &latitude, degree &longitude, metre &altitude) const
	{
		size_t lowerIndex = findLowerIndex(time);
		if (lowerIndex > getPlatformInfo().times.size() - 2)
		{
			latitude = std::numeric_limits<degree>::quiet_NaN();
			longitude = std::numeric_limits<degree>::quiet_NaN();
			altitude = std::numeric_limits<metre>::quiet_NaN();
			return;
		}

		latitude = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], getPlatformInfo().latitudes[lowerIndex], getPlatformInfo().latitudes[lowerIndex + 1]);
		longitude = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], getPlatformInfo().longitudes[lowerIndex], getPlatformInfo().longitudes[lowerIndex + 1]);
		altitude = getPlatformInfo().altitudes[0];
	}
	virtual void getInstrumentVelocity(sci::UtcTime time, metrePerSecond &eastwardVelocity, metrePerSecond &northwardVelocity, metrePerSecond &upwardVelocity) const override
	{
		upwardVelocity = metrePerSecond(0.0);

		size_t lowerIndex = findLowerIndex(time);
		if (lowerIndex > getPlatformInfo().times.size() - 2)
		{
			eastwardVelocity = std::numeric_limits<metrePerSecond>::quiet_NaN();
			northwardVelocity = std::numeric_limits<metrePerSecond>::quiet_NaN();
			return;
		}

		metrePerSecond speed = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_shipSpeeds[lowerIndex], m_shipSpeeds[lowerIndex + 1]);
		degree course = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_shipCourses[lowerIndex], m_shipCourses[lowerIndex + 1]);

		northwardVelocity = sci::cos(course)*speed;
		eastwardVelocity = sci::sin(course)*speed;

	}
	virtual void getMotion(sci::UtcTime time, metrePerSecond &speed, degree &course, degree & azimuth) const override
	{
		size_t lowerIndex = findLowerIndex(time);
		if (lowerIndex > getPlatformInfo().times.size() - 2)
		{
			speed = std::numeric_limits<metrePerSecond>::quiet_NaN();
			course = std::numeric_limits<degree>::quiet_NaN();
			azimuth = std::numeric_limits<degree>::quiet_NaN();
			return;
		}

		speed = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_shipSpeeds[lowerIndex], m_shipSpeeds[lowerIndex + 1]);
		course = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_shipCourses[lowerIndex], m_shipCourses[lowerIndex + 1]);
		azimuth = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_shipAzimuths[lowerIndex], m_shipAzimuths[lowerIndex + 1]);
	}
	virtual void getInstrumentAttitudes(sci::UtcTime time, degree &elevation, degree &azimuth, degree &roll) const override
	{
		size_t lowerIndex = findLowerIndex(time);
		if (lowerIndex > getPlatformInfo().times.size() - 2)
		{
			elevation = std::numeric_limits<degree>::quiet_NaN();
			azimuth = std::numeric_limits<degree>::quiet_NaN();
			roll = std::numeric_limits<degree>::quiet_NaN();
			return;
		}

		elevation = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_instrumentElevationsAbsolute[lowerIndex], m_instrumentElevationsAbsolute[lowerIndex + 1]);
		azimuth = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_instrumentAzimuthsAbsolute[lowerIndex], m_instrumentAzimuthsAbsolute[lowerIndex + 1]);
		roll = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_instrumentRollsAbsolute[lowerIndex], m_instrumentRollsAbsolute[lowerIndex + 1]);
	}
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime time, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const override
	{
		size_t lowerIndex = findLowerIndex(time);
		if (lowerIndex > getPlatformInfo().times.size() - 2)
		{
			sinInstrumentElevation = std::numeric_limits<unitless>::quiet_NaN();
			sinInstrumentAzimuth = std::numeric_limits<unitless>::quiet_NaN();
			sinInstrumentRoll = std::numeric_limits<unitless>::quiet_NaN();
			cosInstrumentElevation = std::numeric_limits<unitless>::quiet_NaN();
			cosInstrumentAzimuth = std::numeric_limits<unitless>::quiet_NaN();
			cosInstrumentRoll = std::numeric_limits<unitless>::quiet_NaN();
			return;
		}
		sinInstrumentElevation = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_sinInstrumentElevationsAbsolute[lowerIndex], m_sinInstrumentElevationsAbsolute[lowerIndex + 1]);
		sinInstrumentAzimuth = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_sinInstrumentAzimuthsAbsolute[lowerIndex], m_sinInstrumentAzimuthsAbsolute[lowerIndex + 1]);
		sinInstrumentRoll = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_sinInstrumentRollsAbsolute[lowerIndex], m_sinInstrumentRollsAbsolute[lowerIndex + 1]);
		cosInstrumentElevation = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_cosInstrumentElevationsAbsolute[lowerIndex], m_cosInstrumentElevationsAbsolute[lowerIndex + 1]);
		cosInstrumentAzimuth = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_cosInstrumentAzimuthsAbsolute[lowerIndex], m_cosInstrumentAzimuthsAbsolute[lowerIndex + 1]);
		cosInstrumentRoll = interpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_cosInstrumentRollsAbsolute[lowerIndex], m_cosInstrumentRollsAbsolute[lowerIndex + 1]);

	}
private:
	std::vector<sci::UtcTime> m_times;
	std::vector<degree> m_instrumentElevationsAbsolute;
	std::vector<degree> m_instrumentAzimuthsAbsolute;
	std::vector<degree> m_instrumentRollsAbsolute;
	std::vector<degree> m_shipCourses;
	std::vector<degree> m_shipAzimuths;
	std::vector<metrePerSecond> m_shipSpeeds;

	std::vector<unitless> m_sinInstrumentElevationsAbsolute;
	std::vector<unitless> m_sinInstrumentAzimuthsAbsolute;
	std::vector<unitless> m_sinInstrumentRollsAbsolute;
	std::vector<unitless> m_cosInstrumentElevationsAbsolute;
	std::vector<unitless> m_cosInstrumentAzimuthsAbsolute;
	std::vector<unitless> m_cosInstrumentRollsAbsolute;
};

class ShipPlatformShipRelativeCorrected : public ShipPlatform
{
public:
	ShipPlatformShipRelativeCorrected(sci::string name, metre altitude, const std::vector<sci::UtcTime> &times, const std::vector<degree> &latitudes, const std::vector<degree> &longitudes, const std::vector<sci::string> &locationKeywords, degree instrumentElevationShipRelative, degree instrumentAzimuthShipRelative, degree instrumentRollShipRelative, const std::vector<degree> &shipCourses, const std::vector<metrePerSecond> &shipSpeeds, const std::vector<degree> &shipElevations, const std::vector<degree> &shipAzimuths, const std::vector<degree> &shipRolls)
		: ShipPlatform(name, altitude, times, latitudes, longitudes, locationKeywords, instrumentElevationShipRelative, instrumentAzimuthShipRelative, instrumentRollShipRelative, shipCourses, shipSpeeds, shipElevations, shipAzimuths, shipRolls)
	{
		m_sinInstrumentElevation = sci::sin(instrumentElevationShipRelative);
		m_sinInstrumentAzimuth = sci::sin(instrumentAzimuthShipRelative);
		m_sinInstrumentRoll = sci::sin(instrumentRollShipRelative);
		m_cosInstrumentElevation = sci::cos(instrumentElevationShipRelative);
		m_cosInstrumentAzimuth = sci::cos(instrumentAzimuthShipRelative);
		m_cosInstrumentRoll = sci::cos(instrumentRollShipRelative);
	}
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime time, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const override
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

template <class T>
class AmfNcVariable : public sci::NcVariable<T>
{
public:
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &standardName, const sci::string &units, T validMin, T validMax, const sci::string &comment = sU(""))
		:NcVariable<T>(name, ncFile, dimension)
	{
		setAttributes(ncFile, longName, standardName, units, validMin, validMax, comment);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &standardName, const sci::string &units, T validMin, T validMax, const sci::string &comment = sU(""))
		:NcVariable<T>(name, ncFile, dimensions)
	{
		setAttributes(ncFile, longName, standardName, units, validMin, validMax, comment);
	}
private:
	void setAttributes(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &standardName, const sci::string &units, T validMin, T validMax, const sci::string &comment)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
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
		if (standardName.length() > 0)
			addAttribute(standardNameAttribute, ncFile);
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
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &standardName, sci::Physical<T> validMin, sci::Physical<T> validMax, const sci::string &comment = sU(""))
		:NcVariable<sci::Physical<T>>(name, ncFile, dimension)
	{
		setAttributes(ncFile, longName, standardName, validMin, validMax, comment);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &standardName, sci::Physical<T> validMin, sci::Physical<T> validMax, const sci::string &comment = sU(""))
		:NcVariable<sci::Physical<T>>(name, ncFile, dimensions)
	{
		setAttributes(ncFile, longName, standardName, validMin, validMax, comment);
	}
	template <class U>
	static auto convertValues(const std::vector<U> &physicals) -> decltype(sci::physicalsToValues<T>(physicals))
	{
		return sci::physicalsToValues<sci::Physical<T>>(physicals);
	}

private:
	void setAttributes(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &standardName, sci::Physical<T> validMin, sci::Physical<T> validMax, const sci::string &comment)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
		sci::NcAttribute validMinAttribute(sU("valid_min"), validMin.value<T>());
		sci::NcAttribute validMaxAttribute(sU("valid_max"), validMax.value<T>());
		sci::NcAttribute commentAttribute(sU("comment"), comment);
		addAttribute(longNameAttribute, ncFile);
		addAttribute(validMinAttribute, ncFile);
		addAttribute(validMaxAttribute, ncFile);
		if (comment.length() > 0)
			addAttribute(commentAttribute, ncFile);
		if (standardName.length() > 0)
			addAttribute(standardNameAttribute, ncFile);
	}
};

class AmfNcTimeVariable : public AmfNcVariable<double>
{
public:
	AmfNcTimeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension)
		:AmfNcVariable<double>(sU("time"), ncFile, dimension, sU("Time (seconds since 1970-01-01 00:00:00)"), sU("time"), sU("seconds since 1970-01-01 00:00:00"), 0, std::numeric_limits<double>::max())
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("T")), ncFile);
		addAttribute(sci::NcAttribute(sU("calendar"), sU("standard")), ncFile);
	}
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
		:AmfNcVariable<double>(sU("longitude"), ncFile, dimension, sU("Longitude"), sU("longitude"), sU("degrees_north"), -360.0, 360.0)
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("X")), ncFile);
	}
};
class AmfNcLatitudeVariable : public AmfNcVariable<double>
{
public:
	AmfNcLatitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension)
		:AmfNcVariable<double>(sU("latitude"), ncFile, dimension, sU("Latitude"), sU("latitude"), sU("degrees_east"), -90.0, 90.0)
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("Y")), ncFile);
	}
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
		const std::vector<sci::UtcTime> &times,
		const std::vector<sci::NcDimension *> &nonTimeDimensions= std::vector<sci::NcDimension *>(0));
	sci::NcDimension &getTimeDimension() { return m_timeDimension; }
	void writeTimeAndLocationData( const Platform &platform );
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

	//only used for moving platforms
	std::unique_ptr<AmfNcVariable<degree>> m_courseVariable;
	std::unique_ptr<AmfNcVariable<degree>> m_orientationVariable;
	std::unique_ptr<AmfNcVariable<metrePerSecond>> m_speedVariable;
	std::unique_ptr<AmfNcVariable<degree>> m_instrumentPitchVariable;
	std::unique_ptr<AmfNcVariable<degree>> m_instrumentRollVariable;
	std::unique_ptr<AmfNcVariable<degree>> m_instrumentYawVariable;

	std::vector<sci::UtcTime> m_times;
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
