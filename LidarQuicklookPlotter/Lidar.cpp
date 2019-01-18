#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"HplHeader.h"
#include"HplProfile.h"
#include"ProgressReporter.h"
#include<svector/sstring.h>

void LidarBackscatterDopplerProcessor::readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent)
{
	m_hasData = false;
	m_profiles.clear();

	std::fstream fin;
	fin.open(sci::nativeUnicode(inputFilename), std::ios::in);
	if (!fin.is_open())
		throw(sU("Could not open lidar file ") + inputFilename + sU("."));

	progressReporter << sU("Reading file.\n");
	fin >> m_hplHeader;

	bool readingOkay = true;
	while (readingOkay)
	{
		m_profiles.resize(m_profiles.size() + 1);
		readingOkay = m_profiles.back().readFromStream(fin, m_hplHeader);
		if (!readingOkay) //we hit the end of the file while reading this profile
			m_profiles.resize(m_profiles.size() - 1);
		if (readingOkay)
		{
			if (m_profiles.size() == 1)
				progressReporter << sU("Read profile 1");
			else if (m_profiles.size() <= 50)
				progressReporter << sU(", ") << m_profiles.size();
			else if (m_profiles.size() % 10 == 0)
				progressReporter << sU(", ") << m_profiles.size() - 9 << "-" << m_profiles.size();
		}
		if (progressReporter.shouldStop())
			break;
	}
	if (progressReporter.shouldStop())
	{
		progressReporter << ", stopped at user request.\n";
		fin.close();
		return;

	}
	progressReporter << ", reading done.\n";

	bool gatesGood = true;
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		std::vector<size_t> gates = m_profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			if (gates[j] != j)
			{
				sci::ostringstream error;
				error << sU("The plotting code currently assumes gates go 0, 1, 2, 3, ... but profile ") << i << sU(" (0 indexed) in file ") << inputFilename << sU(" did not have this layout.");
				throw(error.str());
			}
	}
}


std::vector<second> LidarBackscatterDopplerProcessor::getTimes() const
{
	std::vector<second> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getTime<second>();
	}
	return result;
}

std::vector<std::vector<perSteradianPerMetre>> LidarBackscatterDopplerProcessor::getBetas() const
{
	std::vector<std::vector<perSteradianPerMetre>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getBetas();
	}
	return result;
}

std::vector<std::vector<metrePerSecond>> LidarBackscatterDopplerProcessor::getDopplerVelocities() const
{
	std::vector<std::vector<metrePerSecond>> result(m_profiles.size());
	for (size_t i = 0; i < m_profiles.size(); ++i)
	{
		result[i] = m_profiles[i].getDopplerVelocities();
	}
	return result;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateBoundariesForPlotting() const
{
	//because thee may be gate overlapping, for plotting the lower and upper gate boundaries
	//aren't quite what we want.
	metre interval = m_hplHeader.rangeGateLength;
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (m_profiles[0].nGates() > 533)
		interval /= unitless(m_hplHeader.pointsPerGate);

	std::vector<metre> result = getGateCentres() - unitless(0.5)*interval;
	result.push_back(result.back() + interval);

	return result;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateLowerBoundaries() const
{
	if (m_profiles.size() == 0)
		return std::vector<metre>(0);

	std::vector<metre> result(m_profiles[0].nGates() + 1);
	for (size_t i = 0; i < m_profiles.size(); ++i)
		result[i] = unitless(i) * m_hplHeader.rangeGateLength;
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (m_profiles[0].nGates() > 533)
		result /= unitless(m_hplHeader.pointsPerGate);
	return result;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateUpperBoundaries() const
{
	return getGateLowerBoundaries() + m_hplHeader.rangeGateLength;
}

std::vector<metre> LidarBackscatterDopplerProcessor::getGateCentres() const
{
	return getGateLowerBoundaries() + unitless(0.5) * m_hplHeader.rangeGateLength;
}

void LidarStareProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i] != metre(std::numeric_limits<double>::max()))
			rangeLimitedfilename << sU("_maxRange_") << maxRanges[i];

		splotframe *window;
		splot2d *plot;
		setupCanvas(&window, &plot, sU(""), parent);
		WindowCleaner cleaner(window);

		std::shared_ptr<PhysicalGridData<second::unit, metre::unit, perSteradianPerMetre::unit>> gridData(new PhysicalGridData<second::unit, metre::unit, perSteradianPerMetre::unit>(getTimes(), getGateBoundariesForPlotting(), getBetas(), g_lidarColourscale, true, true));
		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Time"));
		plot->getxaxis()->settimeformat("%H:%M:%S");
		plot->getyaxis()->settitle(sU("Height ") + gridData->getYAxisUnits());

		if (getGateBoundariesForPlotting().back() > maxRanges[i])
			plot->setmaxy(sci::physicalsToValues<decltype(gridData)::element_type::yUnitType>(maxRanges[i]));

		createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
	}
}

