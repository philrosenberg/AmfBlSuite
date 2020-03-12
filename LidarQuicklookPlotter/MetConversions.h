#pragma once

#include"units.h"

//////////////////////////////////////////////////////////////////////
//All the parameters here have the following units                  //
//Temperature		K                                               //
//Pressure			hPa                                             //
//Mixing ratio		kg kg-1                                         //
//Specific humidity	kg kg-1                                         //
//Relative humidity 0-1 scale                                       //
//note of course that dew point is equivalent to boiling point when //
//vapour pressure is set as the actual pressure and vapour pressure //
//is the saturation vapour pressure when dew point is set to the    //
//actual temperature.                                               //
//The only physics that go into the water conversions is the Wexler //
//relation, linking temperature and saturation vapour pressure, or  //
//if you use the _bolton versions then the Bolton relation is used. //
//Bolton should be a bit quicker as it can be solved analytically   //
//in both directions, but Wexler is more accurate and covers a wider//
//range. See Bolton 1980, The computation of equivalent potential   //
//temperature, Monthly Weather Reviews, 108 p1046 for both          //
//equations. The same bolton paper is used to define theta_e.       //
//For virtual (potential) temperatures, calculations can include the//
//effect of drag by hydrometeors by including condensed mixing      //
//ratio. In these instances we may expect to be saturated so you    //
//could derive vapour mixing ratio from the temperature.            //
//////////////////////////////////////////////////////////////////////

//constants

typedef unitless::valueType MET_FLT;

//g0-g7 constants for the wexler equation
const MET_FLT gWexler[] = { (MET_FLT)-2.9912729e3, (MET_FLT)-6.0170128e3, (MET_FLT)1.887643854e1, (MET_FLT)-2.8354721e-2,
						(MET_FLT)1.7838301e-5, (MET_FLT)-8.4150417e-10, (MET_FLT)4.4412543e-13, (MET_FLT)2.858487e0 };
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::MultipliedEncodedUnit<sci::Joule<>, sci::Kilogram<-1>>, sci::Kelvin<-1>>, MET_FLT> heatCapacity;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Metre<>, sci::Second<-2>>, MET_FLT> acceleration;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Radian<>, sci::Second<-1>>, MET_FLT> radianPerSecond;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Joule<>, sci::Kilogram<-1>>, MET_FLT> joulePerKilogram;
typedef sci::Physical<sci::MultipliedEncodedUnit<sci::Kilogram<>, sci::Metre<-3>>, MET_FLT> kilogramPerMetreCubed;
const heatCapacity dryAirGasConstant(287.04f); //J kg-1 K-1
const heatCapacity dryAirCp(1005.7f); //J kg-1 K-1
const unitless dryAirKappa(0.2854f); //1-cv/cp or 1-gamma
const heatCapacity waterSpecificHeat(4190.0f); //J kg-1 K-1 for T>=0, i.e. liquid
const heatCapacity waterVapourGasConstant(461.50f); //J kg-1 K-1
const heatCapacity waterVapourCp(1875.0f); // J kg-1 K-1
const unitless ratioGasConstants = dryAirGasConstant / waterVapourGasConstant;
const joulePerKilogram latentHeatVapourisationWater(2501000.0f); //J/kg

const radianPerSecond earthAngularSpeed((MET_FLT)7.2921159e-5);
const metre earthRadius((MET_FLT)6371.e3);
const acceleration earthGravity((MET_FLT)9.81); //m/s2

inline joulePerKilogram waterLatentHeatVapourisation(kelvin temperature)
{
	return joulePerKilogram(2.501e6) - heatCapacity(0.00237e6)*temperature;
}

inline kilogramPerMetreCubed density(kelvin temperature, hectoPascal pressure)
{
	return pressure / temperature / dryAirGasConstant;
}

//water vapour conversions


inline unitless massMixingRatio(hectoPascal vapourpressure, hectoPascal pressure)
{
	return unitless((unitless::valueType)0.622)*vapourpressure / (pressure - vapourpressure);
}
	
