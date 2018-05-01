#include"plotting.h"
#include<svector\splot.h>
#include"HplHeader.h"
#include"HplProfile.h"

void plotStareProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, wxWindow *parent);
void plotVadPlanProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, wxWindow *parent);
void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, wxWindow *parent);
void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double viewAzimuth, size_t nSegmentsMin, splot2d *plot);
void plotRhiProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, wxWindow *parent);

void plotProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, wxWindow *parent)
{
	if (header.scanType == st_rhi)
		plotRhiProfiles(header, profiles, filename, parent);
	else if (header.scanType == st_vad || header.scanType == st_wind)
	{
		//plotVadPlanProfiles(header, profiles, filename, 24, parent);
		plotVadConeProfiles(header, profiles, filename + std::string("cone0.png"), 1, parent);
	}
	else //stare or user
		plotStareProfiles(header, profiles, filename, parent);
}

void plotStareProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	window->SetSize(1000, 1000);
	splot2d *plot = window->addplot();
	std::vector<std::vector<double>> data(profiles.size());
	for (size_t i = 0; i < profiles.size(); ++i)
		data[i] = profiles[i].getIntensities();
	std::vector<double> xs(data.size()+1);
	std::vector<double> ys(data[0].size() + 1);

	//calculating our heights assumes that the profiles have range gates of 0, 1, 2, 3, ... so check this;
	bool gatesGood = true;
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<size_t> gates = profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			if (gates[j] != j)
				throw("The plotting code currently assumes gates go 0, 1, 2, 3, ... but it found a profile where this was not the case.");
	}

	xs[0] = profiles[0].getTime().getUnixTime();
	for (size_t i = 1; i < xs.size() - 1; ++i)
		xs[i] = (profiles[i].getTime().getUnixTime() + profiles[i - 1].getTime().getUnixTime()) / 2.0;
	xs.back() = profiles.back().getTime().getUnixTime();

	for (size_t i = 0; i < ys.size(); ++i)
		ys[i] = i * header.rangeGateLength;

	splotcolourscale colourscale({ 0, 10 }, { rgbcolour(1,1,1), rgbcolour(0,0,0) }, false, false);
	std::shared_ptr<GridData> gridData(new GridData(xs, ys, data, colourscale, true, true));

	plot->addData(gridData);

	plot->getxaxis()->settitle("Time");
	plot->getxaxis()->settimeformat("%H:%M:%S");
	plot->getyaxis()->settitle("Height (m)");

	window->writetofile(filename);

	window->Destroy();
}

class RhiTransformer : public splotTransformer
{
public:
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		double elevDeg = oldx;
		double elevRad = elevDeg / 180.0*M_PI;
		double range = oldy;
		newx = range * std::cos(elevRad);
		newy = range * std::sin(elevRad);
	}
};

void plotRhiProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	window->SetSize(1000, 1000);
	splot2d *plot = window->addplot();
	std::vector<std::vector<double>> data(profiles.size());
	for (size_t i = 0; i < profiles.size(); ++i)
		data[i] = profiles[i].getIntensities();
	std::vector<double> angles(data.size() + 1);
	std::vector<double> ranges(data[0].size() + 1);

	//calculating our ranges assumes that the profiles have range gates of 0, 1, 2, 3, ... so check this;
	bool gatesGood = true;
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<size_t> gates = profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			if (gates[j] != j)
				throw("The plotting code currently assumes gates go 0, 1, 2, 3, ... but it found a profile where this was not the case.");
	}

	double angleIntervalDeg = 180.0 / (data.size() - 1);
	for (size_t i = 0; i < angles.size(); ++i)
		angles[i] = (i - 0.5)*angleIntervalDeg;

	for (size_t i = 0; i < ranges.size(); ++i)
		ranges[i] = i * header.rangeGateLength;

	splotcolourscale colourscale({ 0, 10 }, { rgbcolour(1,1,1), rgbcolour(0,0,0) }, false, false);
	std::shared_ptr<GridData> gridData(new GridData(angles, ranges, data, colourscale, true, true, std::shared_ptr<splotTransformer>(new RhiTransformer)));

	//manually set limits as the transformer messes that up
	plot->setxlimits(-ranges.back(), ranges.back());
	plot->setylimits(0, ranges.back());

	plot->addData(gridData);

	plot->getxaxis()->settitle("Horizontal Distance (m)");
	plot->getyaxis()->settitle("Height (m)");

	window->writetofile(filename);

	window->Destroy();
}

