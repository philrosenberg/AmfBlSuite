#include "mainFrame.h"
#include<wx/dir.h>
#include<wx/filename.h>
#include<svector/serr.h>
#include<svector/time.h>
#include <wx/evtloop.h>
#include<svector/splot.h>
#include<svector/controls.h>
#include"resample.h"


Series1d::Series1d(wxString label, wxTreeCtrl* plotTreeCtrl, Plot* parent, wxWindow* panelParent, wxSizer* panelSizer, const DataSource* dataSource)

{
	m_seriesData = nullptr;
	m_parent = parent;
	m_plotTreeCtrl = plotTreeCtrl;
	bool addedToTree = false;
	m_inputPanel = nullptr;
	m_xResolutionAtPreviousPlot = std::numeric_limits<double>::quiet_NaN();
	m_yResolutionAtPreviousPlot = std::numeric_limits<double>::quiet_NaN();
	try
	{
		m_inputPanel = new sci::InputPanel<ParameterTuple>(panelParent, wxID_ANY, Parameters::getLabels(), Parameters().toTuple(), false, true, true);
		m_inputPanel->Show(false);
		panelSizer->Add(m_inputPanel);
		panelSizer->Layout();
		m_treeItemId = m_plotTreeCtrl->AppendItem(parent->getTreeItemId(), label);
		addedToTree = true;
		if (m_plotTreeCtrl->GetChildrenCount(parent->getTreeItemId()) == 1)
			m_plotTreeCtrl->Expand(parent->getTreeItemId());
		setSelectors(dataSource);
	}
	catch (...)
	{
		if (m_inputPanel)
			m_inputPanel->Destroy();
		if (addedToTree)
			m_plotTreeCtrl->Delete(m_treeItemId);
		throw;
		//only other place to throw is in the addplot call which means m_splot was never assigned. No nedd to delete it
	}
}

Series1d::~Series1d()
{
	if (m_plotTreeCtrl)
		m_plotTreeCtrl->Delete(m_treeItemId);
	if (m_parent && m_parent->getPlot())
		m_parent->getPlot()->removeData(m_seriesData);
	if (m_inputPanel)
		m_inputPanel->Destroy();
}

std::vector<double> getEdges(const std::vector<double>& values, size_t requiredLength, sci::string name)
{
	if (values.size() == requiredLength)
		return values;
	sci::assertThrow(values.size() == requiredLength - 1, sci::err(sci::SERR_USER, 0, sU("Trying to interpolate ") + name + sU(" but the variable has the incorrect number of values.")));
	sci::assertThrow(values.size() > 1, sci::err(sci::SERR_USER, 0, sU("Trying to interpolate ") + name + sU(" but the variable has less than 2 elements.")));
	std::vector<double> edges(values.size() + 1);
	edges[0] = values[0] - 0.5 * (values[1] - values[0]);
	for (size_t i = 0; i < values.size() - 1; ++i)
		edges[i + 1] = (values[i] + values[i + 1]) / 2.0;
	edges.back() = values.back() + 0.5 * (values.back() - values[values.size() - 2]);
	return edges;
}

std::vector<std::vector<double>> getEdges(const std::vector<std::vector<double>>& values, size_t requiredLengthDim1, size_t requiredLengthDim2, sci::string name)
{
	sci::assertThrow(values.size() > 1, sci::err(sci::SERR_USER, 0, sU("Trying to interpolate ") + name + sU(" but the variable has less than 2 elements.")));
	if (values.size() == requiredLengthDim1 && values[0].size() == requiredLengthDim2)
		return values;

	sci::assertThrow(values.size() == requiredLengthDim1 || values.size() == requiredLengthDim1 - 1, sci::err(sci::SERR_USER, 0, sU("Trying to interpolate ") + name + sU(" but the variable has the incorrect number of values.")));
	sci::assertThrow(values[0].size() == requiredLengthDim2 || values[0].size() == requiredLengthDim2 - 1, sci::err(sci::SERR_USER, 0, sU("Trying to interpolate ") + name + sU(" but the variable has the incorrect number of values.")));
	sci::assertThrow(values[0].size() > 1, sci::err(sci::SERR_USER, 0, sU("Trying to interpolate ") + name + sU(" but the variable has less than 2 elements.")));
	
	if (values.size() == requiredLengthDim1)
	{
		std::vector<std::vector<double>> edges(values.size());
		for (size_t i = 0; i < edges.size(); ++i)
		{
			edges[i] = getEdges(values[i], requiredLengthDim2, name);
		}
		return edges;
	}
	else if(values[0].size()==requiredLengthDim2)
	{
		std::vector<std::vector<double>> edges(values.size()+1);
		edges[0] = values[0] - 0.5 * (values[1] - values[0]);
		for (size_t i = 0; i < values.size() - 1; ++i)
			edges[i + 1] = (values[i] + values[i + 1]) / 2.0;
		edges.back() = values.back() + 0.5 * (values.back() - values[values.size() - 2]);
		return edges;
	}
	else
	{
		std::vector<std::vector<double>> edges(requiredLengthDim1);
		for (size_t i = 0; i < edges.size(); ++i)
			edges[i].resize(requiredLengthDim2);

		//corners
		edges[0][0] = values[0][0] - 0.5 * (values[0][1] - values[0][0]) - 0.5 * (values[1][0] - values[0][0]);
		edges[0].back() = values[0].back() - 0.5 * (values[1].back() - values[0].back()) + 0.5 * (values[0].back() - values[0][values[0].size()-2]);
		edges.back()[0] = values.back()[0] + 0.5 * (values.back()[0] - values[values.size() - 2][0]) - 0.5 * (values.back()[1] - values.back()[0]);
		edges.back().back() = values.back().back() + 0.5 * (values.back().back() - values.back()[values.back().size()-2]) + 0.5 * (values.back().back() - values[values.size() - 2].back());
		//edges
		for (size_t i = 1; i < requiredLengthDim1 - 1; ++i)
		{
			//top/bottom
			edges[i][0] = values[i - 1][0] - 0.5 * (values[i - 1][1] - values[i - 1][0]);
			edges[i].back() = values[i - 1].back() + 0.5 * (values[i - 1].back() - values[i - 1][values[i - 1].size() - 2]);
		}
		for (size_t i = 1; i < requiredLengthDim2 - 1; ++i)
		{
			//left/right
			edges[0][i] = values[0][i-1] - 0.5 * (values[1][i - 1] - values[0][i - 1]);
			edges.back()[i] = values.back()[i - 1] + 0.5 * (values.back()[i - 1] - values[values.size() - 2][i - 1]);
		}
		//remainder
		for (size_t i = 1; i < requiredLengthDim1 - 1; ++i)
		{
			for (size_t j = 1; j < requiredLengthDim2 - 1; ++j)
			{
				edges[i][j] = 0.25*(values[i - 1][j - 1] + values[i - 1][j] + values[i][j - 1] + values[i][j]);
			}
		}
		return edges;
	}
}

