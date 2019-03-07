#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"HplHeader.h"
#include"HplProfile.h"
#include"ProgressReporter.h"
#include<svector/sstring.h>
#include"ProgressReporter.h"

class VadPlanTransformer : public splotTransformer
{
public:
	VadPlanTransformer(degree elevation)
		:m_cosElevationRad(sci::cos(elevation)),
		m_sinElevationRad(sci::sin(elevation))
	{

	}
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		degree azimuth(oldx);

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
	VadSideTransformer(degree elevation)
		:m_cosElevationRad(sci::cos(elevation)),
		m_sinElevationRad(sci::sin(elevation))
	{

	}
	//note that the input azimuths for dx, should be centred on the viewing direction 
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		degree enteredAzimuth = degree(oldx);
		//clip the azimuth to the view azimuth +/-90.
		degree clippedAzimuth = std::max(std::min(enteredAzimuth, degree(90.0)), degree(-90.0));

		metre range = metre(oldy);
		metre horizDistance = range * m_cosElevationRad;
		metre verticalDistance = range * m_sinElevationRad;


		newx = (horizDistance * sci::sin(clippedAzimuth)).value<metre>();
		newy = verticalDistance.value<metre>();
	}
private:
	const unitless m_cosElevationRad;
	const unitless m_sinElevationRad;
};

void ConicalScanningProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(getNFilesRead() == 1, sci::err(sci::SERR_USER, 0, "Attempted to plot a VAD with either no files or multiple files. This code can only plot one file at a time."));

	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		plotDataPlan(outputFilename + sU("_plan"), maxRanges[i], progressReporter, parent);
		plotDataPlan(outputFilename + sU("_cone"), maxRanges[i], progressReporter, parent);
		plotDataPlan(outputFilename + sU("_unwrapped"), maxRanges[i], progressReporter, parent);
	}
}

void ConicalScanningProcessor::plotDataPlan(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
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
	std::vector<degree> sortedElevations;
	std::vector<degree> sortedMidAzimuths;
	std::vector<degree> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);
	std::vector<metre> ranges = getGateBoundariesForPlotting(0);

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less like a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)m_nSegmentsMin / (double)sortedBetas.size());

	//Plot each segment as a separate data set
	std::vector< std::shared_ptr<PhysicalGridData<degree::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>>gridData(sortedBetas.size());
	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		std::vector<degree> segmentAzimuths(duplicationFactor + 1);
		std::vector<std::vector<perSteradianPerMetre>> segmentBetas(duplicationFactor);
		for (size_t j = 0; j < duplicationFactor; ++j)
		{
			segmentAzimuths[j] = azimuths[i] + (azimuths[i + 1] - azimuths[i]) / (unitless)(duplicationFactor*(double)j);
			segmentBetas[j] = sortedBetas[i];
		}
		segmentAzimuths.back() = azimuths[i + 1];
		gridData[i] = std::shared_ptr<PhysicalGridData<degree::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>(new PhysicalGridData<degree::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>(segmentAzimuths, ranges, segmentBetas, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadPlanTransformer(sortedElevations[i]))));
	}
	for (size_t i = 0; i < gridData.size(); ++i)
		plot->addData(gridData[i]);

	//manually set limits as the transformer messes that up
	metre maxHorizDistance = metre(0.0);
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


void ConicalScanningProcessor::plotDataCone(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	WindowCleaner cleaner(window);
	window->SetClientSize(1000, 2000);
	splot2d *plot;

	plot = window->addplot(0.1, 0.8, 0.8, 0.15, false, false);
	plotDataCone(degree(0.0), maxRange, plot);

	plot = window->addplot(0.1, 0.55, 0.8, 0.15, false, false);
	plotDataCone(degree(90.0), maxRange, plot);

	plot = window->addplot(0.1, 0.3, 0.8, 0.15, false, false);
	plotDataCone(degree(180.0), maxRange, plot);

	plot = window->addplot(0.1, 0.05, 0.8, 0.15, false, false);
	plotDataCone(degree(270.0), maxRange, plot);

	createDirectoryAndWritePlot(window, outputFilename, 1000, 2000, progressReporter);
}

