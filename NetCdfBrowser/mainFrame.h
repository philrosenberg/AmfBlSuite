#ifndef APEX_MAINFRAME_H
#define APEX_MAINFRAME_H

#include "app.h"
#include<svector/svector.h>
#include<svector/splot.h>
#include<svector/sreadwrite.h>
#include<svector/sstring.h>
#include<memory>
#include"../LidarQuicklookPlotter/AmfNc.h"
#include<wx/notebook.h>
#include<wx/treectrl.h>
#include <wx/weakref.h>
#include<list>
#include<svector/controls.h>
#include"../LidarQuicklookPlotter/Plotting.h"
#include"NcFile Source.h"



struct AmfVariableAttributesBase
{
	enum class VariableType
	{
		Unknown,
		Float32,
		Float64,
		Uint8
	};
	sci::string name;
	std::vector<sci::string> dimensions;
	sci::string units;
	sci::string standardName;
	sci::string longName;
	std::string cellMethod;
	std::vector<sci::string> coordinates;
	bool hasFillValue;
	VariableType type;
};

template<class T>
struct AmfVariableAttributes : public AmfVariableAttributesBase
{
	T fillValue;
	T validMin;
	T validMax;
};

inline void mergeAxis(const std::vector<double>& original, double pixelsPerUnit, double minPixelsPerSpan, std::vector<double>& merged, std::vector<size_t>& nMerged)
{
	merged.resize(0);
	nMerged.resize(0);
	merged.reserve(original.size());
	nMerged.reserve(original.size());

	merged.push_back(original[0]);
	size_t n = 1;
	for (size_t i = 1; i < original.size(); ++i)
	{
		if ((original[i] - original[i - n])*pixelsPerUnit >= minPixelsPerSpan || i == original.size()-1)
		{
			merged.push_back(original[i]);
			nMerged.push_back(n);
			n = 1;
		}
		else
			++n;
	}
}

inline std::vector<std::vector<double>> mergeZ(const std::vector<std::vector<double>>& original, const std::vector<size_t>& nMergedX, const std::vector<size_t>& nMergedY)
{
	std::vector<std::vector<double>> result(nMergedX.size());
	size_t originalXStart = 0;
	for (size_t i = 0; i < result.size(); ++i)
	{
		size_t originalYStart = 0;
		result[i].resize(nMergedY.size());
		for (size_t j = 0; j < result[i].size(); ++j)
		{
			for (size_t k = 0; k < nMergedX[i]; ++k)
				for (size_t l = 0; l < nMergedY[j]; ++l)
					result[i][j] += original[originalXStart + k][originalYStart + l];
			result[i][j] /= double(nMergedX[i] * nMergedY[j]);
			originalYStart += nMergedY[j];
		}
		originalXStart += nMergedX[i];
	}
	return result;
}

class Series1d
{
public:
	friend class Plot;
	const wxTreeItemId& getTreeItemId() const
	{
		return m_treeItemId;
	}
	Series1d& Series1d::operator= (Series1d&& plot)
	{
		m_plotTreeCtrl = nullptr;
		m_parent = nullptr;
		m_inputPanel = nullptr;
		std::swap(m_plotTreeCtrl, plot.m_plotTreeCtrl);
		std::swap(m_parent, plot.m_parent);
		std::swap(m_inputPanel, plot.m_inputPanel);
		//m_parameters = plot.m_parameters;
		m_treeItemId = plot.m_treeItemId;
	}
	Series1d(Series1d&& plot)
	{
		m_plotTreeCtrl = nullptr;
		m_parent = nullptr;
		m_inputPanel = nullptr;
		std::swap(m_plotTreeCtrl, plot.m_plotTreeCtrl);
		std::swap(m_parent, plot.m_parent);
		std::swap(m_inputPanel, plot.m_inputPanel);
		//m_parameters = plot.m_parameters;
		m_treeItemId = plot.m_treeItemId;
	}
	//remove copy constructors
	Series1d(const Series1d&) = delete;
	const Series1d& operator=(const Series1d&) = delete;
	~Series1d();
	bool containsTreeItem(const wxTreeItemId& id)
	{
		return id == m_treeItemId;
	}

	void updateSelection(const wxTreeItemId& selected)
	{
		if (selected == m_treeItemId)
		{
			m_inputPanel->Show(true);
		}
		else
		{
			m_inputPanel->Show(false);
		}
	}

