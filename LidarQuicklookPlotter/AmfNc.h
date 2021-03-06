#pragma once
#include<svector/time.h>
#include<vector>
#include<svector/sreadwrite.h>
#include<string>
#include<svector/sstring.h>
#include"Units.h"
#include<svector/svector.h>
#include<locale>
#include<map>

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
	sci::string logFileName;
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

enum class FeatureType
{
	timeSeriesPoint,
	trajectory,
	timeSeriesProfile
};
const std::map<FeatureType, sci::string> g_featureTypeStrings{ {FeatureType::timeSeriesPoint, sU("timeSeriesPoint")}, {FeatureType::trajectory, sU("trajectory")}, {FeatureType::timeSeriesProfile, sU("timeSeriesProfile"}) };

struct DataInfo
{
	secondF samplingInterval;
	secondF averagingPeriod;
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

enum class PlatformType
{
	stationary,
	moving
};
const std::map<PlatformType, sci::string> g_platformTypeStrings{ {PlatformType::stationary, sU("stationary_platform")}, {PlatformType::moving, sU("moving_platform")} };

enum class DeploymentMode
{
	air,
	land,
	sea
};
const std::map<DeploymentMode, sci::string> g_deploymentModeStrings{ {DeploymentMode::air, sU("air")}, {DeploymentMode::land, sU("land")}, {DeploymentMode::sea, sU("sea")} };

#pragma warning(push)
#pragma warning(disable : 26495)
struct PlatformInfo
{
	sci::string name;
	PlatformType platformType;
	DeploymentMode deploymentMode;
	std::vector<sci::string> locationKeywords;
};
#pragma warning(pop)

template< class T >
void correctDirection(T measuredX, T measuredY, T measuredZ, unitlessF sinInstrumentElevation, unitlessF sinInstrumentAzimuth, unitlessF sinInstrumentRoll, unitlessF cosInstrumentElevation, unitlessF cosInstrumentAzimuth, unitlessF cosInstrumentRoll, T &correctedX, T &correctedY, T &correctedZ)
{
	//make things a little easier with notation
	unitlessF cosE = cosInstrumentElevation;
	unitlessF sinE = sinInstrumentElevation;
	unitlessF cosR = cosInstrumentRoll;
	unitlessF sinR = sinInstrumentRoll;
	unitlessF cosA = cosInstrumentAzimuth;
	unitlessF sinA = sinInstrumentAzimuth;

	//transform the unit vector into the correct direction
	correctedX = measuredX * (cosR*cosA - sinR * sinE*sinA) + measuredY * (-cosE * sinA) + measuredZ * (sinR*cosA + cosR * sinE*sinA);
	correctedY = measuredX * (cosR*sinA + sinR * sinE*cosA) + measuredY * (cosE * cosA) + measuredZ * (sinR*sinA - cosR * sinE*cosA);
	correctedZ = measuredX * (-sinR * cosE) + measuredY * (sinE)+measuredZ * (cosR*cosE);
}

void correctDirection(degreeF measuredElevation, degreeF measuredAzimuth, unitlessF sinInstrumentElevation, unitlessF sinInstrumentAzimuth, unitlessF sinInstrumentRoll, unitlessF cosInstrumentElevation, unitlessF cosInstrumentAzimuth, unitlessF cosInstrumentRoll, degreeF &correctedElevation, degreeF &correctedAzimuth);
degreeF angleBetweenDirections(degreeF elevation1, degreeF azimuth1, degreeF elevation2, degreeF azimuth2);

struct AttitudeAverage
{
	AttitudeAverage(degreeF mean, degreeF min, degreeF max, degreeF stdev)
		:m_mean(mean), m_min(min), m_max(max), m_stdev(stdev)
	{}
	AttitudeAverage() {}
	degreeF m_mean;
	degreeF m_min;
	degreeF m_max;
	degreeF m_stdev;
	degreePerSecondF m_rate;
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
	void correctDirection(sci::UtcTime startTime, sci::UtcTime endTime, degreeF instrumentRelativeAzimuth, degreeF instrumentRelativeElevation, degreeF &correctedAzimuth, degreeF &correctedElevation) const
	{
		unitlessF sinInstrumentElevation;
		unitlessF sinInstrumentAzimuth;
		unitlessF sinInstrumentRoll;
		unitlessF cosInstrumentElevation;
		unitlessF cosInstrumentAzimuth;
		unitlessF cosInstrumentRoll;
		getInstrumentTrigAttitudesForDirectionCorrection(startTime, endTime, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll);

		::correctDirection(instrumentRelativeElevation, instrumentRelativeAzimuth, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedElevation, correctedAzimuth);
	}
	template<class T>
	void correctVector(sci::UtcTime startTime, sci::UtcTime endTime, T measuredX, T measuredY, T measuredZ, T &correctedX, T &correctedY, T &correctedZ) const
	{
		unitlessF sinInstrumentElevation;
		unitlessF sinInstrumentAzimuth;
		unitlessF sinInstrumentRoll;
		unitlessF cosInstrumentElevation;
		unitlessF cosInstrumentAzimuth;
		unitlessF cosInstrumentRoll;
		getInstrumentTrigAttitudesForDirectionCorrection(time, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll);

		::correctDirection(measuredX, measuredY, measuredZ, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedX, correctedY, correctedZ);
	}
	virtual void getInstrumentVelocity(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecondF &eastwardVelocity, metrePerSecondF &northwardVelocity, metrePerSecondF &upwardVelocity) const = 0;
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime startTime, sci::UtcTime endTime, unitlessF &sinInstrumentElevation, unitlessF &sinInstrumentAzimuth, unitlessF &sinInstrumentRoll, unitlessF &cosInstrumentElevation, unitlessF &cosInstrumentAzimuth, unitlessF &cosInstrumentRoll) const = 0;
	virtual void getLocation(sci::UtcTime startTime, sci::UtcTime endTime, degreeF &latitude, degreeF &longitude, metreF &altitude) const = 0;
	virtual void getInstrumentVelocity(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecondF &speed, degreeF &course) const
	{
		metrePerSecondF u;
		metrePerSecondF v;
		metrePerSecondF w;
		getInstrumentVelocity(startTime, endTime, u, v, w);
		if (u == metrePerSecondF(0) && v == metrePerSecondF(0))
		{
			speed = metrePerSecondF(0);
			course = std::numeric_limits<degreeF>::quiet_NaN();
		}
		speed = sci::sqrt(u*u + v * v);
		course = sci::atan2(u, v);

	}
	virtual void getInstrumentAttitudes(sci::UtcTime startTime, sci::UtcTime endTime, AttitudeAverage &elevation, AttitudeAverage &azimuth, AttitudeAverage &roll) const = 0;
	virtual void getPlatformAttitudes(sci::UtcTime startTime, sci::UtcTime endTime, AttitudeAverage &elevation, AttitudeAverage &azimuth, AttitudeAverage &roll) const = 0;
	virtual bool getFixedAltitude() const = 0;
private:
	PlatformInfo m_platformInfo;
};

