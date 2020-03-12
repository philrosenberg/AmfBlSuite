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

void LidarUserProcessor::plotData(const sci::string &outputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent)
{
	sci::assertThrow(getNFilesRead() == 1, sci::err(sci::SERR_USER, 0, "Attempted to plot a User scan with either no files or multiple files. This code can only plot one file at a time."));
	for (size_t i = 0; i < maxRanges.size(); ++i)
	{
		sci::ostringstream rangeLimitedfilename;
		rangeLimitedfilename << outputFilename;
		if (maxRanges[i] != std::numeric_limits<metre>::max())
			rangeLimitedfilename << sU("_maxRange_") << maxRanges[i];

		splotframe *window;
		splot2d *plot;
		setupCanvas(&window, &plot, sU(""), parent);
		WindowCleaner cleaner(window);

		//create time boundaries between plots;
		std::vector<second> times = getTimesSeconds();
		sci::assertThrow(times.size() > 0, sci::err(sci::SERR_USER, 0, "Attempted to plot a set of User scans with no profiles."));
		if (times.size() == 1)
			times.push_back(times.back() + second(2.5));
		else
			times.push_back(times.back() + times.back() - times[times.size() - 2]);

		std::shared_ptr<PhysicalGridData<second::unit, metre::unit, perSteradianPerMetre::unit>> gridData(new PhysicalGridData<second::unit, metre::unit, perSteradianPerMetre::unit>(times, getGateBoundariesForPlotting(0), getBetas(), g_lidarColourscale, true, true));
		plot->addData(gridData);

		plot->getxaxis()->settitle(sU("Time"));
		plot->getxaxis()->settimeformat(sU("%H:%M:%S"));
		plot->getyaxis()->settitle(sU("Height ") + gridData->getYAxisUnits());

		if (getGateBoundariesForPlotting(0).back() > maxRanges[i])
			plot->setmaxy(sci::physicalsToValues<decltype(gridData)::element_type::yUnitType>(maxRanges[i]));

		createDirectoryAndWritePlot(window, rangeLimitedfilename.str(), 1000, 1000, progressReporter);
	}
}
