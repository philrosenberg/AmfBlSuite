#pragma once

#include<svector/Units.h>

typedef sci::Physical<sci::Metre<>, float> metre;
typedef sci::Physical<sci::Second<>, double> second;
typedef sci::Physical<sci::Second<>, float> secondf;
typedef sci::Physical<sci::Kelvin<>, float> kelvin;
typedef sci::Physical<sci::Degree<>, float> degree;
//typedef sci::Physical<sci::Radian<>, valueType> radian;
typedef sci::Physical<sci::Hertz<>, float> hertz;
typedef sci::Physical<sci::Hertz<1, 6>, float> megahertz;
typedef sci::Physical<sci::Unitless, float> unitless;
typedef sci::Physical<sci::Percent, float> percent;
typedef sci::Physical<sci::Volt<1, -3>, float> millivolt;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Steradian<>, sci::Metre<>>, float> steradianPerMetre;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Steradian<>, sci::Metre<1, 3>>, float> steradianPerKilometre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Steradian<-1>, sci::Metre<-1>>, float> perSteradianPerMetre;
typedef sci::Physical<sci::Steradian<-1>, float> perSteradian;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Metre<>, sci::Second<>>, float> metrePerSecond;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Degree<>, sci::Second<>>, float> degreePerSecond;
typedef sci::Physical<sci::Metre<1, -3>, float> millimetre;
typedef sci::Physical<sci::Metre<1, -1>, float> perMetre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<-3>, sci::Metre<-1, -3>>, float> perMetreCubedPerMillimetre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<-3>, sci::Metre<-6, -3>>, float> reflectivity;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<1, -3>, sci::Hour<-1>>, float> millimetrePerHour;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Gram<>, sci::Metre<-3>>, float> gramPerMetreCubed;
typedef sci::Physical<sci::Pascal<1, 2>, float> hectoPascal;


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