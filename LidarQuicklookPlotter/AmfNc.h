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
#include<svector/array.h>
#include<ranges>
#include"CellMethods.h"
#include<svector/ArrayManipulation.h>

struct HplHeader;
class HplProfile;
class wxWindow;
class ProgressReporter;
class CampbellMessage2;
class CampbellCeilometerProfile;

template<class DEST_PHYSICAL>
constexpr inline auto PhysicalToValueTransform(DEST_PHYSICAL source)
{
	return source.value<typename DEST_PHYSICAL::unit>();
}


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
	bool closeOnCompletion;
	second waitTime;
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
	point,
	timeSeries,
	trajectory,
	profile,
	timeSeriesProfile,
	trajectoryProfile
};
const std::map<FeatureType, sci::string> g_featureTypeStrings{ 
	{FeatureType::point, sU("point")},
	{FeatureType::timeSeries, sU("timeSeries")},
	{FeatureType::trajectory, sU("trajectory")},
	{FeatureType::profile, sU("profile")},
	{FeatureType::timeSeriesProfile, sU("timeSeriesProfile"}),
	{FeatureType::trajectoryProfile, sU("trajectoryProfile")} };

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
	sci::GridData<sci::string, 1> locationKeywords;
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
	Platform(sci::string name, PlatformType platformType, DeploymentMode deploymentMode, sci::GridData<sci::string, 1> locationKeywords, sci::string hatproRetrieval)
	{
		m_platformInfo.name = name;
		m_platformInfo.platformType = platformType;
		m_platformInfo.deploymentMode = deploymentMode;
		m_platformInfo.locationKeywords = locationKeywords;
		m_hatproRetrieval = hatproRetrieval;
	}

	PlatformInfo getPlatformInfo() const
	{
		return m_platformInfo;
	};
	sci::string getHatproRetrieval() const
	{
		return m_hatproRetrieval;
	}
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
	sci::string m_hatproRetrieval;
};

class StationaryPlatform : public Platform
{
public:
	StationaryPlatform(sci::string name, metreF altitude, degreeF latitude, degreeF longitude, sci::GridData<sci::string, 1> locationKeywords, degreeF instrumentElevation, degreeF instrumentAzimuth, degreeF instrumentRoll, sci::string hatproRetrieval)
		:Platform(name, PlatformType::stationary, DeploymentMode::land, locationKeywords, hatproRetrieval)
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
void makeContinuous(sci::GridData<degreeF, 1> &angles);


//this function puts an angle into the -180 to +180 degree range
inline degreeF rangeLimitAngle(degreeF angle)
{
	degreeF jumpAmount = sci::ceil<unitlessF::unit>((angle - degreeF(180)) / degreeF(360))*degreeF(360);
	return angle - jumpAmount;
}

class ShipPlatform : public Platform
{
public:
	ShipPlatform(sci::string name, metreF altitude, const sci::GridData<sci::UtcTime, 1> &times, const sci::GridData<degreeF, 1> &latitudes, const sci::GridData<degreeF, 1> &longitudes, const sci::GridData<sci::string, 1> &locationKeywords, const sci::GridData<degreeF, 1> &instrumentElevationsShipRelative, const sci::GridData<degreeF, 1> &instrumentAzimuthsShipRelative, const sci::GridData<degreeF, 1> &instrumentRollsShipRelative, const sci::GridData<degreeF, 1> &shipCourses, const sci::GridData<metrePerSecondF, 1> &shipSpeeds, const sci::GridData<degreeF, 1> &shipElevations, const sci::GridData<degreeF, 1> &shipAzimuths, const sci::GridData<degreeF, 1> &shipRolls, sci::string hatproRetrieval)
		:Platform(name, PlatformType::moving, DeploymentMode::sea, locationKeywords, hatproRetrieval)
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


		m_instrumentElevationsAbsolute = sci::GridData<degreeF, 1>(times.size(), degreeF(0.0));
		m_instrumentAzimuthsAbsolute = sci::GridData<degreeF, 1>(times.size(), degreeF(0.0));
		m_instrumentRollsAbsolute = sci::GridData<degreeF, 1>(times.size(), degreeF(0.0));
		m_sinInstrumentElevationsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_sinInstrumentAzimuthsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_sinInstrumentRollsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_cosInstrumentElevationsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_cosInstrumentAzimuthsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_cosInstrumentRollsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));

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
	ShipPlatform(sci::string name, metreF altitude, const sci::GridData<sci::UtcTime, 1> &times, const sci::GridData<degreeF, 1> &latitudes, const sci::GridData<degreeF, 1> &longitudes, const sci::GridData<sci::string, 1> &locationKeywords, degreeF instrumentElevationShipRelative, degreeF instrumentAzimuthShipRelative, degreeF instrumentRollShipRelative, const sci::GridData<degreeF, 1> &shipCourses, const sci::GridData<metrePerSecondF, 1> &shipSpeeds, const sci::GridData<degreeF, 1> &shipElevations, const sci::GridData<degreeF, 1> &shipAzimuths, const sci::GridData<degreeF, 1> &shipRolls, sci::string hatproRetrieval)
		:Platform(name, PlatformType::moving, DeploymentMode::sea, locationKeywords, hatproRetrieval)
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

		m_instrumentElevationsAbsolute = sci::GridData<degreeF, 1>(times.size(), degreeF(0.0));
		m_instrumentAzimuthsAbsolute = sci::GridData<degreeF, 1>(times.size(), degreeF(0.0));
		m_instrumentRollsAbsolute = sci::GridData<degreeF, 1>(times.size(), degreeF(0.0));
		m_sinInstrumentElevationsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_sinInstrumentAzimuthsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_sinInstrumentRollsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_cosInstrumentElevationsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_cosInstrumentAzimuthsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));
		m_cosInstrumentRollsAbsolute = sci::GridData<unitlessF, 1>(times.size(), unitlessF(0.0));

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
	sci::GridData<sci::UtcTime, 1> m_times;
	metreF m_altitude;
	sci::GridData<degreeF, 1> m_latitudes;
	sci::GridData<degreeF, 1> m_longitudes;
	sci::GridData<degreeF, 1> m_longitudesNoJumps;
	sci::GridData<degreeF, 1> m_instrumentElevationsAbsolute;
	sci::GridData<degreeF, 1> m_instrumentAzimuthsAbsolute;
	sci::GridData<degreeF, 1> m_instrumentAzimuthsAbsoluteNoJumps;
	sci::GridData<degreeF, 1> m_instrumentRollsAbsolute;
	sci::GridData<degreeF, 1> m_shipCourses;
	sci::GridData<degreeF, 1> m_shipElevations;
	sci::GridData<degreeF, 1> m_shipAzimuths;
	sci::GridData<degreeF, 1> m_shipRolls;
	sci::GridData<degreeF, 1> m_shipCoursesNoJumps;
	sci::GridData<degreeF, 1> m_shipAzimuthsNoJumps;
	sci::GridData<metrePerSecondF, 1> m_shipSpeeds;
	sci::GridData<metrePerSecondF, 1> m_u;
	sci::GridData<metrePerSecondF, 1> m_v;

	sci::GridData<unitlessF, 1> m_sinInstrumentElevationsAbsolute;
	sci::GridData<unitlessF, 1> m_sinInstrumentAzimuthsAbsolute;
	sci::GridData<unitlessF, 1> m_sinInstrumentRollsAbsolute;
	sci::GridData<unitlessF, 1> m_cosInstrumentElevationsAbsolute;
	sci::GridData<unitlessF, 1> m_cosInstrumentAzimuthsAbsolute;
	sci::GridData<unitlessF, 1> m_cosInstrumentRollsAbsolute;

	mutable size_t m_previousLowerIndexStartTime;
	mutable size_t m_previousLowerIndexEndTime;

	template<class T>
	T findMean(const sci::UtcTime &startTime, const sci::UtcTime &endTime, const sci::GridData<T, 1> &property) const
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
	void findStatistics(const sci::UtcTime &startTime, const sci::UtcTime &endTime, const sci::GridData<T, 1> &property, T &mean, T &stdev, T &min, T &max, decltype(T(1) / sci::Physical<sci::Second<>, typename T::valueType>(1)) &rate) const
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
			rate = std::numeric_limits<std::remove_reference_t<decltype(rate)>>::quiet_NaN();
		}
		size_t startLowerIndex = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
		size_t endLowerIndex = findLowerIndex(endTime, false);
		size_t endUpperIndex = std::min(endLowerIndex + 1, m_times.size() - 1);
		sci::GridData<sci::UtcTime, 1> subTimes(m_times.begin() + startLowerIndex, m_times.begin() + endUpperIndex + 1);
		sci::GridData<degreeF, 1> subProperty(property.begin() + startLowerIndex, property.begin() + endUpperIndex + 1);
		mean = T(sci::integrate(subTimes, subProperty, startTime, endTime) / (endTime - startTime));
		min = sci::min(subProperty);
		max = sci::max(subProperty);
		size_t lowerIndexStart = std::min(findLowerIndex(startTime, true), m_times.size() - 1);
		size_t lowerIndexEnd = std::min(findLowerIndex(endTime, false), m_times.size() - 1);
		rate = rateType((sci::linearinterpolate(endTime, m_times[lowerIndexEnd], m_times[lowerIndexEnd + 1], property[lowerIndexEnd], property[lowerIndexEnd + 1]) - sci::linearinterpolate(startTime, m_times[lowerIndexStart], m_times[lowerIndexStart + 1], property[lowerIndexStart], property[lowerIndexStart + 1])) / (endTime - startTime));
		sci::GridData<decltype(subProperty[0] * subProperty[0]), 1>subPropertySquared(subProperty.size());
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
	ShipPlatformShipRelativeCorrected(sci::string name, metreF altitude, const sci::GridData<sci::UtcTime, 1> &times, const sci::GridData<degreeF, 1> &latitudes, const sci::GridData<degreeF, 1> &longitudes, const sci::GridData<sci::string, 1> &locationKeywords, degreeF instrumentElevationShipRelative, degreeF instrumentAzimuthShipRelative, degreeF instrumentRollShipRelative, const sci::GridData<degreeF, 1> &shipCourses, const sci::GridData<metrePerSecondF, 1> &shipSpeeds, const sci::GridData<degreeF, 1> &shipElevations, const sci::GridData<degreeF, 1> &shipAzimuths, const sci::GridData<degreeF, 1> &shipRolls, sci::string hatproRetrieval)
		: ShipPlatform(name, altitude, times, latitudes, longitudes, locationKeywords, instrumentElevationShipRelative, instrumentAzimuthShipRelative, instrumentRollShipRelative, shipCourses, shipSpeeds, shipElevations, shipAzimuths, shipRolls, hatproRetrieval)
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
template<class T>
class AmfNcVariable;

