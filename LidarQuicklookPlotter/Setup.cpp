#include"Setup.h"
#include"ProgressReporter.h"
#include"InstrumentProcessor.h"

//Rules for file format
//The file must be netcdf.
//The file must have the following variables, each with a Units attribute with the unit listed in brackets. capitalisation and spaces matter in the variable names and units. 
//  time(seconds since 1970-1-1 00:00:00), latitude(degrees_north), longitude(degrees_east), elevation(degree), azimuth(degree), roll(degree), course(degree), speed(m s-1).
//The variables must all have the same number of elements
//The file can have either a variable altidue(metre) or a global attribute altitude and a global attribute altitude_unit
//for course and azimuth 0 degrees is due north
//for elevation 0 degrees is horizontal.
//data do not need to be equally spaced, but if there is missing data this should be indicated with a nan. If there is missing
//data for all parameters then add a valid time within the period of missing data and set all parameters to nan. This will
//stop the code interpolating accross missing data periods.
void readLocationData(const sci::string &locationFile, std::vector<sci::UtcTime> &times, std::vector<degree> &latitudes, std::vector<degree> &longitudes, std::vector<metre> &altitudes, std::vector<degree> &elevations, std::vector<degree> &azimuths, std::vector<degree> &rolls, std::vector<degree> &courses, std::vector<metrePerSecond> &speeds, ProgressReporter &progressReporter)
{
	sci::InputNcFile file(locationFile);

	std::vector<sci::string> names = file.getVariableNames();

	progressReporter << sU("Reading platform time data.\n");
	std::vector<double> timesUnitless = file.getVariable<double>(sU("time"));
	if (progressReporter.shouldStop())
		return;
	progressReporter << sU("Reading platform latitude data.\n");
	std::vector<double> latitudesUnitless = file.getVariable<double>(sU("latitude"));
	if (progressReporter.shouldStop())
		return;
	progressReporter << sU("Reading platform longitude data.\n");
	std::vector<double> longitudesUnitless = file.getVariable<double>(sU("longitude"));
	if (progressReporter.shouldStop())
		return;
	progressReporter << sU("Reading platform elevation data.\n");
	std::vector<double> elevationsUnitless = file.getVariable<double>(sU("elevation"));
	if (progressReporter.shouldStop())
		return;
	progressReporter << sU("Reading platform azimuth data.\n");
	std::vector<double> azimuthsUnitless = file.getVariable<double>(sU("azimuth"));
	if (progressReporter.shouldStop())
		return;
	progressReporter << sU("Reading platform roll data.\n");
	std::vector<double> rollsUnitless = file.getVariable<double>(sU("roll"));
	if (progressReporter.shouldStop())
		return;
	progressReporter << sU("Reading platform course data.\n");
	std::vector<double> coursesUnitless = file.getVariable<double>(sU("course"));
	if (progressReporter.shouldStop())
		return;
	progressReporter << sU("Reading platform speed data.\n");
	std::vector<double> speedsUnitless = file.getVariable<double>(sU("speed"));
	if (progressReporter.shouldStop())
		return;

	progressReporter << sU("Reading platform altitude data.\n");
	std::vector<double> altitudesUnitless;
	if (sci::anyTrue(names == sci::string(sU("altitude"))))
		altitudesUnitless = file.getVariable<double>(sU("altitude"));
	else
		altitudesUnitless = std::vector<double>{ file.getGlobalAttribute<double>(sU("altitude")) };
	if (progressReporter.shouldStop())
		return;

	sci::assertThrow(latitudesUnitless.size() == timesUnitless.size(), sci::err(sci::SERR_USER, 0, sU("When reading location file, the latitude variable has a different numer of elements to the time variable.")));
	sci::assertThrow(longitudesUnitless.size() == timesUnitless.size(), sci::err(sci::SERR_USER, 0, sU("When reading location file, the longitude variable has a different numer of elements to the time variable.")));
	sci::assertThrow(elevationsUnitless.size() == timesUnitless.size(), sci::err(sci::SERR_USER, 0, sU("When reading location file, the elevation variable has a different numer of elements to the time variable.")));
	sci::assertThrow(azimuthsUnitless.size() == timesUnitless.size(), sci::err(sci::SERR_USER, 0, sU("When reading location file, the azimuth variable has a different numer of elements to the time variable.")));
	sci::assertThrow(rollsUnitless.size() == timesUnitless.size(), sci::err(sci::SERR_USER, 0, sU("When reading location file, the roll variable has a different numer of elements to the time variable.")));
	sci::assertThrow(coursesUnitless.size() == timesUnitless.size(), sci::err(sci::SERR_USER, 0, sU("When reading location file, the courses variable has a different numer of elements to the time variable.")));
	sci::assertThrow(speedsUnitless.size() == timesUnitless.size(), sci::err(sci::SERR_USER, 0, sU("When reading location file, the speed variable has a different numer of elements to the time variable.")));

	sci::assertThrow(altitudesUnitless.size() == timesUnitless.size() || altitudesUnitless.size() == 1, sci::err(sci::SERR_USER, 0, sU("When reading location file, the altitude variable has a different numer of elements to the time variable.")));

	progressReporter << sU("Performing an internal copy of the position and attitude data.\n");
	times.resize(timesUnitless.size());
	latitudes.resize(timesUnitless.size());
	longitudes.resize(timesUnitless.size());
	elevations.resize(timesUnitless.size());
	azimuths.resize(timesUnitless.size());
	rolls.resize(timesUnitless.size());
	speeds.resize(timesUnitless.size());
	courses.resize(timesUnitless.size());
	for (size_t i = 0; i < timesUnitless.size(); ++i)
	{
		times[i] = sci::UtcTime::getPosixEpoch() + second(timesUnitless[i]);
		latitudes[i] = degree(latitudesUnitless[i]);
		longitudes[i] = degree(longitudesUnitless[i]);
		elevations[i] = degree(elevationsUnitless[i]);
		azimuths[i] = degree(azimuthsUnitless[i]);
		rolls[i] = degree(rollsUnitless[i]);
		speeds[i] = metrePerSecond(speedsUnitless[i]);
		courses[i] = degree(coursesUnitless[i]);
	}
	altitudes.resize(altitudesUnitless.size());
	for (size_t i = 0; i < altitudes.size(); ++i)
		altitudes[i] = metre(altitudesUnitless[i]);

	/*
	//sci::assertThrow(false, sci::err(sci::SERR_USER, 0, "Not set up to read location data yet"));
	times = { sci::UtcTime(1900, 1, 1, 0, 0, 0.0), sci::UtcTime(2100, 1, 1, 0, 0, 0.0) };
	latitudes = { degree(0), degree(0) };
	longitudes = { degree(0), degree(0) };
	altitudes = { metre(0), metre(0) };
	elevations = { degree(0), degree(0) };
	azimuths = { degree(0), degree(0) };
	rolls = { degree(0), degree(0) };
	courses = { degree(0), degree(0) };
	speeds = { metrePerSecond(0), metrePerSecond(0) };
	*/
}

