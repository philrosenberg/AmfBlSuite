#pragma once
#include<list>
#include<vector>
#include<map>
#include<svector/sstring.h>
#include<svector/sreadwrite.h>
#include<wx/string.h>

class VariableStore
{
public:
	void AddData(std::vector<double> data, std::vector<size_t> shape, sci::string name)
	{
		m_datas.try_emplace(name, data, shape);
	}
	double * AddVariable(sci::string name)
	{
		return m_variables.emplace_back(name, m_datas).getPointerToValue();
	}
	void incrementVariablePositions()
	{
		for (auto iter = m_variables.begin(); iter != m_variables.end(); ++iter)
			iter->incrementPosition();
	}
	bool isPastAllArrayEnds() const
	{
		for (auto iter = m_variables.begin(); iter != m_variables.end(); ++iter)
			if (!iter->isPastArrayEnd())
				return false;
		return true;
	}
	bool getVariablesAreConsistent() const
	{
		size_t nonZeroDimensions = 0;
		for (auto iter = m_variables.begin(); iter != m_variables.end(); ++iter)
		{
			size_t nDimensions = iter->getNumberNonUnityDimesions();
			if (nonZeroDimensions == 0)
				nonZeroDimensions = nDimensions;
			else if (nDimensions != nonZeroDimensions)
				return false;
		}
		return true;
	}
	size_t getLargestVariableNumberDimensions() const
	{
		size_t result = 0;
		for (auto iter = m_variables.begin(); iter != m_variables.end(); ++iter)
			result = std::max(result, iter->getNumberNonUnityDimesions());
		return result;
	}
private:
	class Data
	{
	public:
		Data(std::vector<double> data, std::vector<size_t> shape)
		{
			m_data = data;
			m_shape = shape;
		}
		const std::vector<double>* getData() const
		{
			return &m_data;
		}
		const std::vector<size_t>& getShape() const
		{
			return m_shape;
		}
	private:
		std::vector<double> m_data;
		std::vector<size_t> m_shape;
	};
	class Variable
	{
	public:
		Variable(sci::string name, const std::map<sci::string, Data>& datas)
		{
			m_lastIncrementableDimension = 0;
			m_wholeArrayPosition = 0;
			m_data = nullptr;
			m_value = std::numeric_limits<double>::quiet_NaN();

			auto dataIter = datas.find(name);
			if (dataIter != datas.end())
			{
				m_data = dataIter->second.getData();
				if (m_data->size() == 1)
				{
					m_value = (*m_data)[0];
					m_data = nullptr;
					m_nNonUnityDimensions = 0;
				}
				else if (m_data->size() == 0)
				{
					m_value = std::numeric_limits<double>::quiet_NaN();
					m_data = nullptr;
					m_nNonUnityDimensions = 0;
				}
				else
				{
					m_value = (*m_data)[0];
					m_nNonUnityDimensions = sci::count(dataIter->second.getShape() > 1);
				}
			}
			else
			{
				//split the array indexing off the end of the name
				sci::string splitName = name.substr(0, name.find(sU('[')));
				if (splitName.length() == name.length())
				{
					return;
				}

				//try finding the data again
				dataIter = datas.find(splitName);
				if (dataIter == datas.end())
				{
					return;
				}
				else
				{

					std::vector<size_t> shape = dataIter->second.getShape();
					m_data = dataIter->second.getData();
					if (m_data->size() == 0)
					{
						m_data = nullptr;
						return;
					}

					sci::string sliceOperator = name.substr(splitName.length());


					m_strides.resize(shape.size(), 1);
					for (size_t i = 0; i < m_strides.size(); ++i)
					{
						for (size_t j = i + 1; j < shape.size(); ++j)
							m_strides[i] *= shape[j];
					}

					m_beginPosition.assign(shape.size(), 0);
					m_endPosition = shape;

					size_t currentDimension = -1;
					for (size_t i = 0; i < sliceOperator.length(); ++i)
					{
						if (sliceOperator[i] == sU('['))
						{
							++currentDimension;
							++i;
							sci::string indexString;
							while (sliceOperator[i] != sU(']') && i < sliceOperator.length())
							{
								indexString.push_back(sliceOperator[i]);
								++i;
							}
							unsigned long long index;
							wxString wxIndexString = sci::nativeUnicode(indexString);
							bool isNumber = wxIndexString.ToULongLong(&index);
							if (isNumber)
							{
								m_beginPosition[currentDimension] = index;
								m_endPosition[currentDimension] = index + 1;
							}
						}
					}

					if (sci::product(m_endPosition - m_beginPosition) == 1)
					{
						size_t index = sci::sum(m_beginPosition * m_strides);
						if (index < m_data->size())
							m_value = (*m_data)[index];
						else
							m_value = std::numeric_limits<double>::quiet_NaN();
						m_data = nullptr;
						m_nNonUnityDimensions = 0;
					}
					else
					{
						m_lastIncrementableDimension = 0;
						for (size_t i = 0; i < m_beginPosition.size(); ++i)
							if (m_endPosition[i] - m_beginPosition[i] > 1)
								m_lastIncrementableDimension = i;

						m_nNonUnityDimensions = sci::count((m_endPosition - m_beginPosition) > 1);

						m_position = m_beginPosition;
						size_t index = sci::sum(m_position * m_strides);
						if (index >= m_data->size() || m_position[0] >= m_endPosition[0])
						{
							m_value = std::numeric_limits<double>::quiet_NaN();
							m_data = nullptr;
						}
						else
							m_value = (*m_data)[index];
					}
				}
			}
		}
		void incrementPosition()
		{
			if (m_data)
			{
				if (m_position.size() == 0)
				{
					++m_wholeArrayPosition;
					if(m_wholeArrayPosition < m_data->size())
						m_value = (*m_data)[m_wholeArrayPosition];
					else
					{
						m_value = std::numeric_limits<double>::quiet_NaN();
						m_data = nullptr;
					}
				}
				else
				{
					m_position[m_lastIncrementableDimension]++;
					if (m_lastIncrementableDimension > 0)
					{
						for (size_t j = 0; j < m_lastIncrementableDimension; ++j)
						{
							if (m_position[m_lastIncrementableDimension - j] == m_endPosition[m_lastIncrementableDimension - j])
							{
								m_position[m_lastIncrementableDimension - j] = m_beginPosition[m_lastIncrementableDimension - j];
								m_position[m_lastIncrementableDimension - j - 1]++;
							}
							else
								break;
						}
					}
					size_t index = sci::sum(m_position * m_strides);
					if (index >= m_data->size() || m_position[0] >= m_endPosition[0])
					{
						m_value = std::numeric_limits<double>::quiet_NaN();
						m_data = nullptr;
					}
					else
						m_value = (*m_data)[index];
				}
			}
		}
		double* getPointerToValue()
		{
			return &m_value;
		}
		bool isPastArrayEnd() const
		{
			return m_data == nullptr;
		}
		size_t getNumberNonUnityDimesions() const
		{
			return m_nNonUnityDimensions;
		}
	private:
		double m_value;
		const std::vector<double>* m_data;
		std::vector<size_t> m_strides;
		std::vector<size_t> m_position;
		std::vector<size_t> m_beginPosition;
		std::vector<size_t> m_endPosition;
		size_t m_lastIncrementableDimension;
		size_t m_wholeArrayPosition;
		size_t m_nNonUnityDimensions;
	};
	std::map<sci::string, Data> m_datas;
	std::list<Variable> m_variables;
};