	void actOnPropertyModified(wxObject* control, const DataSource* dataSource)
	{
		//if the filename input has changed, we need to update the variable names
		if (m_inputPanel->getControlIndexFromPointer(control) == 3)
			setVariableSelectors(dataSource);

		/*std::vector<sci::string>variables = dataSource->getDataVariableNames(filename);
		std::vector<sci::string>axeses = dataSource->getAxisVariableNames(filename);

		std::pair<size_t, std::vector<wxString>> xVariableValue;
		std::pair<size_t, std::vector<wxString>> yVariableValue;

		std::vector<wxString> variables(3);
		for (size_t i = 0; i < variables.size(); ++i)
		{
			variables[i] << filename << ":variable " << i;
		}
		m_inputPanel->setValue<4>(std::make_pair(size_t(0), variables), true);*/
	}

	template <size_t I>
	void setSelector(const std::vector<sci::string> &newStrings, size_t indexIfNoMatch, bool markModifiedIfChanged, bool markModifiedIfNotChanged)
	{
		auto originalValue = m_inputPanel->getTempValue<I>();
		if (originalValue.second.size() == 0)
		{
			bool changed = newStrings.size() > 0;
			size_t newIndex = newStrings.size() == 0 ? -1 : indexIfNoMatch;
			m_inputPanel->setValue<I>(std::make_pair(newIndex, newStrings), (changed && markModifiedIfChanged) || (!changed && markModifiedIfNotChanged));
		}
		else
		{
			auto originalString = originalValue.second[originalValue.first];
			size_t newIndex = newStrings.size() == 0 ? -1 : indexIfNoMatch;
			bool changed = true;
			for (size_t i = 0; i < newStrings.size(); ++i)
			{
				if (newStrings[i] == originalString)
				{
					newIndex = i;
					changed = false;
				}
			}
			m_inputPanel->setValue<I>(std::make_pair(newIndex, newStrings), (changed && markModifiedIfChanged) || (!changed && markModifiedIfNotChanged));
		}
		m_inputPanel->Layout();
	}

	void setVariableSelectors(const DataSource* dataSource)
	{
		auto filenameValues = m_inputPanel->getTempValue<3>();
		if (filenameValues.second.size() == 0)
		{
			setSelector<4>({}, -1, true, false);
			setSelector<6>({}, -1, true, false);
			setSelector<8>({}, -1, true, false);
		}
		else
		{
			sci::string filename = filenameValues.second[filenameValues.first];
			setSelector<4>(dataSource->getDataVariableNames(filename), 0, true, false);
			setSelector<6>(dataSource->getAxisVariableNames(filename), 0, true, false);
			setSelector<8>(dataSource->getAxisVariableNames(filename), 0, true, false);
		}
	}

	void setSelectors(const DataSource* dataSource)
	{
		setSelector<3>(dataSource->getFilenames(), 0, true, false);
		setVariableSelectors(dataSource);
	}