class RhiTransformer : public splotTransformer
{
public:
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		radian elevation(oldx);
		metre range(oldy);
		newx = (range * sci::cos(elevation)).value<metre>();
		newy = (range * sci::sin(elevation)).value<metre>();
	}
};

void LidarRhiProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i] != metre(std::numeric_limits<double>::max()))
			rangeLimitedfilename << sU("_maxRange_") << maxRanges[i];

		splotframe *window;
		splot2d *plot;
		setupCanvas(&window, &plot, sU(""), parent);
		WindowCleaner cleaner(window);

		std::vector<std::vector<perSteradianPerMetre>> betas = getBetas();
		std::vector<radian> angles(betas.size() + 1);
		std::vector<metre> ranges=getGateBoundariesForPlotting();

		radian angleIntervalDeg(M_PI / (betas.size() - 1));
		for (size_t i = 0; i < angles.size(); ++i)
			angles[i] = unitless(i - 0.5)*angleIntervalDeg;

		std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>> gridData(new PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>(angles, ranges, betas, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new RhiTransformer)));
		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Time"));
		plot->getxaxis()->settimeformat("%H:%M:%S");
		plot->getyaxis()->settitle(sU("Height ") + gridData->getYAxisUnits());

		if (getGateBoundariesForPlotting().back() > maxRanges[i])
			plot->setmaxy(sci::physicalsToValues<decltype(gridData)::element_type::yUnitType>(maxRanges[i]));

		createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
	}
}

class VadPlanTransformer : public splotTransformer
{
public:
	VadPlanTransformer(radian elevation)
		:m_cosElevationRad(sci::cos(elevation)),
		m_sinElevationRad(sci::sin(elevation))
	{

	}
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		radian azimuth(oldx);

		metre range(oldy);
		metre horizDistance = range * m_cosElevationRad;

		newx = (horizDistance * sci::sin(azimuth)).value<metre>();
		newy = (horizDistance * sci::cos(azimuth)).value<metre>();
	}
private:
	const unitless m_cosElevationRad;
	const unitless m_sinElevationRad;
};

class VadSideTransformer : public splotTransformer
{
public:
	VadSideTransformer(radian elevation)
		:m_cosElevationRad(sci::cos(elevation)),
		m_sinElevationRad(sci::sin(elevation))
	{

	}
	//note that the input azimuths for dx, should be centred on the viewing direction 
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		radian enteredAzimuth = oldx;
		//clip the azimuth to the view azimuth +/-90.
		radian clippedAzimuth = std::max(std::min(enteredAzimuth, radian(M_PI/2.0)), radian(-M_PI/2.0));

		metre range = oldy;
		metre horizDistance = range * m_cosElevationRad;
		metre verticalDistance = range * m_sinElevationRad;


		newx = (horizDistance * sci::sin(clippedAzimuth)).value<metre>();
		newy = verticalDistance.value<metre>();
	}
private:
	const unitless m_cosElevationRad;
	const unitless m_sinElevationRad;
};

void LidarVadProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		plotDataPlan(outputFilename + sU("_plan"), maxRanges[i], progressReporter, parent);
		plotDataPlan(outputFilename + sU("_cone"), maxRanges[i], progressReporter, parent);
		plotDataPlan(outputFilename + sU("_unwrapped"), maxRanges[i], progressReporter, parent);
	}
}