enum class AmfVersion
{
	v1_1_0,
	v2_0_0
};

sci::string getFormattedDateTime(const sci::UtcTime& time, sci::string dateSeparator, sci::string timeSeparator, sci::string dateTimeSeparator);
sci::string getFormattedDateOnly(const sci::UtcTime& time, sci::string separator);
sci::string getFormattedTimeOnly(const sci::UtcTime& time, sci::string separator);

//template<sci::IsGridDimsVt<1, degreeF> LAT_GRID, sci::IsGridDimsVt<1, degreeF> LON_GRID>
template<class LAT_GRID, class LON_GRID>
sci::string getBoundsString(const LAT_GRID& latitudes, const LON_GRID& longitudes)
{
	sci::stringstream result;
	if (latitudes.size() == 1)
	{
		result << latitudes[0].value<degreeF>() << sU("N ") << longitudes[0].value<degreeF>() << sU("E");
	}
	else
	{
		result << sci::min(latitudes).value<degreeF>() << "N " << sci::min(longitudes).value<degreeF>() << "E, "
			<< sci::max(latitudes).value<degreeF>() << "N " << sci::max(longitudes).value<degreeF>() << "E";
	}

	return result.str();
}

template < class REFERENCE_UNIT>
class AmfNcDbVariableFromLogarithmicData;

class OutputAmfNcFile : public sci::OutputNcFile
{
	//friend class AmfNcVariable;
public:
	//use this constructor to get the position info from the platform
	template<class TIMES_GRID>
	requires sci::IsGridDims<TIMES_GRID, 1>
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
		const TIMES_GRID &times,
		const std::vector<sci::NcDimension *> &nonTimeDimensions = std::vector<sci::NcDimension *>(0),
		sci::string comment = sU(""),
		const std::vector< sci::NcAttribute *> &globalAttributes = std::vector< sci::NcAttribute*>(0),
		bool incrementMajorVersion = false)
		:OutputNcFile(), m_timeDimension(sU("time"), times.size()), m_times(times)
	{
		initialise(amfVersion, directory, instrumentInfo, author, processingsoftwareInfo, calibrationInfo, dataInfo,
			projectInfo, platform, title, times, sci::GridData<degreeF, 1>(), sci::GridData<degreeF, 1>(), nonTimeDimensions,
			comment, globalAttributes, incrementMajorVersion);
	}
	//use this constructor to provide instrument derived lats and lons, e.g. for sondes
	template<class TIMES_GRID, class LAT_GRID, class LON_GRID>
	requires (sci::IsGridDims<TIMES_GRID, 1> &&
		sci::IsGridDims <LAT_GRID, 1> &&
		sci::IsGridDims <LON_GRID, 1>)
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
		const TIMES_GRID &times,
		const LAT_GRID &latitudes,
		const LON_GRID &longitudes,
		const std::vector<sci::NcDimension *> &nonTimeDimensions = std::vector<sci::NcDimension *>(0),
		sci::string comment = sU(""),
		const std::vector< sci::NcAttribute*>& globalAttributes = std::vector< sci::NcAttribute*>(0),
		bool incrementMajorVersion = false)
		:OutputNcFile(), m_timeDimension(sU("time"), times.size()), m_times(times)
	{
		initialise(amfVersion, directory, instrumentInfo, author, processingsoftwareInfo, calibrationInfo, dataInfo,
			projectInfo, platform, title, times, latitudes, longitudes, nonTimeDimensions, comment, globalAttributes,
			incrementMajorVersion);
	}
	sci::NcDimension &getTimeDimension() { return m_timeDimension; }
	void writeTimeAndLocationData(const Platform &platform);
	/*template<class T>
	void write(const T& variable)
	{
		auto data = variable.getData();
		sci::replacenans(data, getFillValue<sci::VectorTraits<decltype(data)>::baseType>());
		sci::OutputNcFile::write(variable, data);
	}*/
	void write(const sci::NcAttribute & attribute)
	{
		sci::OutputNcFile::write(attribute);
	}
	void write(const sci::NcDimension & dimension)
	{
		sci::OutputNcFile::write(dimension);
	}
	template<class T, class U>
	void write(const T &variable, const U &data)
	{
		auto ncOutputView = sci::make_gridtransform_view(data, [](const U::value_type& val) { return T::transformForOutput(val); });
		sci::OutputNcFile::write(variable, ncOutputView);
	}
	//template<size_t NDIMS>
	//void writeDbData(const AmfNcDbVariableFromLogarithmicData<unitlessF>& variable, const sci::GridData<unitlessF, NDIMS>& data);
	template<class T, sci::IsGrid U>
	void writeIgnoreDefunctLastDim(const T& variable, const U &data)
	{
		static_assert(U::ndims > 1, "Cannot remove a defunct last dimension when the data has zero or 1 dimension");

		//get the original shape
		auto shape = data.shape();

		//check that the last size is 1
		sci::assertThrow(shape.back() == 1, sci::err(sci::SERR_USER, 0, "cannot remove defunct last dimension when the size of that dimension is not 0"));

		//calculate the strides of the new grid
		std::array<size_t, U::ndims - 2> strides;
		for (size_t i = 0; i < strides.size(); ++i)
		{
			strides[i] = 1;
			for (size_t j = i + 1; j < shape.size(); ++j)
				strides[i] *= shape[j];
		}

		if constexpr (U::ndims > 2)
		{
			//create a new view of the data, but with the new strides
			auto reshapedView(data | sci::views::grid<U::ndims - 1>(sci::GridPremultipliedStridesReference< U::ndims - 1>(&strides[0])));
			//make a view applying any needed transform
			auto ncOutputView = sci::make_gridtransform_view(reshapedView, [](const U::value_type& val) { return T::transformForOutput(val); });
			//write out the data
			sci::OutputNcFile::write(variable, ncOutputView);
		}
		else
		{
			auto reshapedView(data | sci::views::grid<U::ndims - 1>(sci::GridPremultipliedStridesReference< U::ndims - 1>()));
			//make a view applying any needed transform
			auto ncOutputView = sci::make_gridtransform_view(reshapedView, [](const U::value_type& val) { return T::transformForOutput(val); });
			//write out the data
			sci::OutputNcFile::write(variable, ncOutputView);
		}
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
	sci::string getFileName() const
	{
		return m_filename;
	}
private:
	template<class TIMES_GRID, class LAT_GRID, class LON_GRID>
		requires (sci::IsGridDims<TIMES_GRID, 1>&&
	sci::IsGridDims <LAT_GRID, 1>&&
		sci::IsGridDims <LON_GRID, 1>)
		void initialise(AmfVersion amfVersion,
			const sci::string& directory,
			const InstrumentInfo& instrumentInfo,
			const PersonInfo& author,
			const ProcessingSoftwareInfo& processingsoftwareInfo,
			const CalibrationInfo& calibrationInfo,
			const DataInfo& dataInfo,
			const ProjectInfo& projectInfo,
			const Platform& platform,
			sci::string title,
			TIMES_GRID times,
			LAT_GRID latitudes,
			LON_GRID longitudes,
			const std::vector<sci::NcDimension*>& nonTimeDimensions,
			sci::string comment,
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

	sci::string m_filename;

	sci::NcDimension m_timeDimension;
	std::unique_ptr<AmfNcTimeVariable> m_timeVariable;
	std::unique_ptr<AmfNcLatitudeVariable> m_latitudeVariable;
	std::unique_ptr<AmfNcLongitudeVariable> m_longitudeVariable;
	std::unique_ptr<AmfNcVariable<float>> m_dayOfYearVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_yearVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_monthVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_dayVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_hourVariable;
	std::unique_ptr<AmfNcVariable<int32_t>> m_minuteVariable;
	std::unique_ptr<AmfNcVariable<float>> m_secondVariable;

	//only used for moving platforms
	std::unique_ptr<AmfNcVariable<degreeF>> m_courseVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_orientationVariable;
	std::unique_ptr<AmfNcVariable<metrePerSecondF>> m_speedVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentPitchVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentPitchStdevVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentPitchMinVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentPitchMaxVariable;
	std::unique_ptr<AmfNcVariable<degreePerSecondF>> m_instrumentPitchRateVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentRollVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentRollStdevVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentRollMinVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentRollMaxVariable;
	std::unique_ptr<AmfNcVariable<degreePerSecondF>> m_instrumentRollRateVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentYawVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentYawStdevVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentYawMinVariable;
	std::unique_ptr<AmfNcVariable<degreeF>> m_instrumentYawMaxVariable;
	std::unique_ptr<AmfNcVariable<degreePerSecondF>> m_instrumentYawRateVariable;

	sci::GridData<sci::UtcTime, 1> m_times;
	sci::GridData<int, 1> m_years;
	sci::GridData<int, 1> m_months;
	sci::GridData<int, 1> m_dayOfMonths;
	sci::GridData<float, 1> m_dayOfYears;
	sci::GridData<int, 1> m_hours;
	sci::GridData<int, 1> m_minutes;
	sci::GridData<float, 1> m_seconds;
	sci::GridData<degreeF, 1> m_latitudes;
	sci::GridData<degreeF, 1> m_longitudes;
	sci::GridData<degreeF, 1> m_elevations;
	sci::GridData<degreeF, 1> m_elevationStdevs;
	sci::GridData<degreeF, 1> m_elevationMins;
	sci::GridData<degreeF, 1> m_elevationMaxs;
	sci::GridData<degreePerSecondF, 1> m_elevationRates;
	sci::GridData<degreeF, 1> m_azimuths;
	sci::GridData<degreeF, 1> m_azimuthStdevs;
	sci::GridData<degreeF, 1> m_azimuthMins;
	sci::GridData<degreeF, 1> m_azimuthMaxs;
	sci::GridData<degreePerSecondF, 1> m_azimuthRates;
	sci::GridData<degreeF, 1> m_rolls;
	sci::GridData<degreeF, 1> m_rollStdevs;
	sci::GridData<degreeF, 1> m_rollMins;
	sci::GridData<degreeF, 1> m_rollMaxs;
	sci::GridData<degreePerSecondF, 1> m_rollRates;
	sci::GridData<degreeF, 1> m_courses;
	sci::GridData<metrePerSecondF, 1> m_speeds;
	sci::GridData<degreeF, 1> m_headings;
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

template<sci::IsGrid DATA_GRID, sci::IsGrid FLAGS_GRID, class T>
void getMinMax(const DATA_GRID &data, const FLAGS_GRID &flags, T& min, T& max)
{
	static_assert(FLAGS_GRID::ndims <= DATA_GRID::ndims, "Cannot find the min/max of data when the flag has more dimensions than the data.");
	min = getDefault<T>();
	max = getDefault<T>();

	if (data.size() == 0)
		return;

	min = std::numeric_limits<T>::max();
	max = std::numeric_limits<T>::lowest();
	if constexpr (FLAGS_GRID::ndims == 0)
	{
		//flags is a scalar

		//if flags is zero, we have essentially flagged everything out
		//so do nothing, otherwise we have flagged everything in, so
		//find the min and max

		if (flags[{}])
		{
			for (auto const& d : data)
			{
				if (d == d)
				{
					min = std::min(min, T(d));
					max = std::max(max, T(d));
				}
			}
		}
	}
	else if constexpr (FLAGS_GRID::ndims == DATA_GRID::ndims)
	{
		//flags has the same number of dimensions as data, we can work through them both in sync

		auto iterData = data.begin();
		auto iterFlags = flags.begin();
		for (; iterData != data.end(); ++iterData, ++iterFlags)
		{
			if (*iterFlags ==1 && *iterData == *iterData)
			{
				min = std::min(min, (T)(*iterData));
				max = std::max(max, (T)(*iterData));
			}
		}
	}
	else
	{
		//flags has fewer dimensions than data, we have to work through flags slower than data
		const size_t dimsDiff = DATA_GRID::ndims - FLAGS_GRID::ndims;
		const size_t dimToWatch = DATA_GRID::ndims - 1 - dimsDiff;
		size_t stride = data.getStride<dimToWatch>();

		auto iterData = data.begin();
		auto iterFlags = flags.begin();
		size_t index = 0;
		for (; iterData != data.end(); ++iterData, ++index)
		{
			if (*iterFlags ==1 && *iterData == *iterData)
			{
				min = std::min(min, (T)(*iterData));
				max = std::max(max, (T)(*iterData));
			}
			if (index > 0 && index % stride == 0)
				++iterFlags;
		}
	}
	if (min == std::numeric_limits<T>::max() || min != min)
		min = getDefault<T>();
	if (max == std::numeric_limits<T>::lowest() || max != max)
		max = getDefault<T>();
}



template <class T>
class AmfNcVariable : public sci::NcVariable<T>
{
public:
	template<sci::IsGrid DATAGRID, sci::IsGrid FLAGSGRID>
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &standardName, const sci::string &units, const DATAGRID &data, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, const FLAGSGRID &flags)
		:sci::NcVariable<T>(name, ncFile, dimension)
	{
		if (data.size() > 0)
		{
			static_assert(std::is_same_v<FLAGSGRID::value_type, uint8_t>, "flags must be of uint8_t type.");
			T validMin;
			T validMax;
			getMinMax(data, flags, validMin, validMax);
			setAttributes(ncFile, longName, standardName, units, validMin, validMax, hasFillValue, coordinates, cellMethods);
		}
		else
		{
			setAttributesExceptMinMax(ncFile, longName, standardName, units, hasFillValue, coordinates, cellMethods);
		}
	}
	template<sci::IsGrid DATAGRID, sci::IsGrid FLAGSGRID>
	AmfNcVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &standardName, const sci::string &units, const DATAGRID& data, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, const FLAGSGRID& flags)
		:sci::NcVariable<T>(name, ncFile, dimensions)
	{
		if (data.size() > 0)
		{
			static_assert(std::is_same_v<FLAGSGRID::value_type, uint8_t>, "flags must be of uint8_t type.");
			T validMin;
			T validMax;
			getMinMax(data, flags, validMin, validMax);
			setAttributes(ncFile, longName, standardName, units, validMin, validMax, hasFillValue, coordinates, cellMethods);
		}
	}
	template<class U>
	static T transformForOutput(const U& val)
	{
		return val == val ? T(val) : OutputAmfNcFile::getFillValue<T>();
	}
private:
	void setAttributes(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &standardName, const sci::string &units, T validMin, T validMax, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods)
	{
		setAttributesExceptMinMax(ncFile, longName, standardName, units, hasFillValue, coordinates, cellMethods);
		setMinMaxAttributes(ncFile, validMin, validMax);
	}
	void setAttributesExceptMinMax(const sci::OutputNcFile& ncFile, const sci::string& longName, const sci::string& standardName, const sci::string& units, bool hasFillValue, const std::vector<sci::string>& coordinates, const std::vector<std::pair<sci::string, CellMethod>>& cellMethods)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
		sci::NcAttribute unitsAttribute(sU("units"), units);
		//sci::NcAttribute typeAttribute(sU("type"), OutputAmfNcFile::getTypeName<T>());
		sci::NcAttribute fillValueAttribute(sU("_FillValue"), OutputAmfNcFile::getFillValue<T>());
		sci::NcAttribute coordinatesAttribute(sU("coordinates"), getCoordinatesAttributeText(coordinates));
		sci::NcAttribute cellMethodsAttribute(sU("cell_methods"), getCellMethodsAttributeText(cellMethods));
		sci::NcVariable<T>::addAttribute(longNameAttribute, ncFile);
		sci::NcVariable<T>::addAttribute(unitsAttribute, ncFile);
		//addAttribute(typeAttribute, ncFile);
		if (standardName.length() > 0)
			sci::NcVariable<T>::addAttribute(standardNameAttribute, ncFile);
		if (hasFillValue)
			sci::NcVariable<T>::addAttribute(fillValueAttribute, ncFile);
		if (coordinates.size() > 0)
			sci::NcVariable<T>::addAttribute(coordinatesAttribute, ncFile);
		if (cellMethods.size() > 0)
			sci::NcVariable<T>::addAttribute(cellMethodsAttribute, ncFile);
	}
	void setMinMaxAttributes(const sci::OutputNcFile& ncFile, T validMin, T validMax)
	{
		sci::NcAttribute validMinAttribute(sU("valid_min"), validMin);
		sci::NcAttribute validMaxAttribute(sU("valid_max"), validMax);
		sci::NcVariable<T>::addAttribute(validMinAttribute, ncFile);
		sci::NcVariable<T>::addAttribute(validMaxAttribute, ncFile);
	}
};