class VadPlanTransformer : public splotTransformer
{
public:
	VadPlanTransformer(double elevationDeg)
		:m_cosElevationRad(std::cos(elevationDeg / 180.0*M_PI)),
		m_sinElevationRad(std::sin(elevationDeg / 180.0*M_PI))
	{

	}
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		double azDeg = oldx;
		double azRad = azDeg / 180.0*M_PI;

		double range = oldy;
		double horizDistance = range * m_cosElevationRad;


		newx = horizDistance * std::sin(azRad);
		newy = horizDistance * std::cos(azRad);
	}
private:
	const double m_cosElevationRad;
	const double m_sinElevationRad;
};

class VadSideTransformer : public splotTransformer
{
public:
	VadSideTransformer(double elevationDeg)
		:m_cosElevationRad(std::cos(elevationDeg / 180.0*M_PI)),
		m_sinElevationRad(std::sin(elevationDeg / 180.0*M_PI))
	{

	}
	//note that the input azimuths for dx, should be centred on the viewing direction 
	virtual void transform(double oldx, double oldy, double& newx, double& newy)const override
	{
		double enteredAzDeg = oldx;
		//clip the azimuth to the view azimuth +/-90.
		double clippedAzDeg = std::max(std::min(enteredAzDeg, 90.00), -90.0);
		double clippedAzRad = clippedAzDeg / 180.0*M_PI;

		double range = oldy;
		double horizDistance = range * m_cosElevationRad;
		double verticalDistance = range *m_sinElevationRad;


		newx = horizDistance * std::sin(clippedAzRad);
		newy = verticalDistance;
	}
private:
	const double m_cosElevationRad;
	const double m_sinElevationRad;
};

void plotVadPlanProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	window->SetSize(1000, 1000);
	splot2d *plot = window->addplot();
	std::vector<double> azimuths(profiles.size() + 1);

	//calculating our ranges assumes that the profiles have range gates of 0, 1, 2, 3, ... so check this;
	bool gatesGood = true;
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<size_t> gates = profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			if (gates[j] != j)
				throw("The plotting code currently assumes gates go 0, 1, 2, 3, ... but it found a profile where this was not the case.");
	}

	//sort the data into ascending azimuth
	std::vector<HplProfile> sortedProfiles;
	{
		std::vector<double> midAzimuths(profiles.size());
		std::vector<double> sortedMidAzimuths;
		for (size_t i = 0; i < profiles.size(); ++i)
			midAzimuths[i] = profiles[i].getAzimuthDegrees();
		std::vector<size_t> newLocations;
		sci::sort(midAzimuths, sortedMidAzimuths, newLocations);
		sortedProfiles = sci::reorder(profiles, newLocations);
	}

	//grab the data from the profiles ow they have been sorted
	std::vector<std::vector<double>> data(sortedProfiles.size());
	for (size_t i = 0; i < sortedProfiles.size(); ++i)
		data[i] = sortedProfiles[i].getIntensities();
	std::vector<double> ranges(data[0].size() + 1);

	//work out the halfway points between each azimuth - accounting for going round a circle at an arbitrary index;
	if (sortedProfiles.back().getAzimuthDegrees() > sortedProfiles[0].getAzimuthDegrees())
		azimuths[0] = (sortedProfiles[0].getAzimuthDegrees() + sortedProfiles.back().getAzimuthDegrees() - 360.0) / 2.0;
	else
		azimuths[0] = (sortedProfiles[0].getAzimuthDegrees() + sortedProfiles.back().getAzimuthDegrees()) / 2.0;

	for (size_t i = 1; i < azimuths.size() - 1; ++i)
	{
		if (sortedProfiles[i-1].getAzimuthDegrees() > sortedProfiles[i].getAzimuthDegrees())
			azimuths[i] = (sortedProfiles[i].getAzimuthDegrees() + sortedProfiles[i-1].getAzimuthDegrees() - 360.0) / 2.0;
		else
			azimuths[i] = (sortedProfiles[i].getAzimuthDegrees() + sortedProfiles[i-1].getAzimuthDegrees()) / 2.0;
	}

	azimuths.back() = azimuths[0] + 360.0;

	//get the ranges
	for (size_t i = 0; i < ranges.size(); ++i)
		ranges[i] = i * header.rangeGateLength;

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less lie a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)nSegmentsMin / (double)sortedProfiles.size());

	//Plot each segment as a separate data set
	splotcolourscale colourscale({ 0, 10 }, { rgbcolour(1,1,1), rgbcolour(0,0,0) }, false, false);
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<double> segmentAzimuths(duplicationFactor + 1);
		std::vector<std::vector<double>> segmentData(duplicationFactor);
		for (size_t j = 0; j < duplicationFactor; ++j)
		{
			segmentAzimuths[j] = azimuths[i] + (azimuths[i + 1] - azimuths[i]) / (double)duplicationFactor*(double)j;
			segmentData[j] = data[i];
		}
		segmentAzimuths.back() = azimuths[i + 1];
		std::shared_ptr<GridData> gridData(new GridData(segmentAzimuths, ranges, segmentData, colourscale, true, true, std::shared_ptr<splotTransformer>(new VadPlanTransformer(sortedProfiles[i].getElevationDegrees()))));
		plot->addData(gridData);
	}	

	//manually set limits as the transformer messes that up
	double maxHorizDistance = 0.0;
	for (size_t i = 0; i < sortedProfiles.size(); ++i)
	{
		double thisHorizDistance = ranges.back()*std::cos(sortedProfiles[i].getElevationDegrees() / 180.0 * M_PI);
		if (thisHorizDistance > maxHorizDistance)
			maxHorizDistance = thisHorizDistance;
	}
	plot->setxlimits(-maxHorizDistance, maxHorizDistance);
	plot->setylimits(-maxHorizDistance, maxHorizDistance);


	plot->getxaxis()->settitle("Horizontal Distance (m)");
	plot->getyaxis()->settitle("Horizontal Distance (m)");

	window->writetofile(filename);

	window->Destroy();
}

