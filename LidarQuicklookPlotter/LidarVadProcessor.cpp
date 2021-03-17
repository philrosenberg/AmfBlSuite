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
	VadPlanTransformer(degreeF elevation)
		:m_cosElevationRad(sci::cos(elevation)),
		m_sinElevationRad(sci::sin(elevation))
	{

	}
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		degreeF azimuth((degreeF::valueType)oldx);

		metreF range((metreF::valueType)oldy);
		metreF horizDistance = range * m_cosElevationRad;

		newx = (horizDistance * sci::sin(azimuth)).value<metreF>();
		newy = (horizDistance * sci::cos(azimuth)).value<metreF>();
	}
private:
	const unitlessF m_cosElevationRad;
	const unitlessF m_sinElevationRad;
};

class VadSideTransformer : public splotTransformer
{
public:
	VadSideTransformer(degreeF elevation)
		:m_cosElevationRad(sci::cos(elevation)),
		m_sinElevationRad(sci::sin(elevation))
	{

	}
	//note that the input azimuths for dx, should be centred on the viewing direction 
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		degreeF enteredAzimuth = degreeF((degreeF::valueType)oldx);
		//clip the azimuth to the view azimuth +/-90.
		degreeF clippedAzimuth = std::max(std::min(enteredAzimuth, degreeF(90.0)), degreeF(-90.0));

		metreF range = metreF((metreF::valueType)oldy);
		metreF horizDistance = range * m_cosElevationRad;
		metreF verticalDistance = range * m_sinElevationRad;


		newx = (horizDistance * sci::sin(clippedAzimuth)).value<metreF>();
		newy = verticalDistance.value<metreF>();
	}
private:
	const unitlessF m_cosElevationRad;
	const unitlessF m_sinElevationRad;
};

void ConicalScanningProcessor::plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(getNFilesRead() == 1, sci::err(sci::SERR_USER, 0, "Attempted to plot a VAD with either no files or multiple files. This code can only plot one file at a time."));

	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		plotDataPlan(outputFilename + sU("_plan"), maxRanges[i], progressReporter, parent);
		plotDataCone(outputFilename + sU("_cone"), maxRanges[i], progressReporter, parent);
		plotDataUnwrapped(outputFilename + sU("_unwrapped"), maxRanges[i], progressReporter, parent);
	}
}

void ConicalScanningProcessor::plotDataPlan(const sci::string &outputFilename, metreF maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::ostringstream rangeLimitedfilename;
	rangeLimitedfilename << outputFilename;
	if (maxRange != std::numeric_limits<metreF>::max())
		rangeLimitedfilename << sU("_maxRange_") << maxRange;

	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, sU(""), parent);
	WindowCleaner cleaner(window);


	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetreF>> sortedBetas;
	std::vector<degreeF> sortedElevations;
	std::vector<degreeF> sortedMidAzimuths;
	std::vector<degreeF> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);
	std::vector<metreF> ranges = getGateBoundariesForPlotting(0);

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less like a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)m_nSegmentsMin / (double)sortedBetas.size());

	//Plot each segment as a separate data set
	std::vector< std::shared_ptr<PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit, metreF::unit, metreF::unit>>>gridData(sortedBetas.size());
	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		std::vector<degreeF> segmentAzimuths(duplicationFactor + 1);
		std::vector<std::vector<perSteradianPerMetreF>> segmentBetas(duplicationFactor);
		for (size_t j = 0; j < duplicationFactor; ++j)
		{
			segmentAzimuths[j] = azimuths[i] + (azimuths[i + 1] - azimuths[i]) / (unitlessF)((unitlessF::valueType)duplicationFactor*(unitlessF::valueType)j);
			segmentBetas[j] = sortedBetas[i];
		}
		segmentAzimuths.back() = azimuths[i + 1];
		gridData[i] = std::shared_ptr<PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit, metreF::unit, metreF::unit>>(new PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit, metreF::unit, metreF::unit>(segmentAzimuths, ranges, segmentBetas, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadPlanTransformer(sortedElevations[i]))));
	}
	for (size_t i = 0; i < gridData.size(); ++i)
		plot->addData(gridData[i]);

	//manually set limits as the transformer messes that up
	metreF maxHorizDistance = metreF(0.0);
	metreF farthestRange = ranges.back();
	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		metreF thisHorizDistance = std::min(farthestRange, maxRange)*sci::cos(sortedElevations[i]);
		if (thisHorizDistance > maxHorizDistance)
			maxHorizDistance = thisHorizDistance;
	}
	plot->setxlimits(-maxHorizDistance.value<decltype(gridData)::value_type::element_type::xUnitType>(), maxHorizDistance.value<decltype(gridData)::value_type::element_type::xUnitType>());
	plot->setylimits(-maxHorizDistance.value<decltype(gridData)::value_type::element_type::yUnitType>(), maxHorizDistance.value<decltype(gridData)::value_type::element_type::yUnitType>());

	plot->getxaxis()->settitle(sU("Horizontal Distance ") + gridData[0]->getXAxisUnits());
	plot->getyaxis()->settitle(sU("Horizontal Distance ") + gridData[0]->getYAxisUnits());


	createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
}