	void applyPropertiesChange(const wxTreeItemId& selected, splotwindow* window, splot2d* plot, const DataSource* dataSource);
	typedef std::tuple<double, double, double, std::pair<size_t, std::vector<sci::string>>, std::pair<size_t, std::vector<sci::string>>, sci::string, std::pair<size_t, std::vector<sci::string>>, sci::string, std::pair<size_t, std::vector<sci::string>>, sci::string, double, double, bool> ParameterTuple;
	void getLimits(double& xMin, double& xMax, double &yMin, double &yMax) const
	{
		xMin = std::numeric_limits<double>::infinity();
		xMax = -std::numeric_limits<double>::infinity();
		yMin = std::numeric_limits<double>::infinity();
		yMax = -std::numeric_limits<double>::infinity();
		if (m_seriesData)
			m_seriesData->getLimits(xMin, xMax, yMin, yMax);
	}
	void updateResolution(splotwindow* window, splot2d* plot, const DataSource* dataSource, bool forceRefresh);
	void refreshData(splotwindow* window, splot2d* plot, const DataSource* dataSource);
private:
	Series1d::Series1d(wxString label, wxTreeCtrl* plotTreeCtrl, Plot* parent, wxWindow* panelParent, wxSizer* panelSizer, const DataSource* dataSource);
	struct Parameters
	{
		double red;
		double green;
		double blue;
		std::pair<size_t, std::vector<sci::string>> file;
		std::pair<size_t, std::vector<sci::string>> variableName;
		sci::string variableFilter;
		std::pair<size_t,std::vector<sci::string>> xVariableName;
		sci::string xFilter;
		std::pair<size_t, std::vector<sci::string>> yVariableName;
		sci::string yFilter;
		double colourscaleMin;
		double colourscaleMax;
		bool autoColourscale;
		Parameters()
		{
			red = 0.0;
			green = 0.0;
			blue = 0.0;
			file = std::make_pair(0, std::vector<sci::string>(0));
			variableName = std::make_pair(0, std::vector<sci::string>(0));
			variableFilter = sci::string(sU(""));
			xVariableName = std::make_pair(0, std::vector<sci::string>(0));
			xFilter = sci::string(sU(""));
			yVariableName = std::make_pair(0, std::vector<sci::string>(0));
			yFilter = sci::string(sU(""));
			colourscaleMin = 0.0;
			colourscaleMax = 1.0;
			autoColourscale = true;
		}
		ParameterTuple toTuple()
		{
			return std::make_tuple(red, green, blue, file, variableName, variableFilter, xVariableName, xFilter, yVariableName, yFilter, colourscaleMin, colourscaleMax, autoColourscale);
		}
		Parameters& fromTuple(const ParameterTuple& tuple)
		{
			red = std::get<0>(tuple);
			green = std::get<1>(tuple);
			blue = std::get<2>(tuple);
			file = std::get<3>(tuple);
			variableName = std::get<4>(tuple);
			variableFilter = std::get<5>(tuple);
			xVariableName = std::get<6>(tuple);
			xFilter = std::get<7>(tuple);
			yVariableName = std::get<8>(tuple);
			yFilter = std::get<9>(tuple);
			colourscaleMin = std::get<10>(tuple);
			colourscaleMax = std::get<11>(tuple);
			autoColourscale = std::get<12>(tuple);

			return *this;
		}
		static std::vector<wxString> getLabels()
		{
			return { "Red", "Geeen", "Blue", "File", "Variable", "Variable filter", "X variable", " X variable filter", "Y variable (2d)", "Y variable filter", "Colourscale min", "Colourscale max", "Auto Colourscale" };
		}
	};
	//Parameters m_parameters;
	wxTreeItemId m_treeItemId;
	wxWeakRef< wxTreeCtrl> m_plotTreeCtrl;
	Plot* m_parent;
	wxWeakRef <sci::InputPanel<ParameterTuple>> m_inputPanel;
	std::shared_ptr<DrawableItem> m_seriesData;
	double m_xResolutionAtPreviousPlot;
	double m_yResolutionAtPreviousPlot;
};


class Plot
{
	friend class Page;
public:
	~Plot();
	const wxTreeItemId& getTreeItemId() const
	{
		return m_treeItemId;
	}
	Plot& Plot::operator= (Plot&& plot)
	{
		m_plotTreeCtrl = nullptr;
		m_splot = nullptr;
		m_parent = nullptr;
		m_inputPanel = nullptr;
		std::swap(m_plotTreeCtrl, plot.m_plotTreeCtrl);
		std::swap(m_splot, plot.m_splot);
		std::swap(m_parent, plot.m_parent);
		std::swap(m_inputPanel, plot.m_inputPanel);
		m_parameters = plot.m_parameters;
		m_treeItemId = plot.m_treeItemId;
		m_nextSeriesId = plot.m_nextSeriesId;
		std::swap(m_series, plot.m_series);
	}
	Plot(Plot&& plot)
	{
		m_plotTreeCtrl = nullptr;
		m_splot = nullptr;
		m_parent = nullptr;
		m_inputPanel = nullptr;
		std::swap(m_plotTreeCtrl, plot.m_plotTreeCtrl);
		std::swap(m_splot, plot.m_splot);
		std::swap(m_parent, plot.m_parent);
		std::swap(m_inputPanel, plot.m_inputPanel);
		m_parameters = plot.m_parameters;
		m_treeItemId = plot.m_treeItemId;
		m_nextSeriesId = plot.m_nextSeriesId;
		std::swap(m_series, plot.m_series);
	}
	//remove copy constructors
	Plot(const Plot&) = delete;
	const Plot& operator=(const Plot&) = delete;

	void updateSelection(const wxTreeItemId& selected)
	{
		if (selected == m_treeItemId)
		{
			m_inputPanel->Show(true);
		}
		else
		{
			m_inputPanel->Show(false);
		}
		for (auto i = m_series.begin(); i != m_series.end(); ++i)
			i->updateSelection(selected);
	}