wxXmlNode *findNode(const sci::string &type, const sci::string &id, wxXmlNode *nodeToSearch)
{
	//if thsi isn't an element node then return null
	if (nodeToSearch->GetType() != wxXML_ELEMENT_NODE)
		return nullptr;

	size_t pos = type.find_first_of(sU("/"));
	sci::string name = type.substr(0, pos);

	//if the names don't match then return null
	if (sci::fromWxString(nodeToSearch->GetName()) != name)
		return nullptr;

	//if this is the final level and the ids don't match return null
	if (pos == sci::string::npos)
	{
		sci::string thisid = sci::fromWxString(nodeToSearch->GetAttribute(sci::nativeUnicode(sU("id"))));
		if (sci::fromWxString(nodeToSearch->GetAttribute(sci::nativeUnicode(sU("id")))) == id)
			return nodeToSearch;
		else
			return nullptr;
	}
	else
	{
		//check each child node for the one we're looking for
		sci::string childType = type.substr(pos + 1, sci::string::npos);
		wxXmlNode *child = nodeToSearch->GetChildren();
		while (child)
		{
			wxXmlNode *next = findNode(childType, id, child);
			if (next)
				return next;
			child = child->GetNext();
		}
	}

	//if we get here then we didn't find the node we're looking for
	return nullptr;
}

template<class T>
struct nameVarPair
{
	nameVarPair(sci::string name, T *var)
		:m_name(name), m_var(var), m_read(false) {}
	sci::string m_name;
	T *m_var;
	bool m_read;
};

template<class T>
T textToValue(const sci::string & string)
{
	static_assert(!std::is_reference<T>::value, "The function textToValue is not designed to return a reference.");
	T result;
	sci::istringstream stream(string);
	stream >> result;
	sci::assertThrow(!stream.bad(), sci::err(sci::SERR_USER, 0, "Could not convert a string to a value."));
	return result;
}