void Series1d::applyPropertiesChange(const wxTreeItemId& selected, splotwindow* window, splot2d* plot, const DataSource *dataSource)
{
	if (selected == m_treeItemId)
	{
		updateResolution(window, plot, dataSource, true);
	}
}

void Series1d::updateResolution(splotwindow* window, splot2d* plot, const DataSource* dataSource, bool forceRefresh)
{
	double plotXPixels = m_parent->getParameters().width * double(window->GetClientSize().GetWidth());
	double plotYPixels = m_parent->getParameters().height * double(window->GetClientSize().GetHeight());
	double xMin;
	double xMax;
	double yMin;
	double yMax;
	m_parent->getLimits(xMin, xMax, yMin, yMax);
	double xRange = xMax - xMin;
	double yRange = yMax - yMin;
	double xResolution = xRange / plotXPixels;
	double yResolution = yRange / plotYPixels;
	if (forceRefresh || xResolution != m_xResolutionAtPreviousPlot || yResolution != m_yResolutionAtPreviousPlot)
		refreshData(window, plot, dataSource);
}

void Series1d::refreshData(splotwindow* window, splot2d* plot, const DataSource* dataSource)
{

	if (!m_inputPanel->hasFlushedChanged())
	{
		return;
	}
	else
	{
		plot->removeData(m_seriesData);
		m_xResolutionAtPreviousPlot = std::numeric_limits<double>::quiet_NaN();
		m_yResolutionAtPreviousPlot = std::numeric_limits<double>::quiet_NaN();
		Parameters parameters;
		parameters.fromTuple(m_inputPanel->getValues());
		if (parameters.file.second.size() == 0)
			return;
		if (parameters.variableName.second.size() == 0)
			return;
		if (parameters.xVariableName.second.size() == 0)
			return;

		auto filename = parameters.file.second[parameters.file.first];
		auto variableName = parameters.variableName.second[parameters.variableName.first];
		auto xVariableName = parameters.xVariableName.second[parameters.xVariableName.first];
		auto variableFilter = parameters.variableFilter;
		auto xFilter = parameters.xFilter;
		auto yFilter = parameters.yFilter;

		double plotXPixels = m_parent->getParameters().width * double(window->GetClientSize().GetWidth());
		double plotYPixels = m_parent->getParameters().height * double(window->GetClientSize().GetHeight());
		double xMin;
		double xMax;
		double yMin;
		double yMax;
		m_parent->getLimits(xMin, xMax, yMin, yMax);
		double xResolution;
		double yResolution;

		//1d data
		if (dataSource->is1d(filename, variableName, variableFilter) && dataSource->is1d(filename, xVariableName, xFilter))
		{
			std::vector<double> y = dataSource->get1dData(filename, variableName, variableFilter);
			std::vector<double> x = dataSource->get1dData(filename, xVariableName, xFilter);

			m_seriesData.reset(new LineData(x, y, LineStyle(2.0, rgbcolour(parameters.red, parameters.green, parameters.blue))));
			plot->addData(m_seriesData);
			m_parent->getLimits(xMin, xMax, yMin, yMax);
			m_xResolutionAtPreviousPlot = (xMax-xMin) / plotXPixels;
			m_yResolutionAtPreviousPlot = (yMax-yMin) / plotYPixels;
		}
		//2d
		else if (dataSource->is2d(filename, variableName, variableFilter))
		{
			if (parameters.yVariableName.second.size() == 0)
				return;
			auto yVariableName = parameters.yVariableName.second[parameters.yVariableName.first];
			//2d rectilinear
			if (dataSource->is1d(filename, xVariableName, xFilter) && dataSource->is1d(filename, yVariableName, yFilter))
			{
				std::vector<std::vector<double>> z = dataSource->get2dData(filename, variableName, variableFilter);
				sci::assertThrow(z.size() > 0, sci::err(sci::SERR_USER, 0, sU("Attempted to plot 2d rectilinear data, but the data was empty.")));
				sci::assertThrow(z[0].size() > 0, sci::err(sci::SERR_USER, 0, sU("Attempted to plot 2d rectilinear data, but the data was empty.")));
				std::vector<double> x = dataSource->get1dData(filename, xVariableName, xFilter);
				std::vector<double> y = dataSource->get1dData(filename, yVariableName, yFilter);

				std::vector<double> xEdges = getEdges(x, z.size() + 1, sU("x"));
				std::vector<double> yEdges = getEdges(y, z[0].size() + 1, sU("y"));

				double plotXPixels = m_parent->getParameters().width * double(window->GetClientSize().GetWidth());
				double plotYPixels = m_parent->getParameters().height * double(window->GetClientSize().GetHeight());
				double xRange = m_parent->getParameters().xMax - m_parent->getParameters().xMin;
				double yRange = m_parent->getParameters().yMax - m_parent->getParameters().yMin;
				if (m_parent->getParameters().xAuto)
					xRange = sci::max<double>(xEdges) - sci::min<double>(xEdges);
				if (m_parent->getParameters().yAuto)
					yRange = sci::max<double>(yEdges) - sci::min<double>(yEdges);

				std::vector<double> xEdgesResampled;
				std::vector<size_t> xEdgesNMerged;
				mergeAxis(xEdges, plotXPixels / xRange, 0.5, xEdgesResampled, xEdgesNMerged);
				std::vector<double> yEdgesResampled;
				std::vector<size_t> yEdgesNMerged;
				mergeAxis(yEdges, plotYPixels / yRange, 0.5, yEdgesResampled, yEdgesNMerged);
				std::vector<std::vector<double>> zResampled = mergeZ(z, xEdgesNMerged, yEdgesNMerged);
				m_seriesData.reset(new GridData(xEdgesResampled, yEdgesResampled, zResampled, CubehelixColourscale(parameters.colourscaleMin, parameters.colourscaleMax, 101, 1000, 540.0, 1.0, 0.0, 1.0, 1.0, false, false), true, true));
				plot->addData(m_seriesData);
				m_parent->getLimits(xMin, xMax, yMin, yMax);
				m_xResolutionAtPreviousPlot = (xMax - xMin) / plotXPixels;
				m_yResolutionAtPreviousPlot = (yMax - yMin) / plotYPixels;
			}
			//2d curvilinear
			else if ((dataSource->is2d(filename, xVariableName, xFilter) && dataSource->is2d(filename, yVariableName, yFilter))
				|| (dataSource->is1d(filename, xVariableName, xFilter) && dataSource->is2d(filename, yVariableName, yFilter))
				|| (dataSource->is2d(filename, xVariableName, xFilter) && dataSource->is1d(filename, yVariableName, yFilter)))
			{
				std::vector<std::vector<double>> z = dataSource->get2dData(filename, variableName, variableFilter);
				sci::assertThrow(z.size() > 0, sci::err(sci::SERR_USER, 0, sU("Attempted to plot 2d curvilinear data, but the data was empty.")));
				sci::assertThrow(z[0].size() > 0, sci::err(sci::SERR_USER, 0, sU("Attempted to plot 2d curvilinear data, but the data was empty.")));

				std::vector<std::vector<double>> xEdges;
				std::vector<std::vector<double>> yEdges;
				std::vector<std::vector<double>> x;
				std::vector<std::vector<double>> y;
				std::vector<double > x1d;
				std::vector<double > y1d;
				std::vector<double > x1dEdges;
				std::vector<double > y1dEdges;



				//read in the x data, this could be 1d or 2d
				if (dataSource->is1d(filename, xVariableName, xFilter))
				{
					x1d = dataSource->get1dData(filename, xVariableName, xFilter);
					x1dEdges = getEdges(x1d, z.size() + 1, sU("x"));
				}
				else
				{
					x = dataSource->get2dData(filename, xVariableName, xFilter);
					xEdges = getEdges(x, z.size() + 1, z[0].size() + 1, sU("x"));
				}

				if (dataSource->is1d(filename, yVariableName, yFilter))
				{
					y1d = dataSource->get1dData(filename, yVariableName, yFilter);
					y1dEdges = getEdges(y1d, z[0].size() + 1, sU("y"));
				}
				else
				{
					y = dataSource->get2dData(filename, yVariableName, yFilter);
					yEdges = getEdges(y, z.size() + 1, z[0].size() + 1, sU("y"));
				}


				
				double thisXMin = x1dEdges.size() > 0 ? sci::min<double>(x1dEdges) : sci::min<double>(xEdges);
				double thisXMax = x1dEdges.size() > 0 ? sci::max<double>(x1dEdges) : sci::max<double>(xEdges);
				double thisYMin = y1dEdges.size() > 0 ? sci::min<double>(y1dEdges) : sci::min<double>(yEdges);
				double thisYMax = y1dEdges.size() > 0 ? sci::max<double>(y1dEdges) : sci::max<double>(yEdges);
				xMin = thisXMin < xMin ? thisXMin : xMin;
				xMax = thisXMax > xMax ? thisXMax : xMax;
				yMin = thisYMin < yMin ? thisYMin : yMin;
				yMax = thisYMax > yMax ? thisYMax : yMax;
				double xRange = xMax - xMin;
				double yRange = yMax - yMin;
				xResolution = xRange / plotXPixels;
				yResolution = yRange / plotYPixels;


				//calculate the edges and swap to 1d x and y if suitable
				if (x.size() > 0)
				{
					//check if all x values are the same - in which case we can do use 1d
					bool couldBe1d = true;
					for (size_t i = 0; i < x.size(); ++i)
						couldBe1d = couldBe1d && sci::allTrue(sci::abs(x[i][0] - x[i]) < (xResolution * 0.5));
					if (couldBe1d)
					{
						x1d.resize(x.size());
						for (size_t i = 0; i < x1d.size(); ++i)
							x1d[i] = x[i][0];
						x1dEdges = getEdges(x1d, z.size() + 1, sU("x"));
					}

				}
				if (y.size() > 0)
				{
					//check if all y values are the same - in which case we can do use 1d
					bool couldBe1d = true;
					for (size_t i = 1; i < y.size(); ++i)
						couldBe1d = couldBe1d && sci::allTrue(sci::abs(y[i] - y[0]) < (yResolution * 0.5));
					if (couldBe1d)
					{
						y1d = y[0];
						y1dEdges = getEdges(y1d, z[0].size() + 1, sU("y"));
					}
				}


				if (x1dEdges.size() > 0 && y1dEdges.size() > 0)
				{
					//we managed to make the data rectilinear after all
					if (m_parent->getParameters().xAuto)
						xRange = sci::max<double>(xEdges) - sci::min<double>(xEdges);
					if (m_parent->getParameters().yAuto)
						yRange = sci::max<double>(yEdges) - sci::min<double>(yEdges);

					std::vector<double> xEdgesResampled;
					std::vector<size_t> xEdgesNMerged;
					mergeAxis(x1dEdges, plotXPixels / xRange, 0.5, xEdgesResampled, xEdgesNMerged);
					std::vector<double> yEdgesResampled;
					std::vector<size_t> yEdgesNMerged;
					mergeAxis(y1dEdges, plotYPixels / yRange, 0.5, yEdgesResampled, yEdgesNMerged);
					std::vector<std::vector<double>> zResampled = mergeZ(z, xEdgesNMerged, yEdgesNMerged);
					m_seriesData.reset(new GridData(xEdgesResampled, yEdgesResampled, zResampled, CubehelixColourscale(parameters.colourscaleMin, parameters.colourscaleMax, 101, 1000, 540.0, 1.0, 0.0, 1.0, 1.0, false, false), true, true));
				}
				if (y1dEdges.size() > 0)
				{
					std::vector<std::vector<double>> yEdgesResampled;
					std::vector<std::vector<double>> xEdgesResampled;
					std::vector<std::vector<double>> zResampled;
					resample(xEdges, y1dEdges, z, xRange / plotXPixels * 0.5, yRange / plotYPixels * 0.5, xEdgesResampled, yEdgesResampled, zResampled);
					m_seriesData.reset(new GridDataCurvilinear(xEdgesResampled, yEdgesResampled, zResampled, CubehelixColourscale(parameters.colourscaleMin, parameters.colourscaleMax, 101, 1000, 540.0, 1.0, 0.0, 1.0, 1.0, false, false), true, true));
				}
				else if (x1dEdges.size() > 0)
				{
					std::vector<std::vector<double>> yEdgesResampled;
					std::vector<std::vector<double>> xEdgesResampled;
					std::vector<std::vector<double>> zResampled;
					resample(x1dEdges, yEdges, z, xRange / plotXPixels * 0.5, yRange / plotYPixels * 0.5, xEdgesResampled, yEdgesResampled, zResampled);
					m_seriesData.reset(new GridDataCurvilinear(xEdgesResampled, yEdgesResampled, zResampled, CubehelixColourscale(parameters.colourscaleMin, parameters.colourscaleMax, 101, 1000, 540.0, 1.0, 0.0, 1.0, 1.0, false, false), true, true));
				}
				else
				{
					std::vector<std::vector<double>> yEdgesResampled;
					std::vector<std::vector<double>> xEdgesResampled;
					std::vector<std::vector<double>> zResampled;
					resample(xEdges, yEdges, z, xRange / plotXPixels * 0.5, yRange / plotYPixels * 0.5, xEdgesResampled, yEdgesResampled, zResampled);
					m_seriesData.reset(new GridDataCurvilinear(xEdgesResampled, yEdgesResampled, zResampled, CubehelixColourscale(parameters.colourscaleMin, parameters.colourscaleMax, 101, 1000, 540.0, 1.0, 0.0, 1.0, 1.0, false, false), true, true));
				}
				plot->addData(m_seriesData);
				m_parent->getLimits(xMin, xMax, yMin, yMax);
				m_xResolutionAtPreviousPlot = (xMax - xMin) / plotXPixels;
				m_yResolutionAtPreviousPlot = (yMax - yMin) / plotYPixels;
			}
		}
		m_parent->getLimits(xMin, xMax, yMin, yMax);


		window->Refresh();
	}
}