void ConicalScanningProcessor::plotDataCone(degree viewAzimuth, metre maxRange, splot2d * plot)
{
	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetre>> sortedBetas;
	std::vector<degree> sortedElevations;
	std::vector<degree> sortedMidAzimuths;
	std::vector<degree> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);

	std::vector<metre> ranges = getGateBoundariesForPlotting(0);

	//Plot each segment as a separate data set

	std::vector<std::shared_ptr<PhysicalGridData<degree::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>> plotData;
	std::vector<degree> absRelativeAzimuths;

	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		std::vector<std::vector<perSteradianPerMetre>> segmentData(1);
		segmentData[0] = sortedBetas[i];
		degree lowerReleativeAzimuth = azimuths[i] - viewAzimuth;
		degree upperReleativeAzimuth = azimuths[i + 1] - viewAzimuth;
		if (upperReleativeAzimuth < lowerReleativeAzimuth)
			upperReleativeAzimuth += degree(360.0);
		if (lowerReleativeAzimuth >= degree(180.0))
		{
			lowerReleativeAzimuth -= degree(360.0);
			upperReleativeAzimuth -= degree(360.0);
		}
		if (upperReleativeAzimuth <= -degree(360.0))
		{
			lowerReleativeAzimuth += degree(360.0);
			upperReleativeAzimuth += degree(360.0);
		}

		//don't plot if limits are outside view range
		if (upperReleativeAzimuth <= -degree(90.0) || lowerReleativeAzimuth >= degree(90.0))
			continue;


		std::vector<degree> segmentAzimuths = std::vector<degree>{ std::max(lowerReleativeAzimuth, degree(-90.0)), std::min(upperReleativeAzimuth, degree(90.0)) };

		plotData.push_back(std::shared_ptr<PhysicalGridData<degree::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>>(new PhysicalGridData<degree::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>(segmentAzimuths, ranges, segmentData, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadSideTransformer(sortedElevations[i])))));
		absRelativeAzimuths.push_back(sci::abs(lowerReleativeAzimuth + upperReleativeAzimuth) / unitless(2.0));
	}

	//plot the segments farthest first.
	std::vector<size_t> plotDataNewLocations;
	std::vector<degree> sortedAbsRelativeAzimuths;
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

void ConicalScanningProcessor::plotDataUnwrapped(const sci::string &outputFilename, metre maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	splot2d *plot;
	setupCanvas(&window, &plot, sU(""), parent);
	WindowCleaner cleaner(window);
	window->SetClientSize(1000, 2000);

	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetre>> sortedBetas;
	std::vector<degree> sortedElevations;
	std::vector<degree> sortedMidAzimuths;
	std::vector<degree> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);

	std::vector<metre> ranges = getGateBoundariesForPlotting(0);

	std::shared_ptr<PhysicalGridData<degree::unit, metre::unit, perSteradianPerMetre::unit>> gridData(new PhysicalGridData<degree::unit, metre::unit, perSteradianPerMetre::unit>(azimuths, ranges, sortedBetas, g_lidarColourscale, true, true));

	plot->addData(gridData);

	plot->getxaxis()->settitle(sU("Azimuth ") + gridData->getXAxisUnits());
	plot->getyaxis()->settitle(sU("Range ") + gridData->getYAxisUnits());
	if (ranges.back() > maxRange)
		plot->setmaxy(maxRange.value<decltype(gridData)::element_type::yUnitType>());
	plot->setxlimits(0, M_PI*2.0);

	createDirectoryAndWritePlot(window, outputFilename, 1000, 1000, progressReporter);
}

void ConicalScanningProcessor::getDataSortedByAzimuth(std::vector<std::vector<perSteradianPerMetre>> &sortedBetas, std::vector<degree> &sortedElevations, std::vector<degree> &sortedMidAzimuths, std::vector<degree> &azimuthBoundaries)
{

	std::vector<degree> midAzimuths = getAzimuths();
	std::vector<size_t> newLocations;
	sci::sort(midAzimuths, sortedMidAzimuths, newLocations);
	std::vector<std::vector<perSteradianPerMetre>> betas = getBetas();
	sortedBetas = sci::reorder(betas, newLocations);
	std::vector<degree> elevations = getElevations();
	sortedElevations = sci::reorder(elevations, newLocations);

	azimuthBoundaries.resize(sortedBetas.size() + 1);
	if (sortedMidAzimuths.back() > sortedMidAzimuths[0])
		azimuthBoundaries[0] = (sortedMidAzimuths[0] + sortedMidAzimuths.back() - degree(360.0)) / unitless(2.0);
	else
		azimuthBoundaries[0] = (sortedMidAzimuths[0] + sortedMidAzimuths.back()) / unitless(2.0);

	for (size_t i = 1; i < azimuthBoundaries.size() - 1; ++i)
	{
		if (sortedMidAzimuths[i - 1] > sortedMidAzimuths[i])
			azimuthBoundaries[i] = (sortedMidAzimuths[i] + sortedMidAzimuths[i - 1] - degree(360.0)) / unitless(2.0);
		else
			azimuthBoundaries[i] = (sortedMidAzimuths[i] + sortedMidAzimuths[i - 1]) / unitless(2.0);
	}

	azimuthBoundaries.back() = azimuthBoundaries[0] + degree(360.0);
}

std::vector<std::vector<sci::string>> LidarVadProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	return InstrumentProcessor::groupFilesPerDayForReprocessing(newFiles, allFiles, 7);
}

bool LidarVadProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	return InstrumentProcessor::fileCoversTimePeriod(fileName, startTime, endTime, 7, 16, 18, 20, second(0));
}

std::vector<std::vector<sci::string>> LidarWindVadProcessor::groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const
{
	return InstrumentProcessor::groupFilesPerDayForReprocessing(newFiles, allFiles, 16);
}

bool LidarWindVadProcessor::fileCoversTimePeriod(sci::string fileName, sci::UtcTime startTime, sci::UtcTime endTime) const
{
	return InstrumentProcessor::fileCoversTimePeriod(fileName, startTime, endTime, 16, 25, 27, 29, second(0));
}