template<>
sci::string textToValue<sci::string>(const sci::string & string)
{
	return string;
}

template<>
bool textToValue<bool>(const sci::string & string)
{
	bool result;
	if (string == sU("true"))
		result = true;
	else if (string == sU("false"))
		result = false;
	else
		sci::assertThrow(false, sci::err(sci::SERR_USER, 0, "A boolean element must be set to true or false (case sensitive)."));
	return result;
}

//parse an xml node, grabbing the values of all the variables in begin-end
//begin and end must be iterators dereferencing to become nameVarPair structs
template < class ITER >
void parseXmlNode(wxXmlNode *node, ITER begin, ITER end)
{
	for (ITER iter = begin; iter != end; ++iter)
		iter->m_read = false;

	wxXmlNode *child = node->GetChildren();

	while (child)
	{
		for (ITER iter = begin; iter != end; ++iter)
		{
			if (sci::fromWxString(child->GetName()) == iter->m_name && child->GetType() == wxXML_ELEMENT_NODE)
			{
				wxXmlNode *textNode = child->GetChildren();
				//get the fully qualified node name for any error reporting
				sci::string nodeName = sU("<") + sci::fromWxString(child->GetName()) + sU(">");
				wxXmlNode *parent = child->GetParent();
				while (parent)
				{
					nodeName = sU("<") + sci::fromWxString(parent->GetName()) + sU(" ") + nodeName + sU(">");
					parent = parent->GetParent();
				}
				wxString lineNumber;
				lineNumber << child->GetLineNumber();
				nodeName = nodeName + sU(" at line ") + sci::fromWxString(lineNumber);
				sci::assertThrow(textNode != nullptr, sci::err(sci::SERR_USER, 0, sU("Could not parse xml data in node ") + nodeName + sU(", the node does not have content.")));
				sci::assertThrow(textNode->GetType() == wxXML_TEXT_NODE, sci::err(sci::SERR_USER, 0, sU("Could not parse xml data in node ") + nodeName + sU(", the content is not text.")));
				*(iter->m_var) = textToValue<std::remove_pointer<decltype(iter->m_var)>::type>(sci::fromWxString(textNode->GetContent()));
				iter->m_read = true;
			}
		}
		child = child->GetNext();
	}
}

//parse an xml node grabbing all occurances of a specific name 
template < class CONTAINER >
void parseXmlNode(wxXmlNode *node, const sci::string &name, CONTAINER &values, bool &read)
{
	read = false;
	wxXmlNode *child = node->GetChildren();
	while (child)
	{
		if (sci::fromWxString(child->GetName()) == name && child->GetType() == wxXML_ELEMENT_NODE)
		{
			wxXmlNode *textNode = child->GetChildren();
			//get the fully qualified node name for any error reporting
			sci::string nodeName = sU("<") + sci::fromWxString(child->GetName()) + sU(">");
			wxXmlNode *parent = child->GetParent();
			while (parent)
			{
				nodeName = sU("<") + sci::fromWxString(parent->GetName()) + sU(" ") + nodeName + sU(">");
				parent = parent->GetParent();
			}
			wxString lineNumber;
			lineNumber << child->GetLineNumber();
			nodeName = nodeName + sU(" at line ") + sci::fromWxString(lineNumber);
			sci::assertThrow(textNode != nullptr, sci::err(sci::SERR_USER, 0, sU("Could not parse xml data in node ") + nodeName + sU(", the node does not have content.")));
			sci::assertThrow(textNode->GetType() == wxXML_TEXT_NODE, sci::err(sci::SERR_USER, 0, sU("Could not parse xml data in node ") + nodeName + sU(", the content is not text.")));
			values.push_back(textToValue<CONTAINER::value_type>(sci::fromWxString(textNode->GetContent())));
			read = true;
		}
		child = child->GetNext();
	}
}

