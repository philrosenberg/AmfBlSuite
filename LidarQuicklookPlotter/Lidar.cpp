#define _USE_MATH_DEFINES
#include"Lidar.h"
#include<fstream>
#include"Units.h"
#include<cmath>
#include"Plotting.h"
#include<svector\splot.h>

void LidarWindProfileProcessor::readData(const sci::string &inputFilename, ProgressReporter &progressReporter, wxWindow *parent)
{
	m_hasData = false;
	m_heights.clear();
	m_windDirections.clear();
	m_windSpeeds.clear();


	std::fstream fin;
	fin.open(sci::nativeUnicode(inputFilename), std::ios::in);
	if (!fin.is_open())
		throw("Could not open file");

	size_t nPoints;
	fin >> nPoints;
	m_heights.resize(nPoints);
	m_windDirections.resize(nPoints);
	m_windSpeeds.resize(nPoints);

	for (size_t i = 0; i < nPoints; ++i)
	{
		double windDirectionDegrees;
		fin >> m_heights[i] >> windDirectionDegrees >> m_windSpeeds[i];
		m_windDirections[i] = radian(windDirectionDegrees*(M_PI / 180.0));
		if (fin.fail())
			throw(sU("When openning file ") + inputFilename + sU(" some lines were missing."));
	}
	fin.close();

	m_hasData = true;
}

void LidarWindProfileProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{

	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i] != decltype(maxRanges[i])(std::numeric_limits<double>::max()))
			rangeLimitedfilename << sU("_maxRange_") << maxRanges[i];

		splotframe *window = new splotframe(parent, true);
		WindowCleaner cleaner(window);
		splot2d *speedPlot = window->addplot(0.1, 0.1, 0.4, 0.7, false, false);
		splot2d *directionPlot = window->addplot(0.5, 0.1, 0.4, 0.7, false, false);
		std::shared_ptr<PhysicalPointData<decltype(m_windSpeeds)::value_type::unit, decltype(m_heights)::value_type::unit>> speedData; (new PhysicalPointData<decltype(m_windSpeeds)::value_type::unit, decltype(m_heights)::value_type::unit>(m_windSpeeds, m_heights, Symbol(sym::filledCircle, 1.0)));
		std::shared_ptr<PhysicalPointData<decltype(m_windDirections)::value_type::unit, decltype(m_heights)::value_type::unit>> directionData(new PhysicalPointData<decltype(m_windDirections)::value_type::unit, decltype(m_heights)::value_type::unit>(m_windDirections, m_heights, Symbol(sym::filledCircle, 1.0)));

		speedPlot->addData(speedData);
		directionPlot->addData(directionData);
		if (maxRanges[i] < m_heights.back())
		{
			speedPlot->setmaxy(maxRanges[i].value<decltype(speedData)::element_type::yUnitType>());
			directionPlot->setmaxy(maxRanges[i].value<decltype(directionData)::element_type::yUnitType>());
		}

		speedPlot->setminy(0.0);
		directionPlot->setminy(0.0);

		speedPlot->setminx(0.0);
		directionPlot->setxlimits(0.0, 2.0*M_PI);


		speedPlot->getyaxis()->settitle(sU("Height ") + speedData->getYAxisUnits());
		speedPlot->getyaxis()->settitlesize(15);
		speedPlot->getyaxis()->settitledistance(4.5);
		speedPlot->getyaxis()->setlabelsize(15);
		speedPlot->getxaxis()->settitle(sU("Wind Speed ") + speedData->getXAxisUnits());
		speedPlot->getxaxis()->settitlesize(15);
		speedPlot->getxaxis()->settitledistance(4.5);
		speedPlot->getxaxis()->setlabelsize(15);

		directionPlot->getyaxis()->settitle(sU(""));
		directionPlot->getyaxis()->setlabelsize(0);
		directionPlot->getxaxis()->settitle(sU("Wind Direction ") + speedData->getXAxisUnits());
		directionPlot->getxaxis()->settitlesize(15);
		directionPlot->getxaxis()->settitledistance(4.5);
		directionPlot->getxaxis()->setlabelsize(15);
		directionPlot->getxaxis()->setmajorinterval(45);

		splotlegend* legend = window->addlegend(0.1, 0.8, 0.8, 0.15, wxColour(0, 0, 0), 0);
		legend->settitlesize(20);
		sci::ostringstream plotTitle;
		plotTitle << sU("Lidar Wind Retrieval\n");
		if (outputFilename.find(sU('/')) != sci::string::npos)
			plotTitle << outputFilename.substr(outputFilename.find_last_of(sU('/')) + 1, sci::string::npos);
		else if (outputFilename.find(sU('\\')) != sci::string::npos)
			plotTitle << outputFilename.substr(outputFilename.find_last_of(sU('\\')) + 1, sci::string::npos);
		else
			plotTitle << outputFilename;

		legend->settitle(plotTitle.str());

		createDirectoryAndWritePlot(window, outputFilename, 1000, 1000, progressReporter);
	}
	return;
}