class AmfNcFlagVariable : public sci::NcVariable<uint8_t>
{
public:
	AmfNcFlagVariable(sci::string name, const std::vector<std::pair<uint8_t, sci::string>> &flagDefinitions, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, sci::string longNameSuffix = sU(""))
		: sci::NcVariable<uint8_t>(name, ncFile, dimensions)
	{
		initialise(flagDefinitions, ncFile, longNameSuffix);
	}
	AmfNcFlagVariable(sci::string name, const std::vector<std::pair<uint8_t, sci::string>>& flagDefinitions, const sci::OutputNcFile& ncFile, sci::NcDimension& dimension, sci::string longNameSuffix = sU(""))
		: sci::NcVariable<uint8_t>(name, ncFile, dimension)
	{
		initialise(flagDefinitions, ncFile, longNameSuffix);
	}
	static uint8_t transformForOutput(const uint8_t& val)
	{
		return val;
	}
private:
	void initialise(const std::vector<std::pair<uint8_t, sci::string>>& flagDefinitions, const sci::OutputNcFile& ncFile, sci::string longNameSuffix)
	{
		//split the values and definitions into their own variables. The descriptions should have their
		//spaces replaces with underscores and be separated by spaces
		std::vector<uint8_t> flagValues;
		sci::stringstream flagDescriptions;
		for (size_t i = 0; i < flagDefinitions.size(); ++i)
		{
			if (i != 0)
			{
				flagDescriptions << sU(" ");
			}
			sci::string thisFlagDefinition = flagDefinitions[i].second;
			for (auto& c : thisFlagDefinition)
				if (c == sU(' '))
					c = sU('_');
			flagValues.push_back(flagDefinitions[i].first);
			flagDescriptions << thisFlagDefinition;
		}
		sci::string flagDescriptionsString = flagDescriptions.str();
		//Swap spaces for underscores and make lower case
		std::locale locale;
		for (size_t i = 0; i < flagDescriptionsString.length(); ++i)
		{
			if (flagDescriptionsString[i] == sU(' '))
				flagDescriptionsString[i] = sU('_');
			else
				flagDescriptionsString[i] = std::tolower(flagDescriptionsString[i], locale);
		}
		if (longNameSuffix.length() == 0)
			addAttribute(sci::NcAttribute(sU("long_name"), sU("Data Quality Flag")), ncFile);
		else
			addAttribute(sci::NcAttribute(sU("long_name"), sU("Data Quality Flag: ") + longNameSuffix), ncFile);
		addAttribute(sci::NcAttribute(sU("flag_values"), flagValues), ncFile);
		addAttribute(sci::NcAttribute(sU("flag_meanings"), flagDescriptionsString), ncFile);
		//addAttribute(sci::NcAttribute(sU("type"), OutputAmfNcFile::getTypeName<uint8_t>()), ncFile);
		addAttribute(sci::NcAttribute(sU("units"), sU("1")), ncFile);
	}
};