class StationaryPlatform : public Platform
{
public:
	StationaryPlatform(sci::string name, metreF altitude, degreeF latitude, degreeF longitude, std::vector<sci::string> locationKeywords, degreeF instrumentElevation, degreeF instrumentAzimuth, degreeF instrumentRoll)
		:Platform(name, PlatformType::stationary, DeploymentMode::land, locationKeywords)
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
	virtual void getInstrumentVelocity(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecondF &eastwardVelocity, metrePerSecondF &northwardVelocity, metrePerSecondF &upwardVelocity) const override
	{
		eastwardVelocity = metrePerSecondF(0.0);
		northwardVelocity = metrePerSecondF(0.0);
		upwardVelocity = metrePerSecondF(0.0);
	}
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime startTime, sci::UtcTime endTime, unitlessF &sinInstrumentElevation, unitlessF &sinInstrumentAzimuth, unitlessF &sinInstrumentRoll, unitlessF &cosInstrumentElevation, unitlessF &cosInstrumentAzimuth, unitlessF &cosInstrumentRoll) const override
	{
		sinInstrumentElevation = m_sinInstrumentElevation;
		sinInstrumentAzimuth = m_sinInstrumentAzimuth;
		sinInstrumentRoll = m_sinInstrumentRoll;
		cosInstrumentElevation = m_cosInstrumentElevation;
		cosInstrumentAzimuth = m_cosInstrumentAzimuth;
		cosInstrumentRoll = m_cosInstrumentRoll;
	}
	virtual void getLocation(sci::UtcTime startTime, sci::UtcTime endTime, degreeF &latitude, degreeF &longitude, metreF &altitude) const override
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
		elevation = AttitudeAverage(m_elevation, m_elevation, m_elevation, degreeF(0));
		azimuth = AttitudeAverage(m_azimuth, m_azimuth, m_azimuth, degreeF(0));
		roll = AttitudeAverage(m_roll, m_roll, m_roll, degreeF(0));
	}
	virtual void getPlatformAttitudes(sci::UtcTime startTime, sci::UtcTime endTime, AttitudeAverage &elevation, AttitudeAverage &azimuth, AttitudeAverage &roll) const override
	{
		elevation.m_mean = std::numeric_limits<degreeF>::quiet_NaN();
		elevation.m_min = std::numeric_limits<degreeF>::quiet_NaN();
		elevation.m_max = std::numeric_limits<degreeF>::quiet_NaN();
		elevation.m_stdev = std::numeric_limits<degreeF>::quiet_NaN();
		azimuth = elevation;
		roll = elevation;

	}
	virtual bool getFixedAltitude() const override
	{
		return true;
	}
private:
	degreeF m_latitude;
	degreeF m_longitude;
	metreF m_altitude;
	degreeF m_elevation;
	degreeF m_azimuth;
	degreeF m_roll;
	unitlessF m_sinInstrumentElevation;
	unitlessF m_sinInstrumentAzimuth;
	unitlessF m_sinInstrumentRoll;
	unitlessF m_cosInstrumentElevation;
	unitlessF m_cosInstrumentAzimuth;
	unitlessF m_cosInstrumentRoll;
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

//this function takes an array of angles and starting from
//the second element it adds or subtracts mutiples of 360 degrees
//so that the absolute differnce between two elements is never
//more than 180 degrees
void makeContinuous(std::vector<degreeF> &angles);


//this function puts an angle into the -180 to +180 degree range
inline degreeF rangeLimitAngle(degreeF angle)
{
	degreeF jumpAmount = sci::ceil<unitlessF::unit>((angle - degreeF(180)) / degreeF(360))*degreeF(360);
	return angle - jumpAmount;
}

class ShipPlatform : public Platform
{
public:
	ShipPlatform(sci::string name, metreF altitude, const std::vector<sci::UtcTime> &times, const std::vector<degreeF> &latitudes, const std::vector<degreeF> &longitudes, const std::vector<sci::string> &locationKeywords, const std::vector<degreeF> &instrumentElevationsShipRelative, const std::vector<degreeF> &instrumentAzimuthsShipRelative, const std::vector<degreeF> &instrumentRollsShipRelative, const std::vector<degreeF> &shipCourses, const std::vector<metrePerSecondF> &shipSpeeds, const std::vector<degreeF> &shipElevations, const std::vector<degreeF> &shipAzimuths, const std::vector<degreeF> &shipRolls)
		:Platform(name, PlatformType::moving, DeploymentMode::sea, locationKeywords)
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
		makeContinuous(m_shipAzimuthsNoJumps);

		//set up a series of courses that do not jump at the 0/360 point
		m_shipCoursesNoJumps = shipCourses;
		makeContinuous(m_shipCoursesNoJumps);