void LidarVadProcessor::plotDataPlan(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::ostringstream rangeLimitedfilename;
	rangeLimitedfilename << outputFilename;
	if (maxRange != metre(std::numeric_limits<double>::max()))
		rangeLimitedfilename << sU("_maxRange_") << maxRange;

	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, sU(""), parent);
	WindowCleaner cleaner(window);


	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetre>> sortedBetas;
	std::vector<radian> sortedElevations;
	std::vector<radian> sortedMidAzimuths;
	std::vector<radian> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);
	std::vector<metre> ranges = getGateBoundariesForPlotting();

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less like a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)m_nSegmentsMin / (double)sortedBetas.size());

	//Plot each segment as a separate data set
	std::vector< std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>>gridData(sortedBetas.size());
	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		std::vector<radian> segmentAzimuths(duplicationFactor + 1);
		std::vector<std::vector<perSteradianPerMetre>> segmentBetas(duplicationFactor);
		for (size_t j = 0; j < duplicationFactor; ++j)
		{
			segmentAzimuths[j] = azimuths[i] + (azimuths[i + 1] - azimuths[i]) / (unitless)(duplicationFactor*(double)j);
			segmentBetas[j] = sortedBetas[i];
		}
		segmentAzimuths.back() = azimuths[i + 1];
		gridData[i] = std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>(new PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>(segmentAzimuths, ranges, segmentBetas, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadPlanTransformer(sortedElevations[i]))));
	}
	for (size_t i = 0; i < gridData.size(); ++i)
		plot->addData(gridData[i]);

	//manually set limits as the transformer messes that up
	metre maxHorizDistance = 0.0;
	metre farthestRange = ranges.back();
	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		metre thisHorizDistance = std::min(farthestRange, maxRange)*sci::cos(sortedElevations[i]);
		if (thisHorizDistance > maxHorizDistance)
			maxHorizDistance = thisHorizDistance;
	}
	plot->setxlimits(-maxHorizDistance.value<decltype(gridData)::value_type::element_type::xUnitType>(), maxHorizDistance.value<decltype(gridData)::value_type::element_type::xUnitType>());
	plot->setylimits(-maxHorizDistance.value<decltype(gridData)::value_type::element_type::yUnitType>(), maxHorizDistance.value<decltype(gridData)::value_type::element_type::yUnitType>());

	plot->getxaxis()->settitle(sU("Horizontal Distance ") + gridData[0]->getXAxisUnits());
	plot->getyaxis()->settitle(sU("Horizontal Distance ") + gridData[0]->getYAxisUnits());


	createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
}


void LidarVadProcessor::plotDataCone(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	WindowCleaner cleaner(window);
	window->SetClientSize(1000, 2000);
	splot2d *plot;

	plot = window->addplot(0.1, 0.8, 0.8, 0.15, false, false);
	plotDataCone( radian(0.0), maxRange, plot);

	plot = window->addplot(0.1, 0.55, 0.8, 0.15, false, false);
	plotDataCone(radian(M_PI/2.0), maxRange, plot);

	plot = window->addplot(0.1, 0.3, 0.8, 0.15, false, false);
	plotDataCone(radian(M_PI), maxRange, plot);

	plot = window->addplot(0.1, 0.05, 0.8, 0.15, false, false);
	plotDataCone(radian(M_PI*3.0/2.0), maxRange, plot);

	createDirectoryAndWritePlot(window, outputFilename, 1000, 2000, progressReporter);
}

void LidarVadProcessor::plotDataCone(radian viewAzimuth, metre maxRange, splot2d * plot)
{
	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetre>> sortedBetas;
	std::vector<radian> sortedElevations;
	std::vector<radian> sortedMidAzimuths;
	std::vector<radian> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);

	std::vector<metre> ranges = getGateBoundariesForPlotting();

	//Plot each segment as a separate data set

	std::vector<std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>> plotData;
	std::vector<radian> absRelativeAzimuths;

	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		std::vector<std::vector<perSteradianPerMetre>> segmentData(1);
		segmentData[0] = sortedBetas[i];
		radian lowerReleativeAzimuth = azimuths[i] - viewAzimuth;
		radian upperReleativeAzimuth = azimuths[i + 1] - viewAzimuth;
		if (upperReleativeAzimuth < lowerReleativeAzimuth)
			upperReleativeAzimuth += radian(2.0*M_PI);
		if (lowerReleativeAzimuth >= radian(M_PI))
		{
			lowerReleativeAzimuth -= radian(2.0*M_PI);
			upperReleativeAzimuth -= radian(2.0*M_PI);
		}
		if (upperReleativeAzimuth <= -radian(M_PI))
		{
			lowerReleativeAzimuth += radian(2.0*M_PI);
			upperReleativeAzimuth += radian(2.0*M_PI);
		}

		//don't plot if limits are outside view range
		if (upperReleativeAzimuth <= -radian(M_PI/2.0) || lowerReleativeAzimuth >= radian(M_PI / 2.0))
			continue;


		std::vector<radian> segmentAzimuths = std::vector<radian>{ std::max(lowerReleativeAzimuth, radian(-M_PI/2.)), std::min(upperReleativeAzimuth, radian(M_PI/2.0)) };

		plotData.push_back(std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>(new PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>(segmentAzimuths, ranges, segmentData, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadSideTransformer(sortedElevations[i])))));
		absRelativeAzimuths.push_back(sci::abs(lowerReleativeAzimuth + upperReleativeAzimuth) / unitless(2.0));
	}

	//plot the segments farthest first.
	std::vector<size_t> plotDataNewLocations;
	std::vector<radian> sortedAbsRelativeAzimuths;
	sci::sort(absRelativeAzimuths, sortedAbsRelativeAzimuths, plotDataNewLocations);
	plotData = sci::reorder(plotData, plotDataNewLocations);
	for (size_t i = 0; i < plotData.size(); ++i)
		plot->addData(plotData[i]);

	plot->setxlimits(-ranges.back().value<decltype(plotData)::value_type::element_type::xUnitType>(), ranges.back().value<decltype(plotData)::value_type::element_type::xUnitType>());
	plot->setylimits(0, ranges.back().value<decltype(plotData)::value_type::element_type::yUnitType>());


	plot->getxaxis()->settitle(sU("Horizontal Distance ") + plotData[0]->getXAxisUnits());
	plot->getyaxis()->settitle(sU("Height ") + plotData[0]->getYAxisUnits());
	plot->getxaxis()->settitlesize(15);
	plot->getyaxis()->settitlesize(15);
	plot->getxaxis()->setlabelsize(15);
	plot->getyaxis()->setlabelsize(15);
	plot->getyaxis()->settitledistance(4.5);
}

