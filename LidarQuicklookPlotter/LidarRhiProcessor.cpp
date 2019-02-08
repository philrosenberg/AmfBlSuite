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
		radian elevation(oldx);
		metre range(oldy);
		newx = (range * sci::cos(elevation)).value<metre>();
		newy = (range * sci::sin(elevation)).value<metre>();
	}
};

void LidarRhiProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(getNFilesRead()==1, sci::err(sci::SERR_USER, 0, "Attempted to plot an RHI with either no files or multiple files. This code can only plot one file at a time."));
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
		std::vector<metre> ranges = getGateBoundariesForPlotting(0);

		radian angleIntervalDeg(M_PI / (betas.size() - 1));
		for (size_t i = 0; i < angles.size(); ++i)
			angles[i] = unitless(i - 0.5)*angleIntervalDeg;

		std::shared_ptr<PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>> gridData(new PhysicalGridData<radian::unit, metre::unit, perSteradianPerMetre::unit, metre::unit, metre::unit>(angles, ranges, betas, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new RhiTransformer)));
		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Time"));
		plot->getxaxis()->settimeformat(sU("%H:%M:%S"));
		plot->getyaxis()->settitle(sU("Height ") + gridData->getYAxisUnits());

		if (getGateBoundariesForPlotting(0).back() > maxRanges[i])
			plot->setmaxy(sci::physicalsToValues<decltype(gridData)::element_type::yUnitType>(maxRanges[i]));

		createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
	}
}