		m_instrumentElevationsAbsolute = std::vector<degreeF>(times.size(), degreeF(0.0));
		m_instrumentAzimuthsAbsolute = std::vector<degreeF>(times.size(), degreeF(0.0));
		m_instrumentRollsAbsolute = std::vector<degreeF>(times.size(), degreeF(0.0));
		m_sinInstrumentElevationsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_sinInstrumentAzimuthsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_sinInstrumentRollsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_cosInstrumentElevationsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_cosInstrumentAzimuthsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_cosInstrumentRollsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));

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
			degreeF upElevation;
			degreeF upAzimuth;
			::correctDirection(degreeF(90.0),
				degreeF(0.0),
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
			degreeF upElevationNoRoll = m_instrumentElevationsAbsolute[i] + degreeF(90.0);
			degreeF upAzimuthNoRoll = m_instrumentAzimuthsAbsolute[i];
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
		makeContinuous(m_instrumentAzimuthsAbsoluteNoJumps);
	}
	ShipPlatform(sci::string name, metreF altitude, const std::vector<sci::UtcTime> &times, const std::vector<degreeF> &latitudes, const std::vector<degreeF> &longitudes, const std::vector<sci::string> &locationKeywords, degreeF instrumentElevationShipRelative, degreeF instrumentAzimuthShipRelative, degreeF instrumentRollShipRelative, const std::vector<degreeF> &shipCourses, const std::vector<metrePerSecondF> &shipSpeeds, const std::vector<degreeF> &shipElevations, const std::vector<degreeF> &shipAzimuths, const std::vector<degreeF> &shipRolls)
		:Platform(name, PlatformType::moving, DeploymentMode::sea, locationKeywords)
	{
		m_previousLowerIndexEndTime = 0;
		m_previousLowerIndexStartTime = 0;

		m_times = times;
		m_altitude = altitude;
		m_latitudes = latitudes;
		m_longitudes = longitudes;
		m_shipElevations = shipElevations;
		m_shipAzimuths = shipAzimuths;
		m_shipRolls = shipRolls;
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
		makeContinuous(m_shipAzimuthsNoJumps);

		//set up a series of courses that do not jump at the 0/360 point
		m_shipCoursesNoJumps = shipCourses;
		makeContinuous(m_shipCoursesNoJumps);

		//set up a series of longitudes that do not jump at the 0/360 point
		m_longitudesNoJumps = longitudes;
		makeContinuous(m_longitudesNoJumps);

		m_instrumentElevationsAbsolute = std::vector<degreeF>(times.size(), degreeF(0.0));
		m_instrumentAzimuthsAbsolute = std::vector<degreeF>(times.size(), degreeF(0.0));
		m_instrumentRollsAbsolute = std::vector<degreeF>(times.size(), degreeF(0.0));
		m_sinInstrumentElevationsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_sinInstrumentAzimuthsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_sinInstrumentRollsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_cosInstrumentElevationsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_cosInstrumentAzimuthsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));
		m_cosInstrumentRollsAbsolute = std::vector<unitlessF>(times.size(), unitlessF(0.0));

		//We can save some calculations by doing these outside of the loop
		unitlessF sinE = sci::sin(instrumentElevationShipRelative);
		unitlessF sinA = sci::sin(instrumentAzimuthShipRelative);
		unitlessF sinR = sci::sin(instrumentRollShipRelative);
		unitlessF cosE = sci::cos(instrumentElevationShipRelative);
		unitlessF cosA = sci::cos(instrumentAzimuthShipRelative);
		unitlessF cosR = sci::cos(instrumentRollShipRelative);

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
			degreeF upElevation;
			degreeF upAzimuth;
			::correctDirection(degreeF(90.0),
				degreeF(0.0),
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
			degreeF upElevationNoRoll = m_instrumentElevationsAbsolute[i] + degreeF(90.0);
			degreeF upAzimuthNoRoll = m_instrumentAzimuthsAbsolute[i];
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
		makeContinuous(m_instrumentAzimuthsAbsoluteNoJumps);
	}
	virtual void getLocation(sci::UtcTime startTime, sci::UtcTime endTime, degreeF &latitude, degreeF &longitude, metreF &altitude) const
	{
		latitude = findMean(startTime, endTime, m_latitudes);
		longitude = findMean(startTime, endTime, m_longitudesNoJumps);
		longitude = rangeLimitAngle(longitude);
		altitude = m_altitude;
	}
	virtual void getInstrumentVelocity(sci::UtcTime startTime, sci::UtcTime endTime, metrePerSecondF &eastwardVelocity, metrePerSecondF &northwardVelocity, metrePerSecondF &upwardVelocity) const override
	{
		upwardVelocity = metrePerSecondF(0.0);
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
		findStatistics(startTime, endTime, m_instrumentElevationsAbsolute, elevation.m_mean, elevation.m_stdev, elevation.m_min, elevation.m_max, elevation.m_rate);
		findStatistics(startTime, endTime, m_instrumentAzimuthsAbsoluteNoJumps, azimuth.m_mean, azimuth.m_stdev, azimuth.m_min, azimuth.m_max, azimuth.m_rate);
		findStatistics(startTime, endTime, m_instrumentRollsAbsolute, roll.m_mean, roll.m_stdev, roll.m_min, roll.m_max, roll.m_rate);
		degreeF azimuthOffset = rangeLimitAngle(azimuth.m_mean) - azimuth.m_mean;
		azimuth.m_mean += azimuthOffset;
		azimuth.m_max += azimuthOffset;
		azimuth.m_min += azimuthOffset;
	}
	virtual void getPlatformAttitudes(sci::UtcTime startTime, sci::UtcTime endTime, AttitudeAverage &elevation, AttitudeAverage &azimuth, AttitudeAverage &roll) const override
	{
		findStatistics(startTime, endTime, m_shipElevations, elevation.m_mean, elevation.m_stdev, elevation.m_min, elevation.m_max, elevation.m_rate);
		findStatistics(startTime, endTime, m_shipAzimuthsNoJumps, azimuth.m_mean, azimuth.m_stdev, azimuth.m_min, azimuth.m_max, azimuth.m_rate);
		findStatistics(startTime, endTime, m_shipRolls, roll.m_mean, roll.m_stdev, roll.m_min, roll.m_max, roll.m_rate);
		degreeF azimuthOffset = rangeLimitAngle(azimuth.m_mean) - azimuth.m_mean;
		azimuth.m_mean += azimuthOffset;
		azimuth.m_max += azimuthOffset;
		azimuth.m_min += azimuthOffset;
	}
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime startTime, sci::UtcTime endTime, unitlessF &sinInstrumentElevation, unitlessF &sinInstrumentAzimuth, unitlessF &sinInstrumentRoll, unitlessF &cosInstrumentElevation, unitlessF &cosInstrumentAzimuth, unitlessF &cosInstrumentRoll) const override
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
	metreF m_altitude;
	std::vector<degreeF> m_latitudes;
	std::vector<degreeF> m_longitudes;
	std::vector<degreeF> m_longitudesNoJumps;
	std::vector<degreeF> m_instrumentElevationsAbsolute;
	std::vector<degreeF> m_instrumentAzimuthsAbsolute;
	std::vector<degreeF> m_instrumentAzimuthsAbsoluteNoJumps;
	std::vector<degreeF> m_instrumentRollsAbsolute;
	std::vector<degreeF> m_shipCourses;
	std::vector<degreeF> m_shipElevations;
	std::vector<degreeF> m_shipAzimuths;
	std::vector<degreeF> m_shipRolls;
	std::vector<degreeF> m_shipCoursesNoJumps;
	std::vector<degreeF> m_shipAzimuthsNoJumps;
	std::vector<metrePerSecondF> m_shipSpeeds;
	std::vector<metrePerSecondF> m_u;
	std::vector<metrePerSecondF> m_v;

	std::vector<unitlessF> m_sinInstrumentElevationsAbsolute;
	std::vector<unitlessF> m_sinInstrumentAzimuthsAbsolute;
	std::vector<unitlessF> m_sinInstrumentRollsAbsolute;
	std::vector<unitlessF> m_cosInstrumentElevationsAbsolute;
	std::vector<unitlessF> m_cosInstrumentAzimuthsAbsolute;
	std::vector<unitlessF> m_cosInstrumentRollsAbsolute;

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
			return T(sci::linearinterpolate(startTime, m_times[lowerIndex], m_times[lowerIndex + 1], property[lowerIndex], property[lowerIndex + 1]));
		}
		size_t startLowerIndex = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
		size_t endLowerIndex = findLowerIndex(endTime, false);
		size_t endUpperIndex = std::min(endLowerIndex + 1, m_times.size() - 1);
		std::vector<sci::UtcTime> subTimes(m_times.begin() + startLowerIndex, m_times.begin() + endUpperIndex + 1);
		std::vector<T> subProperty(property.begin() + startLowerIndex, property.begin() + endUpperIndex + 1);
		return T(sci::integrate(subTimes, subProperty, startTime, endTime) / (endTime - startTime));
	}
	template<class T>
	void findStatistics(const sci::UtcTime &startTime, const sci::UtcTime &endTime, const std::vector<T> &property, T &mean, T &stdev, T &min, T &max, decltype(T(1) / sci::Physical<sci::Second<>, T::valueType>(1)) &rate) const
	{
		typedef std::remove_reference<decltype(rate)>::type rateType;
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
			mean = T(sci::linearinterpolate(startTime, m_times[lowerIndex], m_times[lowerIndex + 1], property[lowerIndex], property[lowerIndex + 1]));
			min = mean;
			max = mean;
			stdev = T(0);
			rate = std::numeric_limits<std::remove_reference<decltype(rate)>::type>::quiet_NaN();
		}
		size_t startLowerIndex = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
		size_t endLowerIndex = findLowerIndex(endTime, false);
		size_t endUpperIndex = std::min(endLowerIndex + 1, m_times.size() - 1);
		std::vector<sci::UtcTime> subTimes(m_times.begin() + startLowerIndex, m_times.begin() + endUpperIndex + 1);
		std::vector<degreeF> subProperty(property.begin() + startLowerIndex, property.begin() + endUpperIndex + 1);
		mean = T(sci::integrate(subTimes, subProperty, startTime, endTime) / (endTime - startTime));
		min = sci::min<T>(subProperty);
		max = sci::max<T>(subProperty);
		size_t lowerIndexStart = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
		size_t lowerIndexEnd = std::min(findLowerIndex(endTime, false), m_times.size() - 1);
		rate = rateType((sci::linearinterpolate(endTime, m_times[lowerIndexEnd], m_times[lowerIndexEnd + 1], property[lowerIndexEnd], property[lowerIndexEnd + 1]) - sci::linearinterpolate(startTime, m_times[lowerIndexStart], m_times[lowerIndexStart + 1], property[lowerIndexStart], property[lowerIndexStart + 1])) / (endTime - startTime));
		std::vector<decltype(subProperty[0] * subProperty[0])>subPropertySquared(subProperty.size());
		for (size_t i = 0; i < subProperty.size(); ++i)
			subPropertySquared[i] = sci::pow<2>(subProperty[i] - mean);
		auto variance = sci::integrate(subTimes, subPropertySquared, startTime, endTime) / (endTime - startTime);
		stdev = T(sci::TypeTraits<decltype(variance)>::sqrt(variance));
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
	ShipPlatformShipRelativeCorrected(sci::string name, metreF altitude, const std::vector<sci::UtcTime> &times, const std::vector<degreeF> &latitudes, const std::vector<degreeF> &longitudes, const std::vector<sci::string> &locationKeywords, degreeF instrumentElevationShipRelative, degreeF instrumentAzimuthShipRelative, degreeF instrumentRollShipRelative, const std::vector<degreeF> &shipCourses, const std::vector<metrePerSecondF> &shipSpeeds, const std::vector<degreeF> &shipElevations, const std::vector<degreeF> &shipAzimuths, const std::vector<degreeF> &shipRolls)
		: ShipPlatform(name, altitude, times, latitudes, longitudes, locationKeywords, instrumentElevationShipRelative, instrumentAzimuthShipRelative, instrumentRollShipRelative, shipCourses, shipSpeeds, shipElevations, shipAzimuths, shipRolls)
	{
		m_sinInstrumentElevation = sci::sin(instrumentElevationShipRelative);
		m_sinInstrumentAzimuth = sci::sin(instrumentAzimuthShipRelative);
		m_sinInstrumentRoll = sci::sin(instrumentRollShipRelative);
		m_cosInstrumentElevation = sci::cos(instrumentElevationShipRelative);
		m_cosInstrumentAzimuth = sci::cos(instrumentAzimuthShipRelative);
		m_cosInstrumentRoll = sci::cos(instrumentRollShipRelative);
	}
	virtual void getInstrumentTrigAttitudesForDirectionCorrection(sci::UtcTime startTime, sci::UtcTime endTime, unitlessF &sinInstrumentElevation, unitlessF &sinInstrumentAzimuth, unitlessF &sinInstrumentRoll, unitlessF &cosInstrumentElevation, unitlessF &cosInstrumentAzimuth, unitlessF &cosInstrumentRoll) const override
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
	unitlessF m_sinInstrumentElevation;
	unitlessF m_sinInstrumentAzimuth;
	unitlessF m_sinInstrumentRoll;
	unitlessF m_cosInstrumentElevation;
	unitlessF m_cosInstrumentAzimuth;
	unitlessF m_cosInstrumentRoll;
};