//create a partial specialization for any AmfNcVariable based on a sci::Physical value
template <class UNIT, class VALUE_TYPE>
class AmfNcVariable<sci::Physical<UNIT, VALUE_TYPE>> : public sci::NcVariable<VALUE_TYPE>
{
public:
	template<sci::IsGridDims<1> DATA_GRID, sci::IsGrid FLAGS_GRID>
	AmfNcVariable(const sci::string &name,
		const sci::OutputNcFile &ncFile,
		const sci::NcDimension &dimension,
		const sci::string &longName,
		const sci::string &standardName,
		DATA_GRID data,
		bool hasFillValue,
		const std::vector<sci::string> &coordinates,
		const std::vector<std::pair<sci::string, CellMethod>> &cellMethods,
		FLAGS_GRID flags,
		const sci::string &comment = sU(""))
		:sci::NcVariable<VALUE_TYPE>(name, ncFile, dimension)
	{
		if (data.size() > 0)
		{
			sci::Physical<UNIT, VALUE_TYPE> validMin;
			sci::Physical<UNIT, VALUE_TYPE> validMax;
			getMinMax(data, flags, validMin, validMax);
			if (validMin != validMin) //this indicates all data was flagged out. If this happens, retry but using all non-nan data
				getMinMax(data, sci::GridData<unsigned char, 0>(1), validMin, validMax);
			setAttributes(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
		}
		else
			setAttributesExcludingMinMax(ncFile, longName, standardName, hasFillValue, coordinates, cellMethods, comment);
	}
	template<sci::IsGrid DATA_GRID, sci::IsGrid FLAGS_GRID>
	AmfNcVariable(const sci::string &name,
		const sci::OutputNcFile &ncFile,
		const std::vector<sci::NcDimension *> &dimensions,
		const sci::string &longName,
		const sci::string &standardName,
		const DATA_GRID &data,
		bool hasFillValue,
		const std::vector<sci::string> &coordinates,
		const std::vector<std::pair<sci::string, CellMethod>> &cellMethods,
		const FLAGS_GRID &flags,
		const sci::string &comment = sU(""))
		:sci::NcVariable<VALUE_TYPE>(name, ncFile, dimensions)
	{
		sci::Physical<UNIT, VALUE_TYPE> validMin;
		sci::Physical<UNIT, VALUE_TYPE> validMax;
		getMinMax(data, flags, validMin, validMax);
		if (validMin != validMin) //this indicates all data was flagged out. If this happens, retry but using all non-nan data
			getMinMax(data, sci::GridData<unsigned char, 0>(1), validMin, validMax);
		setAttributes(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
	}
	template<class U>
	static VALUE_TYPE transformForOutput(const U& val)
	{
		return val == val ? val.value<UNIT>() : OutputAmfNcFile::getFillValue<VALUE_TYPE>();
	}

private:
	void setAttributes(const sci::OutputNcFile& ncFile, const sci::string& longName, const sci::string& standardName, sci::Physical<UNIT, VALUE_TYPE> validMin, sci::Physical<UNIT, VALUE_TYPE> validMax, bool hasFillValue, const std::vector<sci::string>& coordinates, const std::vector<std::pair<sci::string, CellMethod>>& cellMethods, const sci::string& comment)
	{
		setAttributesExcludingMinMax(ncFile, longName, standardName, hasFillValue, coordinates, cellMethods, comment);
		setMinMax(ncFile, validMin, validMax);
	}
	void setAttributesExcludingMinMax(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &standardName, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, const sci::string &comment)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
		//the unit needs a little editing to comply with AMF standard
		sci::string unit = sci::Physical<UNIT, VALUE_TYPE>::template getShortUnitString<sci::string>();
		//replace blank string (unitless) with 1
		if (unit == sU(""))
			unit = sU("1");
		//replace degree symbol with the word degree
#if defined UNITS_H_USING_CP1253
		while (unit.find_first_of(sU('\xf8')) != sci::string::npos)
		{
			unit.replace(unit.find_first_of(sU('\u00b0')), 1, sU("degree"), 0, sci::string::npos);
		}
#else
		while (unit.find_first_of(sU('\u00b0')) != sci::string::npos)
		{
			unit.replace(unit.find_first_of(sU('\u00b0')), 1, sU("degree"), 0, sci::string::npos);
		}
#endif
		sci::NcAttribute unitsAttribute(sU("units"), unit);
		//sci::NcAttribute typeAttribute(sU("type"), OutputAmfNcFile::getTypeName<sci::Physical<T,VALUE_TYPE>>());
		sci::NcAttribute fillValueAttribute(sU("_FillValue"), OutputAmfNcFile::getFillValue<VALUE_TYPE>());
		sci::NcAttribute coordinatesAttribute(sU("coordinates"), getCoordinatesAttributeText(coordinates));
		sci::NcAttribute cellMethodsAttribute(sU("cell_methods"), getCellMethodsAttributeText(cellMethods));
		sci::NcAttribute commentAttribute(sU("comment"), comment);
		sci::NcVariable< VALUE_TYPE>::addAttribute(longNameAttribute, ncFile);
		sci::NcVariable<VALUE_TYPE>::addAttribute(unitsAttribute, ncFile);
		//sci::NcVariable<VALUE_TYPE>::addAttribute(typeAttribute, ncFile);
		if (comment.length() > 0)
			sci::NcVariable<VALUE_TYPE>::addAttribute(commentAttribute, ncFile);
		if (standardName.length() > 0)
			sci::NcVariable<VALUE_TYPE>::addAttribute(standardNameAttribute, ncFile);
		if (hasFillValue)
			sci::NcVariable<VALUE_TYPE>::addAttribute(fillValueAttribute, ncFile);
		if (coordinates.size() > 0)
			sci::NcVariable<VALUE_TYPE>::addAttribute(coordinatesAttribute, ncFile);
		if(cellMethods.size() > 0)
			sci::NcVariable<VALUE_TYPE>::addAttribute(cellMethodsAttribute, ncFile);
	}
	void setMinMax(const sci::OutputNcFile& ncFile, sci::Physical<UNIT, VALUE_TYPE> validMin, sci::Physical<UNIT, VALUE_TYPE> validMax)
	{
		sci::NcAttribute validMinAttribute(sU("valid_min"), validMin.value<UNIT>());
		sci::NcAttribute validMaxAttribute(sU("valid_max"), validMax.value<UNIT>());
		sci::NcVariable<VALUE_TYPE>::addAttribute(validMinAttribute, ncFile);
		sci::NcVariable<VALUE_TYPE>::addAttribute(validMaxAttribute, ncFile);
	}
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
	/*template<class UNIT>
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
	}*/
	template<class U>
	static valueType transformForOutput(const U& val)
	{
		return val == val ? Decibel<REFERENCE_UNIT>::linearToDecibel(val).value<unitlessF>() : OutputAmfNcFile::getFillValue<valueType>();
	}
};

//logarithmic variables need to inherit from the Decibel specialization above so that the data gets converted to
//dB during the flattenData call
//REFERENCE_UNIT is the sci::Unit that is the reference unit for the dB
//U is the data type - it will be a 1d or multi-d vector of Physicals
template < class REFERENCE_UNIT>
class AmfNcDbVariable : public sci::NcVariable<unitlessF::valueType>
{
public:

	//typedef typename Decibel<REFERENCE_UNIT>::referencePhysical::valueType valueType;
	typedef typename Decibel<REFERENCE_UNIT>::referencePhysical referencePhysical;
	//Data must be passed in in linear units, not decibels!!!
	template<size_t NDATADIMS, size_t NFLAGDIMS>
	AmfNcDbVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const sci::NcDimension &dimension, const sci::string &longName, const sci::string &standardName, sci::grid_view< REFERENCE_UNIT, NDATADIMS> data, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, bool isDbZ, bool outputReferenceUnit, bool isLinearData, sci::grid_view< uint8_t, NFLAGDIMS> flags, const sci::string &comment = sU(""))
		:sci::NcVariable<unitlessF::valueType>(name, ncFile, dimension)
	{
		m_isDbZ = isDbZ;
		m_outputReferenceUnit = outputReferenceUnit;
		referencePhysical validMin;
		referencePhysical validMax;
		getMinMax(data, flags, validMin, validMax);
		if(isLinearData)
			setAttributesLinearData(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
		else
			setAttributesLogarithmicData(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
	}
	template<sci::IsGrid DATA_GRID, sci::IsGrid FLAGS_GRID>
	AmfNcDbVariable(const sci::string &name, const sci::OutputNcFile &ncFile, const std::vector<sci::NcDimension *> &dimensions, const sci::string &longName, const sci::string &standardName, DATA_GRID data, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, bool isDbZ, bool outputReferenceUnit, bool isLinearData, FLAGS_GRID flags, const sci::string &comment = sU(""))
		:sci::NcVariable<unitlessF::valueType>(name, ncFile, dimensions)
	{
		m_isDbZ = isDbZ;
		m_outputReferenceUnit = outputReferenceUnit;
		typename DATA_GRID::value_type validMin;
		typename DATA_GRID::value_type validMax;
		getMinMax(data, flags, validMin, validMax);
		if (isLinearData)
		{
			if constexpr (DATA_GRID::value_type::template compatibleWith<referencePhysical>())
				setAttributesLinearData(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
			else
				sci::assertThrow(false, sci::err(sci::SERR_USER, 0, sU("Passed an incompatible type as linear data into a AmfNcDbVariable constructor")));
		}
		else
		{
			if constexpr (DATA_GRID::value_type::unit::isUnitless())
				setAttributesLogarithmicData(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);
			else
				sci::assertThrow(false, sci::err(sci::SERR_USER, 0, sU("Passed a non-unitless type as logarithmic data into a AmfNcDbVariable constructor")));
		}
	}
	//template<size_t NDATADIMS, size_t NFLAGDIMS>
	AmfNcDbVariable(const sci::string& name, const sci::OutputNcFile& ncFile, const std::vector<sci::NcDimension*>& dimensions, const sci::string& longName, const sci::string& standardName)
		:sci::NcVariable<unitlessF::valueType>(name, ncFile, dimensions)
	{
		/*m_isDbZ = isDbZ;
		m_outputReferenceUnit = outputReferenceUnit;
		referencePhysical validMin;
		referencePhysical validMax;
		getMinMax(dataLinear, flags, validMin, validMax);
		setAttributes(ncFile, longName, standardName, validMin, validMax, hasFillValue, coordinates, cellMethods, comment);*/
	}
private:
	void setAttributesLinearData(const sci::OutputNcFile &ncFile, const sci::string &longName, const sci::string &standardName, referencePhysical validMinLinear, referencePhysical validMaxLinear, bool hasFillValue, const std::vector<sci::string> &coordinates, const std::vector<std::pair<sci::string, CellMethod>> &cellMethods, const sci::string &comment)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
		sci::NcAttribute unitsAttribute(sU("units"), m_isDbZ ? sU("dBZ") : sU("dB"));
		sci::string referenceUnit = referencePhysical::template getShortUnitString<sci::string>();
		if (referenceUnit.length() == 0)
			referenceUnit = sU("1");
		sci::NcAttribute referenceUnitAttribute(sU("reference_unit"), referenceUnit);
		sci::NcAttribute validMinAttribute(sU("valid_min"), Decibel<REFERENCE_UNIT>::linearToDecibel(validMinLinear));
		sci::NcAttribute validMaxAttribute(sU("valid_max"), Decibel<REFERENCE_UNIT>::linearToDecibel(validMaxLinear));
		//sci::NcAttribute typeAttribute(sU("type"), OutputAmfNcFile::getTypeName<Decibel<REFERENCE_UNIT>::valueType>());
		sci::NcAttribute fillValueAttribute(sU("_FillValue"), OutputAmfNcFile::getFillValue< Decibel<REFERENCE_UNIT>::valueType>());
		sci::NcAttribute coordinatesAttribute(sU("coordinates"), getCoordinatesAttributeText(coordinates));
		sci::NcAttribute cellMethodsAttribute(sU("cell_methods"), getCellMethodsAttributeText(cellMethods));
		sci::NcAttribute commentAttribute(sU("comment"), comment);
		sci::NcVariable<unitlessF::valueType>::addAttribute(longNameAttribute, ncFile);
		sci::NcVariable<unitlessF::valueType>::addAttribute(unitsAttribute, ncFile);
		if (m_outputReferenceUnit)
			sci::NcVariable<unitlessF::valueType>::addAttribute(referenceUnitAttribute, ncFile);
		sci::NcVariable<unitlessF::valueType>::addAttribute(validMinAttribute, ncFile);
		sci::NcVariable<unitlessF::valueType>::addAttribute(validMaxAttribute, ncFile);
		//addAttribute(typeAttribute, ncFile);
		if (comment.length() > 0)
			sci::NcVariable<unitlessF::valueType>::addAttribute(commentAttribute, ncFile);
		if (standardName.length() > 0)
			sci::NcVariable<unitlessF::valueType>::addAttribute(standardNameAttribute, ncFile);
		if (hasFillValue)
			sci::NcVariable<unitlessF::valueType>::addAttribute(fillValueAttribute, ncFile);
		if (coordinates.size() > 0)
			sci::NcVariable<unitlessF::valueType>::addAttribute(coordinatesAttribute, ncFile);
		if(cellMethods.size() > 0)
			sci::NcVariable<unitlessF::valueType>::addAttribute(cellMethodsAttribute, ncFile);
	}
	void setAttributesLogarithmicData(const sci::OutputNcFile& ncFile, const sci::string& longName, const sci::string& standardName, unitlessF validMinLogarithmic, unitlessF validMaxLogarithmic, bool hasFillValue, const std::vector<sci::string>& coordinates, const std::vector<std::pair<sci::string, CellMethod>>& cellMethods, const sci::string& comment)
	{
		sci::NcAttribute longNameAttribute(sU("long_name"), longName);
		sci::NcAttribute standardNameAttribute(sU("standard_name"), standardName);
		sci::NcAttribute unitsAttribute(sU("units"), m_isDbZ ? sU("dBZ") : sU("dB"));
		sci::string referenceUnit = referencePhysical::template getShortUnitString<sci::string>();
		if (referenceUnit.length() == 0)
			referenceUnit = sU("1");
		sci::NcAttribute referenceUnitAttribute(sU("reference_unit"), referenceUnit);
		sci::NcAttribute validMinAttribute(sU("valid_min"), validMinLogarithmic.value<unitlessF>());
		sci::NcAttribute validMaxAttribute(sU("valid_max"), validMaxLogarithmic.value<unitlessF>());
		//sci::NcAttribute typeAttribute(sU("type"), OutputAmfNcFile::getTypeName<Decibel<REFERENCE_UNIT>::valueType>());
		sci::NcAttribute fillValueAttribute(sU("_FillValue"), OutputAmfNcFile::getFillValue< Decibel<REFERENCE_UNIT>::valueType>());
		sci::NcAttribute coordinatesAttribute(sU("coordinates"), getCoordinatesAttributeText(coordinates));
		sci::NcAttribute cellMethodsAttribute(sU("cell_methods"), getCellMethodsAttributeText(cellMethods));
		sci::NcAttribute commentAttribute(sU("comment"), comment);
		sci::NcVariable<unitlessF::valueType>::addAttribute(longNameAttribute, ncFile);
		sci::NcVariable<unitlessF::valueType>::addAttribute(unitsAttribute, ncFile);
		if (m_outputReferenceUnit)
			sci::NcVariable<unitlessF::valueType>::addAttribute(referenceUnitAttribute, ncFile);
		sci::NcVariable<unitlessF::valueType>::addAttribute(validMinAttribute, ncFile);
		sci::NcVariable<unitlessF::valueType>::addAttribute(validMaxAttribute, ncFile);
		//addAttribute(typeAttribute, ncFile);
		if (comment.length() > 0)
			sci::NcVariable<unitlessF::valueType>::addAttribute(commentAttribute, ncFile);
		if (standardName.length() > 0)
			sci::NcVariable<unitlessF::valueType>::addAttribute(standardNameAttribute, ncFile);
		if (hasFillValue)
			sci::NcVariable<unitlessF::valueType>::addAttribute(fillValueAttribute, ncFile);
		if (coordinates.size() > 0)
			sci::NcVariable<unitlessF::valueType>::addAttribute(coordinatesAttribute, ncFile);
		if (cellMethods.size() > 0)
			sci::NcVariable<unitlessF::valueType>::addAttribute(cellMethodsAttribute, ncFile);
	}

	bool m_isDbZ;
	bool m_outputReferenceUnit;
};

template < class REFERENCE_UNIT>
class AmfNcDbVariableFromLinearData : public AmfNcDbVariable<REFERENCE_UNIT>
{
public:
	template<size_t NDATADIMS, size_t NFLAGDIMS>
	AmfNcDbVariableFromLinearData(const sci::string& name, const sci::OutputNcFile& ncFile, const sci::NcDimension& dimension, const sci::string& longName, const sci::string& standardName, sci::grid_view< REFERENCE_UNIT, NDATADIMS> dataLinear, bool hasFillValue, const std::vector<sci::string>& coordinates, const std::vector<std::pair<sci::string, CellMethod>>& cellMethods, bool isDbZ, bool outputReferenceUnit, sci::grid_view< uint8_t, NFLAGDIMS> flags, const sci::string& comment = sU(""))
		:AmfNcDbVariable<REFERENCE_UNIT>(name, ncFile, dimension, longName, standardName, dataLinear, hasFillValue, coordinates, cellMethods, isDbZ, outputReferenceUnit, true, flags, comment)
	{
	}
	template<sci::IsGrid DATA_GRID, sci::IsGrid FLAGS_GRID>
	AmfNcDbVariableFromLinearData(const sci::string& name, const sci::OutputNcFile& ncFile, const std::vector<sci::NcDimension*>& dimensions, const sci::string& longName, const sci::string& standardName, DATA_GRID dataLinear, bool hasFillValue, const std::vector<sci::string>& coordinates, const std::vector<std::pair<sci::string, CellMethod>>& cellMethods, bool isDbZ, bool outputReferenceUnit, FLAGS_GRID flags, const sci::string& comment = sU(""))
		: AmfNcDbVariable<REFERENCE_UNIT>(name, ncFile, dimensions, longName, standardName, dataLinear, hasFillValue, coordinates, cellMethods, isDbZ, outputReferenceUnit, true, flags, comment)
	{
	}
	AmfNcDbVariableFromLinearData(const sci::string& name, const sci::OutputNcFile& ncFile, const std::vector<sci::NcDimension*>& dimensions, const sci::string& longName, const sci::string& standardName)
		:AmfNcDbVariable<REFERENCE_UNIT>(name, ncFile, dimensions, longName, standardName)
	{
	}
	template <class DATA_TYPE>
	static DATA_TYPE::valueType transformForOutput(const DATA_TYPE& val)
	{
		return val == val ? Decibel<REFERENCE_UNIT>::linearToDecibel(val).value<sci::Unitless>() : OutputAmfNcFile::getFillValue<DATA_TYPE::valueType>();
	}
};

template < class REFERENCE_UNIT>
class AmfNcDbVariableFromLogarithmicData : public AmfNcDbVariable<REFERENCE_UNIT>
{
public:
	template<size_t NDATADIMS, size_t NFLAGDIMS>
	AmfNcDbVariableFromLogarithmicData(const sci::string& name, const sci::OutputNcFile& ncFile, const sci::NcDimension& dimension, const sci::string& longName, const sci::string& standardName, sci::grid_view< unitlessF, NDATADIMS> dataLogarithmic, bool hasFillValue, const std::vector<sci::string>& coordinates, const std::vector<std::pair<sci::string, CellMethod>>& cellMethods, bool isDbZ, bool outputReferenceUnit, sci::grid_view< uint8_t, NFLAGDIMS> flags, const sci::string& comment = sU(""))
		:AmfNcDbVariable<REFERENCE_UNIT>(name, ncFile, dimension, longName, standardName, dataLogarithmic, hasFillValue, coordinates, cellMethods, isDbZ, outputReferenceUnit, false, flags, comment)
	{
	}
	template<sci::IsGrid DATA_GRID, sci::IsGrid FLAGS_GRID>
	AmfNcDbVariableFromLogarithmicData(const sci::string& name, const sci::OutputNcFile& ncFile, const std::vector<sci::NcDimension*>& dimensions, const sci::string& longName, const sci::string& standardName, DATA_GRID dataLogarithmic, bool hasFillValue, const std::vector<sci::string>& coordinates, const std::vector<std::pair<sci::string, CellMethod>>& cellMethods, bool isDbZ, bool outputReferenceUnit, FLAGS_GRID flags, const sci::string& comment = sU(""))
		: AmfNcDbVariable<REFERENCE_UNIT>(name, ncFile, dimensions, longName, standardName, dataLogarithmic, hasFillValue, coordinates, cellMethods, isDbZ, outputReferenceUnit, false, flags, comment)
	{
	}
	AmfNcDbVariableFromLogarithmicData(const sci::string& name, const sci::OutputNcFile& ncFile, const std::vector<sci::NcDimension*>& dimensions, const sci::string& longName, const sci::string& standardName)
		:AmfNcDbVariable<REFERENCE_UNIT>(name, ncFile, dimensions, longName, standardName)
	{
	}
	static unitlessF::valueType transformForOutput(const unitlessF& val)
	{
		return val == val ? val.value<unitlessF>() : OutputAmfNcFile::getFillValue<unitlessF::valueType>();
	}
};

class AmfNcTimeVariable : public AmfNcVariable<typename second::valueType>
{
public:
	template<sci::IsGrid TIMES_GRID>
	requires (sci::IsGridDims<TIMES_GRID, 1>)
	AmfNcTimeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, TIMES_GRID &times)
		:AmfNcVariable<typename second::valueType> (
			sU("time"),
			ncFile,
			dimension,
			sU("Time (seconds since 1970-01-01 00:00:00)"),
			sU("time"),
			sU("seconds since 1970-01-01 00:00:00"),
			sci::make_gridtransform_view(times - sci::UtcTime(1970, 1, 1, 0, 0, 0),PhysicalToValueTransform<second>),
			false,
			std::vector<sci::string>(0),
			std::vector<std::pair<sci::string, CellMethod>>(0),
			sci::GridData<uint8_t, 0>(1))
	{
		static_assert(TIMES_GRID::ndims == 1, "The time variable must be 1-dimensional");
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
	static second::valueType transformForOutput(const second& val)
	{
		return val == val ? val.value<second>() : OutputAmfNcFile::getFillValue<second::valueType>();
	}
};

std::vector<std::pair<sci::string, CellMethod>> getLatLonCellMethod(FeatureType featureType, DeploymentMode deploymentMode);

class AmfNcLongitudeVariable : public AmfNcVariable<typename degreeF::valueType>
{
public:
	template<sci::IsGrid GRID>
	AmfNcLongitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, const GRID &longitudes, FeatureType featureType, DeploymentMode deploymentMode, PlatformType platformType)
		:AmfNcVariable<typename degreeF::valueType>(
			sU("longitude"),
			ncFile, 
			dimension,
			sU("Longitude"),
			sU("longitude"),
			sU("degrees_east"),
			platformType == PlatformType::stationary ? sci::GridData<degreeF::valueType,1>(0) : sci::make_gridtransform_view(longitudes, PhysicalToValueTransform<degreeF>),
			platformType != PlatformType::stationary,
			std::vector<sci::string>(0),
			getLatLonCellMethod(featureType, deploymentMode),
			sci::GridData<uint8_t,0>(1))
			
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("X")), ncFile);
	}
	static degreeF::valueType transformForOutput(const degreeF& val)
	{
		return val == val ? val.value<degreeF>() : OutputAmfNcFile::getFillValue<degreeF::valueType>();
	}
};
class AmfNcLatitudeVariable : public AmfNcVariable<typename degreeF::valueType>
{
public:
	template<sci::IsGrid GRID>
	AmfNcLatitudeVariable(const sci::OutputNcFile& ncFile, const sci::NcDimension& dimension, const GRID& latitudes, FeatureType featureType, DeploymentMode deploymentMode, PlatformType platformType)
		:AmfNcVariable<typename degreeF::valueType>(
			sU("latitude"),
			ncFile,
			dimension,
			sU("Latitude"),
			sU("latitude"),
			sU("degrees_north"),
			platformType == PlatformType::stationary ? sci::GridData<degreeF::valueType, 1>(0) : sci::make_gridtransform_view(latitudes, PhysicalToValueTransform<degreeF>),
			platformType != PlatformType::stationary,
			std::vector<sci::string>(0),
			getLatLonCellMethod(featureType, deploymentMode),
			sci::GridData<uint8_t, 0>(1))
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("Y")), ncFile);
	}
	static degreeF::valueType transformForOutput(const degreeF& val)
	{
		return val == val ? val.value<degreeF>() : OutputAmfNcFile::getFillValue<degreeF::valueType>();
	}
};