	void applyPropertiesChange(const wxTreeItemId& selected, splotwindow* window, const DataSource* dataSource)
	{
		if (selected == m_treeItemId)
		{
			if (!m_inputPanel->hasFlushedChanged())
			{
				return;
			}
			else
			{
				m_parameters.fromTuple(m_inputPanel->getValues());
				m_splot->setxlimits(m_parameters.xMin, m_parameters.xMax);
				m_splot->setylimits(m_parameters.yMin, m_parameters.yMax);
				m_splot->setxautolimits(m_parameters.xAuto);
				m_splot->setyautolimits(m_parameters.yAuto);
				m_splot->setlogxaxis(m_parameters.xLog);
				m_splot->setlogyaxis(m_parameters.yLog);
				window->moveplot(m_splot, m_parameters.xPosition, m_parameters.yPosition, m_parameters.width, m_parameters.height);
				window->Refresh();

				//ensure any resolution changes are propagated throught to the series
				for (auto i = m_series.begin(); i != m_series.end(); ++i)
					i->updateResolution(window, m_splot, dataSource, false);
			}
				
		}
		for (auto i = m_series.begin(); i != m_series.end(); ++i)
			i->applyPropertiesChange(selected, window, m_splot, dataSource);

	}
	void addSeries(wxWindow* panelParent, wxSizer* panelSizer, splotwindow *plotWindow, const DataSource *dataSource)
	{
		wxString label = "Series ";
		label << m_nextSeriesId;
		++m_nextSeriesId;
		m_series.push_back(std::move(Series1d(label, m_plotTreeCtrl, this, panelParent, panelSizer, dataSource)));
		plotWindow->Refresh();
	}
	splot2d* getPlot()
	{
		return m_splot;
	}
	const splot2d* getPlot() const
	{
		return m_splot;
	}
	bool containsTreeItem(const wxTreeItemId& id)
	{
		if (id == m_treeItemId)
			return true;
		else
			for (auto i = m_series.begin(); i != m_series.end(); ++i)
				if (i->containsTreeItem(id))
					return true;
		return false;
	}
	void removeSeriesIfOwned(const wxTreeItemId& plotTreeItemId)
	{
		for (auto i = m_series.begin(); i != m_series.end(); ++i)
			if (i->getTreeItemId() == plotTreeItemId)
			{
				m_series.erase(i);
				break;
			}
	}
	void actOnPropertyModified(wxObject* control, const DataSource* dataSource)
	{
		for (auto i = m_series.begin(); i != m_series.end(); ++i)
			i->actOnPropertyModified(control, dataSource);
	}
	struct Parameters;
	const Parameters &getParameters() const
	{
		return m_parameters;
	}
	typedef std::tuple<double, double, double, double, double, double, bool, bool, double, double, bool, bool> ParameterTuple;
	struct Parameters
	{
		double xPosition;
		double width;
		double yPosition;
		double height;
		double xMin;
		double xMax;
		bool xAuto;
		bool xLog;
		double yMin;
		double yMax;
		bool yAuto;
		bool yLog;
		Parameters()
		{
			xPosition = 0.1;
			width = 0.85;
			yPosition = 0.1;
			height = 0.85;
			xMin = 0.0;
			xMax = 1.0;
			xAuto = false;
			xLog = false;
			yMin = 0.0;
			yMax = 1.0;
			yAuto = false;
			yLog = false;
		}
		ParameterTuple toTuple()
		{
			return std::make_tuple(xPosition, width, yPosition, height, xMin, xMax, xAuto, xLog, yMin, yMax, yAuto, yLog);
		}
		Parameters& fromTuple(const ParameterTuple &tuple)
		{
			xPosition = std::get<0>(tuple);
			width = std::get<1>(tuple);
			yPosition = std::get<2>(tuple);
			height = std::get<3>(tuple);
			xMin = std::get<4>(tuple);
			xMax = std::get<5>(tuple);
			xAuto = std::get<6>(tuple);
			xLog = std::get<7>(tuple);
			yMin = std::get<8>(tuple);
			yMax = std::get<9>(tuple);
			yAuto = std::get<10>(tuple);
			yLog = std::get<11>(tuple);

			return *this;
		}
		static std::vector<wxString> getLabels()
		{
			return { "Horizontal position", "Width", "Vertical position", "Height", "Minimum x", "Maximum x", "Autoscale x", "Logarithmic x", "Minimum y", "Maximum y", "Autoscale y", "Logarithmic y" };
		}
	};
	void getLimits(double &xMin, double &xMax, double &yMin, double &yMax) const
	{
		xMin = std::numeric_limits<double>::infinity();
		xMax = -std::numeric_limits<double>::infinity();
		yMin = std::numeric_limits<double>::infinity();
		yMax = -std::numeric_limits<double>::infinity();

		if (m_inputPanel)
		{
			Parameters parameters;
			parameters.fromTuple(m_inputPanel->getValues());
			if (!parameters.xAuto && !parameters.yAuto)
			{
				xMin = parameters.xMin;
				xMax = parameters.xMax;
				yMin = parameters.yMin;
				yMax = parameters.yMax;
			}
			else if (!parameters.xAuto)
			{
				xMin = parameters.xMin;
				xMax = parameters.xMax; 
				for (auto i = m_series.begin(); i != m_series.end(); ++i)
				{
					double thisYMin;
					double thisYMax;
					double temp1;
					double temp2;
					i->getLimits(temp1, temp2, thisYMin, thisYMax);
					if (thisYMin < yMin)
						yMin = thisYMin;
					if (thisYMax > yMax)
						yMax = thisYMax;
				}
			}
			else if (!parameters.yAuto)
			{
				yMin = parameters.yMin;
				yMax = parameters.yMax;
				for (auto i = m_series.begin(); i != m_series.end(); ++i)
				{
					double thisXMin;
					double thisXMax;
					double temp1;
					double temp2;
					i->getLimits(thisXMin, thisXMax, temp1, temp2);
					if (thisXMin < xMin)
						xMin = thisXMin;
					if (thisXMax > xMax)
						xMax = thisXMax;
				}
			}
			else
			{
				for (auto i = m_series.begin(); i != m_series.end(); ++i)
				{
					double thisXMin;
					double thisXMax;
					double thisYMin;
					double thisYMax;
					i->getLimits(thisXMin, thisXMax, thisYMin, thisYMax);
					if (thisXMin < xMin)
						xMin = thisXMin;
					if (thisXMax > xMax)
						xMax = thisXMax;
					if (thisYMin < yMin)
						yMin = thisYMin;
					if (thisYMax > yMax)
						yMax = thisYMax;
				}
			}
		}
	}
private:
	Plot(wxString label, wxTreeCtrl* plotTreeCtrl, Page* parent, wxWindow* panelParent, wxSizer* panelSizer);
	Parameters m_parameters;
	splot2d* m_splot;
	wxTreeItemId m_treeItemId;
	wxWeakRef< wxTreeCtrl> m_plotTreeCtrl;
	Page *m_parent;
	wxWeakRef <sci::InputPanel<ParameterTuple>> m_inputPanel;
	size_t m_nextSeriesId;
	std::list<Series1d> m_series;
};