class AmfNcTimeVariable;
class AmfNcLatitudeVariable;
class AmfNcLongitudeVariable;
template<class T, class U, bool SWAP=false>
class AmfNcVariable;

enum class AmfVersion
{
	v1_1_0,
	v2_0_0
};

class OutputAmfNcFile : public sci::OutputNcFile
{
	//friend class AmfNcVariable;
public:
	//use this constructor to get the position info from the platform
	OutputAmfNcFile(AmfVersion amfVersion,
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
		const std::vector<sci::NcDimension *> &nonTimeDimensions = std::vector<sci::NcDimension *>(0),
		const std::vector< sci::NcAttribute *> &globalAttributes = std::vector< sci::NcAttribute*>(0),
		bool incrementMajorVersion = false);
	//use this constructor to provide instrument derived lats and lons, e.g. for sondes
	OutputAmfNcFile(AmfVersion amfVersion,
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
		const std::vector<sci::NcDimension *> &nonTimeDimensions = std::vector<sci::NcDimension *>(0),
		const std::vector< sci::NcAttribute*>& globalAttributes = std::vector< sci::NcAttribute*>(0),
		bool incrementMajorVersion = false);
	sci::NcDimension &getTimeDimension() { return m_timeDimension; }
	void writeTimeAndLocationData(const Platform &platform);
	template<class T>
	void write(const T &variable)
	{
		auto data = variable.getData();
		sci::replacenans(data, getFillValue<sci::VectorTraits<decltype(data)>::baseType>());
		sci::OutputNcFile::write(variable, data);
	}
	template<>
	void write<sci::NcAttribute>(const sci::NcAttribute & attribute)
	{
		sci::OutputNcFile::write(attribute);
	}
	template<>
	void write<sci::NcDimension>(const sci::NcDimension & dimension)
	{
		sci::OutputNcFile::write(dimension);
	}
	template<class T, class U>
	void write(const T &variable, U data)
	{
		sci::replacenans(data, getFillValue<sci::VectorTraits<U>::baseType>());
		sci::OutputNcFile::write(variable, data);
	}
	template<class T>
	static T getFillValue()
	{
		return Fill<T>::value;
	}
	template<class T>
	static sci::string getTypeName()
	{
		return TypeName<T>::name;
	}
private:
	void initialise(AmfVersion amfVersion,
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
		bool incrementMajorVersion);
	template<class T>
	struct Fill
	{
	};
	template<>
	struct Fill<double>
	{
		static const double value;
	};
	template<>
	struct Fill<float>
	{
		static const float value;
	};
	template<class T>
	struct Fill<sci::Physical<T, double>>
	{
		static const sci::Physical<T, double> value;
	};
	template<class T>
	struct Fill<sci::Physical<T, float>>
	{
		static const sci::Physical<T, float> value;
	};
	template<>
	struct Fill<uint8_t>
	{
		static const uint8_t value;
	};
	template<>
	struct Fill<int16_t>
	{
		static const int16_t value;
	};
	template<>
	struct Fill<int32_t>
	{
		static const int32_t value;
	};
	template<>
	struct Fill<int64_t>
	{
		static const int64_t value;
	};
	template<class T>
	struct TypeName
	{

	};
	template<>
	struct TypeName<double>
	{
		static const sci::string name;
	};
	template<>
	struct TypeName<float>
	{
		static const sci::string name;
	};
	template<class T>
	struct TypeName<sci::Physical<T, double>>
	{
		static const sci::string name;
	};
	template<class T>
	struct TypeName<sci::Physical<T, float>>
	{
		static const sci::string name;
	};
	template<>
	struct TypeName<int8_t>
	{
		static const sci::string name;
	};
	template<>
	struct TypeName<uint8_t>
	{
		static const sci::string name;
	};
	template<>
	struct TypeName<int16_t>
	{
		static const sci::string name;
	};
	template<>
	struct TypeName<int32_t>
	{
		static const sci::string name;
	};
	template<>
	struct TypeName<int64_t>
	{
		static const sci::string name;
	};