Plot::Plot(wxString label, wxTreeCtrl* plotTreeCtrl, Page* parent, wxWindow* panelParent, wxSizer* panelSizer)

{
	m_nextSeriesId = 1;
	m_parent = parent;
	m_plotTreeCtrl = plotTreeCtrl;
	bool addedToTree = false;
	m_inputPanel = nullptr;
	try
	{
		m_inputPanel = new sci::InputPanel<ParameterTuple>(panelParent, wxID_ANY, Parameters::getLabels(), m_parameters.toTuple(), false, true, true);
		m_inputPanel->Show(false);
		panelSizer->Add(m_inputPanel);
		panelSizer->Layout();
		m_treeItemId = m_plotTreeCtrl->AppendItem(parent->getTreeItemId(), label);
		addedToTree = true;
		if (m_plotTreeCtrl->GetChildrenCount(parent->getTreeItemId()) == 1)
			m_plotTreeCtrl->Expand(parent->getTreeItemId());
		m_splot = m_parent->getPlotWindow()->addplot(m_parameters.xPosition, m_parameters.yPosition, m_parameters.width, m_parameters.height, m_parameters.xLog, m_parameters.yLog);
		m_splot->setxlimits(m_parameters.xMin, m_parameters.xMax);
		m_splot->setylimits(m_parameters.yMin, m_parameters.yMax);
		m_splot->setxautolimits(m_parameters.xAuto);
		m_splot->setyautolimits(m_parameters.yAuto);
	}
	catch (...)
	{
		if (m_inputPanel)
			m_inputPanel->Destroy();
		if(addedToTree)
			m_plotTreeCtrl->Delete(m_treeItemId);
		//only other place to throw is in the addplot call which means m_splot was never assigned. No nedd to delete it
	}
}