void LidarVadProcessor::plotDataUnwrapped(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	WindowCleaner cleaner(window);
	window->SetClientSize(1000, 2000);
	splot2d *plot;

	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetre>> sortedBetas;
	std::vector<radian> sortedElevations;
	std::vector<radian> sortedMidAzimuths;
	std::vector<radian> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);

	std::vector<metre> ranges = getGateBoundariesForPlotting();

	std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit>> gridData(new PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit>(azimuths, ranges, sortedBetas, g_lidarColourscale, true, true));

	plot->addData(gridData);

	plot->getxaxis()->settitle(sU("Azimuth ")+gridData->getXAxisUnits());
	plot->getyaxis()->settitle(sU("Range ")+gridData->getYAxisUnits());
	if (ranges.back() > maxRange)
		plot->setmaxy(maxRange.value<decltype(gridData)::element_type::yUnitType>());
	plot->setxlimits(0, M_PI*2.0);

	createDirectoryAndWritePlot(window, outputFilename, 1000, 1000, progressReporter);
}

void LidarVadProcessor::getDataSortedByAzimuth(std::vector<std::vector<perSteradianPerMetre>> &sortedBetas, std::vector<radian> &sortedElevations, std::vector<radian> &sortedMidAzimuths, std::vector<radian> &azimuthBoundaries)
{

	std::vector<radian> midAzimuths = getAzimuths();
	std::vector<size_t> newLocations;
	sci::sort(midAzimuths, sortedMidAzimuths, newLocations);
	std::vector<std::vector<perSteradianPerMetre>> betas = getBetas();
	sortedBetas = sci::reorder(betas, newLocations);
	std::vector<radian> elevations = getElevations();
	sortedElevations = sci::reorder(elevations, newLocations);

	azimuthBoundaries.resize(sortedBetas.size() + 1);
	if (sortedMidAzimuths.back() > sortedMidAzimuths[0])
		azimuthBoundaries[0] = (sortedMidAzimuths[0] + sortedMidAzimuths.back() - radian(M_PI*2.0)) / unitless(2.0);
	else
		azimuthBoundaries[0] = (sortedMidAzimuths[0] + sortedMidAzimuths.back()) / unitless(2.0);

	for (size_t i = 1; i < azimuthBoundaries.size() - 1; ++i)
	{
		if (sortedMidAzimuths[i - 1] > sortedMidAzimuths[i])
			azimuthBoundaries[i] = (sortedMidAzimuths[i] + sortedMidAzimuths[i - 1] - radian(M_PI*2.0)) / unitless(2.0);
		else
			azimuthBoundaries[i] = (sortedMidAzimuths[i] + sortedMidAzimuths[i - 1]) / unitless(2.0);
	}

	azimuthBoundaries.back() = azimuthBoundaries[0] + radian(M_PI * 2);
}

void LidarUserProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i] != metre(std::numeric_limits<double>::max()))
			rangeLimitedfilename << sU("_maxRange_") << maxRanges[i];

		splotframe *window;
		splot2d *plot;
		setupCanvas(&window, &plot, sU(""), parent);
		WindowCleaner cleaner(window);

		std::shared_ptr<PhysicalGridData<second::unit, metre::unit, perSteradianPerMetre::unit>> gridData(new PhysicalGridData<second::unit, metre::unit, perSteradianPerMetre::unit>(getTimes(), getGateBoundariesForPlotting(), getBetas(), g_lidarColourscale, true, true));
		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Time"));
		plot->getxaxis()->settimeformat("%H:%M:%S");
		plot->getyaxis()->settitle(sU("Height ") + gridData->getYAxisUnits());

		if (getGateBoundariesForPlotting().back() > maxRanges[i])
			plot->setmaxy(sci::physicalsToValues<decltype(gridData)::element_type::yUnitType>(maxRanges[i]));

		createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
	}
}