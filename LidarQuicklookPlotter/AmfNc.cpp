#define _USE_MATH_DEFINES
#include"AmfNc.h"
#include<sstream>
#include<iomanip>
#include<filesystem>
#include<cmath>
#include<locale>



#include <ranges>
//template<class SOURCE_UNIT, class SOURCE_VALUE_TYPE, class DESTINATION_UNIT=SOURCE_UNIT, class DESTINATION_VALUE_TYPE=SOURCE_VALUE_TYPE>
//using PhysicalsToValuesView = std::ranges::views::transform([](const Physical<SOURCE_UNIT, SOURCE_VALUE_TYPE>& physical) {return (DESTINATION_VALUE_TYPE)(physical.value<DESTINATION_UNIT>()); });



template<class I>
concept passedweak = requires(I i) {
	typename std::iter_difference_t<I>;
	//requires <std::iter_difference_t<I>>;
	{ ++i } -> std::same_as<I&>;   // not required to be equality-preserving
	i++;                           // not required to be equality-preserving
};

template<class _Ty>
concept extraWeakIterable = requires(_Ty __i) {
		 typename std::iter_difference_t<_Ty>;
		 requires std::_Signed_integer_like<std::iter_difference_t<_Ty>>;
		 { ++__i } -> std::same_as<_Ty&>;
		 __i++;
	 };

template<class I>
 requires extraWeakIterable<I>
constexpr bool haspassedweak()
{
	return true;
}
 template<class I>
 requires (!extraWeakIterable<I>)
 constexpr bool haspassedweak()
 {
	 return false;
 }


template <class _It>
concept extraBidirectional = requires(_It __i, const _It __j, const std::iter_difference_t<_It> __n) {
	{ __i += __n } ->std::same_as<_It&>;
	{ __j + __n } -> std::same_as<_It>;
	{ __n + __j } -> std::same_as<_It>;
	{ __i -= __n } -> std::same_as<_It&>;
	{ __j - __n } -> std::same_as<_It>;
	{ __j[__n] } -> std::same_as<std::iter_reference_t<_It>>;
};

template<class T>
requires extraBidirectional<T>
constexpr bool hasPassedExtraBidirectional()
{
	return true;
}

template<class T>
requires (!extraBidirectional<T>)
constexpr bool hasPassedExtraBidirectional()
{
	return false;
}

template <class _It>
concept extraContiguous = requires(const _It & __i) {
	{ _STD to_address(__i) } -> std::same_as<std::add_pointer_t<std::iter_reference_t<_It>>>;
};

 // CONCEPT weakly_incrementable
// clang-format off
// template <class _Ty>
// concept weakly_incrementable = default_initializable<_Ty> && movable<_Ty> && requires(_Ty __i) {
//	 typename iter_difference_t<_Ty>;
//	 requires _Signed_integer_like<iter_difference_t<_Ty>>;
//	 { ++__i } -> same_as<_Ty&>;
//	 __i++;
// };

const double OutputAmfNcFile::Fill<double>::value = -1e20;
const float OutputAmfNcFile::Fill<float>::value = -1e20f;
const uint8_t OutputAmfNcFile::Fill<uint8_t>::value = 255;
const int16_t OutputAmfNcFile::Fill<int16_t>::value = 10000;
const int32_t OutputAmfNcFile::Fill<int32_t>::value = 1000000000;
const int64_t OutputAmfNcFile::Fill<int64_t>::value = 1000000000000000000;

const sci::string OutputAmfNcFile::TypeName<double>::name = sU("float64");
const sci::string OutputAmfNcFile::TypeName<float>::name = sU("float32");
const sci::string OutputAmfNcFile::TypeName<int8_t>::name = sU("int8");
const sci::string OutputAmfNcFile::TypeName<uint8_t>::name = sU("byte");
const sci::string OutputAmfNcFile::TypeName<int16_t>::name = sU("int16");
const sci::string OutputAmfNcFile::TypeName<int32_t>::name = sU("int32");
const sci::string OutputAmfNcFile::TypeName<int64_t>::name = sU("int64");

void correctDirection(degreeF measuredElevation, degreeF measuredAzimuth, unitlessF sinInstrumentElevation, unitlessF sinInstrumentAzimuth, unitlessF sinInstrumentRoll, unitlessF cosInstrumentElevation, unitlessF cosInstrumentAzimuth, unitlessF cosInstrumentRoll, degreeF &correctedElevation, degreeF &correctedAzimuth)
{
	//create a unit vector in the measured direction
	unitlessF x = sci::sin(measuredAzimuth)*sci::cos(measuredElevation);
	unitlessF y = sci::cos(measuredAzimuth)*sci::cos(measuredElevation);
	unitlessF z = sci::sin(measuredElevation);

	//correct the vector
	unitlessF correctedX;
	unitlessF correctedY;
	unitlessF correctedZ;
	correctDirection(x, y, z, sinInstrumentElevation, sinInstrumentAzimuth, sinInstrumentRoll, cosInstrumentElevation, cosInstrumentAzimuth, cosInstrumentRoll, correctedX, correctedY, correctedZ);

	//convert the unit vector back to spherical polar
	correctedElevation = sci::asin(correctedZ);
	correctedAzimuth = sci::atan2(correctedX, correctedY);
}

degreeF angleBetweenDirections(degreeF elevation1, degreeF azimuth1, degreeF elevation2, degreeF azimuth2)
{
	return sci::acos(sci::cos(degreeF(90) - elevation1)*sci::cos(degreeF(90) - elevation2) + sci::sin(degreeF(90) - elevation1)*sin(degreeF(90) - elevation2)*sci::cos(azimuth2 - azimuth1));
}