Plot::~Plot()
{
	if(m_plotTreeCtrl)
		m_plotTreeCtrl->Delete(m_treeItemId);
	if(m_parent && m_parent->getPlotWindow())
		m_parent->getPlotWindow()->removeplot(m_splot);
	if (m_inputPanel)
		m_inputPanel->Destroy();
}

const int mainFrame::ID_FILE_EXIT = ::wxNewId();
const int mainFrame::ID_FILE_OPEN = ::wxNewId();
const int mainFrame::ID_ADDCANVAS = ::wxNewId();
const int mainFrame::ID_DELETECANVAS = ::wxNewId();
const int mainFrame::ID_PLOTTREECTRL = ::wxNewId();
const int mainFrame::ID_ADDPLOT = ::wxNewId();
const int mainFrame::ID_DELETEPLOT = ::wxNewId();
const int mainFrame::ID_ADDSERIES = ::wxNewId();
const int mainFrame::ID_DELETESERIES = ::wxNewId();

BEGIN_EVENT_TABLE(mainFrame, wxFrame)
EVT_MENU(ID_FILE_EXIT, mainFrame::OnExit)
EVT_MENU(ID_FILE_OPEN, mainFrame::OnOpen)
EVT_MENU(ID_ADDCANVAS, mainFrame::OnAddCanvas)
EVT_MENU(ID_DELETECANVAS, mainFrame::OnDeleteCanvas)
EVT_MENU(ID_ADDPLOT, mainFrame::OnAddPlot)
EVT_MENU(ID_DELETEPLOT, mainFrame::OnDeletePlot)
EVT_MENU(ID_ADDSERIES, mainFrame::OnAddSeries)
EVT_MENU(ID_DELETESERIES, mainFrame::OnDeleteSeries)
EVT_TREE_ITEM_MENU(ID_PLOTTREECTRL, mainFrame::OnPlotTreeItemMenu)
EVT_TREE_SEL_CHANGED(ID_PLOTTREECTRL, mainFrame::OnPlotTreeSelect)
END_EVENT_TABLE()


