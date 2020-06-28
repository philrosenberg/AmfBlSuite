#include "NcFile Source.h"
//define MUPARSER_STATIC before we import the muparser header
#define MUPARSER_STATIC
#include "muParser.h"
#include <wx/string.h>

void NcFileDataSource::openFile(const sci::string& filename)
{
	m_fileInfos.push_back(FileInfo(filename));
}

std::vector<sci::string> NcFileDataSource::getFilenames() const
{
	std::vector<sci::string> result;
	for (auto i = m_fileInfos.begin(); i != m_fileInfos.end(); ++i)
		result.push_back(i->filename);
	return result;
}

std::vector<sci::string> NcFileDataSource::getAxisVariableNames(const sci::string& filename) const
{
	for (auto i = m_fileInfos.begin(); i != m_fileInfos.end(); ++i)
		if (i->filename == filename)
			return i->axes;
	return std::vector<sci::string>();
}

std::vector<sci::string> NcFileDataSource::getDataVariableNames(const sci::string& filename) const
{
	for (auto i = m_fileInfos.begin(); i != m_fileInfos.end(); ++i)
		if (i->filename == filename)
			return i->variables;
	return std::vector<sci::string>();
}

mu::value_type* VariableFactory(const mu::char_type* variableName, void* userData)
{
	VariableStore* variableStore = (VariableStore*)(userData);
	return variableStore->AddVariable(sci::fromNativeUnicode(variableName));
}

size_t getFilteredVariableNDimensions(sci::InputNcFile &file, const sci::string& variableName, const sci::string& filter)
{
	std::vector<size_t> originalShape = file.getVariableShape(variableName);
	if (filter.length() == 0)
		return sci::count(originalShape>1);
	VariableStore variableStore;
	try
	{
		//use muparser to parse the expression, but we only send one value in to force the parsing
		std::vector<size_t> testDataShape(originalShape.size(), 2);
		for (size_t i = 0; i < testDataShape.size(); ++i)
			if (originalShape[i] == 1)
				testDataShape[i] = 1;
		std::vector<double> testData(sci::product(testDataShape));
		variableStore.AddData(testData, testDataShape, variableName);
		mu::Parser p;
		p.SetVarFactory(&(VariableFactory), (void*)(&variableStore));
		sci::string customNameChars = sci::fromNativeUnicode(p.ValidNameChars());
		customNameChars = customNameChars + sU("[]");
		p.DefineNameChars(sci::nativeUnicode(customNameChars).c_str());
		p.SetExpr(sci::nativeUnicode(filter));

		//evaluate just once
		p.Eval();
	}
	catch (...)
	{
		return 0;
	}

	//now our variableStore has been updated with all the variables, and has filtered the dimensions.
	if (!variableStore.getVariablesAreConsistent())
		return 0;
	else
		return variableStore.getLargestVariableNumberDimensions();
}

bool NcFileDataSource::is1d(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const
{
	for (auto i = m_fileInfos.begin(); i != m_fileInfos.end(); ++i)
		if (i->filename == filename)
			return getFilteredVariableNDimensions(i->file, variableName, filter) == 1;
	return false;
}

bool NcFileDataSource::is2d(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const
{
	for (auto i = m_fileInfos.begin(); i != m_fileInfos.end(); ++i)
		if (i->filename == filename)
			return getFilteredVariableNDimensions(i->file, variableName, filter) == 2;
	return false;
}

std::vector<double> NcFileDataSource::get1dData(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const
{
	std::vector<double> result;
	std::vector<size_t> shape;
	try
	{
		for (auto i = m_fileInfos.begin(); i != m_fileInfos.end(); ++i)
			if (i->filename == filename)
				result = i->file.getVariable<double>(variableName, shape);
	}
	catch (...)
	{
	}
	if (result.size() > 0 && filter.length() > 0)
	{
		try
		{
			VariableStore variableStore;
			variableStore.AddData(result, shape, variableName);
			result.resize(0);
			mu::Parser p;
			p.SetVarFactory(&(VariableFactory), (void*)(&variableStore));
			sci::string customNameChars = sci::fromNativeUnicode(p.ValidNameChars());
			customNameChars = customNameChars + sU("[]");
			p.DefineNameChars(sci::nativeUnicode(customNameChars).c_str());
			p.SetExpr(sci::nativeUnicode(filter));
			size_t n = 0;
			do
			{
				result.push_back(p.Eval());
				variableStore.incrementVariablePositions();
				++n;
			} while (!variableStore.isPastAllArrayEnds());
			result.resize(n);
		}
		catch (...)
		{
			result.resize(0);
		}
	}

	return result;
}

std::vector<std::vector<double>> NcFileDataSource::get2dData(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const
{

	try
	{
		std::vector<size_t> shape;
		std::vector<double> dataFlattened;
		for (auto i = m_fileInfos.begin(); i != m_fileInfos.end(); ++i)
			if (i->filename == filename)
			{
				dataFlattened = i->file.getVariable<double>(variableName, shape);
				std::vector<std::vector<double>> result;
				sci::reshape(result, dataFlattened, shape);
				return result;
			}
	}
	catch (...)
	{

	}
	return std::vector<std::vector<double>>();
}