class AmfNcAltitudeVariable : public AmfNcVariable<typename metreF::valueType>
{
public:
	template<sci::IsGrid GRID>
	AmfNcAltitudeVariable(const sci::OutputNcFile &ncFile, const sci::NcDimension& dimension, const GRID &altitudes, FeatureType featureType)
		:AmfNcVariable<typename metreF::valueType>(
			sU("altitude"),
			ncFile,
			dimension,
			sU("Geometric height above geoid (WGS84)"),
			sU("altitude"),
			sU("m"),
			sci::make_gridtransform_view(altitudes, PhysicalToValueTransform<metreF>),
			true,
			std::vector<sci::string>(0),
			featureType == FeatureType::trajectory ? std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}} : std::vector<std::pair<sci::string, CellMethod>>(0).
			sci::GridData<uint8_t, 0>(1))
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), ncFile);
	}
	template<sci::IsGrid GRID>
	AmfNcAltitudeVariable(const sci::OutputNcFile& ncFile, const std::vector<sci::NcDimension*>& dimensions, const GRID& altitudes, FeatureType featureType)
		:AmfNcVariable<typename metreF::valueType>(
			sU("altitude"),
			ncFile,
			dimensions,
			sU("Geometric height above geoid (WGS84)"),
			sU("altitude"),
			sU("m"),
			sci::make_gridtransform_view(altitudes, PhysicalToValueTransform<metreF>),
			true,
			std::vector<sci::string>(0),
			featureType == FeatureType::trajectory ? std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}} : std::vector<std::pair<sci::string, CellMethod>>(0),
			sci::GridData<uint8_t, 0>(1))
	{
		addAttribute(sci::NcAttribute(sU("axis"), sU("Z")), ncFile);
	}
	static metreF::valueType transformForOutput(const metreF& val)
	{
		return val == val ? val.value<metreF>() : OutputAmfNcFile::getFillValue<metreF::valueType>();
	}
};