mainFrame::mainFrame(wxFrame *frame, const wxString& title, const wxString &settingsFile)
	: wxFrame(frame, -1, title)
{
	m_nextCanvasId = 1;

	wxMenuBar* mbar = new wxMenuBar();

	wxMenu* fileMenu = new wxMenu(wxT(""));
	fileMenu->Append(ID_FILE_EXIT, wxT("E&xit\tAlt+F4"), wxT("Exit the application"));
	fileMenu->Append(ID_FILE_OPEN, wxT("&Open\tCtrl+O"), wxT("Open netcdf"));
	mbar->Append(fileMenu, wxT("&File"));

	wxMenu* plotMenu = new wxMenu(wxT(""));
	plotMenu->Append(ID_ADDCANVAS, wxT("&New plot canvas\tCtrl+N"), wxT("Plot a variable"));
	mbar->Append(plotMenu, "&Plot");

	//wxMenu* helpMenu = new wxMenu(wxT(""));
	//helpMenu->Append(ID_HELP_ABOUT, wxT("&About\tF1"), wxT("Show info about this application"));
	//mbar->Append(helpMenu, wxT("&Help"));

	SetMenuBar(mbar);
	wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
	m_topPanel = new wxPanel(this);

	wxBoxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* outputSizer = new wxBoxSizer(wxHORIZONTAL);

	m_logText = new wxTextCtrl(m_topPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(300, 500), wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH);
	m_logText->SetMinSize(wxSize(100, 20));
	m_plotNotebook = new wxNotebook(m_topPanel, wxID_ANY);

	outputSizer->Add(m_logText, 1, wxEXPAND);
	outputSizer->Add(m_plotNotebook, 2, wxEXPAND);

	m_propertiesSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* plotSelectionSizer = new wxBoxSizer(wxVERTICAL);
	plotSelectionSizer->Add(new wxStaticText(m_topPanel, wxID_ANY, "plots:"), 0, wxEXPAND);
	m_plotTreeCtrl = new wxTreeCtrl(m_topPanel, ID_PLOTTREECTRL, wxDefaultPosition, wxSize(300, 200), wxTR_TWIST_BUTTONS | wxTR_NO_LINES | wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);
	m_plotTreeCtrl->AddRoot("Plot navigator (select and right click here)");
	m_plotTreeCtrl->SetItemHasChildren(m_plotTreeCtrl->GetRootItem());
	plotSelectionSizer->Add(m_plotTreeCtrl, 0, wxEXPAND);

	m_propertiesSizer->Add(plotSelectionSizer, 0, wxEXPAND);
	//propertiesSizer->AddStretchSpacer(1);
	panelSizer->Add(outputSizer, 1, wxEXPAND);
	panelSizer->Add(m_propertiesSizer, 0, wxEXPAND);

	topSizer->Add(m_topPanel, 1, wxEXPAND);

	m_topPanel->SetSizer(panelSizer);
	this->SetSizerAndFit(topSizer);
	SetClientSize(800, 600);

	//Bind to wxAPPLY, to catch events when the input pannel apply buttons are pressed
	Bind(wxEVT_BUTTON, &mainFrame::OnApply, this, wxID_APPLY);
}