void ConicalScanningProcessor::plotDataCone(const sci::string &outputFilename, metreF maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	WindowCleaner cleaner(window);
	window->SetClientSize(1000, 2000);
	splot2d *plot;

	plot = window->addplot(0.1, 0.8, 0.8, 0.15, false, false);
	plotDataCone(degreeF(0.0), maxRange, plot);

	plot = window->addplot(0.1, 0.55, 0.8, 0.15, false, false);
	plotDataCone(degreeF(90.0), maxRange, plot);

	plot = window->addplot(0.1, 0.3, 0.8, 0.15, false, false);
	plotDataCone(degreeF(180.0), maxRange, plot);

	plot = window->addplot(0.1, 0.05, 0.8, 0.15, false, false);
	plotDataCone(degreeF(270.0), maxRange, plot);

	createDirectoryAndWritePlot(window, outputFilename, 1000, 2000, progressReporter);
}

void ConicalScanningProcessor::plotDataCone(degreeF viewAzimuth, metreF maxRange, splot2d * plot)
{
	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetreF>> sortedBetas;
	std::vector<degreeF> sortedElevations;
	std::vector<degreeF> sortedMidAzimuths;
	std::vector<degreeF> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);

	std::vector<metreF> ranges = getGateBoundariesForPlotting(0);

	//Plot each segment as a separate data set

	std::vector<std::shared_ptr<PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit, metreF::unit, metreF::unit>>> plotData;
	std::vector<degreeF> absRelativeAzimuths;

	for (size_t i = 0; i < sortedBetas.size(); ++i)
	{
		std::vector<std::vector<perSteradianPerMetreF>> segmentData(1);
		segmentData[0] = sortedBetas[i];
		degreeF lowerReleativeAzimuth = azimuths[i] - viewAzimuth;
		degreeF upperReleativeAzimuth = azimuths[i + 1] - viewAzimuth;
		if (upperReleativeAzimuth < lowerReleativeAzimuth)
			upperReleativeAzimuth += degreeF(360.0);
		if (lowerReleativeAzimuth >= degreeF(180.0))
		{
			lowerReleativeAzimuth -= degreeF(360.0);
			upperReleativeAzimuth -= degreeF(360.0);
		}
		if (upperReleativeAzimuth <= -degreeF(360.0))
		{
			lowerReleativeAzimuth += degreeF(360.0);
			upperReleativeAzimuth += degreeF(360.0);
		}

		//don't plot if limits are outside view range
		if (upperReleativeAzimuth <= -degreeF(90.0) || lowerReleativeAzimuth >= degreeF(90.0))
			continue;


		std::vector<degreeF> segmentAzimuths = std::vector<degreeF>{ std::max(lowerReleativeAzimuth, degreeF(-90.0)), std::min(upperReleativeAzimuth, degreeF(90.0)) };

		plotData.push_back(std::shared_ptr<PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit, metreF::unit, metreF::unit>>(new PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit, metreF::unit, metreF::unit>(segmentAzimuths, ranges, segmentData, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadSideTransformer(sortedElevations[i])))));
		absRelativeAzimuths.push_back(sci::abs(lowerReleativeAzimuth + upperReleativeAzimuth) / unitlessF(2.0));
	}

	//plot the segments farthest first.
	std::vector<size_t> plotDataNewLocations;
	std::vector<degreeF> sortedAbsRelativeAzimuths;
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