template<class TIMES_GRID, class LAT_GRID, class LON_GRID>
	requires (sci::IsGridDims<TIMES_GRID, 1>&&
sci::IsGridDims <LAT_GRID, 1>&&
sci::IsGridDims <LON_GRID, 1>)
void OutputAmfNcFile::initialise(AmfVersion amfVersion,
	const sci::string& directory,
	const InstrumentInfo& instrumentInfo,
	const PersonInfo& author,
	const ProcessingSoftwareInfo& processingsoftwareInfo,
	const CalibrationInfo& calibrationInfo,
	const DataInfo& dataInfo,
	const ProjectInfo& projectInfo,
	const Platform& platform,
	sci::string title,
	TIMES_GRID times,
	LAT_GRID latitudes,
	LON_GRID longitudes,
	const std::vector<sci::NcDimension*>& nonTimeDimensions,
	sci::string comment,
	const std::vector< sci::NcAttribute*>& globalAttributes,
	bool incrementMajorVersion)
{
	sci::assertThrow(latitudes.size() == longitudes.size(), sci::err(sci::SERR_USER, 0, sU("Cannot create a netcdf file with latitude data a different length to longitude data.")));
	sci::assertThrow(latitudes.size() == 0 || latitudes.size() == times.size(), sci::err(sci::SERR_USER, 0, sU("Cannot create a netcdf with latitude and longitude data a different length to time data.")));
	//construct the position and time variables;
	m_years.reshape(m_times.shape());
	m_months.reshape(m_times.shape());
	m_dayOfMonths.reshape(m_times.shape());
	m_dayOfYears.reshape(m_times.shape());
	m_hours.reshape(m_times.shape());
	m_minutes.reshape(m_times.shape());
	m_seconds.reshape(m_times.shape());
	for (size_t i = 0; i < times.size(); ++i)
	{
		m_years[i] = times[i].getYear();
		m_months[i] = times[i].getMonth();
		m_dayOfMonths[i] = times[i].getDayOfMonth();
		m_dayOfYears[i] = (float)std::floor(((sci::UtcTime((int)m_years[i], (unsigned int)m_months[i], (unsigned int)m_dayOfMonths[i], 0, 0, 0) - sci::UtcTime((int)m_years[i], 1, 1, 0, 0, 0)) / secondF(60.0 * 60.0 * 24.0)).value<unitlessF>());
		m_hours[i] = times[i].getHour();
		m_minutes[i] = times[i].getMinute();
		m_seconds[i] = (float)times[i].getSecond();
	}
	//sort out the position and attitude data
	bool overriddenPlatformPosition = false;
	if (latitudes.size() > 0)
	{
		//This means we are overriding the platform position data with instrument position data
		//This would be the case for e.g. sondes
		m_longitudes = longitudes;
		m_latitudes = latitudes;
		overriddenPlatformPosition = true;
	}
	else
	{
		//This means we didn't override the platform position
		if (platform.getPlatformInfo().platformType == PlatformType::stationary)
		{
			degreeF longitude = std::numeric_limits<degreeF>::quiet_NaN();
			degreeF latitude = std::numeric_limits<degreeF>::quiet_NaN();
			metreF altitude = std::numeric_limits<metreF>::quiet_NaN();
			platform.getLocation(sci::UtcTime(), sci::UtcTime(), latitude, longitude, altitude); //the times are ignored for stationary platforms - using a fixed time avoids an error if times has zero size
			m_longitudes = { longitude };
			m_latitudes = { latitude };
		}
		else
		{
			m_longitudes.reshape(m_times.shape(), degreeF(0));
			m_latitudes.reshape(m_times.shape(), degreeF(0));
			m_elevations.reshape(m_times.shape(), degreeF(0));
			m_elevationStdevs.reshape(m_times.shape(), degreeF(0));
			m_elevationMins.reshape(m_times.shape(), degreeF(0));
			m_elevationMaxs.reshape(m_times.shape(), degreeF(0));
			m_elevationRates.reshape(m_times.shape(), degreePerSecondF(0));
			m_azimuths.reshape(m_times.shape(), degreeF(0));
			m_azimuthStdevs.reshape(m_times.shape(), degreeF(0));
			m_azimuthMins.reshape(m_times.shape(), degreeF(0));
			m_azimuthMaxs.reshape(m_times.shape(), degreeF(0));
			m_azimuthRates.reshape(m_times.shape(), degreePerSecondF(0));
			m_rolls.reshape(m_times.shape(), degreeF(0));
			m_rollStdevs.reshape(m_times.shape(), degreeF(0));
			m_rollMins.reshape(m_times.shape(), degreeF(0));
			m_rollMaxs.reshape(m_times.shape(), degreeF(0));
			m_rollRates.reshape(m_times.shape(), degreePerSecondF(0));
			m_courses.reshape(m_times.shape(), degreeF(0));
			m_speeds.reshape(m_times.shape(), metrePerSecondF(0));
			m_headings.reshape(m_times.shape(), degreeF(0));
			metreF altitude;
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
	}

	//construct the file path, without the version number of suffix
	sci::string baseFilename = directory;
	//append a directory separator if needed
	if (baseFilename.length() > 0 && baseFilename.back() != sU('\\') && baseFilename.back() != sU('/'))
	{
		baseFilename = baseFilename + sU('/');
	}

	//add the instument name
	baseFilename = baseFilename + instrumentInfo.name;
	//add the platform if there is one
	if (platform.getPlatformInfo().name.length() > 0)
		baseFilename = baseFilename + sU("_") + platform.getPlatformInfo().name;
	//add either the date or date and time depending upon whether the data is
	//continuous or a single measurement
	if (dataInfo.continuous)
		baseFilename = baseFilename + sU("_") + getFormattedDateOnly(dataInfo.startTime, sU(""));
	else
		baseFilename = baseFilename + sU("_") + getFormattedDateTime(dataInfo.startTime, sU(""), sU(""), sU("-"));
	//add the data product name
	baseFilename = baseFilename + sU("_") + dataInfo.productName;
	//add any options
	for (size_t i = 0; i < dataInfo.options.size(); ++i)
		if (dataInfo.options[i].length() > 0)
			baseFilename = baseFilename + sU("_") + dataInfo.options[i];

	//Swap spaces for hyphens and make lower case
	std::locale locale;
	size_t replaceStart = baseFilename.find_last_of(sU("\\/"));
	if (replaceStart == sci::string::npos)
		replaceStart = 0;
	else
		++replaceStart;
	for (size_t i = replaceStart; i < baseFilename.length(); ++i)
	{
		if (baseFilename[i] == sU(' '))
			baseFilename[i] = sU('-');
		else
			baseFilename[i] = std::tolower(baseFilename[i], locale);
	}

	//construct the title for the dataset if one was not provided
	if (title.length() == 0)
	{
		if (baseFilename.find_last_of(sU("\\/")) != sci::string::npos)
			title = baseFilename.substr(baseFilename.find_last_of(sU("\\/")) + 1);
		else
			title = baseFilename;
	}

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
	size_t prevMajorVersion = 0;
	size_t prevMinorVersion = 0;
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

				int thisPrevMajorVersion;
				int thisPrevMinorVersion;
				//remove the v from the beginning of the version string
				previousVersionString = previousVersionString.substr(1);
				//replace the . with a space
				if (previousVersionString.find_first_of(sU('.')) != sci::string::npos)
				{
					previousVersionString[previousVersionString.find_first_of(sU('.'))] = sU(' ');

					//replace the b in beta (if it is there) with a space
					if (previousVersionString.find_first_of(sU('b')) != sci::string::npos)
					{
						previousVersionString[previousVersionString.find_first_of(sU('b'))] = sU(' ');
					}
				}
				else
					previousVersionString = previousVersionString + sU(" 0"); //if the minor version isn't there, then add it

				//put the string into a stream and read out the major and minor versions
				sci::istringstream versionStream(previousVersionString);
				versionStream >> thisPrevMajorVersion;
				versionStream >> thisPrevMinorVersion;

				if (thisPrevMajorVersion > prevMajorVersion || (thisPrevMajorVersion == prevMajorVersion && thisPrevMinorVersion > prevMinorVersion))
				{
					prevMajorVersion = thisPrevMajorVersion;
					prevMinorVersion = thisPrevMinorVersion;
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

	size_t majorVersion;
	size_t minorVersion;
	if (incrementMajorVersion || prevMajorVersion == 0)
	{
		majorVersion = prevMajorVersion + 1;
		minorVersion = 0;
	}
	else
	{
		majorVersion = prevMajorVersion;
		minorVersion = prevMinorVersion + 1;
	}

	sci::ostringstream versionString;
	versionString << sU("v") << majorVersion << sU(".") << minorVersion << (dataInfo.processingOptions.beta ? sU("beta") : sU(""));

	//Now construct the final filename
	sci::ostringstream filenameStream;
	filenameStream << baseFilename << sU("_") << versionString.str() << sU(".nc");
	m_filename = filenameStream.str();

	//create the new file
	openWritable(m_filename, '_');
	sci::string amfVersionString;
	if (amfVersion == AmfVersion::v1_1_0)
		amfVersionString = sU("1.1.0");
	else if (amfVersion == AmfVersion::v2_0_0)
		amfVersionString = sU("2.0.0");
	else
		throw(sci::err(sci::SERR_USER, 0, sU("Attempting to write a netcdf file with an unknown standard version.")));

	write(sci::NcAttribute(sU("Conventions"), sci::string(sU("CF-1.6, NCAS-AMF-")) + amfVersionString));
	write(sci::NcAttribute(sU("source"), instrumentInfo.description.length() > 0 ? instrumentInfo.description : sU("not available")));
	write(sci::NcAttribute(sU("instrument_manufacturer"), instrumentInfo.manufacturer.length() > 0 ? instrumentInfo.manufacturer : sU("not available")));
	write(sci::NcAttribute(sU("instrument_model"), instrumentInfo.model.length() > 0 ? instrumentInfo.model : sU("not available")));
	write(sci::NcAttribute(sU("instrument_serial_number"), instrumentInfo.serial.length() > 0 ? instrumentInfo.serial : sU("not available")));
	write(sci::NcAttribute(sU("instrument_software"), instrumentInfo.operatingSoftware.length() > 0 ? instrumentInfo.operatingSoftware : sU("not available")));
	write(sci::NcAttribute(sU("instrument_software_version"), instrumentInfo.operatingSoftwareVersion.length() > 0 ? instrumentInfo.operatingSoftwareVersion : sU("not available")));
	write(sci::NcAttribute(sU("creator_name"), author.name.length() > 0 ? author.name : sU("not available")));
	write(sci::NcAttribute(sU("creator_email"), author.email.length() > 0 ? author.email : sU("not available")));
	write(sci::NcAttribute(sU("creator_url"), author.orcidUrl.length() > 0 ? author.orcidUrl : sU("not available")));
	write(sci::NcAttribute(sU("institution"), author.institution.length() > 0 ? author.institution : sU("not available")));
	write(sci::NcAttribute(sU("processing_software_url"), processingsoftwareInfo.url.length() > 0 ? processingsoftwareInfo.url : sU("not available")));
	write(sci::NcAttribute(sU("processing_software_version"), processingsoftwareInfo.version.length() > 0 ? processingsoftwareInfo.version : sU("not available")));
	write(sci::NcAttribute(sU("calibration_sensitivity"), calibrationInfo.calibrationDescription.length() > 0 ? calibrationInfo.calibrationDescription : sU("not available")));
	if (calibrationInfo.certificationDate == sci::UtcTime(0, 1, 1, 0, 0, 0))
		write(sci::NcAttribute(sU("calibration_certification_date"), sU("not available")));
	else
		write(sci::NcAttribute(sU("calibration_certification_date"), getFormattedDateTime(calibrationInfo.certificationDate, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("calibration_certification_url"), calibrationInfo.certificateUrl.length() > 0 ? calibrationInfo.certificateUrl : sU("not available")));
	sci::stringstream samplingIntervalString;
	samplingIntervalString << dataInfo.samplingInterval;
	write(sci::NcAttribute(sU("sampling_interval"), samplingIntervalString.str()));
	sci::stringstream averagingIntervalString;
	averagingIntervalString << dataInfo.averagingPeriod;
	write(sci::NcAttribute(sU("averaging_interval"), averagingIntervalString.str()));
	write(sci::NcAttribute(sU("processing_level"), dataInfo.processingLevel));
	write(sci::NcAttribute(sU("project"), projectInfo.name.length() > 0 ? projectInfo.name : sU("not available")));
	write(sci::NcAttribute(sU("project_principal_investigator"), projectInfo.principalInvestigatorInfo.name.length() > 0 ? projectInfo.principalInvestigatorInfo.name : sU("not available")));
	write(sci::NcAttribute(sU("project_principal_investigator_email"), projectInfo.principalInvestigatorInfo.email.length() > 0 ? projectInfo.principalInvestigatorInfo.email : sU("not available")));
	write(sci::NcAttribute(sU("project_principal_investigator_url"), projectInfo.principalInvestigatorInfo.orcidUrl.length() > 0 ? projectInfo.principalInvestigatorInfo.orcidUrl : sU("not available")));
	write(sci::NcAttribute(sU("licence"), sci::string(sU("Data usage licence - UK Government Open Licence agreement: http://www.nationalarchives.gov.uk/doc/open-government-licence"))));
	write(sci::NcAttribute(sU("acknowledgement"), sci::string(sU("Acknowledgement of NCAS as the data provider is required whenever and wherever these data are used"))));
	write(sci::NcAttribute(sU("platform"), platform.getPlatformInfo().name.length() > 0 ? platform.getPlatformInfo().name : sU("not available")));
	write(sci::NcAttribute(sU("platform_type"), g_platformTypeStrings.find(platform.getPlatformInfo().platformType)->second));
	write(sci::NcAttribute(sU("deployment_mode"), g_deploymentModeStrings.find(platform.getPlatformInfo().deploymentMode)->second));
	write(sci::NcAttribute(sU("title"), title));
	write(sci::NcAttribute(sU("featureType"), g_featureTypeStrings.find(dataInfo.featureType)->second));
	write(sci::NcAttribute(sU("time_coverage_start"), getFormattedDateTime(dataInfo.startTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("time_coverage_end"), getFormattedDateTime(dataInfo.endTime, sU("-"), sU(":"), sU("T"))));
	write(sci::NcAttribute(sU("geospacial_bounds"), getBoundsString(m_latitudes, m_longitudes)));
	if (platform.getFixedAltitude())
	{
		degreeF latitude;
		degreeF longitude;
		metreF altitude;
		platform.getLocation(sci::UtcTime(), sci::UtcTime(), latitude, longitude, altitude); //the times are ignored for stationary platforms - using a fixed time avoids an error if times has zero size
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
	if (amfVersionString == sU("1.1.0"))
		write(sci::NcAttribute(sU("amf_vocabularies_release"), sci::string(sU("https://github.com/barbarabrooks/NCAS-GENERAL-v1.1.0"))));
	else
		write(sci::NcAttribute(sU("amf_vocabularies_release"), sci::string(sU("https://github.com/ncasuk/AMF_CVs/releases/tag/v")) + amfVersionString));
	write(sci::NcAttribute(sU("comment"), dataInfo.processingOptions.comment + sci::string(dataInfo.processingOptions.beta ? sU("\nBeta release - not to be used in published science.") : sU(""))));

	if (comment.length() > 0)
	{
		write(sci::NcAttribute(sU("comment"), comment));
	}

	for (size_t i = 0; i < globalAttributes.size(); ++i)
		write(*globalAttributes[i]);

	sci::string processingReason = dataInfo.reasonForProcessing;
	if (processingReason.length() == 0)
		processingReason = sU("Processed.");

	if (existingHistory.length() == 0)
	{
		existingHistory = getFormattedDateTime(dataInfo.endTime, sU("-"), sU(":"), sU("T")) + sU(" - Data collected.");
	}

	sci::UtcTime now = sci::UtcTime::now();
	sci::ostringstream history;
	history << existingHistory + sU("\n") + getFormattedDateTime(now, sU("-"), sU(":"), sU("T")) + sU(" ") + versionString.str() << sU(" ") + processingReason;
	write(sci::NcAttribute(sU("history"), history.str()));
	write(sci::NcAttribute(sU("product_version"), versionString.str()));
	write(sci::NcAttribute(sU("last_revised_date"), getFormattedDateTime(now, sU("-"), sU(":"), sU("T"))));

	//Now add the time dimension
	write(m_timeDimension);
	//we also need to add a lat and lon dimension, although we don't actually use them
	//this helps indexing. They have size 1 for stationary platforms or same as time
	//for moving platforms.
	//Note for trajectory data we do not include the lat and lon dimensions
	sci::NcDimension longitudeDimension(sU("longitude"), m_longitudes.size());
	sci::NcDimension latitudeDimension(sU("latitude"), m_latitudes.size());
	if (dataInfo.featureType != FeatureType::trajectory)
	{
		write(longitudeDimension);
		write(latitudeDimension);
	}
	//and any other dimensions
	for (size_t i = 0; i < nonTimeDimensions.size(); ++i)
		write(*nonTimeDimensions[i]);

	//create the time variables, but do not write the data as we need to stay in define mode
	//so the user can add other variables
	m_timeVariable.reset(new AmfNcTimeVariable(*this, getTimeDimension(), m_times));
	m_dayOfYearVariable.reset(new AmfNcVariable<float>(sU("day_of_year"), *this, getTimeDimension(), sU("Day of Year"), sU(""), sU("1"), m_dayOfYears, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1)));
	m_yearVariable.reset(new AmfNcVariable<int>(sU("year"), *this, getTimeDimension(), sU("Year"), sU(""), sU("1"), m_years, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1)));
	m_monthVariable.reset(new AmfNcVariable<int>(sU("month"), *this, getTimeDimension(), sU("Month"), sU(""), sU("1"), m_months, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1)));
	m_dayVariable.reset(new AmfNcVariable<int>(sU("day"), *this, getTimeDimension(), sU("Day"), sU(""), sU("1"), m_dayOfMonths, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1)));
	m_hourVariable.reset(new AmfNcVariable<int>(sU("hour"), *this, getTimeDimension(), sU("Hour"), sU(""), sU("1"), m_hours, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1)));
	m_minuteVariable.reset(new AmfNcVariable<int>(sU("minute"), *this, getTimeDimension(), sU("Minute"), sU(""), sU("1"), m_minutes, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1)));
	m_secondVariable.reset(new AmfNcVariable<float>(sU("second"), *this, getTimeDimension(), sU("Second"), sU(""), sU("1"), m_seconds, false, std::vector<sci::string>(0), std::vector<std::pair<sci::string, CellMethod>>(0), sci::GridData<uint8_t, 0>(1)));

	//for trajectories, the lat/lon depend on the time dimension, not the lat/lon dimensions (which aren't created)
	if (dataInfo.featureType == FeatureType::trajectory)
	{
		m_longitudeVariable.reset(new AmfNcLongitudeVariable(*this, m_timeDimension, m_longitudes, dataInfo.featureType, platform.getPlatformInfo().deploymentMode, platform.getPlatformInfo().platformType));
		m_latitudeVariable.reset(new AmfNcLatitudeVariable(*this, m_timeDimension, m_latitudes, dataInfo.featureType, platform.getPlatformInfo().deploymentMode, platform.getPlatformInfo().platformType));
	}
	else
	{
		m_longitudeVariable.reset(new AmfNcLongitudeVariable(*this, longitudeDimension, m_longitudes, dataInfo.featureType, platform.getPlatformInfo().deploymentMode, platform.getPlatformInfo().platformType));
		m_latitudeVariable.reset(new AmfNcLatitudeVariable(*this, latitudeDimension, m_latitudes, dataInfo.featureType, platform.getPlatformInfo().deploymentMode, platform.getPlatformInfo().platformType));
	}

	if (platform.getPlatformInfo().platformType == PlatformType::moving && !overriddenPlatformPosition)
	{
		std::vector<std::pair<sci::string, CellMethod>> motionCellMethods{ {sU("time"), CellMethod::mean} };
		std::vector<std::pair<sci::string, CellMethod>> motionStdevCellMethods{ {sU("time"), CellMethod::standardDeviation} };
		std::vector<std::pair<sci::string, CellMethod>> motionMinCellMethods{ {sU("time"), CellMethod::min} };
		std::vector<std::pair<sci::string, CellMethod>> motionMaxCellMethods{ {sU("time"), CellMethod::max} };
		std::vector<sci::string> motionCoordinates{ sU("longitude"), sU("latitude") };


		if (sci::anyTrue(m_courses == m_courses))
			m_courseVariable.reset(new AmfNcVariable<degreeF>(sU("platform_course"), *this, getTimeDimension(), sU("Direction in which the platform is travelling"), sU("platform_course"), m_courses, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));
		if (sci::anyTrue(m_speeds == m_speeds))
			m_speedVariable.reset(new AmfNcVariable<metrePerSecondF>(sU("platform_speed_wrt_ground"), *this, getTimeDimension(), sU("Platform speed with respect to ground"), sU("platform_speed_wrt_ground"), m_speeds, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));
		if (sci::anyTrue(m_headings == m_headings))
			m_orientationVariable.reset(new AmfNcVariable<degreeF>(sU("platform_orientation"), *this, getTimeDimension(), sU("Direction in which \"front\" of platform is pointing"), sU("platform_orientation"), m_headings, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));

		if (sci::anyTrue(m_elevations == m_elevations))
		{
			m_instrumentPitchVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_pitch_angle"), *this, getTimeDimension(), sU("Instrument Pitch Angle"), sU(""), m_elevations, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentPitchStdevVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_pitch_standard_deviation"), *this, getTimeDimension(), sU("Instrument Pitch Angle Standard Deviation"), sU(""), m_elevationStdevs, true, motionCoordinates, motionStdevCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentPitchMinVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_pitch_minimum"), *this, getTimeDimension(), sU("Instrument Pitch Angle Minimum"), sU(""), m_elevationMins, true, motionCoordinates, motionMinCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentPitchMaxVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_pitch_maximum"), *this, getTimeDimension(), sU("Instrument Pitch Angle Maximum"), sU(""), m_elevationMaxs, true, motionCoordinates, motionMaxCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentPitchRateVariable.reset(new AmfNcVariable<degreePerSecondF>(sU("instrument_pitch_rate"), *this, getTimeDimension(), sU("Instrument Pitch Angle Rate of Change"), sU(""), m_elevationRates, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));
		}

		if (sci::anyTrue(m_azimuths == m_azimuths))
		{
			m_instrumentYawVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_yaw_angle"), *this, getTimeDimension(), sU("Instrument Yaw Angle"), sU(""), m_azimuths, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentYawStdevVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_yaw_standard_deviation"), *this, getTimeDimension(), sU("Instrument Yaw Angle Standard Deviation"), sU(""), m_azimuthStdevs, true, motionCoordinates, motionStdevCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentYawMinVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_yaw_minimum"), *this, getTimeDimension(), sU("Instrument Yaw Angle Minimum"), sU(""), m_azimuthMins, true, motionCoordinates, motionMinCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentYawMaxVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_yaw_maximum"), *this, getTimeDimension(), sU("Instrument Yaw Angle Maximum"), sU(""), m_azimuthMaxs, true, motionCoordinates, motionMaxCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentYawRateVariable.reset(new AmfNcVariable<degreePerSecondF>(sU("instrument_yaw_rate"), *this, getTimeDimension(), sU("Instrument Yaw Angle Rate of Change"), sU(""), m_azimuthRates, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));
		}

		if (sci::anyTrue(m_rolls == m_rolls))
		{
			m_instrumentRollVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_roll_angle"), *this, getTimeDimension(), sU("Instrument Roll Angle"), sU(""), m_rolls, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentRollStdevVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_roll_standard_deviation"), *this, getTimeDimension(), sU("Instrument Roll Angle Standard Deviation"), sU(""), m_rollStdevs, true, motionCoordinates, motionStdevCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentRollMinVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_roll_minimum"), *this, getTimeDimension(), sU("Instrument Roll Angle Minimum"), sU(""), m_rollMins, true, motionCoordinates, motionMinCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentRollMaxVariable.reset(new AmfNcVariable<degreeF>(sU("instrument_roll_maximum"), *this, getTimeDimension(), sU("Instrument Roll Angle Maximum"), sU(""), m_rollMaxs, true, motionCoordinates, motionMaxCellMethods, sci::GridData<uint8_t, 0>(1)));
			m_instrumentRollRateVariable.reset(new AmfNcVariable<degreePerSecondF>(sU("instrument_roll_rate"), *this, getTimeDimension(), sU("Instrument Roll Angle Rate of Change"), sU(""), m_rollRates, true, motionCoordinates, motionCellMethods, sci::GridData<uint8_t, 0>(1)));
		}

	}
	//and the time variable
	/*std::vector<double> secondsAfterEpoch(times.size());
	sci::UtcTime epoch(1970, 1, 1, 0, 0, 0.0);
	for (size_t i = 0; i < times.size(); ++i)
		secondsAfterEpoch[i] = times[i] - epoch;
	//this will end define mode!
	write(AmfNcTimeVariable(*this, m_timeDimension), secondsAfterEpoch);*/

}

/*template<size_t NDIMS>
void OutputAmfNcFile::writeDbData(const AmfNcDbVariableFromLogarithmicData<unitlessF>& variable, const sci::GridData<unitlessF, NDIMS>& data)
{
	auto ncOutputView = sci::make_gridtransform_view(data, [](const unitlessF& val) { return AmfNcDbVariableFromLogarithmicData<unitlessF>::transformForOutput(val); });
	//auto test = ncOutputView[0];
	//unitlessF test2 = ncOutputView[0];
	const sci::NcVariable<float>& var = variable;
	sci::OutputNcFile::write(var, ncOutputView);
}*/



