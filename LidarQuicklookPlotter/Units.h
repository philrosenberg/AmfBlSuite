#pragma once

#include<svector/Units.h>

typedef sci::Physical<sci::Metre<>> metre;
typedef sci::Physical<sci::Second<>> second;
typedef sci::Physical<sci::Kelvin<>> kelvin;
typedef sci::Physical<sci::Degree<>> degree;
//typedef sci::Physical<sci::Radian<>> radian;
typedef sci::Physical<sci::Hertz<>> hertz;
typedef sci::Physical<sci::Hertz<1,6>> megahertz;
typedef sci::Physical<sci::Unitless> unitless;
typedef sci::Physical<sci::Percent> percent;
typedef sci::Physical<sci::Volt<1,-3>> millivolt;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Steradian<>, sci::Metre<>>> steradianPerMetre;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Steradian<>, sci::Metre<1, 3>>> steradianPerKilometre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Steradian<-1>, sci::Metre<-1>>> perSteradianPerMetre;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Metre<>, sci::Second<>>> metrePerSecond;
typedef sci::Physical<sci::DividedEncodedUnit<sci::Degree<>, sci::Second<>>> degreePerSecond;
typedef sci::Physical<sci::Metre<1, -3>> millimetre;
typedef sci::Physical<sci::Metre<1, -1>> perMetre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<-3>, sci::Metre<-1,-3>>> perMetreCubedPerMillimetre;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<-3>, sci::Metre<-6, -3>>> reflectivity;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<1, -3>, sci::Hour<-1>>> millimetrePerHour;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Gram<>, sci::Metre<-3>>> gramPerMetreCubed;


template<class REFERENCE_UNIT>
class Decibel
{
public:
	static sci::Physical<typename REFERENCE_UNIT::unit> decibelToLinear(sci::Physical<sci::Unitless> value)
	{
		return std::pow(10, value.value<sci::Unitless>() / 10.0);
	}
	template<class U>
	static sci::Physical<sci::Unitless> linearToDecibel(sci::Physical<U> value)
	{
		return sci::Physical<sci::Unitless>(std::log10(value.value<typename REFERENCE_UNIT::unit>())*10.0);
	}
};