#include"plotting.h"
#include<svector\splot.h>
#include"HplHeader.h"
#include"HplProfile.h"
#include<fstream>
#include<wx/filename.h>
#include"ProgressReporter.h"

void plotStareProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadPlanProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadUnrolledProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double viewAzimuth, size_t nSegmentsMin, double maxRange, splot2d *plot);
void plotRhiProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, ProgressReporter& progressReporter, wxWindow *parent);

class CubehelixColourscale : public splotcolourscale
{
public:
	CubehelixColourscale(const std::vector<double> &values, double startHue, double hueRotation, double startBrightness, double endBrightness, double saturation, double gamma, bool logarithmic, bool autostretch)
		: splotcolourscale(values,
			logarithmic ? CubehelixColourscale::getCubehelixColours(sci::log10(values), startHue, hueRotation, startBrightness, endBrightness, saturation, gamma) : CubehelixColourscale::getCubehelixColours(values, startHue, hueRotation, startBrightness, endBrightness, saturation, gamma),
			logarithmic, autostretch)
	{

	}
	CubehelixColourscale(double minValue, double maxValue, size_t nPoints, double startHue, double hueRotation, double startBrightness, double endBrightness, double saturation, double gamma, bool logarithmic, bool autostretch)
		: splotcolourscale( logarithmic ? sci::pow(10.0, sci::indexvector<double>(nPoints+1) / (double)nPoints*(std::log10(maxValue) - std::log10(minValue)) + std::log10(minValue)) : sci::indexvector<double>(nPoints + 1) / (double)nPoints*(maxValue - minValue) + minValue,
			logarithmic ? CubehelixColourscale::getCubehelixColours(sci::indexvector<double>(nPoints + 1) / (double)nPoints*(std::log10(maxValue) - std::log10(minValue)) + std::log10(minValue), startHue, hueRotation, startBrightness, endBrightness, saturation, gamma) : CubehelixColourscale::getCubehelixColours(sci::indexvector<double>(nPoints + 1) / (double)nPoints*(maxValue - minValue) + minValue, startHue, hueRotation, startBrightness, endBrightness, saturation, gamma),
			logarithmic, autostretch)
	{

	}
	static std::vector<rgbcolour> getCubehelixColours(const std::vector<double> &values, double startHue, double hueRotation, double startBrightness, double endBrightness, double saturation, double gamma)
	{
		//make the hue consistent with the start parameter used in D. A. Green 2011
		startHue -= std::floor(startHue / 360.0)*360.0;
		startHue /= 120;
		startHue += 1;

		//create a vector to hold the colours
		std::vector<rgbcolour> colours(values.size());

		//calculate each colour
		double valuesRange = values.back() - values.front();
		double brightnessRange = endBrightness - startBrightness;
		for (size_t i = 0; i < colours.size(); ++i)
		{
			double brightness = startBrightness + (values[i]-values[0]) / valuesRange*brightnessRange;
			double angle = M_2PI*(startHue / 3.0 + 1.0 + hueRotation / 360.0*brightness);
			double gammaBrightness = std::pow(brightness, gamma);
			double amplitude = saturation*gammaBrightness*(1 - gammaBrightness) / 2.0;
			double red = gammaBrightness + amplitude*(-0.14861*std::cos(angle) + 1.78277*std::sin(angle));
			double green = gammaBrightness + amplitude*(-0.29227*std::cos(angle) - 0.90649*std::sin(angle));
			double blue = gammaBrightness + amplitude*(1.97294*std::cos(angle));
			colours[i] = rgbcolour(red, green, blue);
		}
		//std::fstream fout;
		//fout.open("colourscale", std::ios::out);
		//for (size_t i = 0; i < colours.size(); ++i)
		//	fout << values[i] << "," << colours[i].r() << "," << colours[i].g() << "," << colours[i].b() << "\n";
		//fout.close();
		return colours;
	}
};


//const splotcolourscale g_lidarColourscale(std::vector<double>{1e-8, 1e-3}, std::vector<rgbcolour>{rgbcolour(1.0, 0.0, 0.0), rgbcolour(0.0, 0.0, 1.0)}, true, false);
//const CubehelixColourscale g_lidarColourscale(sci::pow(10.0, sci::indexvector<double>(101) / 100.0*5.0 - 8.0), 0, -540.0, 0.0, 1.0, 1.0, 1.0, true, false);
const CubehelixColourscale g_lidarColourscale(1e-8, 1e-3, 101, 180., 540.0, 1.0, 0.0, 1.0, 1.0 , true, false);


void plotProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	if (header.scanType == st_rhi)
		plotRhiProfiles(header, profiles, filename, progressReporter, parent);
	else if (header.scanType == st_vad || header.scanType == st_wind)
	{
		plotVadPlanProfiles(header, profiles, filename, 24, maxRange, progressReporter, parent);
		plotVadConeProfiles(header, profiles, filename + std::string("cone.png"), 1, maxRange, progressReporter, parent);
		plotVadUnrolledProfiles(header, profiles, filename + std::string("unrolled.png"), 1, maxRange, progressReporter, parent);
	}
	else //stare or user
		plotStareProfiles(header, profiles, filename, maxRange, progressReporter, parent);
}

std::string getScanTypeString (const HplHeader &header)
{
	if (header.scanType == st_stare)
		return "Stare";
	if (header.scanType == st_rhi)
		return "RHI";
	if (header.scanType == st_vad)
		return "VAD";
	if (header.scanType == st_wind)
		return "Wind Profile";
	if (header.scanType == st_user1)
		return "User Scan 1";
	if (header.scanType == st_user2)
		return "User Scan 2";
	if (header.scanType == st_user3)
		return "User Scan 3";
	if (header.scanType == st_user4)
		return "User Scan 4";

	return "Unknown Scan Type";
}

void setupCanvas(splotframe **window, splot2d **plot, const std::string &extraDescriptor, wxWindow *parent, const HplHeader &header)
{
	*window = new splotframe(parent, true);
	(*window)->SetClientSize(1000, 1000);

	std::ostringstream plotTitle;
	plotTitle << "Lidar Backscatter (m#u-1#d sr#u-1#d)" << extraDescriptor << "\n" << header.filename;
	*plot = (*window)->addplot(0.1, 0.1, 0.75, 0.75, false, false, plotTitle.str(), 20.0, 3.0);
	(*plot)->getxaxis()->settitlesize(15);
	(*plot)->getyaxis()->settitledistance(4.5);
	(*plot)->getyaxis()->settitlesize(15);
	(*plot)->getxaxis()->setlabelsize(15);
	(*plot)->getyaxis()->setlabelsize(15);

	splotlegend *legend = (*window)->addlegend(0.86, 0.25, 0.13, 0.65, wxColour(0, 0, 0), 0.0);
	legend->addentry("", g_lidarColourscale, false, false, 0.05, 0.3, 15, "", 0, 0.05, wxColour(0, 0, 0), 128, false, 150, false);
}

void createDirectoryAndWritePlot(splotframe *plotCanvas, std::string filename, size_t width, size_t height, ProgressReporter &progressReporter)
{
	size_t lastSlashPosition = filename.find_last_of("/\\");
	std::string directory;
	if (lastSlashPosition == std::string::npos)
		directory = ".";
	else
		directory = filename.substr(0, lastSlashPosition + 1);
	
	if (!wxDirExists(directory))
		wxFileName::Mkdir(directory, 770, wxPATH_MKDIR_FULL);
	if (!wxDirExists(directory))
	{
		std::ostringstream message;
		message << "The output directory " << directory << "does not exist and could not be created.";
		throw (message.str());
	}

	std::string lastFourChars = filename.length() < 4 ? "" : filename.substr(filename.length() - 4, std::string::npos);
	if (lastFourChars != ".png" && lastFourChars != ".PNG")
		filename = filename + ".png";

	progressReporter << "Rendering plot to file - the application may be unresponsive for a minute.\n";
	if (progressReporter.shouldStop())
	{
		return;
	}
	if( !plotCanvas->writetofile(filename, width, height, 1.0))
	{
		std::ostringstream message;
		message << "The output file " << filename << " could not be created.";
		throw (message.str());
	}
	progressReporter << "Plot rendered to " << filename << "\n";
}

void plotStareProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double maxRange, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, "", parent, header);
	
	std::vector<std::vector<double>> data(profiles.size());
	for (size_t i = 0; i < profiles.size(); ++i)
		data[i] = profiles[i].getBetas();
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
		//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
		if (ys.size() > 533)
			ys /= double(header.pointsPerGate);

	std::shared_ptr<GridData> gridData(new GridData(xs, ys, data, g_lidarColourscale, true, true));

	plot->addData(gridData);

	plot->getxaxis()->settitle("Time");
	plot->getxaxis()->settimeformat("%H:%M:%S");
	plot->getyaxis()->settitle("Height (m)");

	if (ys.back() > maxRange)
		plot->setmaxy(maxRange);

	createDirectoryAndWritePlot(window, filename, 1000, 1000, progressReporter);

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

void plotRhiProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, "", parent, header);

	std::vector<std::vector<double>> data(profiles.size());
	for (size_t i = 0; i < profiles.size(); ++i)
		data[i] = profiles[i].getBetas();
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
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (ranges.size() > 533)
		ranges /= double(header.pointsPerGate);

	splotcolourscale colourscale({ 0, 10 }, { rgbcolour(1,1,1), rgbcolour(0,0,0) }, false, false);
	std::shared_ptr<GridData> gridData(new GridData(angles, ranges, data, colourscale, true, true, std::shared_ptr<splotTransformer>(new RhiTransformer)));

	//manually set limits as the transformer messes that up
	plot->setxlimits(-ranges.back(), ranges.back());
	plot->setylimits(0, ranges.back());

	plot->addData(gridData);

	plot->getxaxis()->settitle("Horizontal Distance (m)");
	plot->getyaxis()->settitle("Height (m)");

	createDirectoryAndWritePlot(window, filename, 1000, 1000, progressReporter);

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

void plotVadPlanProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, "VAD Plan", parent, header);
	
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

	//grab the data from the profiles now they have been sorted
	std::vector<std::vector<double>> data(sortedProfiles.size());
	for (size_t i = 0; i < sortedProfiles.size(); ++i)
		data[i] = sortedProfiles[i].getBetas();
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
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (ranges.size() > 533)
		ranges /= double(header.pointsPerGate);

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less lie a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)nSegmentsMin / (double)sortedProfiles.size());

	//Plot each segment as a separate data set
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
		std::shared_ptr<GridData> gridData(new GridData(segmentAzimuths, ranges, segmentData, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadPlanTransformer(sortedProfiles[i].getElevationDegrees()))));
		plot->addData(gridData);
	}	

	//manually set limits as the transformer messes that up
	double maxHorizDistance = 0.0;
	for (size_t i = 0; i < sortedProfiles.size(); ++i)
	{
		double thisHorizDistance = std::min(ranges.back(), maxRange)*std::cos(sortedProfiles[i].getElevationDegrees() / 180.0 * M_PI);
		if (thisHorizDistance > maxHorizDistance)
			maxHorizDistance = thisHorizDistance;
	}
	plot->setxlimits(-maxHorizDistance, maxHorizDistance);
	plot->setylimits(-maxHorizDistance, maxHorizDistance);


	plot->getxaxis()->settitle("Horizontal Distance (m)");
	plot->getyaxis()->settitle("Horizontal Distance (m)");

	createDirectoryAndWritePlot(window, filename, 1000, 1000, progressReporter);

	window->Destroy();
}

void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	window->SetClientSize(1000, 2000);
	splot2d *plot;

	plot = window->addplot(0.1, 0.8, 0.8, 0.15, false, false);
	plotVadConeProfiles(header, profiles, filename, 0.0, nSegmentsMin, maxRange, plot);

	plot = window->addplot(0.1, 0.55, 0.8, 0.15, false, false);
	plotVadConeProfiles(header, profiles, filename, 90.0, nSegmentsMin, maxRange, plot);

	plot = window->addplot(0.1, 0.3, 0.8, 0.15, false, false);
	plotVadConeProfiles(header, profiles, filename, 180.0, nSegmentsMin, maxRange, plot);

	plot = window->addplot(0.1, 0.05, 0.8, 0.15, false, false);
	plotVadConeProfiles(header, profiles, filename, 270.0, nSegmentsMin, maxRange, plot);

	createDirectoryAndWritePlot(window, filename, 1000, 2000, progressReporter);
	window->Destroy();
}

void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double viewAzimuth, size_t nSegmentsMin, double maxRange, splot2d * plot)
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

	//grab the data from the profiles now they have been sorted
	std::vector<std::vector<double>> data(sortedProfiles.size());
	for (size_t i = 0; i < sortedProfiles.size(); ++i)
		data[i] = sortedProfiles[i].getBetas();
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
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if (ranges.size() > 533)
		ranges /= double(header.pointsPerGate);

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less lie a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)nSegmentsMin / (double)sortedProfiles.size());

	//Plot each segment as a separate data set

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
		plotData.push_back(std::shared_ptr<GridData>(new GridData(segmentAzimuths, ranges, segmentData, g_lidarColourscale, true, true, std::shared_ptr<splotTransformer>(new VadSideTransformer(sortedProfiles[i].getElevationDegrees())))));
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
	plot->getxaxis()->settitlesize(15);
	plot->getyaxis()->settitlesize(15);
	plot->getxaxis()->setlabelsize(15);
	plot->getyaxis()->setlabelsize(15);
	plot->getyaxis()->settitledistance(4.5);
}

void plotVadUnrolledProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, "VAD Unrolled", parent, header);
	
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

	//grab the data from the profiles now they have been sorted
	std::vector<std::vector<double>> data(sortedProfiles.size());
	for (size_t i = 0; i < sortedProfiles.size(); ++i)
		data[i] = sortedProfiles[i].getBetas();
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
	//if we have more than 533 gates then we must be using gate overlapping - annoyingly this isn't recorded in the header
	if(ranges.size() > 533)
		ranges/= double(header.pointsPerGate);

	//We want to duplicate Each ray multiple times so that it looks more like a circle and less lie a triangle or hexagon
	size_t duplicationFactor = (size_t)std::ceil((double)nSegmentsMin / (double)sortedProfiles.size());

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
		std::vector<double> segmentHeights(ranges.size());
		for (size_t j = 0; j < ranges.size(); ++j)
			segmentHeights[j] = ranges[j] * std::cos(profiles[i].getElevationDegrees() / 180.0*M_PI);
		std::shared_ptr<GridData> gridData(new GridData(segmentAzimuths, segmentHeights, segmentData, g_lidarColourscale, true, true));
		plot->addData(gridData);
	}


	plot->getxaxis()->settitle("Azimuth (degrees)");
	plot->getyaxis()->settitle("Range (m)");
	if (ranges.back() > maxRange)
		plot->setmaxy(maxRange);

	createDirectoryAndWritePlot(window, filename, 1000, 1000, progressReporter);

	window->Destroy();
}

void plotProcessedWindProfile(const std::vector<double> &height, const std::vector<double> &degrees, const std::vector<double> &speed, const std::string &filename, double maxRange, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	splot2d *speedPlot = window->addplot(0.1, 0.1, 0.4, 0.7, false, false);
	splot2d *directionPlot = window->addplot(0.5, 0.1, 0.4, 0.7, false, false);
	std::shared_ptr<PointData> speedData(new PointData(speed, height, Symbol(sym::filledCircle, 1.0)));
	std::shared_ptr<PointData> directionData(new PointData(degrees, height, Symbol(sym::filledCircle, 1.0)));

	speedPlot->addData(speedData);
	directionPlot->addData(directionData);
	if (maxRange < height.back())
	{
		speedPlot->setmaxy(maxRange);
		directionPlot->setmaxy(maxRange);
	}

	speedPlot->setminy(0.0);
	directionPlot->setminy(0.0);

	speedPlot->setminx(0.0);
	directionPlot->setxlimits(0.0, 360.0);


	speedPlot->getyaxis()->settitle("Height (m)");
	speedPlot->getyaxis()->settitlesize(15);
	speedPlot->getyaxis()->settitledistance(4.5);
	speedPlot->getyaxis()->setlabelsize(15);
	speedPlot->getxaxis()->settitle("Wind Speed (m s#u-1#d)");
	speedPlot->getxaxis()->settitlesize(15);
	speedPlot->getxaxis()->settitledistance(4.5);
	speedPlot->getxaxis()->setlabelsize(15);

	directionPlot->getyaxis()->settitle("");
	directionPlot->getyaxis()->setlabelsize(0);
	directionPlot->getxaxis()->settitle("Wind Direction (#uo#d)");
	directionPlot->getxaxis()->settitlesize(15);
	directionPlot->getxaxis()->settitledistance(4.5);
	directionPlot->getxaxis()->setlabelsize(15);
	directionPlot->getxaxis()->setmajorinterval(45);

	splotlegend* legend = window->addlegend(0.1, 0.8, 0.8, 0.15,wxColour(0,0,0), 0);
	legend->settitlesize(20);
	std::ostringstream plotTitle;
	plotTitle << "Lidar Wind Retrieval\n";
	if (filename.find('/') != std::string::npos)
		plotTitle << filename.substr(filename.find_last_of('/') + 1, std::string::npos);
	else if (filename.find('\\') != std::string::npos)
		plotTitle << filename.substr(filename.find_last_of('\\') + 1, std::string::npos);
	else
		plotTitle << filename;
	
	legend->settitle(plotTitle.str());

	createDirectoryAndWritePlot(window, filename, 1000, 1000, progressReporter);

	window->Destroy();
}