inline unitless massMixingRatio(unitless specificHumidity)
{
	return specificHumidity / (unitless(1.0) - specificHumidity);
}

inline unitless specificHumidity(unitless mixingratio)
{
	return mixingratio / (unitless(1.0) + mixingratio);
}

inline hectoPascal vapourPressureBolton(kelvin dewpoint)
{
	return hectoPascal((hectoPascal::valueType)6.112)*sci::exp(unitless((unitless::valueType)17.67)*(dewpoint - kelvin((kelvin::valueType)273.15)) / (dewpoint + kelvin((kelvin::valueType)(243.5 - 273.15))));
}

inline hectoPascal vapourPressureWexler(kelvin dewpoint)
{
	unitless sum = sci::Physical<sci::PoweredEncodedUnit<kelvin::unit, 2>, double>(-2.9912729e3) *sci::pow<-2>(dewpoint) +
		sci::Physical<sci::PoweredEncodedUnit<kelvin::unit, 1>, double>(-6.0170128e3) *sci::pow<-1>(dewpoint) +
		sci::Physical<sci::PoweredEncodedUnit<kelvin::unit, 0>, double>(1.887643854e1) *sci::pow<0>(dewpoint) +
		sci::Physical<sci::PoweredEncodedUnit<kelvin::unit, -1>, double>(-2.8354721e-2) *sci::pow<1>(dewpoint) +
		sci::Physical<sci::PoweredEncodedUnit<kelvin::unit, -2>, double>(1.7838301e-5) *sci::pow<2>(dewpoint) +
		sci::Physical<sci::PoweredEncodedUnit<kelvin::unit, -3>, double>(-8.4150417e-10) *sci::pow<3>(dewpoint) +
		sci::Physical<sci::PoweredEncodedUnit<kelvin::unit, -4>, double>(4.4412543e-13) *sci::pow<4>(dewpoint);
		
	return sci::exp(sum + unitless((unitless::valueType)2.858487) * sci::ln(dewpoint/kelvin((kelvin::valueType)1.0)))*hectoPascal((hectoPascal::valueType)0.01);
}

inline hectoPascal vapourPressureBolton(unitless relativehumidity, kelvin temperature)
{
	return  vapourPressureBolton(temperature)*relativehumidity;
}

inline hectoPascal vapourPressureWexler(unitless relativehumidity, kelvin temperature)
{
	return  vapourPressureWexler(temperature)*relativehumidity;
}

inline hectoPascal vapourPressureFromMassMixingRatio(unitless massMixingRatio, hectoPascal pressure)
{
	return massMixingRatio * pressure / (unitless((unitless::valueType)0.622) + massMixingRatio);
}

inline hectoPascal vapourPressureFromSpecificHumidity(unitless specificHumidity, hectoPascal pressure)
{
	return vapourPressureFromMassMixingRatio(massMixingRatio(specificHumidity), pressure);
}
	
inline unitless massMixingRatioBolton(kelvin dewpoint, hectoPascal pressure)
{
	return massMixingRatio(vapourPressureBolton(dewpoint), pressure);
}

inline unitless massMixingRatioWexler(kelvin dewpoint, hectoPascal pressure)
{
	return massMixingRatio(vapourPressureWexler(dewpoint), pressure);
}
	
inline unitless massMixingRatioBolton(unitless relativehumidity, kelvin temperature, hectoPascal pressure)
{
	hectoPascal saturationVapourPressure = vapourPressureBolton(temperature);
	return massMixingRatio(saturationVapourPressure*relativehumidity, pressure);
}

inline unitless massMixingRatioWexler(unitless relativehumidity, kelvin temperature, hectoPascal pressure)
{
	hectoPascal saturationVapourPressure = vapourPressureWexler(temperature);
	return massMixingRatio(saturationVapourPressure*relativehumidity, pressure);
}
	
inline unitless specificHumidity(hectoPascal vapourpressure, hectoPascal pressure)
{
	return specificHumidity(massMixingRatio(vapourpressure, pressure));
}
	
inline unitless specificHumidityBolton(unitless relativehumidity, kelvin temperature, hectoPascal pressure)
{
	return specificHumidity(massMixingRatioBolton(relativehumidity, temperature, pressure));
}