template<>
void parseXmlNode<sci::UtcTime>(wxXmlNode *node, const sci::string &name, sci::UtcTime &value, bool &read)
{
	read = false;
	wxXmlNode *child = node->GetChildren();
	while (child)
	{
		if (sci::fromWxString(child->GetName()) == name && child->GetType() == wxXML_ELEMENT_NODE)
		{
			read = true;
			int year = 0;
			unsigned int month = 1;
			unsigned int day = 1;
			unsigned int hour = 0;
			unsigned int minute = 0;
			double second = 0.0;
			std::vector<nameVarPair<int>> yearLink
			{ nameVarPair<int>(sU("year"), &(year))
			};
			std::vector<nameVarPair<double>> secondLink
			{ nameVarPair<double>(sU("second"), &(second))
			};
			std::vector<nameVarPair<unsigned int>> otherTimeLinks
			{ nameVarPair<unsigned int>(sU("month"), &(month)),
				nameVarPair<unsigned int>(sU("day"), &(day)),
				nameVarPair<unsigned int>(sU("hour"), &(hour)),
				nameVarPair<unsigned int>(sU("minute"), &(minute))
			};
			parseXmlNode(child, yearLink.begin(), yearLink.end());
			parseXmlNode(child, secondLink.begin(), secondLink.end());
			parseXmlNode(child, otherTimeLinks.begin(), otherTimeLinks.end());
			value = sci::UtcTime(year, month, day, hour, minute, second);

			read &= yearLink.begin()->m_read;
			read &= secondLink.begin()->m_read;
			for (auto iter = otherTimeLinks.begin(); iter != otherTimeLinks.end(); ++iter)
				read &= iter->m_read;
			return;
		}
		child = child->GetNext();
	}
}

PersonInfo parsePersonInfo(wxXmlNode *node);

template<>
void parseXmlNode<PersonInfo>(wxXmlNode *node, const sci::string &name, PersonInfo &value, bool &read)
{
	read = false;
	wxXmlNode *child = node->GetChildren();
	while (child)
	{
		if (sci::fromWxString(child->GetName()) == name && child->GetType() == wxXML_ELEMENT_NODE)
		{
			value = parsePersonInfo(child);
			read = true;
			return;
		}
		child = child->GetNext();
	}
}

InstrumentInfo parseInstrumentInfo(wxXmlNode *node)
{
	InstrumentInfo result;
	std::vector<nameVarPair<sci::string>> links
	{ nameVarPair<sci::string>(sU("name"), &(result.name)),
		nameVarPair<sci::string>(sU("description"), &(result.description)),
		nameVarPair<sci::string>(sU("manufacturer"), &(result.manufacturer)),
		nameVarPair<sci::string>(sU("model"), &(result.model)),
		nameVarPair<sci::string>(sU("serial"), &(result.serial)),
		nameVarPair<sci::string>(sU("operatingSoftware"), &(result.operatingSoftware)),
		nameVarPair<sci::string>(sU("operatingSoftwareVersion"), &(result.operatingSoftwareVersion)),
	};

	parseXmlNode(node, links.begin(), links.end());
	for (auto iter = links.begin(); iter != links.end(); ++iter)
	{
		sci::assertThrow(iter->m_read, sci::err(sci::SERR_USER, 0, "missing parameter " + sci::nativeCodepage(iter->m_name) + " when parsing Instrument Information."));
	}

	return result;
}
PersonInfo parsePersonInfo(wxXmlNode *node)
{
	PersonInfo result;
	std::vector<nameVarPair<sci::string>> links
	{ nameVarPair<sci::string>(sU("name"), &(result.name)),
		nameVarPair<sci::string>(sU("email"), &(result.email)),
		nameVarPair<sci::string>(sU("orcidUrl"), &(result.orcidUrl)),
		nameVarPair<sci::string>(sU("institution"), &(result.institution)),
	};

	parseXmlNode(node, links.begin(), links.end());

	for (auto iter = links.begin(); iter != links.end(); ++iter)
	{
		sci::assertThrow(iter->m_read, sci::err(sci::SERR_USER, 0, "missing parameter " + sci::nativeCodepage(iter->m_name) + " when parsing Person Information."));
	}

	return result;
}

