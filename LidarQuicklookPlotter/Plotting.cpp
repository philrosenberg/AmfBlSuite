#include"plotting.h"
#include<svector\splot.h>
#include"HplHeader.h"
#include"HplProfile.h"
#include<fstream>
#include<wx/filename.h>
#include"ProgressReporter.h"
#include"Ceilometer.h"

void plotStareProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadPlanProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadUnrolledProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);
void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, double viewAzimuth, size_t nSegmentsMin, double maxRange, splot2d *plot);
void plotRhiProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, ProgressReporter& progressReporter, wxWindow *parent);




WindowCleaner::WindowCleaner(wxWindow *window)
	: m_window(window)
{}
WindowCleaner::~WindowCleaner()
{
	m_window->Destroy();
}


void plotProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, double maxRange, ProgressReporter &progressReporter, wxWindow *parent)
{
	if (header.scanType == st_rhi)
		plotRhiProfiles(header, profiles, filename, progressReporter, parent);
	else if (header.scanType == st_vad || header.scanType == st_wind)
	{
		plotVadPlanProfiles(header, profiles, filename, 24, maxRange, progressReporter, parent);
		plotVadConeProfiles(header, profiles, filename + sci::string(sU("cone.png")), 1, maxRange, progressReporter, parent);
		plotVadUnrolledProfiles(header, profiles, filename + sci::string(sU("unrolled.png")), 1, maxRange, progressReporter, parent);
	}
	else //stare or user
		plotStareProfiles(header, profiles, filename, maxRange, progressReporter, parent);
}

sci::string getScanTypeString (const HplHeader &header)
{
	if (header.scanType == st_stare)
		return sU("Stare");
	if (header.scanType == st_rhi)
		return sU("RHI");
	if (header.scanType == st_vad)
		return sU("VAD");
	if (header.scanType == st_wind)
		return sU("Wind Profile");
	if (header.scanType == st_user1)
		return sU("User Scan 1");
	if (header.scanType == st_user2)
		return sU("User Scan 2");
	if (header.scanType == st_user3)
		return sU("User Scan 3");
	if (header.scanType == st_user4)
		return sU("User Scan 4");

	return sU("Unknown Scan Type");
}

void setupCanvas(splotframe **window, splot2d **plot, const sci::string &extraDescriptor, wxWindow *parent, const HplHeader &header)
{
	*window = new splotframe(parent, true);
	(*window)->SetClientSize(1000, 1000);

	sci::ostringstream plotTitle;
	plotTitle << sU("Lidar Backscatter (m#u-1#d sr#u-1#d)") << extraDescriptor << sU("\n") << header.filename;
	*plot = (*window)->addplot(0.1, 0.1, 0.75, 0.75, false, false, plotTitle.str(), 20.0, 3.0);
	(*plot)->getxaxis()->settitlesize(15);
	(*plot)->getyaxis()->settitledistance(4.5);
	(*plot)->getyaxis()->settitlesize(15);
	(*plot)->getxaxis()->setlabelsize(15);
	(*plot)->getyaxis()->setlabelsize(15);

	splotlegend *legend = (*window)->addlegend(0.86, 0.25, 0.13, 0.65, wxColour(0, 0, 0), 0.0);
	legend->addentry("", g_lidarColourscale, false, false, 0.05, 0.3, 15, "", 0, 0.05, wxColour(0, 0, 0), 128, false, 150, false);
}

void createDirectoryAndWritePlot(splotframe *plotCanvas, sci::string filename, size_t width, size_t height, ProgressReporter &progressReporter)
{
	size_t lastSlashPosition = filename.find_last_of(sU("/\\"));
	sci::string directory;
	if (lastSlashPosition == sci::string::npos)
		directory = sU(".");
	else
		directory = filename.substr(0, lastSlashPosition + 1);
	
	if (!wxDirExists(sci::nativeUnicode(directory)))
		wxFileName::Mkdir(sci::nativeUnicode(directory), 770, wxPATH_MKDIR_FULL);
	if (!wxDirExists(sci::nativeUnicode(directory)))
	{
		sci::ostringstream message;
		message << sU("The output directory ") << directory << sU("does not exist and could not be created.");
		throw (message.str());
	}

	sci::string lastFourChars = filename.length() < 4 ? sU("") : filename.substr(filename.length() - 4, sci::string::npos);
	if (lastFourChars != sU(".png") && lastFourChars != sU(".PNG"))
		filename = filename + sU(".png");

	progressReporter << sU("Rendering plot to file - the application may be unresponsive for a minute.\n");
	if (progressReporter.shouldStop())
	{
		return;
	}
	if( !plotCanvas->writetofile(filename, width, height, 1.0))
	{
		sci::ostringstream message;
		message << sU("The output file ") << filename << sU(" could not be created.");
		throw (message.str());
	}
	progressReporter << sU("Plot rendered to ") << filename << sU("\n");
}







void plotVadPlanProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, sU("VAD Plan"), parent, header);
	WindowCleaner cleaner(window);
	
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
		ranges[i] = i * header.rangeGateLength.m_v;
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
}

void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window = new splotframe(parent, true);
	WindowCleaner cleaner(window);
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
}

void plotVadConeProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, double viewAzimuth, size_t nSegmentsMin, double maxRange, splot2d * plot)
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
		ranges[i] = i * header.rangeGateLength.m_v;
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

void plotVadUnrolledProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, sci::string filename, size_t nSegmentsMin, double maxRange, ProgressReporter& progressReporter, wxWindow *parent)
{
	splotframe *window;
	splot2d *plot;
	setupCanvas(&window, &plot, sU("VAD Unrolled"), parent, header);
	WindowCleaner cleaner(window);
	
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
		ranges[i] = i * header.rangeGateLength.m_v;
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
}