void mainFrame::OnExit(wxCommandEvent& event)
{
	Close();
}

void mainFrame::OnOpen(wxCommandEvent& event)
{
	try
	{
		/*wxString defaultDir = wxGetCwd();
		wxString defaultFileName = wxEmptyString;
		if (m_currentFileName.length() > 0)
		{
			wxFileName currentFile(sci::nativeUnicode(m_currentFileName));
			defaultDir = currentFile.GetPath();
			defaultFileName = currentFile.GetFullName();
		}
		sci::string filename = sci::fromWxString(wxFileSelector("Select a netcdf file.", defaultDir, defaultFileName, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_FILE_MUST_EXIST | wxFD_OPEN));*/
		sci::string filename = sci::fromWxString(wxFileSelector("Select a netcdf file.", wxEmptyString, wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_FILE_MUST_EXIST | wxFD_OPEN));
		if (filename.length() == 0)
			return;
		openFile(filename);
	}
	catch(sci::err error)
	{
		(*m_logText) << "Error: " << sci::nativeUnicode(error.getErrorMessage()) << "\n";
	}
}

void mainFrame::OnAddCanvas(wxCommandEvent& event)
{
	try
	{
		m_pages.push_back(Page(m_nextCanvasId, m_plotNotebook, m_plotTreeCtrl));
		++m_nextCanvasId;

		//Bind the properties control modified event so we can update the variable options when a file is selected
		Bind(sci::GenericControl<std::pair<size_t, std::vector<wxString>>>::getControlModifiedEventTag(), &mainFrame::OnPropertyModified, this, sci::InputPanel<Series1d::ParameterTuple>::getControlId());
	}
	catch (...)
	{
		(*m_logText) << "Error creating new page\n";
	}
}

void mainFrame::OnDeleteCanvas(wxCommandEvent& event)
{
	try
	{
		for (auto i = m_pages.begin(); i != m_pages.end(); ++i)
		{
			if (m_plotTreeMenuItemId == i->getTreeItemId())
			{
				m_pages.erase(i);
				break;
			}
		}
	}
	catch (...)
	{
		(*m_logText) << "Error deleteing page\n";
	}
}

void mainFrame::OnPlotTreeItemMenu(wxTreeEvent& event)
{
	m_plotTreeMenuItemId = event.GetItem();
	int level = getPlotTreeLevel(m_plotTreeMenuItemId);
	if (level == 0)
	{
		wxMenu menu;
		menu.Append(ID_ADDCANVAS, "Add plot canvas");
		PopupMenu(&menu);
	}
	else if (level == 1)
	{
		wxMenu menu;
		menu.Append(ID_ADDPLOT, "Add plot");
		menu.Append(ID_DELETECANVAS, "Delete canvas");
		PopupMenu(&menu);
	}
	else if (level == 2)
	{
		wxMenu menu;
		menu.Append(ID_ADDSERIES, "Add series");
		menu.Append(ID_DELETEPLOT, "Delete plot");
		PopupMenu(&menu);
	}
	else if (level == 3)
	{
		wxMenu menu;
		menu.Append(ID_DELETESERIES, "Delete series");
		PopupMenu(&menu);
	}
}