ProcessingOptions parseProcessingOptions(wxXmlNode *node)
{
	ProcessingOptions result;
	std::vector<nameVarPair<sci::string>> textLinks
	{ nameVarPair<sci::string>(sU("inputDirectory"), &(result.inputDirectory)),
		nameVarPair<sci::string>(sU("outputDirectory"), &(result.outputDirectory)),
		nameVarPair<sci::string>(sU("comment"), &(result.comment)),
		nameVarPair<sci::string>(sU("reasonForProcessing"), &(result.reasonForProcessing))
	};
	std::vector<nameVarPair<bool>> boolLinks
	{ nameVarPair<bool>(sU("startImmediately"), &(result.startImmediately)),
		nameVarPair<bool>(sU("checkForNewFiles"), &(result.checkForNewFiles)),
		nameVarPair<bool>(sU("onlyProcessNewFiles"), &(result.onlyProcessNewFiles)),
		nameVarPair<bool>(sU("beta"), &(result.beta)),
		nameVarPair<bool>(sU("generateQuicklooks"), &(result.generateQuicklooks)),
		nameVarPair<bool>(sU("generateNetCdf"), &(result.generateNetCdf))
	};

	parseXmlNode(node, textLinks.begin(), textLinks.end());
	parseXmlNode(node, boolLinks.begin(), boolLinks.end());

	bool readStartTime;
	bool readEndTime;
	parseXmlNode(node, sU("startTime"), result.startTime, readStartTime);
	parseXmlNode(node, sU("endTime"), result.endTime, readEndTime);

	for (auto iter = textLinks.begin(); iter != textLinks.end(); ++iter)
		sci::assertThrow(iter->m_read, sci::err(sci::SERR_USER, 0, "missing parameter " + sci::nativeCodepage(iter->m_name) + " when parsing processing options."));
	for (auto iter = boolLinks.begin(); iter != boolLinks.end(); ++iter)
		sci::assertThrow(iter->m_read, sci::err(sci::SERR_USER, 0, "missing parameter " + sci::nativeCodepage(iter->m_name) + " when parsing processing options."));
	sci::assertThrow(readEndTime, sci::err(sci::SERR_USER, 0, "missing parameter endTime when parsing processing options."));
	sci::assertThrow(readStartTime, sci::err(sci::SERR_USER, 0, "missing parameter startTime when parsing processing options."));

	return result;
}

sci::UtcTime parseTime(wxXmlNode *node);

CalibrationInfo parseCalibrationInfo(wxXmlNode *node)
{
	CalibrationInfo result;
	std::vector<nameVarPair<sci::string>> stringLinks
	{ nameVarPair<sci::string>(sU("calibrationDescription"), &(result.calibrationDescription)),
		nameVarPair<sci::string>(sU("certificateUrl"), &(result.certificateUrl))
	};

	bool timeRead;
	parseXmlNode(node, stringLinks.begin(), stringLinks.end());
	parseXmlNode(node, sU("certificationDate"), result.certificationDate, timeRead);

	for (auto iter = stringLinks.begin(); iter != stringLinks.end(); ++iter)
	{
		sci::assertThrow(iter->m_read, sci::err(sci::SERR_USER, 0, "missing parameter " + sci::nativeCodepage(iter->m_name) + " when parsing Person Information."));
	}
	sci::assertThrow(timeRead, sci::err(sci::SERR_USER, 0, "missing parameter certificationDate when parsing Person Information."));

	return result;
}


std::shared_ptr<InstrumentProcessor> parseInstrumentProcessor(wxXmlNode *node, wxXmlNode *infoNode, ProgressReporter &progressReporter)
{
	wxXmlNode *child = node->GetChildren();
	sci::string name;
	InstrumentInfo instrumentInfo;
	CalibrationInfo calibrationInfo;
	bool gotName = false;
	bool gotInstrumentInfo = false;
	bool gotCalibrationInfo = false;
	while (child)
	{
		wxXmlNode *node;
		if (child->HasAttribute(sci::nativeUnicode(sU("type"))) && child->HasAttribute(sci::nativeUnicode(sU("id"))))
			node = findNode(sci::fromWxString(child->GetAttribute(sci::nativeUnicode(sU("type")))),
				sci::fromWxString(child->GetAttribute(sci::nativeUnicode(sU("id")))),
				infoNode);
		else
			node = child;
		if (child->GetName() == sci::nativeUnicode(sU("name")))
		{
			wxXmlNode *valueNode = child->GetChildren();
			if (valueNode)
			{
				name = sci::fromWxString(valueNode->GetContent());
				gotName = true;
				progressReporter << sU("Found ") << name << sU("\n");
			}
		}
		else if (child->GetName() == sci::nativeUnicode(sU("instrument")))
		{
			instrumentInfo = parseInstrumentInfo(node);
			gotInstrumentInfo = true;
		}
		else if (child->GetName() == sci::nativeUnicode(sU("calibrationInfo")))
		{
			calibrationInfo = parseCalibrationInfo(node);
			gotCalibrationInfo = true;
		}
		child = child->GetNext();
	}

	sci::assertThrow(gotName, sci::err(sci::SERR_USER, 0, sU("Found a processor with no name.")));
	sci::assertThrow(gotInstrumentInfo, sci::err(sci::SERR_USER, 0, sU("Found a processor with no instrument.")));
	sci::assertThrow(gotCalibrationInfo, sci::err(sci::SERR_USER, 0, sU("Found a processor with no calibrationInfo.")));

	return getProcessorByName(name, instrumentInfo, calibrationInfo);
}