class Page
{
public:
	Page(size_t index, wxNotebook *plotNotebook, wxTreeCtrl *plotTreeCtrl)
	{
		m_plotWindow = nullptr;

		m_plotNotebook = plotNotebook;
		m_plotTreeCtrl = plotTreeCtrl;
		bool addedPage = false;
		bool addedTreeItem = false;
		try
		{
			m_plotWindow = new splotwindow(m_plotNotebook, true, false);
			m_nextPlotId = 1;
			wxString pageTitle = "Page ";
			pageTitle << index;
			m_plotNotebook->AddPage(m_plotWindow, pageTitle);
			addedPage = true;
			m_treeItemId = m_plotTreeCtrl->AppendItem(m_plotTreeCtrl->GetRootItem(), pageTitle);
			addedTreeItem = true;
			m_plotTreeCtrl->SetItemHasChildren(m_treeItemId);
			if (m_plotTreeCtrl->GetChildrenCount(m_plotTreeCtrl->GetRootItem()) == 1)
				m_plotTreeCtrl->Expand(m_plotTreeCtrl->GetRootItem());
		}
		catch (...)
		{
			try
			{
				if (addedTreeItem)
					m_plotTreeCtrl->Delete(m_treeItemId);
			}
			catch (...)
			{
			}
			try
			{
				if (addedPage)
				{
					size_t pageIndex = m_plotNotebook->FindPage(m_plotWindow);
					m_plotNotebook->RemovePage(pageIndex);
				}
			}
			catch (...)
			{
			}

			if(m_plotWindow)
				m_plotWindow->Destroy();
			throw;
		}
	}
	void addPlot(wxWindow* panelParent, wxSizer* panelSizer)
	{
		wxString label = "Plot ";
		label << m_nextPlotId;
		++m_nextPlotId;
		m_plots.push_back(std::move(Plot(label, m_plotTreeCtrl, this, panelParent, panelSizer)));
		m_plotWindow->Refresh();
	}
	void removePlotIfOwned(const wxTreeItemId &plotTreeId)
	{
		for (auto i = m_plots.begin(); i != m_plots.end(); ++i)
		{
			if (i->getTreeItemId() == plotTreeId)
			{
				m_plots.erase(i);
				m_plotWindow->Refresh();
				break;
			}
		}
	}
	void actOnPropertyModified(wxObject* control, const DataSource* dataSource)
	{
		for (auto i = m_plots.begin(); i != m_plots.end(); ++i)
		{
			i->actOnPropertyModified(control, dataSource);
		}
	}
	splotwindow* getPlotWindow()
	{
		return m_plotWindow;
	}
	const splotwindow* getPlotWindow() const
	{
		return m_plotWindow;
	}
	wxTreeItemId& getTreeItemId()
	{
		return m_treeItemId;
	}
	const wxTreeItemId& getTreeItemId() const
	{
		return m_treeItemId;
	}
	~Page()
	{
		if(m_plotTreeCtrl)
			m_plotTreeCtrl->Delete(m_treeItemId);
		if (m_plotNotebook)
		{
			size_t pageIndex = m_plotNotebook->FindPage(m_plotWindow);
			if(pageIndex != wxNOT_FOUND)
				m_plotNotebook->DeletePage(pageIndex);
		}

		//Deletin the page should happen in the m_plotNotebook->DeletePage call
		//however there is a chance someone could have called m_plotNotebook->RemovePage at some point
		//which means the page will not be found and deleted above
		//so we explicitly check if m_plotWindow still exists and destroy if needed.
		if(m_plotWindow)
			m_plotWindow->Destroy();
	}
	Page& Page::operator= (Page&& page)
	{
		m_plotWindow = nullptr;
		m_plotNotebook = nullptr;
		m_plotTreeCtrl = nullptr;
		std::swap(m_plotWindow, page.m_plotWindow);
		std::swap(m_plotNotebook, page.m_plotNotebook);
		std::swap(m_plotTreeCtrl, page.m_plotTreeCtrl);
		m_treeItemId = page.m_treeItemId;
		m_nextPlotId = page.m_nextPlotId;
		std::swap(m_plots, page.m_plots);
	}
	Page(Page&& page)
	{
		m_plotWindow = nullptr;
		m_plotNotebook = nullptr;
		m_plotTreeCtrl = nullptr;
		std::swap(m_plotWindow, page.m_plotWindow);
		std::swap(m_plotNotebook, page.m_plotNotebook);
		std::swap(m_plotTreeCtrl, page.m_plotTreeCtrl);
		m_treeItemId = page.m_treeItemId;
		m_nextPlotId = page.m_nextPlotId;
		std::swap(m_plots, page.m_plots);
	}
	//remove copy constructors
	Page(const Page&) = delete;
	const Page& operator=(const Page&) = delete;