	sci::NcDimension m_timeDimension;
	std::unique_ptr<AmfNcTimeVariable> m_timeVariable;
	std::unique_ptr<AmfNcLatitudeVariable> m_latitudeVariable;
	std::unique_ptr<AmfNcLongitudeVariable> m_longitudeVariable;
	std::unique_ptr<AmfNcVariable<float, std::vector<float>>> m_dayOfYearVariable;
	std::unique_ptr<AmfNcVariable<int32_t, std::vector<int32_t>>> m_yearVariable;
	std::unique_ptr<AmfNcVariable<int32_t, std::vector<int32_t>>> m_monthVariable;
	std::unique_ptr<AmfNcVariable<int32_t, std::vector<int32_t>>> m_dayVariable;
	std::unique_ptr<AmfNcVariable<int32_t, std::vector<int32_t>>> m_hourVariable;
	std::unique_ptr<AmfNcVariable<int32_t, std::vector<int32_t>>> m_minuteVariable;
	std::unique_ptr<AmfNcVariable<float, std::vector<float>>> m_secondVariable;

	//only used for moving platforms
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_courseVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_orientationVariable;
	std::unique_ptr<AmfNcVariable<metrePerSecondF, std::vector<metrePerSecondF>>> m_speedVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentPitchVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentPitchStdevVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentPitchMinVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentPitchMaxVariable;
	std::unique_ptr<AmfNcVariable<degreePerSecondF, std::vector<degreePerSecondF>>> m_instrumentPitchRateVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentRollVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentRollStdevVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentRollMinVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentRollMaxVariable;
	std::unique_ptr<AmfNcVariable<degreePerSecondF, std::vector<degreePerSecondF>>> m_instrumentRollRateVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentYawVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentYawStdevVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentYawMinVariable;
	std::unique_ptr<AmfNcVariable<degreeF, std::vector<degreeF>>> m_instrumentYawMaxVariable;
	std::unique_ptr<AmfNcVariable<degreePerSecondF, std::vector<degreePerSecondF>>> m_instrumentYawRateVariable;

	std::vector<sci::UtcTime> m_times;
	std::vector<int> m_years;
	std::vector<int> m_months;
	std::vector<int> m_dayOfMonths;
	std::vector<float> m_dayOfYears;
	std::vector<int> m_hours;
	std::vector<int> m_minutes;
	std::vector<float> m_seconds;
	std::vector<degreeF> m_latitudes;
	std::vector<degreeF> m_longitudes;
	std::vector<degreeF> m_elevations;
	std::vector<degreeF> m_elevationStdevs;
	std::vector<degreeF> m_elevationMins;
	std::vector<degreeF> m_elevationMaxs;
	std::vector<degreePerSecondF> m_elevationRates;
	std::vector<degreeF> m_azimuths;
	std::vector<degreeF> m_azimuthStdevs;
	std::vector<degreeF> m_azimuthMins;
	std::vector<degreeF> m_azimuthMaxs;
	std::vector<degreePerSecondF> m_azimuthRates;
	std::vector<degreeF> m_rolls;
	std::vector<degreeF> m_rollStdevs;
	std::vector<degreeF> m_rollMins;
	std::vector<degreeF> m_rollMaxs;
	std::vector<degreePerSecondF> m_rollRates;
	std::vector<degreeF> m_courses;
	std::vector<metrePerSecondF> m_speeds;
	std::vector<degreeF> m_headings;
};

template<class T>
const sci::Physical<T, double> OutputAmfNcFile::Fill<sci::Physical<T, double>>::value = sci::Physical<T, double>(-1e20);
template<class T>
const sci::Physical<T, float> OutputAmfNcFile::Fill<sci::Physical<T, float>>::value = sci::Physical<T, float>(-1e20f);
template<class T>
const sci::string OutputAmfNcFile::TypeName<sci::Physical<T, double>>::name = sU("float64");
template<class T>
const sci::string OutputAmfNcFile::TypeName<sci::Physical<T, float>>::name = sU("float32");

template<class T>
T constexpr getDefault()
{
	return std::numeric_limits<T>::has_quiet_NaN ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::max();
}

/*template<class T>
typename std::enable_if<!std::is_integral<T>::value, T>::type getDefault()
{
	//this is the version that will be called for floating points and physicals
	return T(OutputAmfNcFile::getFillValue());
}
//for integers we cannot use fill value use max instead
template<class T>
typename std::enable_if<std::is_integral<T>::value, T>::type getDefault()
{
	return std::numeric_limits<T>::max();
}*/

template<class T>
bool constexpr isValid(const T &value)
{
	if (std::numeric_limits<T>::has_quiet_NaN)
		return value == value;
	else
		return value != getDefault<T>();
}

template<class T>
void getMinMax(const std::vector<T> &data, T &min, T &max)
{
	min = getDefault<T>();
	max = getDefault<T>();
	//find the first non fill value
	auto iter = data.begin();
	for (; iter != data.end(); ++iter)
	{
		if (isValid(*iter))
		{
			min = *iter;
			max = *iter;
			break;
		}
	}
	//now go through the remaining data and find the min/max
	for (; iter != data.end(); ++iter)
	{
		if (isValid(*iter))
		{
			min = std::min(min, *iter);
			max = std::max(max, *iter);
		}
	}
}

template<class T, class U>
void getMinMax(const std::vector<std::vector<T>> &data, U &min, U &max)
{
	min = getDefault<U>();
	max = getDefault<U>();
	//find the first element which has a min/max which are not fill values
	auto iter = data.begin();
	for (; iter != data.end(); ++iter)
	{
		U thisMin;
		U thisMax;
		getMinMax(*iter, thisMin, thisMax);
		if (isValid(thisMin))
		{
			min = thisMin;
			max = thisMax;
			break;
		}
	}
	//now go through the remaining data and find the min/max
	for (; iter != data.end(); ++iter)
	{
		U thisMin;
		U thisMax;
		getMinMax(*iter, thisMin, thisMax);
		if (isValid(thisMin))
		{
			min = std::min(min, thisMin);
			max = std::max(max, thisMax);
		}
	}
}