inline unitless specificHumidityWexler(unitless relativehumidity, kelvin temperature, hectoPascal pressure)
{
	return specificHumidity(massMixingRatioWexler(relativehumidity, temperature, pressure));
}

inline unitless specificHumidityBolton(kelvin dewpoint, hectoPascal pressure)
{
	return specificHumidity(massMixingRatioBolton(dewpoint, pressure));
}

inline unitless specificHumidityWexler(kelvin dewpoint, hectoPascal pressure)
{
	return specificHumidity(massMixingRatioWexler(dewpoint, pressure));
}

inline unitless relativeHumidityBolton(kelvin dewpoint, kelvin temperature)
{
	return vapourPressureBolton(dewpoint) / vapourPressureBolton(temperature);
}

inline unitless relativeHumidityWexler(kelvin dewpoint, kelvin temperature)
{
	return vapourPressureWexler(dewpoint) / vapourPressureWexler(temperature);
}
	
inline unitless relativeHumidityBolton(hectoPascal vapourpressure, kelvin temperature)
{
	return vapourpressure / vapourPressureBolton(temperature);
}

inline unitless relativeHumidityWexler(hectoPascal vapourpressure, kelvin temperature)
{
	return vapourpressure / vapourPressureWexler(temperature);
}

inline unitless relativeHumidityFromMassMixingRatioBolton(unitless mixingratio, kelvin temperature, hectoPascal pressure)
{
	return vapourPressureFromMassMixingRatio(mixingratio, pressure) / vapourPressureBolton(temperature);
}

inline unitless relativeHumidityFromMassMixingRatioWexler(unitless mixingratio, kelvin temperature, hectoPascal pressure)
{
	return vapourPressureFromMassMixingRatio(mixingratio, pressure) / vapourPressureWexler(temperature);
}

inline unitless relativeHumidityFromSpecificHumidityBolton(unitless specificHumidity, kelvin temperature, hectoPascal pressure)
{
	return relativeHumidityFromMassMixingRatioBolton(massMixingRatio(specificHumidity), temperature, pressure);
}

inline unitless relativeHumidityFromSpecificHumidityWexler(unitless specificHumidity, kelvin temperature, hectoPascal pressure)
{
	return relativeHumidityFromMassMixingRatioWexler(massMixingRatio(specificHumidity), temperature, pressure);
}

inline kelvin dewpointBolton(hectoPascal vapourpressure)
{
	unitless c = sci::ln(vapourpressure / hectoPascal((hectoPascal::valueType)6.112)) / unitless((unitless::valueType)17.67);
	return kelvin((kelvin::valueType)243.5) / (unitless((unitless::valueType)1.0) / c - unitless((unitless::valueType)1.0)) + kelvin((kelvin::valueType)273.15);
}

//double wexlerMinimisation(const std::vector<double> &fitparams, const std::vector<double> &fixedparams);
//template<class T>
//T dewpointfromvapourpressure(const T &vapourpressure)
//{
//	//first guess use the Bolton estimate
//	std::vector<double> dewpoint(1, dewpointfromvapourpressure_bolton(vapourpressure));
//	sci::minimise(dewpoint, std::vector<double>(1, vapourpressure), wexlerMinimisation);
//	return dewpoint[0];
//}
	
inline kelvin dewpointFromMassMixingRatioBolton(unitless massMixingRatio, hectoPascal pressure)
{
	return dewpointBolton(vapourPressureFromMassMixingRatio(massMixingRatio, pressure));
}

inline kelvin dewpointFromSpecificHumidityBolton(unitless specificHumidity, hectoPascal pressure)
{
	return dewpointBolton(vapourPressureFromSpecificHumidity(specificHumidity, pressure));
}
	
inline kelvin dewpoint(unitless relativehumidity, kelvin temperature)
{
	return dewpointBolton(vapourPressureBolton(relativehumidity, temperature));
}

//potential temperature and virtual potential temperature calculations

