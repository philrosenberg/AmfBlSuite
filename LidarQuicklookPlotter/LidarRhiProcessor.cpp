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

class RhiTransformer : public splotTransformer
{
public:
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		degreeF elevation((degreeF::valueType)oldx);
		metreF range((metreF::valueType)oldy);
		newx = (range * sci::cos(elevation)).value<metreF>();
		newy = (range * sci::sin(elevation)).value<metreF>();
	}
};

void LidarRhiProcessor::plotData(const sci::string &outputFilename, const std::vector<metreF> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(getNFilesRead() == 1, sci::err(sci::SERR_USER, 0, "Attempted to plot an RHI with either no files or multiple files. This code can only plot one file at a time."));
	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i] != std::numeric_limits<metreF>::max())
			rangeLimitedfilename << sU("_maxRange_") << maxRanges[i];

		splotframe *window;
		splot2d *plot;
		setupCanvas(&window, &plot, sU(""), parent);
		WindowCleaner cleaner(window);

		std::vector<std::vector<perSteradianPerMetreF>> betas = getBetas();
		std::vector<degreeF> angles(betas.size() + 1);
		std::vector<metreF> ranges = getGateBoundariesForPlotting(0);

		degreeF angleIntervalDeg = degreeF(360.0f) / unitlessF((unitlessF::valueType)(betas.size() - 1));
		for (size_t i = 0; i < angles.size(); ++i)
			angles[i] = unitlessF((unitlessF::valueType)(i - 0.5))*angleIntervalDeg;

		std::shared_ptr<PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit, metreF::unit, metreF::unit>> gridData(new PhysicalGridData<degreeF::unit, metreF::unit, perSteradianPerMetreF::unit, metreF::unit, metreF::unit>(angles, ranges, betas, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new RhiTransformer)));
		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Time"));
		plot->getxaxis()->settimeformat(sU("%H:%M:%S"));
		plot->getyaxis()->settitle(sU("Height ") + gridData->getYAxisUnits());

		if (getGateBoundariesForPlotting(0).back() > maxRanges[i])
			plot->setmaxy(sci::physicalsToValues<decltype(gridData)::element_type::yUnitType>(maxRanges[i]));

		createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
	}
}