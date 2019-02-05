#pragma once

#include<vector>
#include"Units.h"
#include<svector/splot.h>
#include<svector/svector.h>
#include<svector/sstring.h>
#include"AmfNc.h"

struct HplHeader;
class HplProfile;
class wxWindow;
class ProgressReporter;
class CampbellMessage2;
class CampbellCeilometerProfile;



//these are classes/functions that get used by instrument specific plotting routines
void createDirectoryAndWritePlot(splotframe *plotCanvas, sci::string filename, size_t width, size_t height, ProgressReporter &progressReporter);

//this class cleans up wxWindows on destruction in order to ensure they
//get destroyed even if an exception is thrown.
class WindowCleaner
{
public:
	WindowCleaner(wxWindow *window);
	~WindowCleaner();
private:
	wxWindow *m_window;
};

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
		: splotcolourscale(logarithmic ? sci::pow(10.0, sci::indexvector<double>(nPoints + 1) / (double)nPoints*(std::log10(maxValue) - std::log10(minValue)) + std::log10(minValue)) : sci::indexvector<double>(nPoints + 1) / (double)nPoints*(maxValue - minValue) + minValue,
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
			double brightness = startBrightness + (values[i] - values[0]) / valuesRange*brightnessRange;
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

class InstrumentProcessor
{
public:
	//virtual void readDataAndPlot(const std::string &inputFilename, const std::string &outputFilename, const std::vector<double> maxRanges, ProgressReporter &progressReporter, wxWindow *parent);
	virtual void readData(const std::vector<sci::string> &inputFilenames, ProgressReporter &progressReporter, wxWindow *parent) = 0;
	virtual void plotData(const sci::string &baseOutputFilename, const std::vector<metre> maxRanges, ProgressReporter &progressReporter, wxWindow *parent) = 0;
	virtual void writeToNc(const sci::string &directory, const PersonInfo &author,
		const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
		const PlatformInfo &platformInfo, const sci::string &comment, ProgressReporter &progressReporter) = 0;
	virtual bool hasData() const = 0;
	virtual sci::string getFilenameFilter() const = 0;
	virtual std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles) const = 0;
	static std::vector<std::vector<sci::string>> groupFilesPerDayForReprocessing(const std::vector<sci::string> &newFiles, const std::vector<sci::string> &allFiles, size_t dateStartCharacter);
};

//const splotcolourscale g_lidarColourscale(std::vector<double>{1e-8, 1e-3}, std::vector<rgbcolour>{rgbcolour(1.0, 0.0, 0.0), rgbcolour(0.0, 0.0, 1.0)}, true, false);
//const CubehelixColourscale g_lidarColourscale(sci::pow(10.0, sci::indexvector<double>(101) / 100.0*5.0 - 8.0), 0, -540.0, 0.0, 1.0, 1.0, 1.0, true, false);
const CubehelixColourscale g_lidarColourscale(1e-8, 1e-3, 101, 180., 540.0, 1.0, 0.0, 1.0, 1.0, true, false);