enum class CellMethod
{
	none,
	point,
	sum,
	mean,
	max,
	min,
	midRange,
	standardDeviation,
	variance,
	mode,
	median
};

inline sci::string getCellMethodString(CellMethod cellMethod)
{
	if (cellMethod == CellMethod::none)
		return sU("");
	if (cellMethod == CellMethod::point)
		return sU("point");
	if (cellMethod == CellMethod::sum)
		return sU("sum");
	if (cellMethod == CellMethod::mean)
		return sU("mean");
	if (cellMethod == CellMethod::max)
		return sU("maximum");
	if (cellMethod == CellMethod::min)
		return sU("minimum");
	if (cellMethod == CellMethod::midRange)
		return sU("mid_range");
	if (cellMethod == CellMethod::standardDeviation)
		return sU("standard_deviation");
	if (cellMethod == CellMethod::variance)
		return sU("variance");
	if (cellMethod == CellMethod::mode)
		return sU("mode");
	if (cellMethod == CellMethod::median)
		return sU("median");

	return sU("unknown");
}

inline sci::string getCoordinatesAttributeText(const std::vector<sci::string> &coordinates)
{
	if (coordinates.size() == 0)
		return sU("");
	sci::ostringstream result;
	result << coordinates[0];
	for (size_t i = 1; i < coordinates.size(); ++i)
	{
		result << sU(" ") << coordinates[i];
	}
	return result.str();
}

inline sci::string getCellMethodsAttributeText(const std::vector<std::pair<sci::string, CellMethod>> &cellMethods)
{
	if (cellMethods.size() == 0)
		return sU("");
	sci::ostringstream result;
	result << cellMethods[0].first << sU(": ") << getCellMethodString(cellMethods[0].second);
	for (size_t i = 1; i < cellMethods.size(); ++i)
	{
		result << sU(" ") << cellMethods[i].first << sU(": ") << getCellMethodString(cellMethods[i].second);
	}
	return result.str();
}

template<class T, bool SWAP>
struct DataType
{

};

template<class T>
struct DataType<T, true>
{
	typedef T& type;
};

template<class T>
struct DataType<T, false>
{
	typedef const T& type;
};

template <class T, class U, bool SWAP>
class AmfNcVariable : public sci::NcVariable<T>
{
public:
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &standardName, const sci::string &units, typename DataType<U, SWAP>::type data, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods)
		:NcVariable<T>(name, ncFile, dimension)
	{
		if constexpr (SWAP)
			std::swap(m_data, data);
		else
			m_data = data;
		T validMin;
		T validMax;
		getMinMax(m_data, validMin, validMax);
		setAttributes(ncFile, longName, standardName, units, validMin, validMax, hasFillValue, coordinates, cellMethods);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &standardName, const sci::string &units, U data, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods)
		:NcVariable<T>(name, ncFile, dimensions)
	{
		if constexpr (SWAP)
			std::swap(m_data, data);
		else
			m_data = data;
		T validMin;
		T validMax;
		getMinMax(m_data, validMin, validMax);
		setAttributes(ncFile, longName, standardName, units, validMin, validMax, hasFillValue, coordinates, cellMethods);
	}
	const U& getData() const
	{
		return m_data;
	}
private:
	void setAttributes(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &standardName, const sci::string &units, T validMin, T validMax, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
		sci::NcAttribute unitsAttribute(sU("units"), units);
		sci::NcAttribute validMinAttribute(sU("valid_min"), validMin);
		sci::NcAttribute validMaxAttribute(sU("valid_max"), validMax);
		//sci::NcAttribute typeAttribute(sU("type"), OutputAmfNcFile::getTypeName<T>());
		sci::NcAttribute fillValueAttribute(sU("_FillValue"), OutputAmfNcFile::getFillValue<sci::VectorTraits<decltype(m_data)>::baseType>());
		sci::NcAttribute coordinatesAttribute(sU("coordinates"), getCoordinatesAttributeText(coordinates));
		sci::NcAttribute cellMethodsAttribute(sU("cell_methods"), getCellMethodsAttributeText(cellMethods));
		addAttribute(longNameAttribute, ncFile);
		addAttribute(unitsAttribute, ncFile);
		addAttribute(validMinAttribute, ncFile);
		addAttribute(validMaxAttribute, ncFile);
		//addAttribute(typeAttribute, ncFile);
		if (standardName.length() > 0)
			addAttribute(standardNameAttribute, ncFile);
		if (hasFillValue)
			addAttribute(fillValueAttribute, ncFile);
		if (coordinates.size() > 0)
			addAttribute(coordinatesAttribute, ncFile);
		if(cellMethods.size() > 0)
			addAttribute(cellMethodsAttribute, ncFile);
	}
	U m_data;
};

class AmfNcFlagVariable : public sci::NcVariable<uint8_t>
{
public:
	AmfNcFlagVariable(sci::string name, const std::vector<std::pair<uint8_t, sci::string>> &flagDefinitions, const sci::OutputNcFile &ncFile, std::vector<sci::NcDimension *> &dimensions)
		: sci::NcVariable<uint8_t>(name, ncFile, dimensions)
	{
		initialise(flagDefinitions, ncFile);
	}
	AmfNcFlagVariable(sci::string name, const std::vector<std::pair<uint8_t, sci::string>>& flagDefinitions, const sci::OutputNcFile& ncFile, sci::NcDimension& dimension)
		: sci::NcVariable<uint8_t>(name, ncFile, dimension)
	{
		initialise(flagDefinitions, ncFile);
	}
private:
	void initialise(const std::vector<std::pair<uint8_t, sci::string>>& flagDefinitions, const sci::OutputNcFile& ncFile)
	{
		std::vector<uint8_t>  flagValues(flagDefinitions.size());
		std::vector<sci::string>  flagDescriptions(flagDefinitions.size());
		for (size_t i = 0; i < flagDefinitions.size(); ++i)
		{
			flagValues[i] = flagDefinitions[i].first;
			flagDescriptions[i] = flagDefinitions[i].second;
			//Swap spaces for hyphens and make lower case
			std::locale locale;
			for (size_t j = 0; j < flagDescriptions[i].length(); ++j)
			{
				if (flagDescriptions[i][j] == sU(' '))
					flagDescriptions[i][j] = sU('_');
				else
					flagDescriptions[i][j] = std::tolower(flagDescriptions[i][j], locale);
			}
		}
		addAttribute(sci::NcAttribute(sU("long_name"), sU("Data Quality Flag")), ncFile);
		addAttribute(sci::NcAttribute(sU("flag_values"), flagValues), ncFile);
		addAttribute(sci::NcAttribute(sU("flag_meanings"), flagDescriptions, sci::string(sU("\n"))), ncFile);
		//addAttribute(sci::NcAttribute(sU("type"), OutputAmfNcFile::getTypeName<uint8_t>()), ncFile);
		addAttribute(sci::NcAttribute(sU("unit"), sU("1")), ncFile);
	}
};