sci::string getFormattedDateTime(const sci::UtcTime &time, sci::string dateSeparator, sci::string timeSeparator, sci::string dateTimeSeparator)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << time.getYear() << dateSeparator
		<< std::setw(2) << time.getMonth() << dateSeparator
		<< std::setw(2) << time.getDayOfMonth() << dateTimeSeparator
		<< std::setw(2) << time.getHour() << timeSeparator
		<< std::setw(2) << time.getMinute() << timeSeparator
		<< std::setw(2) << std::floor(time.getSecond());

	return timeString.str();
}

sci::string getFormattedDateOnly(const sci::UtcTime &time, sci::string separator)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << time.getYear() << separator
		<< std::setw(2) << time.getMonth() << separator
		<< std::setw(2) << time.getDayOfMonth();

	return timeString.str();
}

sci::string getFormattedTimeOnly(const sci::UtcTime &time, sci::string separator)
{
	sci::ostringstream timeString;
	timeString << std::setfill(sU('0'));
	timeString << std::setw(2) << time.getHour() << separator
		<< std::setw(2) << time.getMinute() << separator
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



//this function takes an array of angles and starting from
//the second element it adds or subtracts mutiples of 360 degrees
//so that the absolute differnce between two elements is never
//more than 180 degrees
void makeContinuous(sci::GridData<degreeF, 1> &angles)
{
	if (angles.size() < 2)
		return;

	//find the first valid angle
	auto iter = angles.begin();
	for (; iter != angles.end(); ++iter)
	{
		if (*iter == *iter)
			break;
	}
	if (iter == angles.end())
		return;

	//if we get to here then we have found the first valid angle.
	//move to the next angle which is the first one we may need
	//to modify
	++iter;

	//Now move throught the array incrementing the points as needed
	for (; iter != angles.end(); ++iter)
	{
		degreeF jumpAmount = sci::ceil<unitlessF::unit>((*(iter - 1) - *iter - degreeF(180)) / degreeF(360))*degreeF(360);
		*iter += jumpAmount;
	}
}

void OutputAmfNcFile::writeTimeAndLocationData(const Platform &platform)
{
	sci::GridData<double, 1> secondsAfterEpoch(std::array<size_t, 1>{ m_times.size() });

	sci::UtcTime epoch(1970, 1, 1, 0, 0, 0.0);
	for (size_t i = 0; i < m_times.size(); ++i)
	{
		secondsAfterEpoch[i] = (m_times[i] - epoch).value<secondF>();
	}

	write(*m_timeVariable, m_times-epoch);
	write(*m_dayOfYearVariable, m_dayOfYears);
	write(*m_yearVariable, m_years);
	write(*m_monthVariable, m_months);
	write(*m_dayVariable, m_dayOfMonths);
	write(*m_hourVariable, m_hours);
	write(*m_minuteVariable, m_minutes);
	write(*m_secondVariable, m_seconds);

	write(*m_latitudeVariable, m_latitudes);
	write(*m_longitudeVariable, m_longitudes);

	if (m_speeds.size() > 0)
	{
		if (m_instrumentPitchVariable)
		{
			write(*m_instrumentPitchVariable, m_elevations);
			write(*m_instrumentPitchStdevVariable, m_elevationStdevs);
			write(*m_instrumentPitchMinVariable, m_elevationMins);
			write(*m_instrumentPitchMaxVariable, m_elevationMaxs);
			write(*m_instrumentPitchRateVariable, m_elevationRates);
		}

		if (m_instrumentYawVariable)
		{
			write(*m_instrumentYawVariable, m_azimuths);
			write(*m_instrumentYawStdevVariable, m_azimuthStdevs);
			write(*m_instrumentYawMinVariable, m_azimuthMins);
			write(*m_instrumentYawMaxVariable, m_azimuthMaxs);
			write(*m_instrumentYawRateVariable, m_azimuthRates);
		}

		if (m_instrumentRollVariable)
		{
			write(*m_instrumentRollVariable, m_rolls);
			write(*m_instrumentRollStdevVariable, m_rollStdevs);
			write(*m_instrumentRollMinVariable, m_rollMins);
			write(*m_instrumentRollMaxVariable, m_rollMaxs);
			write(*m_instrumentRollRateVariable, m_rollRates);
		}

		if(m_courseVariable)
			write(*m_courseVariable, m_courses);
		if(m_speedVariable)
			write(*m_speedVariable, m_speeds);
		if(m_orientationVariable)
			write(*m_orientationVariable, m_headings);
	}


}

std::vector<std::pair<sci::string, CellMethod>> getLatLonCellMethod(FeatureType featureType, DeploymentMode deploymentMode)
{
	if (featureType == FeatureType::trajectory)
		return std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}};
	if (deploymentMode == DeploymentMode::sea)
		return std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::mean}};
	if (deploymentMode == DeploymentMode::air)
		return std::vector<std::pair<sci::string, CellMethod>>{ {sU("time"), CellMethod::point}};

	//land has no cell method
	return
		std::vector<std::pair<sci::string, CellMethod>>(0);
}

/*OutputAmfSeaNcFile::OutputAmfSeaNcFile(const sci::string &directory,
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
	const std::vector<sci::NcDimension *> &nonTimeDimensions)
	:OutputAmfNcFile(directory, instrumentInfo, author, processingsoftwareInfo, calibrationInfo, dataInfo, projectInfo, platformInfo, comment, times)
{
	write(AmfNcLatitudeVariable(*this, getTimeDimension()), latitudes);
	write(AmfNcLongitudeVariable(*this, getTimeDimension()), longitudes);
}*/