class DataSource
{
public:
	virtual std::vector<sci::string> getFilenames() const = 0;
	virtual std::vector<sci::string> getAxisVariableNames(const sci::string &filename) const = 0;
	virtual std::vector<sci::string> getDataVariableNames(const sci::string &filename) const = 0;
	virtual bool is1d(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const = 0;
	virtual bool is2d(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const = 0;
	virtual std::vector<double> get1dData(const sci::string& filename, const sci::string& variableName, const sci::string &filter) const = 0;
	virtual std::vector<std::vector<double>> get2dData(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const = 0;
};

class NcFileDataSource :public DataSource
{
public:
	void openFile(const sci::string& filename);
	virtual std::vector<sci::string> getFilenames() const override;
	virtual std::vector<sci::string> getAxisVariableNames(const sci::string& filename) const override;
	virtual std::vector<sci::string> getDataVariableNames(const sci::string& filename) const override;
	virtual bool is1d(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const override;
	virtual bool is2d(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const override;
	virtual std::vector<double> get1dData(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const override;
	virtual std::vector<std::vector<double>> get2dData(const sci::string& filename, const sci::string& variableName, const sci::string& filter) const override;

private:
	struct FileInfo
	{
		FileInfo(const sci::string &inputFilename)
			:file(inputFilename), filename(inputFilename)
		{
			variables = file.getVariableNames();
			for (size_t i = 0; i < variables.size(); ++i)
			{
				std::vector<sci::string> attributes = file.getVariableAttributeList(variables[i]);
				for (size_t j = 0; j < attributes.size(); ++j)
					if (attributes[j] == sU("axis"))
						axes.push_back(variables[i]);
			}
		}
		sci::string filename;
		mutable sci::InputNcFile file;
		std::vector<sci::string> variables;
		std::vector<sci::string> axes;
	};

	std::list<FileInfo> m_fileInfos;
};