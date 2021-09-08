#pragma once

#define UNITS_H_NOT_CP1253
#include<svector/Units.h>

typedef sci::Physical<sci::Metre<>, float> metreF;
typedef sci::Physical<sci::Second<>, double> second;
typedef sci::Physical<sci::Second<>, float> secondF;
typedef sci::Physical<sci::Kelvin<>, float> kelvinF;
typedef sci::Physical<sci::Degree<>, float> degreeF;
//typedef sci::Physical<sci::Radian<>, valueType> radian;
typedef sci::Physical<sci::Hertz<>, float> hertzF;
typedef sci::Physical<sci::Hertz<1, 6>, float> megahertzF;
typedef sci::Physical<sci::Unitless, float> unitlessF;
typedef sci::Physical<sci::Percent, float> percentF;
typedef sci::Physical<sci::Volt<1, -3>, float> millivoltF;
typedef sci::Physical<sci::DividedUnit<sci::Steradian<>, sci::Metre<>>, float> steradianPerMetreF;
typedef sci::Physical<sci::DividedUnit<sci::Steradian<>, sci::Metre<1, 3>>, float> steradianPerKilometreF;
typedef sci::Physical<sci::DividedUnit<sci::Steradian<-1>, sci::Metre<1, 3>>, float> perSteradianPerKilometreF;
typedef sci::Physical<sci::MultipliedUnit<sci::Steradian<-1>, sci::Metre<-1>>, float> perSteradianPerMetreF;
typedef sci::Physical<sci::Steradian<-1>, float> perSteradianF;
typedef sci::Physical<sci::DividedUnit<sci::Metre<>, sci::Second<>>, float> metrePerSecondF;
typedef sci::Physical<sci::DividedUnit<sci::Degree<>, sci::Second<>>, float> degreePerSecondF;
typedef sci::Physical<sci::Metre<1, -3>, float> millimetreF;
typedef sci::Physical<sci::Metre<1, -1>, float> perMetreF;
typedef sci::Physical<sci::MultipliedUnit<sci::Metre<-3>, sci::Metre<-1, -3>>, float> perMetreCubedPerMillimetreF;
typedef sci::Physical<sci::MultipliedUnit<sci::Metre<-3>, sci::Metre<-6, -3>>, float> reflectivityF;
typedef sci::Physical<sci::MultipliedUnit<sci::Metre<1, -3>, sci::Hour<-1>>, float> millimetrePerHourF;
typedef sci::Physical<sci::MultipliedUnit<sci::Gram<>, sci::Metre<-3>>, float> gramPerMetreCubedF;
typedef sci::Physical<sci::Pascal<1, 2>, float> hectoPascalF;
typedef sci::Physical<sci::MultipliedUnit<sci::Gram<>, sci::Metre<-2>>, float> gramPerMetreSquaredF;


template<class REFERENCE_UNIT>
class Decibel
{
public:
	typedef float valueType;
	static sci::Physical<typename REFERENCE_UNIT::unit, valueType> decibelToLinear(sci::Physical<sci::Unitless, valueType> value)
	{
		return std::pow(10, value.value<sci::Unitless>() / valueType(10.0));
	}
	template<class U>
	static sci::Physical<sci::Unitless, valueType> linearToDecibel(sci::Physical<U, valueType> value)
	{
		return sci::Physical<sci::Unitless, valueType>(std::log10(value.value<typename REFERENCE_UNIT::unit>())*valueType(10.0));
	}
	typedef sci::Physical<typename REFERENCE_UNIT::unit, valueType> referencePhysical;
	typedef typename REFERENCE_UNIT::unit referenceUnit;
};