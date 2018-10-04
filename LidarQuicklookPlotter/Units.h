#pragma once

#include<svector/Units.h>

typedef sci::Physical<sci::Metre<>> metre;
typedef sci::Physical<sci::Second<>> second;
typedef sci::Physical<sci::Kelvin<>> kelvin;
typedef sci::Physical<sci::Radian<>> radian;
typedef sci::Physical<sci::Hertz<>> hertz;
typedef sci::Physical<sci::Hertz<1,6>> megahertz;
typedef sci::Physical<sci::Unitless<>> unitless;
typedef sci::Physical<sci::Unitless<-2>> percent;
typedef sci::Physical<sci::Volt<1,-3>> millivolt;
typedef sci::Physical2<sci::Steradian<>, sci::Metre<-1>> steradianPerMetre;
typedef sci::Physical2<sci::Steradian<>, sci::Metre<-1, 3>> steradianPerKilometre;