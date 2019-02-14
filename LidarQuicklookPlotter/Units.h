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
typedef sci::Physical < sci::DividedEncodedUnit<sci::Metre<>, sci::Second<>>> metrePerSecond;