void mainFrame::OnPlotTreeSelect(wxTreeEvent& event)
{
	m_plotTreeSelectedId = event.GetItem();
	for (auto i = m_pages.begin(); i != m_pages.end(); ++i)
		i->updateSelection(m_plotTreeSelectedId);
	m_topPanel->GetSizer()->Layout();
}

void mainFrame::OnAddPlot(wxCommandEvent& event)
{
	try
	{
		for (auto i = m_pages.begin(); i != m_pages.end(); ++i)
		{
			if (m_plotTreeMenuItemId == i->getTreeItemId())
			{
				i->addPlot(m_topPanel, m_propertiesSizer);
				m_topPanel->GetSizer()->Layout();
			}
		}
		//Bind the properties control modified event so we can update the variable options when a file is selected
		Bind(sci::GenericControl<std::pair<size_t, std::vector<wxString>>>::getControlModifiedEventTag(), &mainFrame::OnPropertyModified, this, sci::InputPanel<Series1d::ParameterTuple>::getControlId());
	}
	catch (...)
	{
		(*m_logText) << "Error adding plot\n";
	}
}

void mainFrame::OnDeletePlot(wxCommandEvent& event)
{
	try
	{
		for (auto i = m_pages.begin(); i != m_pages.end(); ++i)
			i->removePlotIfOwned(m_plotTreeMenuItemId);
	}
	catch (...)
	{
		(*m_logText) << "Error adding plot\n";
	}
}

void mainFrame::OnAddSeries(wxCommandEvent& event)
{
	try
	{
		for (auto i = m_pages.begin(); i != m_pages.end(); ++i)
			if (i->containsTreeItem(m_plotTreeMenuItemId))
				i->addSeries(m_plotTreeMenuItemId, m_topPanel, m_propertiesSizer, &m_dataSource);

		//Bind the properties control modified event so we can update the variable options when a file is selected
		Bind(sci::GenericControl<std::pair<size_t, std::vector<wxString>>>::getControlModifiedEventTag(), &mainFrame::OnPropertyModified, this, sci::InputPanel<Series1d::ParameterTuple>::getControlId());
	}
	catch (...)
	{
		(*m_logText) << "Error adding plot\n";
	}
}

void mainFrame::OnDeleteSeries(wxCommandEvent& event)
{
	try
	{
		for (auto i = m_pages.begin(); i != m_pages.end(); ++i)
			i->removeSeriesIfOwned(m_plotTreeMenuItemId);
	}
	catch (...)
	{
		(*m_logText) << "Error adding plot\n";
	}
}

void mainFrame::OnApply(wxCommandEvent& event)
{
	try
	{
		for (auto i = m_pages.begin(); i != m_pages.end(); ++i)
			i->applyPropertiesChange(m_plotTreeSelectedId, &m_dataSource);
	}
	catch (...)
	{
		(*m_logText) << "Error applying changes to plot\n";
	}
}

void mainFrame::OnPropertyModified(wxCommandEvent& event)
{
	try
	{
		for (auto i = m_pages.begin(); i != m_pages.end(); ++i)
			i->actOnPropertyModified(event.GetEventObject(), &m_dataSource);
	}
	catch (...)
	{
		(*m_logText) << "Error adding plot\n";
	}
}

void mainFrame::openFile(sci::string ncFileName)
{
	m_dataSource.openFile(ncFileName);
	/*

	sci::InputNcFile file(ncFileName);

	(*m_logText) << "Opened file: " << sci::nativeUnicode(ncFileName) << "\n";
	m_currentFileName = sU("");
	m_axisVariableAttributes.clear();
	m_variableAttributes.clear();


	std::vector<sci::string> variableNames = file.getVariableNames();

	for (size_t i = 0; i < variableNames.size(); ++i)
	{
		std::vector<sci::string> attributeNames = file.getVariableAttributeList(variableNames[i]);

		//Find the type and read in the metadata
		AmfVariableAttributesBase::VariableType type = AmfVariableAttributesBase::VariableType::Unknown;
		for (size_t j = 0; j < attributeNames.size(); ++j)
		{
			if (attributeNames[j] == sU("type"))
			{
				sci::string typeString = file.getVariableAttribute<sci::string>(variableNames[i], attributeNames[j]);
				if (typeString == sU("float32"))
					addVariable<float>(file, variableNames[i]);
				else if (typeString == sU("float64"))
					addVariable<double>(file, variableNames[i]);
				else if (typeString == sU("byte"))
					addVariable<uint8_t>(file, variableNames[i]);
			}
		}
	}

	//find the names of all the variables that can be used as axes
	std::vector<sci::string> axisNames;
	for (size_t i = 0; i < m_variableAttributes.size(); ++i)
	{
		for (size_t j = 0; j < m_variableAttributes[i]->dimensions.size(); ++j)
		{
			bool alreadyListed = false;
			for (size_t k = 0; k < axisNames.size(); ++k)
			{
				if (axisNames[k] == m_variableAttributes[i]->dimensions[j])
					alreadyListed = true;
			}
			if (!alreadyListed)
				axisNames.push_back(m_variableAttributes[i]->dimensions[j]);
		}
		for (size_t j = 0; j < m_variableAttributes[i]->coordinates.size(); ++j)
		{
			bool alreadyListed = false;
			for (size_t k = 0; k < axisNames.size(); ++k)
			{
				if (axisNames[k] == m_variableAttributes[i]->coordinates[j])
					alreadyListed = true;
			}
			if (!alreadyListed)
				axisNames.push_back(m_variableAttributes[i]->coordinates[j]);
		}
	}

	m_axisVariableAttributes.resize(axisNames.size());
	for (size_t i = 0; i < axisNames.size(); ++i)
	{
		for (size_t j = 0; j < m_variableAttributes.size(); ++j)
		{
			if (axisNames[i] == m_variableAttributes[j]->name)
			{
				m_axisVariableAttributes[i] = m_variableAttributes[j];
			}
		}
	}

	m_currentFileName = ncFileName;*/
}