ProjectInfo parseProjectInfo(wxXmlNode *node, wxXmlNode *infoNode)
{
	ProjectInfo result;
	std::vector<nameVarPair<sci::string>> stringLinks
	{ nameVarPair<sci::string>(sU("name"), &(result.name)),
	};
	parseXmlNode(node, stringLinks.begin(), stringLinks.end());

	//find the PI
	bool piReferenceRead = false;
	wxXmlNode *child = node->GetChildren();
	wxXmlNode *piNode;
	while (child)
	{
		if (sci::fromWxString(child->GetName()) == sU("principalInvestigator"))
		{
			piReferenceRead = true;
			if (child->HasAttribute(sci::nativeUnicode(sU("type"))) && child->HasAttribute(sci::nativeUnicode(sU("id"))))
				piNode = findNode(sci::fromWxString(child->GetAttribute(sci::nativeUnicode(sU("type")))),
					sci::fromWxString(child->GetAttribute(sci::nativeUnicode(sU("id")))),
					infoNode);
			else
				piNode = child;
			break;
		}
		child = child->GetNext();
	}
	if (piNode)
		result.principalInvestigatorInfo = parsePersonInfo(piNode);

	for (auto iter = stringLinks.begin(); iter != stringLinks.end(); ++iter)
	{
		sci::assertThrow(iter->m_read, sci::err(sci::SERR_USER, 0, "missing parameter " + sci::nativeCodepage(iter->m_name) + " when parsing Person Information."));
	}
	sci::assertThrow(piReferenceRead, sci::err(sci::SERR_USER, 0, "missing parameter principalInvestigator when parsing Project Information."));
	sci::assertThrow(piNode, sci::err(sci::SERR_USER, 0, "The principalInvestigator parameter found when parsing Project Information is invalid."));

	return result;
}

