#pragma once
#include<vector>
#include<svector/sreadwrite.h>
#include<string>
#include<svector/time.h>

struct HplHeader;
class HplProfile;
class wxWindow;
class ProgressReporter;
class CampbellMessage2;
class CampbellCeilometerProfile;


struct InstrumentInfo
{
	std::string description;
	std::string manufacturer;
	std::string model;
	std::string serial;
	std::string operatingSoftware;
	std::string operatingSoftwareVersion;
};

struct ProcessingSoftwareInfo
{
	std::string url;
	std::string version;
};

struct PersonInfo
{
	std::wstring name;
	std::wstring email;
	std::wstring orcidUrl;
	std::wstring institution;
};

struct CalibrationInfo
{
	sci::UtcTime sensitivityCalibrationDate;
	sci::UtcTime certificationDate;
	std::string certificateUrl;
};

enum FeatureType
{
	ft_timeSeriesPoint
};
const std::vector<std::string> g_featureTypeStrings{ "timeSeriesPoint" };

struct DataInfo
{
	std::string title;
	double sampingInterval;
	std::string samplingIntervalUnit;
	double averagingPeriod;
	std::string averagingPeriodUnit;
	int processingLevel;
	FeatureType featureType;
	double minLatDecimalDegrees;//for point measurements set min and max the same or set one to nan
	double maxLatDecimalDegrees;
	double minLonDecimalDegrees;
	double maxLonDecimalDegrees;
	sci::UtcTime startTime;
	sci::UtcTime endTime;
	std::string reasonForProcessing; // This string is put in the history. Add any special reason for processing if there is any e.g. Reprocessing due to x error or initial processing to be updated later for y reason
};

struct ProjectInfo
{
	std::string name;
	PersonInfo principalInvestigatorInfo;
};

enum PlatformType
{
	pt_stationary,
	pt_moving
};
const std::vector<std::string> g_platformTypeStrings{ "stationary_platform", "moving_platform" };

enum DeploymentMode
{
	dm_air,
	dm_land,
	dm_sea
};
const std::vector<std::string> g_deploymentModeStrings{ "air", "sea", "land" };

struct PlatformInfo
{
	std::string name;
	PlatformType platformType;
	DeploymentMode deploymentMode;
	double altitudeMetres; //Use nan for varying altitude airborne
	std::vector<std::string> locationKeywords;
};

class OutputAmfNcFile : public sci::OutputNcFile
{
public:
#ifdef _WIN32
	OutputAmfNcFile(const std::wstring &filename,
		const InstrumentInfo &instrumentInfo,
		const PersonInfo &author,
		const ProcessingSoftwareInfo &processingsoftwareInfo,
		const CalibrationInfo &calibrationInfo,
		const DataInfo &dataInfo,
		const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo,
		const std::string &comment);
#else
	OutputAmfNcFile(const std::string &filename, const std::wstring &authorName, const std::string &authorEmail, const std::string &authorOrchid, const std::string authorInstitution);
#endif
private:
};
void writeCeilometerProfilesToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, std::string filename, double maxRange, ProgressReporter &progressReporter, wxWindow *parent);
