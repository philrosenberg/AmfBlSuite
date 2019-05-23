#pragma once

#include<svector/Units.h>

typedef sci::Physical<sci::Metre<>, double> metre;
typedef sci::Physical<sci::Second<>, double> second;
typedef sci::Physical<sci::Kelvin<>, double> kelvin;
typedef sci::Physical<sci::Degree<>, double> degree;
//typedef sci::Physical<sci::Radian<>, double> radian;
typedef sci::Physical<sci::Hertz<>, double> hertz;
typedef sci::Physical<sci::Hertz<1, 6>, double> megahertz;
typedef sci::Physical<sci::Unitless, double> unitless;
typedef sci::Physical<sci::Percent, double> percent;
typedef sci::Physical<sci::Volt<1, -3>, double> millivolt;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Steradian<>, sci::Metre<>>, double> steradianPerMetre;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Steradian<>, sci::Metre<1, 3>>, double> steradianPerKilometre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Steradian<-1>, sci::Metre<-1>>, double> perSteradianPerMetre;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Metre<>, sci::Second<>>, double> metrePerSecond;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Degree<>, sci::Second<>>, double> degreePerSecond;
typedef sci::Physical<sci::Metre<1, -3>, double> millimetre;
typedef sci::Physical<sci::Metre<1, -1>, double> perMetre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<-3>, sci::Metre<-1, -3>>, double> perMetreCubedPerMillimetre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<-3>, sci::Metre<-6, -3>>, double> reflectivity;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<1, -3>, sci::Hour<-1>>, double> millimetrePerHour;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Gram<>, sci::Metre<-3>>, double> gramPerMetreCubed;
typedef sci::Physical<sci::Pascal<1, 2>, double> hectoPascal;


template<class REFERENCE_UNIT>
class Decibel
{
public:
	static sci::Physical<typename REFERENCE_UNIT::unit, double> decibelToLinear(sci::Physical<sci::Unitless, double> value)
	{
		return std::pow(10, value.value<sci::Unitless>() / 10.0);
	}
	template<class U>
	static sci::Physical<sci::Unitless, double> linearToDecibel(sci::Physical<U, double> value)
	{
		return sci::Physical<sci::Unitless, double>(std::log10(value.value<typename REFERENCE_UNIT::unit>())*10.0);
	}
	typedef sci::Physical<typename REFERENCE_UNIT::unit, double> referencePhysical;
	typedef typename REFERENCE_UNIT::unit referenceUnit;
	typedef double valueType;
};