std::shared_ptr<Platform> parsePlatformInfo(wxXmlNode *node, ProgressReporter &progressReporter)
{
	sci::string name;
	sci::string platformType;
	sci::string deploymentMode;
	sci::string locationFile;
	std::vector<sci::string> locationKeywords;
	std::vector<nameVarPair<sci::string>> stringLinks
	{ nameVarPair<sci::string>(sU("name"), &(name)),
		nameVarPair<sci::string>(sU("type"), &(platformType)),
		nameVarPair<sci::string>(sU("deploymentMode"), &(deploymentMode)),
		nameVarPair<sci::string>(sU("locationFile"), &(locationFile))
	};

	parseXmlNode(node, stringLinks.begin(), stringLinks.end());

	for (auto iter = stringLinks.begin(); iter != stringLinks.end(); ++iter)
	{
		if (iter->m_name != sU("locationFile")) //not needed in all cases - we check this below
			sci::assertThrow(iter->m_read, sci::err(sci::SERR_USER, 0, "missing parameter " + sci::nativeCodepage(iter->m_name) + " when parsing Platform Information."));
	}

	bool locationKeywordsRead;
	parseXmlNode(node, sU("locationKeyword"), locationKeywords, locationKeywordsRead);
	//location keywords are optional - don't throw if they are missing

	if (platformType == sU("stationary"))
	{
		//get position and orientation data
		double latitude_degree;
		double longitude_degree;
		double altitude_m;
		double elevation_degree;
		double azimuth_degree;
		double roll_degree;

		std::vector<nameVarPair<double>> valueLinks
		{ nameVarPair<double>(sU("latitudeDegree"), &(latitude_degree)),
			nameVarPair<double>(sU("longitudeDegree"), &(longitude_degree)),
			nameVarPair<double>(sU("altitudeMetres"), &(altitude_m)),
			nameVarPair<double>(sU("elevationDegree"), &(elevation_degree)),
			nameVarPair<double>(sU("azimuthDegree"), &(azimuth_degree)),
			nameVarPair<double>(sU("rollDegree"), &(roll_degree))
		};
		parseXmlNode(node, valueLinks.begin(), valueLinks.end());
		bool allRead = true;
		for (auto iter = valueLinks.begin(); iter != valueLinks.end(); ++iter)
			allRead &= iter->m_read;
		sci::assertThrow(allRead, sci::err(sci::SERR_USER, 0, "Could not find the fixed positions and orientation when parsing a stationary platform information - require latitude_degree, longitude_degree, altitude_m, elevation_degree, azimuth_degree, roll_degree."));
		if (deploymentMode == sU("land"))
			return std::shared_ptr<Platform>(new StationaryPlatform(name, metre(altitude_m), degree(latitude_degree), degree(longitude_degree), locationKeywords, degree(elevation_degree), degree(azimuth_degree), degree(roll_degree)));
		else if (deploymentMode == sU("sea"))
			sci::assertThrow(false, sci::err(sci::SERR_USER, 0, "The software is not yet set up for stationary sea deployments"));
		else if (deploymentMode == sU("air"))
			sci::assertThrow(false, sci::err(sci::SERR_USER, 0, "The software is not yet set up for stationary air deployments"));
		else
			sci::assertThrow(false, sci::err(sci::SERR_USER, 0, "Found an unrecognised deployment mode when parsing platform info. Valid values are land, sea, air"));
	}
	else if (platformType == sU("moving"))
	{
		double altitude_m;
		double elevation_degree;
		double azimuth_degree;
		double roll_degree;
		std::vector<nameVarPair<double>> valueLinks
		{
			nameVarPair<double>(sU("altitudeMetres"), &(altitude_m)),
			nameVarPair<double>(sU("elevationDegree"), &(elevation_degree)),
			nameVarPair<double>(sU("azimuthDegree"), &(azimuth_degree)),
			nameVarPair<double>(sU("rollDegree"), &(roll_degree))
		};
		parseXmlNode(node, valueLinks.begin(), valueLinks.end());
		bool allRead = true;
		for (auto iter = valueLinks.begin(); iter != valueLinks.end(); ++iter)
			allRead &= iter->m_read;
		sci::assertThrow(allRead, sci::err(sci::SERR_USER, 0, "Could not find the fixed altitude (altitude_m) or ship relative elevation, azimuth and roll (elevation_degree, azimuth_degree, elevation_degree) when parsing a ship (moving/sea) platform information."));
		sci::assertThrow(locationFile.length() > 0, sci::err(sci::SERR_USER, 0, "Could not find the name of the location file when parsing a moving platform information"));

		bool platformRelative;
		std::vector<nameVarPair<bool>> boolLinks
		{
			nameVarPair<bool>(sU("platformRelative"), &platformRelative)
		};
		parseXmlNode(node, boolLinks.begin(), boolLinks.end());
		sci::assertThrow(boolLinks.begin()->m_read, sci::err(sci::SERR_USER, 0, "Could not find the platformRelative parameter when parsing a moving platform."));

		std::vector<sci::UtcTime> times;
		std::vector<degree> latitudes;
		std::vector<degree> longitudes;
		std::vector<metre> altitudes;
		std::vector<degree> elevations;
		std::vector<degree> azimuths;
		std::vector<degree> rolls;
		std::vector<degree> courses;
		std::vector<metrePerSecond> speeds;

		readLocationData(locationFile, times, latitudes, longitudes, altitudes, elevations, azimuths, rolls, courses, speeds, progressReporter);
		if (progressReporter.shouldStop())
			return nullptr;

		if (deploymentMode == sU("sea"))
		{
			progressReporter << sU("Generating motion correction parameters for the platform.\n");
			if (platformRelative)
				return std::shared_ptr<Platform>(new ShipPlatformShipRelativeCorrected(name, metre(altitude_m), times, latitudes, longitudes, locationKeywords, degree(elevation_degree), degree(azimuth_degree), degree(roll_degree), courses, speeds, elevations, azimuths, rolls));
			else
				return std::shared_ptr<Platform>(new ShipPlatform(name, metre(altitude_m), times, latitudes, longitudes, locationKeywords, degree(elevation_degree), degree(azimuth_degree), degree(roll_degree), courses, speeds, elevations, azimuths, rolls));
		}
		else if (deploymentMode == sU("land"))
			sci::assertThrow(false, sci::err(sci::SERR_USER, 0, "The software is not yet set up for moving land deployments"));
		else if (deploymentMode == sU("air"))
			sci::assertThrow(false, sci::err(sci::SERR_USER, 0, "The software is not yet set up for moving air deployments"));
		else
			sci::assertThrow(false, sci::err(sci::SERR_USER, 0, "Found an unrecognised deployment mode when parsing platform info. Valid values are land, sea, air"));
	}
	
	//if we get to here then we found an unrecognised platform type
	sci::assertThrow(false, sci::err(sci::SERR_USER, 0, sU("Found an unrecognised platform type. Accepted values are moving, stationary (case sensitive).")));

}

