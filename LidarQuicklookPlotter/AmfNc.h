#pragma once
#include<svector/time.h>
#include<vector>
#include<svector/sreadwrite.h>
#include<string>
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
	/*degree minLat;//for point measurements set min or max to nan
	degree maxLat;
	degree minLon;
	degree maxLon;*/
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
	std::vector<sci::string> locationKeywords;
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

struct AttitudeAverage
{
	AttitudeAverage(degree mean, degree min, degree max, degree stdev)
		:m_mean(mean), m_min(min), m_max(max), m_stdev(stdev)
	{}
	degree m_mean;
	degree m_min;
	degree m_max;
	degree m_stdev;
};

class Platform
{
public:
	Platform(sci::string name, PlatformType platformType, DeploymentMode deploymentMode, std::vector<sci::string> locationKeywords)
	{
		m_platformInfo.name = name;
		m_platformInfo.platformType = platformType;
		m_platformInfo.deploymentMode = deploymentMode;
		m_platformInfo.locationKeywords = locationKeywords;
	}

	PlatformInfo getPlatformInfo() const 
	{
		return m_platformInfo;
	};
	virtual ~Platform() {}
	void correctDirection(sci::UtcTime startTime, sci::UtcTime endTime, degree instrumentRelativeAzimuth, degree instrumentRelativeElevation,  degree &correctedAzimuth, degree &correctedElevation) const
	{
		unitless sinInstrumentElevation;
		unitless sinInstrumentAzimuth;
		unitless sinInstrumentRoll;
		unitless cosInstrumentElevation;
		unitless cosInstrumentAzimuth;
		unitless cosInstrumentRoll;
		getInstrumentTrigAttitudesForDirectionCorrection(startTime, endTime, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll);
		
		::correctDirection(instrumentRelativeElevation, instrumentRelativeAzimuth, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedElevation, correctedAzimuth);
	}
	template<class T>
	void correctVector(sci::UtcTime startTime, sci::UtcTime endTime, T measuredX, T measuredY, T measuredZ, T &correctedX, T &correctedY, T &correctedZ) const
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
	virtual void getInstrumentVelocity(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecond &eastwardVelocity, metrePerSecond &northwardVelocity, metrePerSecond &upwardVelocity) const = 0;
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime startTime, sci::UtcTime endTime, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const = 0;
	virtual void getLocation(sci::UtcTime startTime, sci::UtcTime endTime, degree &latitude, degree &longitude, metre &altitude) const = 0;
	//virtual void getMotion(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecond &speed, degree &course, degree & azimuth) const = 0;
	virtual void getInstrumentAttitudes(sci::UtcTime startTime, sci::UtcTime endTime, AttitudeAverage &elevation, AttitudeAverage &azimuth, AttitudeAverage &roll) const = 0;
	virtual bool getFixedAltitude() const = 0;
private:
	PlatformInfo m_platformInfo;
};

