#pragma once
#include<vector>
#include<svector/sreadwrite.h>
#include<string>
#include<svector/time.h>
#include<svector/sstring.h>

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
	sci::UtcTime sensitivityCalibrationDate;
	sci::UtcTime certificationDate;
	sci::string certificateUrl;
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
};

class OutputAmfNcFile : public sci::OutputNcFile
{
public:
	OutputAmfNcFile(const sci::string &filename,
		const InstrumentInfo &instrumentInfo,
		const PersonInfo &author,
		const ProcessingSoftwareInfo &processingsoftwareInfo,
		const CalibrationInfo &calibrationInfo,
		const DataInfo &dataInfo,
		const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo,
		const sci::string &comment);
private:
};
void writeCeilometerProfilesToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, sci::string filename, double maxRange, ProgressReporter &progressReporter, wxWindow *parent);
