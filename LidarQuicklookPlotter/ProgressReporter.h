#pragma once
#include<string>
#include<sstream>

class ProgressReporter
{
public:
	ProgressReporter()
	{
		m_normalMode = true;
		m_warningMode = false;
		m_errorMode = false;
	}
	virtual void reportProgress(const sci::string &progress) = 0;
	virtual bool shouldStop() = 0;
	void setNormalMode()
	{
		m_normalMode = true;
		m_warningMode = false;
		m_errorMode = false;
		setNormalModeFormat();
	}
	void setWarningMode()
	{
		m_normalMode = false;
		m_warningMode = true;
		m_errorMode = false;
		setWarningModeFormat();
	}
	void setErrorMode()
	{
		m_normalMode = false;
		m_warningMode = false;
		m_errorMode = true;
		setErrorModeFormat();
	}
	bool isNormalMode()
	{
		return m_normalMode;
	}
	bool isWarningMode()
	{
		return m_warningMode;
	}
	bool isErrorMode()
	{
		return m_errorMode;
	}
private:
	virtual void setNormalModeFormat() = 0;
	virtual void setWarningModeFormat() = 0;
	virtual void setErrorModeFormat() = 0;
	bool m_normalMode;
	bool m_warningMode;
	bool m_errorMode;
};

template <class T>
ProgressReporter & operator<<(ProgressReporter &reporter, const T &report)
{
	sci::ostringstream stream;
	stream << report;
	reporter.reportProgress(stream.str());
	return reporter;
}

class NullProgressReporter : public ProgressReporter
{
public:
	void reportProgress(const sci::string &progress) override {}
	bool shouldStop() override { return false; }
	void setNormalModeFormat() override {}
	void setWarningModeFormat() override {}
	void setErrorModeFormat() override {}
};

template<class STREAM>
class StreamProgressReporter : public ProgressReporter
{
public:
	StreamProgressReporter(STREAM* stream = nullptr)
		:m_stream(stream)
	{
	}
	void setstream(STREAM* stream)
	{
		m_stream = stream;
	}
	void disconnectStream()
	{
		m_stream = nullptr;
	}
	virtual void reportProgress(const sci::string& progress)
	{
		if(m_stream)
			(*m_stream) << progress;
	}
private:
	STREAM* m_stream;
};

class WarningSetter
{
public:
	WarningSetter(ProgressReporter* progressReporter)
	{
		m_progressReporter = progressReporter;
		progressReporter->setWarningMode();
	}
	~WarningSetter()
	{
		m_progressReporter->setNormalMode();

	}
private:
	ProgressReporter* m_progressReporter;
};

class ErrorSetter
{
public:
	ErrorSetter(ProgressReporter* progressReporter)
	{
		m_progressReporter = progressReporter;
		progressReporter->setErrorMode();
	}
	~ErrorSetter()
	{
		m_progressReporter->setNormalMode();

	}
private:
	ProgressReporter* m_progressReporter;
};