class StationaryPlatform : public Platform
{
public:
	StationaryPlatform(sci::string name, metre altitude, degree latitude, degree longitude, std::vector<sci::string> locationKeywords, degree instrumentElevation, degree instrumentAzimuth, degree instrumentRoll)
		:Platform(name, pt_stationary, dm_land, locationKeywords)
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
	virtual void getInstrumentVelocity(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecond &eastwardVelocity, metrePerSecond &northwardVelocity, metrePerSecond &upwardVelocity) const override
	{
		eastwardVelocity = metrePerSecond(0.0);
		northwardVelocity = metrePerSecond(0.0);
		upwardVelocity = metrePerSecond(0.0);
	}
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime startTime, sci::UtcTime endTime, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const override
	{
		sinInstrumentElevation = m_sinInstrumentElevation;
		sinInstrumentAzimuth = m_sinInstrumentAzimuth;
		sinInstrumentRoll = m_sinInstrumentRoll;
		cosInstrumentElevation = m_cosInstrumentElevation;
		cosInstrumentAzimuth = m_cosInstrumentAzimuth;
		cosInstrumentRoll = m_cosInstrumentRoll;
	}
	virtual void getLocation(sci::UtcTime startTime, sci::UtcTime endTime, degree &latitude, degree &longitude, metre &altitude) const override
	{
		latitude = m_latitude;
		longitude = m_longitude;
		altitude = m_altitude;
	}
	/*virtual void getMotion(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecond &speed, degree &course, degree & azimuth) const override
	{
		speed = metrePerSecond(0);
		course = std::numeric_limits<degree>::quiet_NaN();
		azimuth = std::numeric_limits<degree>::quiet_NaN();
	}*/
	virtual void getInstrumentAttitudes(sci::UtcTime startTime, sci::UtcTime endTime, AttitudeAverage &elevation, AttitudeAverage &azimuth, AttitudeAverage &roll) const override
	{
		elevation = AttitudeAverage(m_elevation, m_elevation, m_elevation, degree(0));
		azimuth = AttitudeAverage(m_azimuth, m_azimuth, m_azimuth, degree(0));
		roll = AttitudeAverage(m_roll, m_roll, m_roll, degree(0));
	}
	virtual bool getFixedAltitude() const override
	{
		return true;
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
		:Platform(name, pt_moving, dm_sea, locationKeywords)
	{
		m_previousLowerIndexEndTime = 0;
		m_previousLowerIndexStartTime = 0;

		m_times = times;
		m_altitude = altitude;
		m_latitudes = latitudes;
		m_longitudes = longitudes;
		m_shipAzimuths = shipAzimuths;
		m_shipCourses = shipCourses;
		m_shipSpeeds = shipSpeeds;

		m_u.resize(shipSpeeds.size());
		m_v.resize(shipSpeeds.size());
		for (size_t i = 0; i < shipSpeeds.size(); ++i)
		{
			m_u[i] = shipSpeeds[i] * sci::sin(shipCourses[i]);
			m_v[i] = shipSpeeds[i] * sci::cos(shipCourses[i]);
		}


		//set up a series of azimuths that do not jump at the 0/360 point
		m_shipAzimuthsNoJumps = shipAzimuths;
		for (size_t i = 1; i < m_shipAzimuthsNoJumps.size(); ++i)
		{
			while (m_shipAzimuthsNoJumps[i] - m_shipAzimuthsNoJumps[i - 1] > degree(180))
			{
				m_shipAzimuthsNoJumps[i] -= degree(360);
			}
			while (m_shipAzimuthsNoJumps[i] - m_shipAzimuthsNoJumps[i - 1] < degree(-180))
			{
				m_shipAzimuthsNoJumps[i] += degree(360);
			}
		}

		//set up a series of courses that do not jump at the 0/360 point
		m_shipCoursesNoJumps = shipCourses;
		for (size_t i = 1; i < m_shipCoursesNoJumps.size(); ++i)
		{
			while (m_shipCoursesNoJumps[i] - m_shipCoursesNoJumps[i - 1] > degree(180))
			{
				m_shipCoursesNoJumps[i] -= degree(360);
			}
			while (m_shipCoursesNoJumps[i] - m_shipCoursesNoJumps[i - 1] < degree(-180))
			{
				m_shipCoursesNoJumps[i] += degree(360);
			}
		}


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

		//set up a series of instrument azimuths that do not jump at the 0/360 point
		m_instrumentAzimuthsAbsoluteNoJumps = shipAzimuths;
		for (size_t i = 1; i < m_instrumentAzimuthsAbsoluteNoJumps.size(); ++i)
		{
			while (m_instrumentAzimuthsAbsoluteNoJumps[i] - m_instrumentAzimuthsAbsoluteNoJumps[i - 1] > degree(180))
			{
				m_instrumentAzimuthsAbsoluteNoJumps[i] -= degree(360);
			}
			while (m_instrumentAzimuthsAbsoluteNoJumps[i] - m_instrumentAzimuthsAbsoluteNoJumps[i - 1] < degree(-180))
			{
				m_instrumentAzimuthsAbsoluteNoJumps[i] += degree(360);
			}
		}
	}
	ShipPlatform(sci::string name, metre altitude, const std::vector<sci::UtcTime> &times, const std::vector<degree> &latitudes, const std::vector<degree> &longitudes, const std::vector<sci::string> &locationKeywords, degree instrumentElevationShipRelative, degree instrumentAzimuthShipRelative, degree instrumentRollShipRelative, const std::vector<degree> &shipCourses, const std::vector<metrePerSecond> &shipSpeeds, const std::vector<degree> &shipElevations, const std::vector<degree> &shipAzimuths, const std::vector<degree> &shipRolls)
		:Platform(name, pt_moving, dm_sea, locationKeywords)
	{
		m_previousLowerIndexEndTime = 0;
		m_previousLowerIndexStartTime = 0;

		m_times = times;
		m_altitude = altitude;
		m_latitudes = latitudes;
		m_longitudes = longitudes;
		m_shipAzimuths = shipAzimuths;
		m_shipCourses = shipCourses;
		m_shipSpeeds = shipSpeeds;

		m_u.resize(shipSpeeds.size());
		m_v.resize(shipSpeeds.size());
		for (size_t i = 0; i < shipSpeeds.size(); ++i)
		{
			m_u[i] = shipSpeeds[i] * sci::sin(shipCourses[i]);
			m_v[i] = shipSpeeds[i] * sci::cos(shipCourses[i]);
		}


		//set up a series of azimuths that do not jump at the 0/360 point
		m_shipAzimuthsNoJumps = shipAzimuths;
		for (size_t i = 1; i < m_shipAzimuthsNoJumps.size(); ++i)
		{
			while (m_shipAzimuthsNoJumps[i] - m_shipAzimuthsNoJumps[i - 1] > degree(180))
			{
				m_shipAzimuthsNoJumps[i] -= degree(360);
			}
			while (m_shipAzimuthsNoJumps[i] - m_shipAzimuthsNoJumps[i - 1] < degree(-180))
			{
				m_shipAzimuthsNoJumps[i] += degree(360);
			}
		}

		//set up a series of courses that do not jump at the 0/360 point
		m_shipCoursesNoJumps = shipCourses;
		for (size_t i = 1; i < m_shipCoursesNoJumps.size(); ++i)
		{
			while (m_shipCoursesNoJumps[i] - m_shipCoursesNoJumps[i - 1] > degree(180))
			{
				m_shipCoursesNoJumps[i] -= degree(360);
			}
			while (m_shipCoursesNoJumps[i] - m_shipCoursesNoJumps[i - 1] < degree(-180))
			{
				m_shipCoursesNoJumps[i] += degree(360);
			}
		}

		//set up a series of longitudes that do not jump at the 0/360 point
		m_longitudesNoJumps = longitudes;
		for (size_t i = 1; i < m_longitudesNoJumps.size(); ++i)
		{
			while (m_longitudesNoJumps[i] - m_longitudesNoJumps[i - 1] > degree(180))
			{
				m_longitudesNoJumps[i] -= degree(360);
			}
			while (m_longitudesNoJumps[i] - m_longitudesNoJumps[i - 1] < degree(-180))
			{
				m_longitudesNoJumps[i] += degree(360);
			}
		}

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

		//set up a series of instrument azimuths that do not jump at the 0/360 point
		m_instrumentAzimuthsAbsoluteNoJumps = shipAzimuths;
		for (size_t i = 1; i < m_instrumentAzimuthsAbsoluteNoJumps.size(); ++i)
		{
			while (m_instrumentAzimuthsAbsoluteNoJumps[i] - m_instrumentAzimuthsAbsoluteNoJumps[i - 1] > degree(180))
			{
				m_instrumentAzimuthsAbsoluteNoJumps[i] -= degree(360);
			}
			while (m_instrumentAzimuthsAbsoluteNoJumps[i] - m_instrumentAzimuthsAbsoluteNoJumps[i - 1] < degree(-180))
			{
				m_instrumentAzimuthsAbsoluteNoJumps[i] += degree(360);
			}
		}
	}
	virtual void getLocation(sci::UtcTime startTime, sci::UtcTime endTime, degree &latitude, degree &longitude, metre &altitude) const
	{
		latitude = findMean(startTime, endTime, m_latitudes);
		longitude = findMean(startTime, endTime, m_longitudesNoJumps);
		longitude -= sci::floor(longitude / degree(360))*degree(360);
		if (longitude > degree(180))
			longitude -= degree(360);
		altitude = m_altitude;
	}
	virtual void getInstrumentVelocity(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecond &eastwardVelocity, metrePerSecond &northwardVelocity, metrePerSecond &upwardVelocity) const override
	{
		upwardVelocity = metrePerSecond(0.0);
		eastwardVelocity = findMean(startTime, endTime, m_u);
		northwardVelocity = findMean(startTime, endTime, m_v);
	}
	/*virtual void getMotion(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecond &speed, degree &course, degree & azimuth) const override
	{
		size_t lowerIndex = findLowerIndex(time);
		if (lowerIndex > getPlatformInfo().times.size() - 2)
		{
			speed = std::numeric_limits<metrePerSecond>::quiet_NaN();
			course = std::numeric_limits<degree>::quiet_NaN();
			azimuth = std::numeric_limits<degree>::quiet_NaN();
			return;
		}

		speed = sci::linearinterpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_shipSpeeds[lowerIndex], m_shipSpeeds[lowerIndex + 1]);
		course = sci::linearinterpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_shipCourses[lowerIndex], m_shipCourses[lowerIndex + 1]);
		azimuth = sci::linearinterpolate(time, getPlatformInfo().times[lowerIndex], getPlatformInfo().times[lowerIndex + 1], m_shipAzimuths[lowerIndex], m_shipAzimuths[lowerIndex + 1]);
	}*/
	virtual void getInstrumentAttitudes(sci::UtcTime startTime, sci::UtcTime endTime, AttitudeAverage &elevation, AttitudeAverage &azimuth, AttitudeAverage &roll) const override
	{
		findStatistics(startTime, endTime, m_instrumentElevationsAbsolute, elevation.m_mean, elevation.m_stdev, elevation.m_min, elevation.m_max);
		findStatistics(startTime, endTime, m_instrumentAzimuthsAbsoluteNoJumps, azimuth.m_mean, azimuth.m_stdev, azimuth.m_min, azimuth.m_max);
		findStatistics(startTime, endTime, m_instrumentRollsAbsolute, roll.m_mean, roll.m_stdev, roll.m_min, roll.m_max);
		degree azimuthOffset = sci::floor(azimuth.m_mean / degree(360))*degree(360);
		if (azimuth.m_mean - azimuthOffset > degree(180))
			azimuthOffset += degree(180);
		azimuth.m_mean -= azimuthOffset;
		azimuth.m_max -= azimuthOffset;
		azimuth.m_min -= azimuthOffset;
	}
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime startTime, sci::UtcTime endTime, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const override
	{
		sinInstrumentElevation = findMean(startTime, endTime, m_sinInstrumentElevationsAbsolute);
		sinInstrumentAzimuth = findMean(startTime, endTime, m_sinInstrumentAzimuthsAbsolute);
		sinInstrumentRoll = findMean(startTime, endTime, m_sinInstrumentRollsAbsolute);
		cosInstrumentElevation = findMean(startTime, endTime, m_cosInstrumentElevationsAbsolute);
		cosInstrumentAzimuth = findMean(startTime, endTime, m_cosInstrumentAzimuthsAbsolute);
		cosInstrumentRoll = findMean(startTime, endTime, m_cosInstrumentRollsAbsolute);
	}
	virtual bool getFixedAltitude() const override
	{
		return true;
	}
private:
	std::vector<sci::UtcTime> m_times;
	metre m_altitude;
	std::vector<degree> m_latitudes;
	std::vector<degree> m_longitudes;
	std::vector<degree> m_longitudesNoJumps;
	std::vector<degree> m_instrumentElevationsAbsolute;
	std::vector<degree> m_instrumentAzimuthsAbsolute;
	std::vector<degree> m_instrumentAzimuthsAbsoluteNoJumps;
	std::vector<degree> m_instrumentRollsAbsolute;
	std::vector<degree> m_shipCourses;
	std::vector<degree> m_shipAzimuths;
	std::vector<degree> m_shipCoursesNoJumps;
	std::vector<degree> m_shipAzimuthsNoJumps;
	std::vector<metrePerSecond> m_shipSpeeds;
	std::vector<metrePerSecond> m_u;
	std::vector<metrePerSecond> m_v;

	std::vector<unitless> m_sinInstrumentElevationsAbsolute;
	std::vector<unitless> m_sinInstrumentAzimuthsAbsolute;
	std::vector<unitless> m_sinInstrumentRollsAbsolute;
	std::vector<unitless> m_cosInstrumentElevationsAbsolute;
	std::vector<unitless> m_cosInstrumentAzimuthsAbsolute;
	std::vector<unitless> m_cosInstrumentRollsAbsolute;

	mutable size_t m_previousLowerIndexStartTime;
	mutable size_t m_previousLowerIndexEndTime;

	template<class T>
	T findMean(const sci::UtcTime &startTime, const sci::UtcTime &endTime, const std::vector<T> &property) const
	{
		if (startTime < m_times[0] || endTime > m_times.back())
			return std::numeric_limits<T>::quiet_NaN();
		if (startTime == endTime)
		{
			size_t lowerIndex = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
			return sci::linearinterpolate(startTime, m_times[lowerIndex], m_times[lowerIndex + 1], property[lowerIndex], property[lowerIndex + 1]);
		}
		size_t startLowerIndex = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
		size_t endLowerIndex = findLowerIndex(endTime, false);
		size_t endUpperIndex = std::min(endLowerIndex + 1, m_times.size() - 1);
		std::vector<sci::UtcTime> subTimes(m_times.begin() + startLowerIndex, m_times.begin() + endUpperIndex);
		std::vector<T> subProperty(property.begin() + startLowerIndex, property.begin() + endUpperIndex);
		return sci::integrate(subTimes, subProperty, startTime, endTime) / (endTime - startTime);
	}
	template<class T>
	void findStatistics(const sci::UtcTime &startTime, const sci::UtcTime &endTime, const std::vector<T> &property, T &mean, T &stdev, T &min, T &max) const
	{

		if (startTime < m_times[0] || endTime > m_times.back())
		{
			mean = std::numeric_limits<T>::quiet_NaN();
			min = std::numeric_limits<T>::quiet_NaN();
			max = std::numeric_limits<T>::quiet_NaN();
			stdev = std::numeric_limits<T>::quiet_NaN();
			return;
		}
		if (startTime == endTime)
		{
			size_t lowerIndex = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
			mean = sci::linearinterpolate(startTime, m_times[lowerIndex], m_times[lowerIndex + 1], property[lowerIndex], property[lowerIndex + 1]);
			min = mean;
			max = mean;
			stdev = T(0);
		}
		size_t startLowerIndex = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
		size_t endLowerIndex = findLowerIndex(endTime, false);
		size_t endUpperIndex = std::min(endLowerIndex + 1, m_times.size() - 1);
		std::vector<sci::UtcTime> subTimes(m_times.begin() + startLowerIndex, m_times.begin() + endUpperIndex);
		std::vector<degree> subProperty(property.begin() + startLowerIndex, property.begin() + endUpperIndex);
		mean = sci::integrate(subTimes, subProperty, startTime, endTime) / (endTime - startTime);
		min = sci::min<T>(subProperty);
		max = sci::max<T>(subProperty);
		std::vector<decltype(subProperty[0] * subProperty[0])>subPropertySquared(subProperty.size());
		for(size_t i=0; i<subProperty.size(); ++i)
			subPropertySquared[i] = sci::pow<2>(subProperty[i] - mean);
		stdev = sci::TypeTraits<decltype(subProperty[0] * subProperty[0])>::sqrt(sci::integrate(subTimes, subPropertySquared, startTime, endTime) / (endTime - startTime));
	}
	size_t findLowerIndex(sci::UtcTime time, bool startTime) const
	{
		size_t &previousLowerIndex = startTime ? m_previousLowerIndexStartTime : m_previousLowerIndexEndTime;

		if (time > m_times.back() || time < m_times[0])
			return -1;
		size_t result = std::max(size_t(1), previousLowerIndex); // we use this to keep track of the last place we found - usually we are iterating forward in time so it is a useful optimisation
		if (m_times[result] > time)
			result = 1;
		for (; result < m_times.size(); ++result)
		{
			if (m_times[result] >= time)
			{
				--result;
				break;
			}
		}
		previousLowerIndex = result;
		return result;
	}
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
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime startTime, sci::UtcTime endTime, unitless &sinInstrumentElevation, unitless &sinInstrumentAzimuth, unitless &sinInstrumentRoll, unitless &cosInstrumentElevation, unitless &cosInstrumentAzimuth, unitless &cosInstrumentRoll) const override
	{
		sinInstrumentElevation = m_sinInstrumentElevation;
		sinInstrumentAzimuth = m_sinInstrumentAzimuth;
		sinInstrumentRoll = m_sinInstrumentRoll;
		cosInstrumentElevation = m_cosInstrumentElevation;
		cosInstrumentAzimuth = m_cosInstrumentAzimuth;
		cosInstrumentRoll = m_cosInstrumentRoll;
	}
	virtual bool getFixedAltitude() const override
	{
		return true;
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
		addAttribute(sci::NcAttribute(sU("long_name"), sU("Data Quality Flag")), ncFile);
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
	AmfNcTimeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, sci::UtcTime minTime, sci::UtcTime maxTime)
		:AmfNcVariable<double>(sU("time"), ncFile, dimension, sU("Time (seconds since 1970-01-01 00:00:00)"), sU("time"), sU("seconds since 1970-01-01 00:00:00"), (minTime-sci::UtcTime(1970, 1, 1, 0, 0, 0)).value<second>(), (maxTime - sci::UtcTime(1970, 1, 1, 0, 0, 0)).value<second>())
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("T")), ncFile);
		addAttribute(sci::NcAttribute(sU("calendar"), sU("standard")), ncFile);
	}
	static std::vector<second> convertToSeconds(const std::vector<sci::UtcTime> &times)
	{
		std::vector<second> secondsAfterEpoch(times.size());
		sci::UtcTime epoch(1970, 1, 1, 0, 0, 0.0);
		for (size_t i = 0; i < times.size(); ++i)
			secondsAfterEpoch[i] = times[i] - epoch;
		return secondsAfterEpoch;
	}
};
class AmfNcLongitudeVariable : public AmfNcVariable<double>
{
public:
	AmfNcLongitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, degree min, degree max)
		:AmfNcVariable<double>(sU("longitude"), ncFile, dimension, sU("Longitude"), sU("longitude"), sU("degrees_north"), min.value<degree>(), max.value<degree>())
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("X")), ncFile);
	}
};
class AmfNcLatitudeVariable : public AmfNcVariable<double>
{
public:
	AmfNcLatitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, degree min, degree max)
		:AmfNcVariable<double>(sU("latitude"), ncFile, dimension, sU("Latitude"), sU("latitude"), sU("degrees_east"), min.value<degree>(), max.value<degree>())
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
	std::vector<int> m_years;
	std::vector<int> m_months;
	std::vector<int> m_dayOfMonths;
	std::vector<double> m_dayOfYears;
	std::vector<int> m_hours;
	std::vector<int> m_minutes;
	std::vector<double> m_seconds;
	std::vector<degree> m_latitudes;
	std::vector<degree> m_longitudes;
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