	void updateSelection(const wxTreeItemId& selected)
	{
		if (selected == m_treeItemId)
		{
		}
		for (auto i = m_plots.begin(); i != m_plots.end(); ++i)
			i->updateSelection(selected);
	}
	void applyPropertiesChange(const wxTreeItemId& selected, const DataSource* dataSource)
	{
		if (selected == m_treeItemId)
		{
		}
		for (auto i = m_plots.begin(); i != m_plots.end(); ++i)
			i->applyPropertiesChange(selected, m_plotWindow, dataSource);
	}
	bool containsTreeItem(const wxTreeItemId& id)
	{
		if (id == m_treeItemId)
			return true;
		else
			for (auto i = m_plots.begin(); i != m_plots.end(); ++i)
				if (i->containsTreeItem(id))
					return true;
		return false;
	}
	void addSeries(const wxTreeItemId &plotTreeItemId, wxWindow* panelParent, wxSizer* panelSizer, const DataSource *dataSource)
	{
		for (auto i = m_plots.begin(); i != m_plots.end(); ++i)
			if (i->getTreeItemId() == plotTreeItemId)
				i->addSeries(panelParent, panelSizer, m_plotWindow, dataSource);
	}
	void removeSeriesIfOwned(const wxTreeItemId& plotTreeItemId)
	{
		for (auto i = m_plots.begin(); i != m_plots.end(); ++i)
			i->removeSeriesIfOwned(plotTreeItemId);
	}
private:
	wxWeakRef<splotwindow> m_plotWindow;
	wxWeakRef<wxNotebook> m_plotNotebook;
	wxWeakRef<wxTreeCtrl> m_plotTreeCtrl;
	wxTreeItemId m_treeItemId;
	size_t m_nextPlotId;
	std::list<Plot> m_plots; //list doesn't require copies when erasing elements like vectors. Plot class is not copyable
};