void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	window->SetSize(1000, 2000);
	splot2d *plot;

	plot = window->addplot(0.1, 0.8, 0.8, 0.15, false, false);
	plotVadConeProfiles(header, profiles, filename, 0.0, nSegmentsMin, plot);

	plot = window->addplot(0.1, 0.55, 0.8, 0.15, false, false);
	plotVadConeProfiles(header, profiles, filename, 90.0, nSegmentsMin, plot);

	plot = window->addplot(0.1, 0.3, 0.8, 0.15, false, false);
	plotVadConeProfiles(header, profiles, filename, 180.0, nSegmentsMin, plot);

	plot = window->addplot(0.1, 0.05, 0.8, 0.15, false, false);
	plotVadConeProfiles(header, profiles, filename, 270.0, nSegmentsMin, plot);

	window->writetofile(filename);
	window->Destroy();
}

void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double viewAzimuth, size_t nSegmentsMin, splot2d * plot)
{
	std::vector<double> azimuths(profiles.size() + 1);

	//calculating our ranges assumes that the profiles have range gates of 0, 1, 2, 3, ... so check this;
	bool gatesGood = true;
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<size_t> gates = profiles[i].getGates();
		for (size_t j = 0; j < gates.size(); ++j)
			if (gates[j] != j)
				throw("The plotting code currently assumes gates go 0, 1, 2, 3, ... but it found a profile where this was not the case.");
	}

	//sort the data into ascending azimuth
	std::vector<HplProfile> sortedProfiles;
	{
		std::vector<double> midAzimuths(profiles.size());
		std::vector<double> sortedMidAzimuths;
		for (size_t i = 0; i < profiles.size(); ++i)
			midAzimuths[i] = profiles[i].getAzimuthDegrees();
		std::vector<size_t> newLocations;
		sci::sort(midAzimuths, sortedMidAzimuths, newLocations);
		sortedProfiles = sci::reorder(profiles, newLocations);
	}

	//grab the data from the profiles ow they have been sorted
	std::vector<std::vector<double>> data(sortedProfiles.size());
	for (size_t i = 0; i < sortedProfiles.size(); ++i)
		data[i] = sortedProfiles[i].getIntensities();
	std::vector<double> ranges(data[0].size() + 1);

	//work out the halfway points between each azimuth - accounting for going round a circle at an arbitrary index;
	if (sortedProfiles.back().getAzimuthDegrees() > sortedProfiles[0].getAzimuthDegrees())
		azimuths[0] = (sortedProfiles[0].getAzimuthDegrees() + sortedProfiles.back().getAzimuthDegrees() - 360.0) / 2.0;
	else
		azimuths[0] = (sortedProfiles[0].getAzimuthDegrees() + sortedProfiles.back().getAzimuthDegrees()) / 2.0;

	for (size_t i = 1; i < azimuths.size() - 1; ++i)
	{
		if (sortedProfiles[i - 1].getAzimuthDegrees() > sortedProfiles[i].getAzimuthDegrees())
			azimuths[i] = (sortedProfiles[i].getAzimuthDegrees() + sortedProfiles[i - 1].getAzimuthDegrees() - 360.0) / 2.0;
		else
			azimuths[i] = (sortedProfiles[i].getAzimuthDegrees() + sortedProfiles[i - 1].getAzimuthDegrees()) / 2.0;
	}

	azimuths.back() = azimuths[0] + 360.0;

	//get the ranges
	for (size_t i = 0; i < ranges.size(); ++i)
		ranges[i] = i * header.rangeGateLength;

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less lie a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)nSegmentsMin / (double)sortedProfiles.size());

	//Plot each segment as a separate data set
	splotcolourscale colourscale({ 0, 10 }, { rgbcolour(1,1,1), rgbcolour(0,0,0) }, false, false);
	//for (size_t i = 0; i < profiles.size(); ++i)

	std::vector<std::shared_ptr<GridData>> plotData;
	std::vector<double> absRelativeAzimuths;

	for (size_t i = 0; i < profiles.size(); ++i)
	{
		std::vector<double> segmentAzimuths(duplicationFactor + 1);
		std::vector<std::vector<double>> segmentData(duplicationFactor);
		double lowerReleativeAzimuth = azimuths[i] - viewAzimuth;
		double upperReleativeAzimuth = azimuths[i+1] - viewAzimuth;
		if (upperReleativeAzimuth < lowerReleativeAzimuth)
			upperReleativeAzimuth += 360.0;
		if (lowerReleativeAzimuth >= 180.0)
		{
			lowerReleativeAzimuth -= 360.0;
			upperReleativeAzimuth -= 360.0;
		}
		if (upperReleativeAzimuth <= -180.0)
		{
			lowerReleativeAzimuth += 360.0;
			upperReleativeAzimuth += 360.0;
		}

		//don't plot if limits are outside view range
		if (upperReleativeAzimuth <= -90.0 || lowerReleativeAzimuth >= 90.0)
			continue;

		for (size_t j = 0; j < duplicationFactor; ++j)
		{
			segmentAzimuths[j] = lowerReleativeAzimuth + (upperReleativeAzimuth - lowerReleativeAzimuth) / (double)duplicationFactor*(double)j;
			segmentData[j] = data[i];
		}
		segmentAzimuths.back() = upperReleativeAzimuth;
		plotData.push_back(std::shared_ptr<GridData>(new GridData(segmentAzimuths, ranges, segmentData, colourscale, true, true, std::shared_ptr<splotTransformer>(new VadSideTransformer(sortedProfiles[i].getElevationDegrees())))));
		absRelativeAzimuths.push_back(std::abs(lowerReleativeAzimuth + upperReleativeAzimuth) / 2.0);
	}

	//plot the segments farthest first.
	std::vector<size_t> plotDataNewLocations;
	std::vector<double> sortedAbsRelativeAzimuths;
	sci::sort(absRelativeAzimuths, sortedAbsRelativeAzimuths, plotDataNewLocations);
	plotData = sci::reorder(plotData, plotDataNewLocations);
	for (size_t i = 0; i < plotData.size(); ++i)
		plot->addData(plotData[i]);

	plot->setxlimits(-ranges.back(), ranges.back());
	plot->setylimits(0, ranges.back());


	plot->getxaxis()->settitle("Horizontal Distance (m)");
	plot->getyaxis()->settitle("Vertical Distance (m)");
}