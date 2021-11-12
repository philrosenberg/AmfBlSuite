#pragma once
#include<svector/sstring.h>
#include<vector>


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

inline sci::string getCoordinatesAttributeText(const std::vector<sci::string>& coordinates)
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

inline sci::string getCellMethodsAttributeText(const std::vector<std::pair<sci::string, CellMethod>>& cellMethods)
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