class mainFrame : public wxFrame
{
public:
	static const int ID_FILE_EXIT;
	static const int ID_FILE_OPEN;
	static const int ID_ADDCANVAS;
	static const int ID_DELETECANVAS;
	static const int ID_PLOTTREECTRL;
	static const int ID_ADDPLOT;
	static const int ID_DELETEPLOT;
	static const int ID_ADDSERIES;
	static const int ID_DELETESERIES;

	mainFrame(wxFrame *frame, const wxString& title, const wxString &settingsFile);
	~mainFrame();
private:
	//event methods
	void OnExit(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnAddCanvas(wxCommandEvent& event);
	void OnDeleteCanvas(wxCommandEvent& event);
	void OnPlotTreeItemMenu(wxTreeEvent& event);
	void OnPlotTreeSelect(wxTreeEvent& event);
	void OnAddPlot(wxCommandEvent& event);
	void OnDeletePlot(wxCommandEvent& event);
	void OnAddSeries(wxCommandEvent& event);
	void OnDeleteSeries(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);
	void OnPropertyModified(wxCommandEvent& event);

	//controls
	wxPanel *m_topPanel;
	wxTextCtrl* m_logText;
	wxNotebook* m_plotNotebook;
	std::list<Page> m_pages; //list doesn't require copies when erasing elements like vectors. Page class is not copyable
	wxTreeCtrl* m_plotTreeCtrl;

	wxTreeItemId m_plotTreeMenuItemId;
	wxTreeItemId m_plotTreeSelectedId;
	wxBoxSizer* m_propertiesSizer;

	//internal methods
	void openFile(sci::string ncFileName);
	template< class T>
	void addVariable(sci::InputNcFile& file, sci::string variableName);
	void readBaseAttributes(sci::InputNcFile& file, sci::string variableName, AmfVariableAttributesBase& attributes);
	//Page& getSelectedPage();
	int getPlotTreeLevel(const wxTreeItemId &treeItemId);
	//size_t getPageAttributesIndex(const wxTreeItemId& treeItemId);

	//data
	NcFileDataSource m_dataSource;
	/*std::vector<std::shared_ptr<AmfVariableAttributesBase>> m_variableAttributes;
	std::vector<std::shared_ptr<AmfVariableAttributesBase>> m_axisVariableAttributes;
	sci::string m_currentFileName;*/

	//variables for tracking numbering of elements
	size_t m_nextCanvasId;

	DECLARE_EVENT_TABLE();
};


template<class T>
void mainFrame::addVariable(sci::InputNcFile& file, sci::string variableName)
{
	std::shared_ptr<AmfVariableAttributes<T>> attributes(new AmfVariableAttributes<T>);
	readBaseAttributes(file, variableName, *attributes);
	bool gotMin = false;
	bool gotMax = false;
	attributes->hasFillValue = false;
	std::vector<sci::string> attributeNames = file.getVariableAttributeList(variableName);
	for (size_t i = 0; i < attributeNames.size(); ++i)
	{
		if (attributeNames[i] == sU("valid_min"))
		{
			gotMin = true;
			attributes->validMin = file.getVariableAttribute<T>(variableName, attributeNames[i]);
		}
		else if (attributeNames[i] == sU("valid_max"))
		{
			gotMax = true;
			attributes->validMax = file.getVariableAttribute<T>(variableName, attributeNames[i]);
		}
		else if (attributeNames[i] == sU("_FillValue"))
		{
			attributes->hasFillValue = true;
			attributes->fillValue = file.getVariableAttribute<T>(variableName, attributeNames[i]);
		}
	}

	m_variableAttributes.push_back(attributes);
}

#endif // APEX_MAINFRAME_H
