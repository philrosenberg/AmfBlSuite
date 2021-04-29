#pragma once
#include<string>
namespace sci
{
	using string = std::basic_string<char16_t>;
	std::string nativeCodepage(const sci::string&);
}

inline wxTextCtrl& operator << (wxTextCtrl& stream, const sci::string& str)
{
	return stream << sci::nativeCodepage(str);
}

#include<svector/sstring.h>

#include "ProgressReporter.h"
#include<wx/textctrl.h>

template<class STREAM>
class TextCtrlProgressReporter : public StreamProgressReporter<sci::Oteestream<wxTextCtrl, STREAM>>
{
public:
	TextCtrlProgressReporter(wxTextCtrl *textControl, bool yieldOnReport, wxWindow* windowToRemainEnabledOnYield, STREAM *additionalStream)
		:StreamProgressReporter<sci::Oteestream<wxTextCtrl, STREAM>>(&m_stream), m_stream(textControl, additionalStream)
	{
		m_textControl = textControl;
		m_yieldOnReport = yieldOnReport;
		m_windowToRemainEnabledOnYield = windowToRemainEnabledOnYield;
		m_shouldStop = false;
		m_originalStyle = textControl->GetDefaultStyle();
	}
	void setAdditionalStream(STREAM* stream)
	{
		m_stream.setStream2(stream);
	}
	STREAM* getAdditionalStream() const
	{
		return m_stream.getStream2();
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
	sci::Oteestream<wxTextCtrl, STREAM> m_stream;
};

template<class STREAM>
class ProgressReporterStreamSetter
{
public:
	ProgressReporterStreamSetter(TextCtrlProgressReporter<STREAM>* progressReporter, STREAM* stream)
		:m_stream(progressReporter->getAdditionalStream()), m_progressReporter(progressReporter)
	{
		progressReporter->setAdditionalStream(stream);
	}
	~ProgressReporterStreamSetter()
	{
		m_progressReporter->setAdditionalStream(m_stream);
	}
private:
	STREAM* m_stream;
	TextCtrlProgressReporter<STREAM>* m_progressReporter;
};