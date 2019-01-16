#pragma once
#include<string>
#include<strstream>

class ProgressReporter
{
public:
	virtual void reportProgress(const sci::string &progress) = 0;
	virtual bool shouldStop() = 0;
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
};