void ConicalScanningProcessor::plotDataUnwrapped(const sci::string &outputFilename, metreF maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	splot2d *plot;
	setupCanvas(&window, &plot, sU(""), parent);
	WindowCleaner cleaner(window);
	window->SetClientSize(1000, 2000);

	//sort the data into ascending azimuth
	std::vector<std::vector<perSteradianPerMetreF>> sortedBetas;
	std::vector<degreeF> sortedElevations;
	std::vector<degreeF> sortedMidAzimuths;
	std::vector<degreeF> azimuths(sortedBetas.size() + 1);
	getDataSortedByAzimuth(sortedBetas, sortedElevations, sortedMidAzimuths, azimuths);

	std::vector<metreF> ranges = getGateBoundariesForPlotting(0);

	bool allSameElevation = true;
	for (size_t i = 1; i < sortedElevations.size(); ++i)
		allSameElevation &= (sortedElevations[0] == sortedElevations[i]);

	if (allSameElevation)
	{
		std::shared_ptr<PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit>> gridData(new PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit>(azimuths, ranges, sortedBetas, g_lidarColourscale, true, true));

		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Azimuth ") + gridData->getXAxisUnits());
		plot->getyaxis()->settitle(sU("Range ") + gridData->getYAxisUnits());
		if (ranges.back() > maxRange)
			plot->setmaxy(maxRange.value<decltype(gridData)::element_type::yUnitType>());
	}
	else
	{
		degreeF angleRange =std::min(degreeF(30), degreeF(degreeF(360) / unitlessF((unitlessF::valueType)sortedElevations.size())));
		for (size_t i = 0; i < sortedBetas.size(); ++i)
		{
			std::vector<degreeF> thisAzimuths(2);
			thisAzimuths[0] = sortedMidAzimuths[i] - angleRange / unitlessF(2);
			thisAzimuths[1] = sortedMidAzimuths[i] + angleRange / unitlessF(2);
			std::vector<std::vector<perSteradianPerMetreF>> thisBetas(1, sortedBetas[i]);
			std::shared_ptr<PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit>> gridData(new PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit>(thisAzimuths, ranges, thisBetas, g_lidarColourscale, true, true));
			plot->addData(gridData);


			plot->getxaxis()->settitle(sU("Azimuth ") + gridData->getXAxisUnits());
			plot->getyaxis()->settitle(sU("Range ") + gridData->getYAxisUnits());
			if (ranges.back() > maxRange)
				plot->setmaxy(maxRange.value<decltype(gridData)::element_type::yUnitType>());
		}
	}

	plot->setxlimits(0, 360.0);

	createDirectoryAndWritePlot(window, outputFilename, 1000, 1000, progressReporter);
}

void ConicalScanningProcessor::getDataSortedByAzimuth(std::vector<std::vector<perSteradianPerMetreF>> &sortedBetas, std::vector<degreeF> &sortedElevations, std::vector<degreeF> &sortedMidAzimuths, std::vector<degreeF> &azimuthBoundaries)
{

	std::vector<degreeF> midAzimuths = getInstrumentRelativeAzimuths();
	std::vector<size_t> newLocations;
	sci::sort(midAzimuths, sortedMidAzimuths, newLocations);
	std::vector<std::vector<perSteradianPerMetreF>> betas = getBetas();
	sortedBetas = sci::reorder(betas, newLocations);
	std::vector<degreeF> elevations = getInstrumentRelativeElevations();
	sortedElevations = sci::reorder(elevations, newLocations);

	azimuthBoundaries.resize(sortedBetas.size() + 1);
	if (sortedMidAzimuths.back() > sortedMidAzimuths[0])
		azimuthBoundaries[0] = (sortedMidAzimuths[0] + sortedMidAzimuths.back() - degreeF(360.0)) / unitlessF(2.0);
	else
		azimuthBoundaries[0] = (sortedMidAzimuths[0] + sortedMidAzimuths.back()) / unitlessF(2.0);

	for (size_t i = 1; i < azimuthBoundaries.size() - 1; ++i)
	{
		if (sortedMidAzimuths[i - 1] > sortedMidAzimuths[i])
			azimuthBoundaries[i] = (sortedMidAzimuths[i] + sortedMidAzimuths[i - 1] - degreeF(360.0)) / unitlessF(2.0);
		else
			azimuthBoundaries[i] = (sortedMidAzimuths[i] + sortedMidAzimuths[i - 1]) / unitlessF(2.0);
	}

	azimuthBoundaries.back() = azimuthBoundaries[0] + degreeF(360.0);
}