//create a partial specialization for any AmfNcVariable based on a sci::Physical value
template <class T, class VALUE_TYPE, class U, bool SWAP>
class AmfNcVariable<sci::Physical<T, VALUE_TYPE>, U, SWAP> : public sci::NcVariable<sci::Physical<T, VALUE_TYPE>>
{
public:
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &standardName, typename DataType<U, SWAP>::type data, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, const sci::string &comment = sU(""))
		:NcVariable<sci::Physical<T, VALUE_TYPE>>(name, ncFile, dimension)
	{
		static_assert(std::is_same<sci::VectorTraits<U>::baseType, sci::Physical<T, VALUE_TYPE>>::value, "AmfNcVariable::AmfNcVariable must be called with data with the same type as the template parameter.");
		if constexpr (SWAP)
			std::swap(m_data, data);
		else
			m_data = data;
		sci::Physical<T, VALUE_TYPE> validMin;
		sci::Physical<T, VALUE_TYPE> validMax;
		getMinMax(m_data, validMin, validMax);
		setAttributes(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &standardName, typename DataType<U, SWAP>::type data, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, const sci::string &comment = sU(""))
		:NcVariable<sci::Physical<T, VALUE_TYPE>>(name, ncFile, dimensions)
	{
		static_assert(std::is_same<sci::VectorTraits<U>::baseType, sci::Physical<T, VALUE_TYPE>>::value, "AmfNcVariable::AmfNcVariable must be called with data with the same type as the template parameter.");
		if constexpr (SWAP)
			std::swap(m_data, data);
		else
			m_data = data;
		sci::Physical<T, VALUE_TYPE> validMin;
		sci::Physical<T, VALUE_TYPE> validMax;
		getMinMax(m_data, validMin, validMax);
		setAttributes(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
	}
	const U& getData() const
	{
		return m_data;
	}

private:
	void setAttributes(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &standardName, sci::Physical<T, VALUE_TYPE> validMin, sci::Physical<T, VALUE_TYPE> validMax, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, const sci::string &comment)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
		//the unit needs a little editing to comply with AMF standard
		sci::string unit = sci::Physical<T, VALUE_TYPE>::getShortUnitString<sci::string>();
		//replace blank string (unitless) with 1
		if (unit == sU(""))
			unit = sU("1");
		//replace degree symbol with the word degree
		while (unit.find_first_of(sU('\u00b0')) != sci::string::npos)
		{
			unit.replace(unit.find_first_of(sU('\u00b0')), 1, sU("degree"), 0, sci::string::npos);
		}
		sci::NcAttribute unitsAttribute(sU("units"), unit);
		sci::NcAttribute validMinAttribute(sU("valid_min"), validMin.value<T>());
		sci::NcAttribute validMaxAttribute(sU("valid_max"), validMax.value<T>());
		//sci::NcAttribute typeAttribute(sU("type"), OutputAmfNcFile::getTypeName<sci::Physical<T,VALUE_TYPE>>());
		sci::NcAttribute fillValueAttribute(sU("_FillValue"), OutputAmfNcFile::getFillValue<sci::VectorTraits<decltype(m_data)>::baseType>());
		sci::NcAttribute coordinatesAttribute(sU("coordinates"), getCoordinatesAttributeText(coordinates));
		sci::NcAttribute cellMethodsAttribute(sU("cell_methods"), getCellMethodsAttributeText(cellMethods));
		sci::NcAttribute commentAttribute(sU("comment"), comment);
		addAttribute(longNameAttribute, ncFile);
		addAttribute(unitsAttribute, ncFile);
		addAttribute(validMinAttribute, ncFile);
		addAttribute(validMaxAttribute, ncFile);
		//addAttribute(typeAttribute, ncFile);
		if (comment.length() > 0)
			addAttribute(commentAttribute, ncFile);
		if (standardName.length() > 0)
			addAttribute(standardNameAttribute, ncFile);
		if (hasFillValue)
			addAttribute(fillValueAttribute, ncFile);
		if (coordinates.size() > 0)
			addAttribute(coordinatesAttribute, ncFile);
		if(cellMethods.size() > 0)
			addAttribute(cellMethodsAttribute, ncFile);
	}
	U m_data;
};

//Specialize sci::NcVariable for Decibels
//This gives us the ability to cal flattenData with linear units and the returned data will be in dB.
template < class REFERENCE_UNIT>
class sci::NcVariable<Decibel<REFERENCE_UNIT>> : public sci::NcVariable<typename Decibel<REFERENCE_UNIT>::valueType>
{
public:
	typedef typename Decibel<REFERENCE_UNIT>::valueType valueType;

	NcVariable(sci::string name, const OutputNcFile &ncFile, const NcDimension& dimension) : sci::NcVariable<valueType>(name, ncFile, dimension) {}
	NcVariable(sci::string name, const OutputNcFile &ncFile, const std::vector<NcDimension *> &dimensions) : NcVariable<valueType>(name, ncFile, dimensions) {}
	NcVariable(NcVariable &&) = default;
	
	//This flattenData function accepts a 1d vector of data in units compatible with reference units and returns a 1d
	//array of doubles as dB, using the reference unit - not the unit in which the data is passed. I.e. it is converted
	//to the reference unit before the log10.
	template<class UNIT>
	static std::vector<valueType> flattenData(const std::vector<typename sci::Physical<UNIT, valueType>> &linearPhysicals)
	{
		std::vector<valueType> result(linearPhysicals.size());
		for (size_t i = 0; i < linearPhysicals.size(); ++i)
			result[i] = Decibel<REFERENCE_UNIT>::linearToDecibel(linearPhysicals[i]).value<unitlessF>();
		return result;
	}
	//This flattenData function accepts a multi-d vector of data in units compatible with reference units and returns a 1d
	//array of doubles as dB, using the reference unit - not the unit in which the data is passed. I.e. it is converted
	//to the reference unit before the log10.
	template <class U>
	static std::vector<valueType> flattenData(const std::vector<std::vector<U>> &linearPhysicals)
	{
		std::vector<valueType> result;
		if (linearPhysicals.size() == 0)
			return result;
		result.reserve(sci::nelements(linearPhysicals));
		result = flattenData(linearPhysicals[0]);
		for (size_t i = 1; i < linearPhysicals.size(); ++i)
		{
			std::vector<valueType> temp = flattenData(linearPhysicals[i]);
			result.insert(result.end(), temp.begin(), temp.end());
		}
		return result;
	}
};

//logarithmic variables need to inherit from the Decibel specialization above so that the data gets converted to
//dB during the flattenData call
//REFERENCE_UNIT is the sci::Unit that is the reference unit for the dB
//U is the data type - it will be a 1d or multi-d vector of Physicals
template < class REFERENCE_UNIT, class U>
class AmfNcVariable<Decibel<REFERENCE_UNIT>, U> : public sci::NcVariable<Decibel<REFERENCE_UNIT>>
{
public:

	//typedef typename Decibel<REFERENCE_UNIT>::referencePhysical::valueType valueType;
	typedef typename Decibel<REFERENCE_UNIT>::referencePhysical referencePhysical;
	//Data must be passed in in linear units, not decibels!!!
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &standardName, const U &dataLinear, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, bool isDbZ, bool outputReferenceUnit, const sci::string &comment = sU(""))
		:sci::NcVariable<Decibel<REFERENCE_UNIT>>(name, ncFile, dimension)
	{
		static_assert(std::is_same<sci::VectorTraits<U>::baseType, sci::Physical<typename REFERENCE_UNIT::unit>>::value, "AmfNcVariable::AmfNcVariable<decibel<>> must be called with data with the same linear type as the REFERENCE_UNIT template parameter.");
		m_dataLinear = dataLinear;
		m_isDbZ = isDbZ;
		m_outputReferenceUnit = outputReferenceUnit;
		referencePhysical validMin;
		referencePhysical validMax;
		getMinMax(m_dataLinear, validMin, validMax);
		setAttributes(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
	}
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &standardName, const U &dataLinear, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, bool isDbZ, bool outputReferenceUnit, const sci::string &comment = sU(""))
		:sci::NcVariable<Decibel<REFERENCE_UNIT>>(name, ncFile, dimensions)
	{
		//static_assert(std::is_same<sci::VectorTraits<U>::baseType, referencePhysical>::value, "AmfNcVariable::AmfNcVariable<decibel<>> must be called with data with the same linear type as the REFERENCE_UNIT template parameter.");
		m_dataLinear = dataLinear;
		m_isDbZ = isDbZ;
		m_outputReferenceUnit = outputReferenceUnit;
		sci::VectorTraits<U>::baseType validMin;
		sci::VectorTraits<U>::baseType validMax;
		getMinMax(m_dataLinear, validMin, validMax);
		setAttributes(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
	}
	const U& getData() const
	{
		return m_dataLinear;
	}
private:
	void setAttributes(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &standardName, referencePhysical validMinLinear, referencePhysical validMaxLinear, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, const sci::string &comment)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
		sci::NcAttribute unitsAttribute(sU("units"), m_isDbZ ? sU("dBZ") : sU("dB"));
		sci::string referenceUnit = referencePhysical::getShortUnitString<sci::string>();
		if (referenceUnit.length() == 0)
			referenceUnit = sU("1");
		sci::NcAttribute referenceUnitAttribute(sU("reference_unit"), referenceUnit);
		sci::NcAttribute validMinAttribute(sU("valid_min"), Decibel<REFERENCE_UNIT>::linearToDecibel(validMinLinear));
		sci::NcAttribute validMaxAttribute(sU("valid_max"), Decibel<REFERENCE_UNIT>::linearToDecibel(validMaxLinear));
		//sci::NcAttribute typeAttribute(sU("type"), OutputAmfNcFile::getTypeName<Decibel<REFERENCE_UNIT>::valueType>());
		sci::NcAttribute fillValueAttribute(sU("_FillValue"), OutputAmfNcFile::getFillValue<sci::VectorTraits<U>::baseType>());
		sci::NcAttribute coordinatesAttribute(sU("coordinates"), getCoordinatesAttributeText(coordinates));
		sci::NcAttribute cellMethodsAttribute(sU("cell_methods"), getCellMethodsAttributeText(cellMethods));
		sci::NcAttribute commentAttribute(sU("comment"), comment);
		addAttribute(longNameAttribute, ncFile);
		addAttribute(unitsAttribute, ncFile);
		if (m_outputReferenceUnit)
			addAttribute(referenceUnitAttribute, ncFile);
		addAttribute(validMinAttribute, ncFile);
		addAttribute(validMaxAttribute, ncFile);
		//addAttribute(typeAttribute, ncFile);
		if (comment.length() > 0)
			addAttribute(commentAttribute, ncFile);
		if (standardName.length() > 0)
			addAttribute(standardNameAttribute, ncFile);
		if (hasFillValue)
			addAttribute(fillValueAttribute, ncFile);
		if (coordinates.size() > 0)
			addAttribute(coordinatesAttribute, ncFile);
		if(cellMethods.size() > 0)
			addAttribute(cellMethodsAttribute, ncFile);
	}

	//this is in reference units and is a 1d or multi-d vector.
	U m_dataLinear;
	bool m_isDbZ;
	bool m_outputReferenceUnit;
};

class AmfNcTimeVariable : public AmfNcVariable<typename second::valueType, std::vector<typename second::valueType>>
{
public:
	AmfNcTimeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, const std::vector<sci::UtcTime> &times)
		:AmfNcVariable<typename second::valueType, std::vector<typename second::valueType >> (sU("time"), ncFile, dimension, sU("Time (seconds since 1970-01-01 00:00:00)"), sU("time"), sU("seconds since 1970-01-01 00:00:00"), sci::physicalsToValues<second::unit>(times - sci::UtcTime(1970, 1, 1, 0, 0, 0)), false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0))
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
class AmfNcLongitudeVariable : public AmfNcVariable<typename degreeF::valueType, std::vector<typename degreeF::valueType>>
{
public:
	AmfNcLongitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, const std::vector<degreeF> &longitudes, FeatureType featureType)
		:AmfNcVariable<typename degreeF::valueType, std::vector<typename degreeF::valueType>>(
			sU("longitude"),
			ncFile, 
			dimension,
			sU("Longitude"),
			sU("longitude"),
			sU("degrees_east"),
			sci::physicalsToValues<degreeF>(longitudes),
			true,
			std::vector<sci::string>(0),
			featureType == FeatureType::trajectory ? std::vector<std::pair<sci::string, CellMethod>>{{sU("time"), CellMethod::point}} : std::vector<std::pair<sci::string, CellMethod>>(0))
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("X")), ncFile);
	}
};
class AmfNcLatitudeVariable : public AmfNcVariable<typename degreeF::valueType, std::vector<typename degreeF::valueType>>
{
public:
	AmfNcLatitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, const std::vector<degreeF> &latitudes, FeatureType featureType)
		:AmfNcVariable<typename degreeF::valueType, std::vector<typename degreeF::valueType>>(
			sU("latitude"),
			ncFile,
			dimension,
			sU("Latitude"),
			sU("latitude"),
			sU("degrees_north"),
			sci::physicalsToValues<degreeF>(latitudes),
			true,
			std::vector<sci::string>(0),
			featureType == FeatureType::trajectory ? std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}} : std::vector<std::pair<sci::string, CellMethod>>(0))
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("Y")), ncFile);
	}
};

class AmfNcAltitudeVariable : public AmfNcVariable<typename metreF::valueType, std::vector<typename metreF::valueType>>
{
public:
	AmfNcAltitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, const std::vector<metreF> &altitudes, FeatureType featureType)
		:AmfNcVariable<typename metreF::valueType, std::vector<typename metreF::valueType>>(
			sU("altitude"),
			ncFile,
			dimension,
			sU("Geometric height above geoid (WGS84)"),
			sU("altitude"),
			sU("m"),
			sci::physicalsToValues<metreF>(altitudes),
			true,
			std::vector<sci::string>(0),
			featureType == FeatureType::trajectory ? std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}} : std::vector<std::pair<sci::string, CellMethod>>(0))
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), ncFile);
	}
};