void setup(sci::string setupFilename, sci::string infoFilename, ProgressReporter &progressReporter,
	ProcessingOptions &processingOptions, PersonInfo &author, ProjectInfo &projectInfo, std::shared_ptr<Platform> &platform,
	std::vector<std::shared_ptr<InstrumentProcessor>> &instrumentProcessors)
{
	wxXmlDocument info;
	info.Load(sci::nativeUnicode(infoFilename));
	sci::assertThrow(info.IsOk(), sci::err(sci::SERR_USER, 0, sU("Could not open the info.xml file.")));
	wxXmlNode *infoNode = info.GetRoot();

	wxXmlDocument settings;
	settings.Load(sci::nativeUnicode(setupFilename));
	sci::assertThrow(settings.IsOk(), sci::err(sci::SERR_USER, 0, sU("Could not open the processing setting xml file ") + setupFilename + sU(".")));
	wxXmlNode *settingsNode = settings.GetRoot();

	sci::assertThrow(settingsNode->GetName() == sci::nativeUnicode(sU("processingSettings")), sci::err(sci::SERR_USER, 0, "Found incorrect root node in processing settings xml file."));

	//parse top level processing options
	processingOptions = parseProcessingOptions(settingsNode);

	wxXmlNode *child = settingsNode->GetChildren();
	bool gotAuthor = false;
	bool gotProjectInfo = false;
	bool gotPlatform = false;
	bool gotAtLeastOneInstrument = false;
	while (child)
	{
		//Users can either add the elements directly to the settings file or they can refer to
		//elements in the info file by giving the elements a type and id attribute that matches
		//the item they want to use.
		wxXmlNode *node;
		if (child->HasAttribute(sci::nativeUnicode(sU("type"))) && child->HasAttribute(sci::nativeUnicode(sU("id"))))
			node = findNode(sci::fromWxString(child->GetAttribute(sci::nativeUnicode(sU("type")))),
				sci::fromWxString(child->GetAttribute(sci::nativeUnicode(sU("id")))),
				infoNode);
		else
			node = child;
		if (child->GetName() == sci::nativeUnicode(sU("author")))
		{
			progressReporter << sU("Found author data.\n");
			author = parsePersonInfo(node);
			gotAuthor = true;
		}
		else if (child->GetName() == sci::nativeUnicode(sU("project")))
		{
			progressReporter << sU("Found project data.\n");
			projectInfo = parseProjectInfo(node, infoNode);
			gotProjectInfo = true;
		}
		else if (child->GetName() == sci::nativeUnicode(sU("platform")))
		{
			progressReporter << sU("Found platform data.\n");
			platform = parsePlatformInfo(node, progressReporter);
			if (progressReporter.shouldStop())
				return;
			gotPlatform = true;
		}
		else if (child->GetName() == sci::nativeUnicode(sU("processor")))
		{
			instrumentProcessors.push_back(parseInstrumentProcessor(node, infoNode, progressReporter));
			gotAtLeastOneInstrument = true;
		}
		child = child->GetNext();
		if (progressReporter.shouldStop())
			return;
	}
	sci::assertThrow(gotAuthor, sci::err(sci::SERR_USER, 0, "Did not find the author element in the settings xml file."));
	sci::assertThrow(gotProjectInfo, sci::err(sci::SERR_USER, 0, "Did not find the project element in the settings xml file."));
	sci::assertThrow(gotPlatform, sci::err(sci::SERR_USER, 0, "Did not find the platform element in the settings xml file."));
	sci::assertThrow(gotAtLeastOneInstrument, sci::err(sci::SERR_USER, 0, "Did not find any instrument elements in the settings xml file."));
}

void setupProcessingOptionsOnly(sci::string setupFilename, ProcessingOptions &processingOptions)
{
	wxXmlDocument settings;
	settings.Load(sci::nativeUnicode(setupFilename));
	sci::assertThrow(settings.IsOk(), sci::err(sci::SERR_USER, 0, sU("Could not open the processing setting xml file ") + setupFilename + sU(".")));
	wxXmlNode *settingsNode = settings.GetRoot();

	sci::assertThrow(settingsNode->GetName() == sci::nativeUnicode(sU("processingSettings")), sci::err(sci::SERR_USER, 0, "Found incorrect root node in processing settings xml file."));

	//parse top level processing options
	processingOptions = parseProcessingOptions(settingsNode);
}