//potential temperature. Accurate to about 0.2K because we neglect variation of
//kappa with T and P
inline kelvin potentialTemperature(kelvin temperature, hectoPascal pressure)
{
	return temperature * sci::pow(hectoPascal((hectoPascal::valueType)1000.0) / pressure, dryAirKappa);
}
	
inline kelvin moistPotentialTemperature(kelvin temperature, hectoPascal pressure, unitless massMixingRatio)
{
	return temperature * sci::pow(hectoPascal((hectoPascal::valueType)1000.0) / pressure, dryAirKappa*(unitless((unitless::valueType)1.0) - unitless((unitless::valueType)0.28)*massMixingRatio));
}

//virtual temperature for no condensed water
inline kelvin virtualTemperature(kelvin temperature, unitless vapourMassMixingRatio)
{
	return temperature * (unitless((unitless::valueType)1.0) + vapourMassMixingRatio / unitless((unitless::valueType)0.622)) / (unitless((unitless::valueType)1.0) + vapourMassMixingRatio);
}

//virtual temperature with condensed water
inline kelvin virtualTemperature(kelvin temperature, unitless vapourMassMixingRatio, unitless condensedMassMixingRatio)
{
	return virtualTemperature(temperature, vapourMassMixingRatio) - temperature * condensedMassMixingRatio;
}

//virtual potential temperature for no condensed water
inline kelvin virtualpotentialtemperature(kelvin temperature, hectoPascal pressure, unitless vapourMassMixingRatio)
{
	return virtualTemperature(potentialTemperature(temperature, pressure), vapourMassMixingRatio);
}

//virtual potential temperature with condensed water
inline kelvin virtualpotentialtemperature(kelvin temperature, hectoPascal pressure, unitless vapourMassMixingRatio, unitless condensedMassMixingRatio)
{
	return virtualTemperature(potentialTemperature(temperature, pressure), vapourMassMixingRatio, condensedMassMixingRatio);
}