void mainFrame::readBaseAttributes(sci::InputNcFile& file, sci::string variableName, AmfVariableAttributesBase& attributes)
{
	attributes.name = variableName;
	std::vector<sci::string> attributeNames = file.getVariableAttributeList(variableName);
	for (size_t i = 0; i < attributeNames.size(); ++i)
	{
		if (attributeNames[i] == sU("dimension"))
			attributes.dimensions = sci::splitstring(file.getVariableAttribute<sci::string>(variableName, attributeNames[i]), sU(", "), true);
		else if (attributeNames[i] == sU("units"))
			attributes.units = file.getVariableAttribute<sci::string>(variableName, attributeNames[i]);
		else if (attributeNames[i] == sU("standard_name"))
			attributes.units = file.getVariableAttribute<sci::string>(variableName, attributeNames[i]);
		else if (attributeNames[i] == sU("long_name"))
			attributes.units = file.getVariableAttribute<sci::string>(variableName, attributeNames[i]);
		else if (attributeNames[i] == sU("cell_methods"))
			attributes.units = file.getVariableAttribute<sci::string>(variableName, attributeNames[i]);
		else if (attributeNames[i] == sU("coordinates"))
			attributes.coordinates = sci::splitstring(file.getVariableAttribute<sci::string>(variableName, attributeNames[i]), sU(", "), true);
	}

	std::vector<sci::string> dimensions = file.getVariableDimensions(variableName);
	for (size_t i = 0; i < dimensions.size(); ++i)
	{
		bool alreadyGotDimension = false;
		for (size_t j = 0; j < attributes.dimensions.size(); ++j)
			alreadyGotDimension = alreadyGotDimension || attributes.dimensions[j] == dimensions[i];
		if (!alreadyGotDimension)
			attributes.dimensions.push_back(dimensions[i]);
	}
}

/*Page& mainFrame::getSelectedPage()
{
	if (m_plotNotebook->GetPageCount() == 0)
	{
		throw(sci::err(sci::SERR_USER, 0, "Logic error: no plot canvas selected."));
	}
	int index = m_plotNotebook->GetSelection();
	if (index == wxNOT_FOUND)
		throw(sci::err(sci::SERR_USER, 0, "Logic error: no plot canvas selected."));
	return m_pages[(size_t)index];
}*/

int mainFrame::getPlotTreeLevel(const wxTreeItemId& treeItemId)
{
	if(treeItemId == m_plotTreeCtrl->GetRootItem())
		return 0;
	
	wxTreeItemId parent = m_plotTreeCtrl->GetItemParent(treeItemId);
	int level = 1;
	while (parent != m_plotTreeCtrl->GetRootItem())
	{
		parent = m_plotTreeCtrl->GetItemParent(parent);
		++level;
	}
	return level;
}

/*size_t mainFrame::getPageAttributesIndex(const wxTreeItemId& treeItemId)
{
	sci::assertThrow(getPlotTreeLevel(treeItemId) == 1, sci::err(sci::SERR_USER, 0, "Logic error: tree item id is not a canvas."));
	size_t index = -1;
	for (size_t i = 0; i < m_pageAttributes.size(); ++i)
	{
		if (m_pageAttributes[i].treeItemId == treeItemId)
		{
			index = i;
			break;
		}
	}
	sci::assertThrow(index != size_t(-1), sci::err(sci::SERR_USER, 0, "Logic error: invalid tree item id."));
	return index;
}*/



void mainFrame::OnAbout(wxCommandEvent& event)
{
	
	//wxMessageBox(sci::nativeUnicode(sU("Lidar Quicklook Plotter Version ") + m_processingSoftwareInfo.version + sU("\nAvailable from ") + m_processingSoftwareInfo.url), "About Lidar Quicklook Plotter...");
}

mainFrame::~mainFrame()
{
}