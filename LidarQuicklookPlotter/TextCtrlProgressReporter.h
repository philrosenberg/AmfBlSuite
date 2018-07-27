#pragma once
#include "ProgressReporter.h"
#include<wx/textctrl.h>

class TextCtrlProgressReporter : public ProgressReporter
{
public:
	TextCtrlProgressReporter(wxTextCtrl *textControl, bool yieldOnReport, wxWindow* windowToRemainEnabledOnYield)
	{
		m_textControl = textControl;
		m_yieldOnReport = yieldOnReport;
		m_windowToRemainEnabledOnYield = windowToRemainEnabledOnYield;
		m_shouldStop = false;
	}
	void reportProgress(const std::string &progress) override
	{
		m_textControl->AppendText(progress);
		if (m_yieldOnReport)
			wxSafeYield(m_windowToRemainEnabledOnYield);
	}
	void setShouldStop(bool shouldStop)
	{
		m_shouldStop = shouldStop;
	}
	bool shouldStop() override { return m_shouldStop; }
private:
	wxTextCtrl *m_textControl;
	bool m_yieldOnReport;
	wxWindow* m_windowToRemainEnabledOnYield;
	bool m_shouldStop;
};