/*double liftingCondensationPressureMinimisation(const std::vector<double> &fitparams, const std::vector<double> &fixedparams);
//lifting condensation pressure
double liftingCondensationPressureFromMixingRatio(double mixingRatio, double temperature);
template<class T>
std::vector<T> liftingCondensationPressureFromMixingRatio(const std::vector<T> &mixingRatio, const std::vector<T> &temperature)
{
	assert(mixingRatio.size() == temperature.size());
	std::vector<T> result(mixingRatio.size());
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = liftingCondensationPressureFromMixingRatio(mixingRatio[i], temperature[i]);
	return result;
}

//lifting condensation temperature - note this isn't the same as the dew point because the dew point
//is a function of pressure
template<class T>
T liftingCondensationTemperaturefromLcp(const T &mixingRatio, const T &liftingCondensationPressure)
{
	return dewpointfrommixingratio(mixingRatio, liftingCondensationPressure);
}

template<class T>
T liftingCondensationTemperaturefromTemperature(const T &mixingRatio, const T &temperature)
{
	T liftingCondensationPressure = liftingCondensationPressureFromMixingRatio(mixingRatio, temperature);
	return liftingCondensationTemperaturefromLcp(mixingRatio, liftingCondensationPressure);
}

//Bolton 1980 Eq 15
template<class T>
T liftingCondensationTemperatureQuickFromDewPoint(const T &dewPoint, const T &temperature)
{
	return 1.0 / ((1.0 / (dewPoint - 56.0)) + sci::ln(temperature / dewPoint) / 800.0) + 56.0;
}

//Bolton 1980 Eq 21
template<class T>
T liftingCondensationTemperatureQuickFromVapourPressure(const T &vapourPressure, const T &temperature)
{
	return 2840.0 / (3.5*sci::ln(temperature) - sci::ln(vapourPressure) - 4.805) + 55.0;
}

//Bolton 1980 Eq 22
template<class T>
T liftingCondensationTemperatureQuickFromRelativeHumidity(const T &relativeHumidity, const T &temperature)
{
	return 1.0 / ((1.0 / (temperature - 55.0)) + sci::ln(relativeHumidity) / 2840.0) + 55.0;
}

//Bolton 1980 Eq 22
template<class T>
T liftingCondensationTemperatureQuickFromMixingRatio(const T &mixingRatio, const T &temperature, const T &pressure)
{
	T vapourPressure = vapourpressurefrommixingratio(mixingRatio, pressure);
	return liftingCondensationTemperatureQuickFromVapourPressure(vapourPressure, temperature);
}

//pseusoadiabatic theta_e from Bolton 1980	
template<class T>
T thetaeFromDewpoint(const T &temperature, const T &pressure, const T &dewpoint)
{
	T mixingRatio = mixingratiofromdewpoint(dewpoint, pressure);
	T thetae = thetaeFromMixingRatio(temperature, pressure, mixingRatio);
	return thetae;
}

//pseusoadiabatic theta_e from Bolton 1980
template<class T>
T thetaeFromRelativeHumidity(const T &temperature, const T &pressure, const T &relativehumidity)
{
	T mixingRatio = mixingratiofromrelativehumidity(relativehumidity, temperature, pressure);
	T thetae = thetaeFromMixingRatio(temperature, pressure, mixingRatio);
	return thetae;
}

//pseusoadiabatic theta_e from Bolton 1980
template<class T>
T thetaeFromVapourPressure(const T &temperature, const T &pressure, const T &vapourpressure)
{
	T mixingRatio = mixingratiofromvapourpressure(vapourpressure, pressure); //kg/kg
	T thetae = thetaeFromMixingRatio(temperature, pressure, mixingRatio);
	return thetae;
}

//pseusoadiabatic theta_e from Bolton 1980
template<class T>
T thetaeFromMixingRatio(const T &temperature, const T &pressure, const T &mixingratio)
{
	T liftingCondensationTemperature = liftingCondensationTemperatureQuickFromMixingRatio(mixingratio, temperature, pressure); //K
	T thetae = temperature * (std::pow((1000.0 / pressure), 0.2854*(1.0 - 0.28*mixingratio)))*std::exp((3376.0 / liftingCondensationTemperature - 2.54)*mixingratio*(1.0 + 0.81*mixingratio));
	return thetae;
}
template<class T>
std::vector<T> thetaeFromMixingRatio(const std::vector<T> &temperature, const std::vector<T> &pressure, const std::vector<T> &mixingratio)
{
	std::vector<T> liftingCondensationTemperature = liftingCondensationTemperatureQuickFromMixingRatio(mixingratio, temperature, pressure); //K
	std::vector<T> thetae = temperature * (sci::pow((1000.0 / pressure), 0.2854*(1.0 - 0.28*mixingratio)))*sci::exp((3376.0 / liftingCondensationTemperature - 2.54)*mixingratio*(1.0 + 0.81*mixingratio));
	return thetae;
}

//pseusoadiabatic theta_es from Bolton 1980
template<class T>
T thetaes(const T &temperature, const T &pressure)
{
	T mixingratio = mixingratiofromrelativehumidity(1.0, temperature, pressure);
	T thetae = temperature * (sci::pow((1000.0 / pressure), 0.2854*(1.0 - 0.28*mixingratio)))*sci::exp((3376.0 / temperature - 2.54)*mixingratio*(1.0 + 0.81*mixingratio));
	return thetae;
}

template<class T>
T mixedGasConstantFromVapourPressure(const T &pressure, const T &vapourPressure)
{
	T dryPressure = pressure - vapourPressure;
	return pressure * dryAirGasConstant*waterVapourGasConstant / (dryPressure*waterVapourGasConstant + vapourPressure * dryAirGasConstant);
}

template<class T>
T mixedGasConstantFromRelativeHumidity(const T &pressure, const T & temperature, const T &rh)
{
	T vapourPressure = vapourpressurefromrelativehumidity(rh, temperature);
	return mixedGasConstantFromVapourPressure(pressure, vapourPressure);
}

template<class T>
T mixedGasConstantFromMixingRatio(const T &pressure, const T &mixingRatio)
{
	T vapourPressure = vapourpressurefrommixingratio(mixingRatio, pressure);
	return mixedGasConstantFromVapourPressure(pressure, vapourPressure);
}

template<class T>
T mixedGasConstantFromSpecificHumidity(const T &pressure, const T &specificHumidity)
{
	T vapourPressure = vapourpressurefromspecifichumidity(specificHumidity, pressure);
	return mixedGasConstantFromVapourPressure(pressure, vapourPressure);
}

template<class T>
T mixedGasConstantFromDewPoint(const T &pressure, const T &dewPoint)
{
	T vapourPressure = vapourpressurefromdewpoint(dewPoint);
	return mixedGasConstantFromVapourPressure(pressure, vapourPressure);
}

inline double airCp(double specificHumidity)
{
	return (1.0 - specificHumidity)*dryAirCp + specificHumidity * waterVapourCp;
}

inline double dryAdiabaticLapseRate(double specificHumidity)
{
	return earthGravity / airCp(specificHumidity);
}

inline double moistAdiabaticLapseRate(double temperature, double pressure, double specificHumidity)
{
	double rh = relativehumidityfromspecifichumidity(specificHumidity, temperature, pressure);
	if (rh < 1.0)
		return dryAdiabaticLapseRate(specificHumidity);
	double mixingRatio = mixingratiofromspecifichumidity(specificHumidity);
	return earthGravity * (1.0 + latentHeatVapourisationWater * mixingRatio / dryAirGasConstant / temperature) / (dryAirCp + latentHeatVapourisationWater * latentHeatVapourisationWater*mixingRatio / waterVapourGasConstant / temperature / temperature);
}

inline double adiabaticCondensationRate(double temperature, double pressure, double specificHumidity)
{
	return airCp(specificHumidity) * (dryAdiabaticLapseRate(specificHumidity) - moistAdiabaticLapseRate(temperature, pressure, specificHumidity)) / latentHeatVapourisationWater;
}

inline double airDensity(double temperature, double pressure, double specificHumidity)
{
	double partialPressure = vapourpressurefromspecifichumidity(specificHumidity, pressure);
	return partialPressure / waterVapourGasConstant / temperature + (pressure - partialPressure) / dryAirGasConstant / temperature;
}

//This function calculates a profile starting at the initial conditions. It will use the dry adiabatic lapse rate until we reach saturation
//then it starts condensing material. It assumes hydrostatic equilibrium
inline std::vector<double> adiabaticSpecificCondensateProfile(double temperatureInitial, double pressureInitial, double specificHumidityInitial, double heightInterval, int nIntervals)
{
	std::vector<double> result(nIntervals);
	double currentTemperature = temperatureInitial;
	double currentSpecificHumidity = specificHumidityInitial;
	double currentPressure = pressureInitial;
	for (size_t i = 1; i < nIntervals; ++i)
	{
		double condensedAmount = heightInterval * adiabaticCondensationRate(currentTemperature, currentPressure, currentSpecificHumidity);
		double temperatureChange = -heightInterval * moistAdiabaticLapseRate(currentTemperature, currentPressure, currentSpecificHumidity);
		double pressureChange = -earthGravity * heightInterval*airDensity(currentTemperature, currentPressure, currentSpecificHumidity);
		currentSpecificHumidity -= condensedAmount;
		currentTemperature += temperatureChange;
		currentPressure += pressureChange;
		result[i] = result[i - 1] + condensedAmount;
	}
	return result;
}

//This function calculates a profile starting at the initial conditions and ASSUMING RH=1. It assumes hydrostatic equilibrium
inline std::vector<double> adiabaticSpecificCondensateProfile(double temperatureInitial, double specificHumidityInitial, double heightInterval, int nIntervals)
{
	double saturatedVapourPressure = vapourpressurefromrelativehumidity(1.0, temperatureInitial);
	double vapourPressurePerHpa = vapourpressurefromspecifichumidity(specificHumidityInitial, 1.0);
	double pressureInitial = saturatedVapourPressure / vapourPressurePerHpa;

	return adiabaticSpecificCondensateProfile(temperatureInitial, pressureInitial, specificHumidityInitial, heightInterval, nIntervals);
}
*/