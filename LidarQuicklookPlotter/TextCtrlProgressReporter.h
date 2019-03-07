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
		m_originalStyle = textControl->GetDefaultStyle();
	}
	void reportProgress(const sci::string &progress) override
	{
		m_textControl->AppendText(sci::nativeUnicode(progress));
		if (m_yieldOnReport)
			wxSafeYield(m_windowToRemainEnabledOnYield, true);
	}
	void setShouldStop(bool shouldStop)
	{
		m_shouldStop = shouldStop;
	}
	bool shouldStop() override { return m_shouldStop; }
private:
	void setNormalModeFormat() override
	{
		m_textControl->SetDefaultStyle(m_originalStyle);
	}
	void setWarningModeFormat() override
	{
		m_textControl->SetDefaultStyle(wxTextAttr(wxColour(0, 128, 128)));
		reportProgress(sU("Warning: "));
	}
	void setErrorModeFormat() override
	{
		m_textControl->SetDefaultStyle(wxTextAttr(*wxRED));
		reportProgress(sU("Error: "));
	}

	wxTextCtrl *m_textControl;
	bool m_yieldOnReport;
	wxWindow* m_windowToRemainEnabledOnYield;
	bool m_shouldStop;
	wxTextAttr m_originalStyle;
};