#include"resample.h"
#include<svector/svector.h>

//returns a value > 0 if a point is at one side of a line or a value < 0 if a point is at the other size of a line or a value of zero if a point is on the line
double side(double linex1, double liney1, double linex2, double liney2, double pointx, double pointy)
{
	return (pointx - linex1) * (liney2 - liney1) - (pointy - liney1) * (linex2 - linex1);
}

bool isDart(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
	if (side(x1, y1, x3, y3, x2, y2) * side(x1, y1, x3, y3, x4, y4) > 0.0)
		return true;
	if (side(x2, y2, x4, y4, x1, y1) * side(x2, y2, x4, y4, x3, y3) > 0.0)
		return true;
	return false;
}
double areaTriangle(double length1sq, double length2sq, double length3sq)
{
	return 0.25 * std::sqrt(4.0 * length1sq * length3sq - std::pow(length1sq + length2sq - length3sq, 2));
}
double areaDart(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
	double length1sq = std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2);
	double length2sq = std::pow(x2 - x3, 2) + std::pow(y2 - y3, 2);
	double length3sq = std::pow(x3 - x4, 2) + std::pow(y3 - y4, 2);
	double length4sq = std::pow(x4 - x1, 2) + std::pow(y4 - y1, 2);
	double diag1sq = std::pow(x1 - x3, 2) + std::pow(y1 - y3, 2);

	double area1 = areaTriangle(length1sq, length2sq, diag1sq);
	double area2 = areaTriangle(length3sq, length4sq, diag1sq);
	if (side(x1, y1, x3, y3, x2, y2) * side(x1, y1, x3, y3, x4, y4) > 0)
		return std::abs(area1 - area2);
	else
		return area1 + area2;

}
double areaQuadrilateral(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
	if (isDart(x1, y1, x2, y2, x3, y3, x4, y4))
		return areaDart(x1, y1, x2, y2, x3, y3, x4, y4);

	double length1sq = std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2);
	double length2sq = std::pow(x2 - x3, 2) + std::pow(y2 - y3, 2);
	double length3sq = std::pow(x3 - x4, 2) + std::pow(y3 - y4, 2);
	double length4sq = std::pow(x4 - x1, 2) + std::pow(y4 - y1, 2);

	double diag1sq = std::pow(x1 - x3, 2) + std::pow(y1 - y3, 2);
	double diag2sq = std::pow(x2 - x4, 2) + std::pow(y2 - y4, 2);

	return 0.25 * std::sqrt(4.0 * diag1sq * diag2sq - std::pow(length1sq + length3sq - length2sq - length4sq, 2));
}

double areaQuadrilateral(size_t index1, size_t index2, const std::vector<std::vector<double>>& x, const std::vector<std::vector<double>>& y)
{
	return areaQuadrilateral(x[index1][index2], y[index1][index2], x[index1 + 1][index2], y[index1 + 1][index2], x[index1 + 1][index2 + 1], y[index1 + 1][index2 + 1], x[index1][index2 + 1], y[index1][index2 + 1]);
}

double areaQuadrilateral(size_t index1, size_t index2, const std::vector<double>& x, const std::vector<std::vector<double>>& y)
{
	return areaQuadrilateral(x[index1], y[index1][index2], x[index1 + 1], y[index1 + 1][index2], x[index1 + 1], y[index1 + 1][index2 + 1], x[index1], y[index1][index2 + 1]);
}

double areaQuadrilateral(size_t index1, size_t index2, const std::vector<std::vector<double>>& x, const std::vector<double>& y)
{
	return areaQuadrilateral(x[index1][index2], y[index2], x[index1 + 1][index2], y[index2], x[index1 + 1][index2 + 1], y[index2 + 1], x[index1][index2 + 1], y[index2 + 1]);
}

void resample(const std::vector<std::vector<double>>& x, const std::vector<std::vector<double>>& y, const std::vector<std::vector<double>>& z, double xResolution, double yResolution,
	std::vector<std::vector<double>>& xResampled, std::vector<std::vector<double>>& yResampled, std::vector<std::vector<double>>& zResampled)
{
	//std::vector<std::vector<double>> area = sci::makevector<double>(0.0, z.size(), z[0].size());
	//std::vector<std::vector<SBOOL>> averaged = sci::makevector<SBOOL>(0, z.size(), z[0].size());
	std::vector<std::vector<double>> xTemp = x;
	std::vector<std::vector<double>> yTemp = y;
	std::vector<std::vector<double>> zTemp = z;

	//for (size_t i = 0; i < area.size(); ++i)
	//	for (size_t j = 0; j < area[i].size(); ++j)
	//		area[i][j] = areaQuadrilateral(i, j, x, y);

	for (size_t i = 0; i < xTemp.size(); ++i)
		for (size_t j = 0; j < xTemp[i].size(); ++j)
			xTemp[i][j] = sci::round(xTemp[i][j] / xResolution) * xResolution;
	for (size_t i = 0; i < yTemp.size(); ++i)
		for (size_t j = 0; j < yTemp[i].size(); ++j)
			yTemp[i][j] = sci::round(yTemp[i][j] / yResolution) * yResolution;

	for (size_t i = 0; i < zTemp.size(); ++i)
	{
		for (size_t j = 0; j < zTemp[i].size(); ++j)
		{
			if(xTemp[i][j]==xTemp[i+1][j+1] && yTemp[i][j]==yTemp[i+1][j+1]) //this will just avoid doing the full area calculation in a common 0 area scenario.
				zTemp[i][j] = std::numeric_limits<double>::quiet_NaN();
			else if (areaQuadrilateral(i, j, xTemp, yTemp) == 0)
				zTemp[i][j] = std::numeric_limits<double>::quiet_NaN();
		}
	}
	std::swap(xTemp, xResampled);
	std::swap(yTemp, yResampled);
	std::swap(zTemp, zResampled);
}

void resample_old(const std::vector<std::vector<double>>& x, const std::vector<std::vector<double>>& y, const std::vector<std::vector<double>>& z, double xResolution, double yResolution,
	std::vector<std::vector<double>>& xResampled, std::vector<std::vector<double>>& yResampled, std::vector<std::vector<double>>& zResampled)
{
	std::vector<std::vector<double>> area = sci::makevector<double>(0.0, z.size(), z[0].size());
	std::vector<std::vector<SBOOL>> averaged = sci::makevector<SBOOL>(0, z.size(), z[0].size());
	std::vector<std::vector<double>> xTemp = x;
	std::vector<std::vector<double>> yTemp = y;
	std::vector<std::vector<double>> zTemp = z;

	/*xTemp.resize(100);
	yTemp.resize(100);
	zTemp.resize(99);
	area.resize(99);
	averaged.resize(99);

	for (size_t i = 0; i < 100; ++i)
	{
		xTemp[i].resize(100);
		yTemp[i].resize(100);
		if (i != 99)
		{
			zTemp[i].resize(99);
			area[i].resize(99);
			averaged[i].resize(99);
		}
	}*/
	/*std::swap(xTemp, xResampled);
	std::swap(yTemp, yResampled);
	std::swap(zTemp, zResampled);
	return;*/

	for (size_t i = 0; i < area.size(); ++i)
		for (size_t j = 0; j < area[i].size(); ++j)
			area[i][j] = areaQuadrilateral(i, j, x, y);

	//find quadrillaterals smaller than the resolution and remove them by making all 4 coordinates identical
	size_t minWidth = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < xTemp.size() - 2; i+=minWidth)
	{
		size_t height = 1;
		minWidth = std::numeric_limits<size_t>::max();
		for (size_t j = 0; j < xTemp[i].size() - 2; j += height)
		{
			height = 1;
			size_t width = 1;
			double bottom;
			if(yTemp[i][j+1] < yTemp[i][j])
				bottom = std::max(yTemp[i][j], yTemp[i + 1][j]);
			else
				bottom = std::min(yTemp[i][j], yTemp[i + 1][j]);
			double left;
			if(xTemp[i+1][j] < xTemp[i][j])
				left = std::max(xTemp[i][j], xTemp[i][j + 1]);
			else
				left = std::min(xTemp[i][j], xTemp[i][j + 1]);

			bool tryAddingMore = averaged[i][j] == 0;
			bool canAddHeight = true;
			bool canAddWidth = true;
			while (tryAddingMore)
			{
				canAddHeight &= j + height + 1 < xTemp[i].size();
				for (size_t k = 0; k < width+1; ++k)
				{
					if (!canAddHeight)
						break;
					canAddHeight &= std::abs(xTemp[i + k][j + height + 1] - left) < xResolution;
					canAddHeight &= std::abs(yTemp[i + k][j + height + 1] - bottom) < yResolution;
				}
				if (canAddHeight)
					++height;
				canAddWidth &= i + width + 1 < xTemp.size();
				for (size_t k = 0; k < height+1; ++k)
				{
					if (!canAddWidth)
						break;
					canAddWidth &= std::abs(xTemp[i + width + 1][j + k] - left) < xResolution;
					canAddWidth &= std::abs(yTemp[i + width + 1][j + k] - bottom) < yResolution;
				}
				if (canAddWidth)
					++width;

				tryAddingMore = canAddHeight || canAddWidth;
			}
			minWidth = std::min(width, minWidth);

			if (width > 1 || height > 1)
			{
				//reshape the bottom left quadrilateral
				double top = yTemp[i + width][j + height];
				double right = xTemp[i + width][j + height];
				for(size_t k=0; k<height+1; ++k)
					xTemp[i][j + k] = left;
				//for (size_t k = 0; k < width + 1; ++k)
				//	yTemp[i + k ][j] = bottom;
				for (size_t k = 0; k < width; ++k)
				{
					for (size_t l = 0; l < height; ++l)
					{
						xTemp[i + k + 1][j + l + 1] = right;
						//yTemp[i + k + 1][j + l + 1] = top;
					}
				}

				//make the value of the bottom left quadrilateral, the average for the rest
				double totalArea = 0.0;
				double sum = 0.0;
				for (size_t k = 0; k < width; ++k)
				{
					for (size_t l = 0; l < height; ++l)
					{
						totalArea += area[i + k][j + l];
						sum += area[i + k][j + l] * zTemp[i + k][j + l];
					}
				}
				zTemp[i][j] = sum / totalArea;
				area[i][j] = std::abs(right - left) * std::abs(top - bottom);
				averaged[i][j] = 1;

				//set all the other values to have 0 area, nan value and mark as averaged
				for (size_t k = 0; k < width; ++k)
				{
					for (size_t l = 0; l < height; ++l)
					{
						if (k > 0 || l > 0)
						{
							area[i+k][j+l] = 0.0;
							zTemp[i+k][j+l] = std::numeric_limits<double>::quiet_NaN();
							averaged[i+k][j+l] = 1;
						}
					}
				}
			}
		}
	}
	
	//if we have any rows or columns with areas of all zeros we can remove them
	std::vector<SBOOL> filter(area.size(), 0);
	for (size_t i = 0; i < area.size(); ++i)
	{
		for (size_t j = 0; j < area[i].size(); ++j)
		{
			if (area[i][j] != 0.0)
			{
				filter[i] = 1;
				break;
			}
		}
	}
	if (sci::anyFalse(filter))
	{
		area = sci::subvector(area, filter);
		zTemp = sci::subvector(zTemp, filter);
		filter.push_back(1);
		xTemp = sci::subvector(xTemp, filter);
		yTemp = sci::subvector(yTemp, filter);
	}

	filter.assign(area[0].size(), 0);
	for (size_t i = 0; i < area.size(); ++i)
	{
		for (size_t j = 0; j < area[i].size(); ++j)
		{
			if (area[i][j] != 0.0)
			{
				filter[j] = 1;
			}
		}
	}
	if (sci::anyFalse(filter))
	{
		for(size_t i=0; i<area.size(); ++i)
			area[i] = sci::subvector(area[i], filter);
		for (size_t i = 0; i < zTemp.size(); ++i)
			zTemp[i] = sci::subvector(zTemp[i], filter);
		filter.push_back(1);
		for (size_t i = 0; i < xTemp.size(); ++i)
			xTemp[i] = sci::subvector(xTemp[i], filter);
		for (size_t i = 0; i < yTemp.size(); ++i)
			yTemp[i] = sci::subvector(yTemp[i], filter);
	}

	std::swap(xTemp, xResampled);
	std::swap(yTemp, yResampled);
	std::swap(zTemp, zResampled);
}

void resample(const std::vector<double>& x, const std::vector<std::vector<double>>& y, const std::vector<std::vector<double>>& z, double xResolution, double yResolution,
	std::vector<std::vector<double>>& xResampled, std::vector<std::vector<double>>& yResampled, std::vector<std::vector<double>>& zResampled)
{
	std::vector<std::vector<double>> xTemp(x.size());
	for (size_t i = 0; i < xTemp.size(); ++i)
		xTemp[i].resize(y[i].size(), x[i]);

	resample(xTemp, y, z, xResolution, yResolution, xResampled, yResampled, zResampled);
	/*
	std::vector<std::vector<double>> area = sci::makevector<double>(0.0, z.size(), z[0].size());
	std::vector<double> xTemp = x;
	std::vector<std::vector<double>> yTemp = y;
	std::vector<std::vector<double>> zTemp = z;

	for (size_t i = 0; i < area.size(); ++i)
		for (size_t j = 0; j < area[i].size(); ++j)
			area[i][j] = areaQuadrilateral(i, j, x, y);

	//find quadrillaterals smaller than the resolution and remove them by making all 4 coordinates identical
	for (size_t i = 0; i < xTemp.size() - 2; ++i)
	{
		for (size_t j = 0; j < yTemp[i].size() - 2; ++j)
		{

			if (std::abs(xTemp[i] - xTemp[i + 1]) < xResolution &&
				std::abs(yTemp[i][j] - yTemp[i][j + 1]) < yResolution && std::abs(yTemp[i][j] - yTemp[i + 1][j + 1]) < yResolution && std::abs(yTemp[i][j] - yTemp[i + 1][j]) < yResolution)
			{
				//assign all 4 coordinates identical
				xTemp[i + 1] = xTemp[i];
				yTemp[i + 1][j] = yTemp[i + 1][j + 1] = yTemp[i][j + 1] = yTemp[i][j];
				//note the original area of the three quadrilaterals above and to the right
				double oldArea1 = area[i][j + 1];
				double oldArea2 = area[i + 1][j];
				double oldArea3 = area[i + 1][j + 1];
				//calculate the new areas of the quadrilaterals above and to the right
				area[i][j + 1] = areaQuadrilateral(i, j + 1, xTemp, yTemp);
				area[i + 1][j] = areaQuadrilateral(i + 1, j, xTemp, yTemp);
				area[i + 1][j + 1] = areaQuadrilateral(i + 1, j + 1, xTemp, yTemp);
				//assign the z from the removed quadrilateral to the 3 quadrilaterals above and to the right 
				zTemp[i][j + 1] = (zTemp[i][j + 1] * oldArea1 + z[i][j] * (area[i][j + 1] - oldArea1)) / area[i][j + 1];
				zTemp[i + 1][j] = (zTemp[i + 1][j] * oldArea2 + z[i][j] * (area[i + 1][j] - oldArea2)) / area[i + 1][j];
				zTemp[i + 1][j + 1] = (zTemp[i + 1][j + 1] * oldArea3 + z[i][j] * (area[i + 1][j + 1] - oldArea3)) / area[i + 1][j + 1];
				//set the area of the removed box to zero and it's z to NaN.
				area[i][j] = 0.0;
				zTemp[i][j] = std::numeric_limits<double>::quiet_NaN();
			}
		}
	}

	//if we have any rows or columns with areas of all zeros we can remove them
	std::vector<SBOOL> filter(area.size(), 0);
	for (size_t i = 0; i < area.size(); ++i)
	{
		for (size_t j = 0; j < area[i].size(); ++j)
		{
			if (area[i][j] != 0.0)
			{
				filter[i] = 1;
				break;
			}
		}
	}
	if (sci::anyFalse(filter))
	{
		area = sci::subvector(area, filter);
		zTemp = sci::subvector(zTemp, filter);
		filter.push_back(1);
		yTemp = sci::subvector(yTemp, filter);
	}

	filter.assign(area[0].size(), 0);
	for (size_t i = 0; i < area.size(); ++i)
	{
		for (size_t j = 0; j < area[i].size(); ++j)
		{
			if (area[i][j] != 0.0)
			{
				filter[j] = 1;
			}
		}
	}
	if (sci::anyFalse(filter))
	{
		for (size_t i = 0; i < area.size(); ++i)
			area[i] = sci::subvector(area[i], filter);
		for (size_t i = 0; i < zTemp.size(); ++i)
			zTemp[i] = sci::subvector(zTemp[i], filter);
		filter.push_back(1);
		xTemp = sci::subvector(xTemp, filter);
		for (size_t i = 0; i < yTemp.size(); ++i)
			yTemp[i] = sci::subvector(yTemp[i], filter);
	}

	std::swap(xTemp, xResampled);
	std::swap(yTemp, yResampled);
	std::swap(zTemp, zResampled);
	*/
}

void resample(const std::vector<std::vector<double>>& x, const std::vector<double>& y, const std::vector<std::vector<double>>& z, double xResolution, double yResolution,
	std::vector<std::vector<double>>& xResampled, std::vector<std::vector<double>>& yResampled, std::vector<std::vector<double>>& zResampled)
{
	std::vector<std::vector<double>> yTemp(x.size());
	for (size_t i = 0; i < yTemp.size(); ++i)
		yTemp[i] = y;

	resample(x, yTemp, z, xResolution, yResolution, xResampled, yResampled, zResampled);

	/*std::vector<std::vector<double>> area = sci::makevector<double>(0.0, z.size(), z[0].size());
	std::vector<std::vector<double>> xTemp = x;
	std::vector<double> yTemp = y;
	std::vector<std::vector<double>> zTemp = z;

	for (size_t i = 0; i < area.size(); ++i)
		for (size_t j = 0; j < area[i].size(); ++j)
			area[i][j] = areaQuadrilateral(i, j, x, y);

	//find quadrillaterals smaller than the resolution and remove them by making all 4 coordinates identical
	for (size_t i = 0; i < xTemp.size() - 2; ++i)
	{
		for (size_t j = 0; j < xTemp[i].size() - 2; ++j)
		{

			if (std::abs(xTemp[i][j] - xTemp[i][j + 1]) < xResolution && std::abs(xTemp[i][j] - xTemp[i + 1][j + 1]) < xResolution && std::abs(xTemp[i][j] - xTemp[i + 1][j]) < xResolution &&
				std::abs(yTemp[j] - yTemp[j + 1]) < yResolution)
			{
				//assign all 4 coordinates identical
				xTemp[i + 1][j] = xTemp[i + 1][j + 1] = xTemp[i][j + 1] = xTemp[i][j];
				yTemp[j + 1] = yTemp[j];
				//note the original area of the three quadrilaterals above and to the right
				double oldArea1 = area[i][j + 1];
				double oldArea2 = area[i + 1][j];
				double oldArea3 = area[i + 1][j + 1];
				//calculate the new areas of the quadrilaterals above and to the right
				area[i][j + 1] = areaQuadrilateral(i, j + 1, xTemp, yTemp);
				area[i + 1][j] = areaQuadrilateral(i + 1, j, xTemp, yTemp);
				area[i + 1][j + 1] = areaQuadrilateral(i + 1, j + 1, xTemp, yTemp);
				//assign the z from the removed quadrilateral to the 3 quadrilaterals above and to the right 
				zTemp[i][j + 1] = (zTemp[i][j + 1] * oldArea1 + z[i][j] * (area[i][j + 1] - oldArea1)) / area[i][j + 1];
				zTemp[i + 1][j] = (zTemp[i + 1][j] * oldArea2 + z[i][j] * (area[i + 1][j] - oldArea2)) / area[i + 1][j];
				zTemp[i + 1][j + 1] = (zTemp[i + 1][j + 1] * oldArea3 + z[i][j] * (area[i + 1][j + 1] - oldArea3)) / area[i + 1][j + 1];
				//set the area of the removed box to zero and it's z to NaN.
				area[i][j] = 0.0;
				zTemp[i][j] = std::numeric_limits<double>::quiet_NaN();
			}
		}
	}

	//if we have any rows or columns with areas of all zeros we can remove them
	std::vector<SBOOL> filter(area.size(), 0);
	for (size_t i = 0; i < area.size(); ++i)
	{
		for (size_t j = 0; j < area[i].size(); ++j)
		{
			if (area[i][j] != 0.0)
			{
				filter[i] = 1;
				break;
			}
		}
	}
	if (sci::anyFalse(filter))
	{
		area = sci::subvector(area, filter);
		zTemp = sci::subvector(zTemp, filter);
		filter.push_back(1);
		xTemp = sci::subvector(xTemp, filter);
		yTemp = sci::subvector(yTemp, filter);
	}

	filter.assign(area[0].size(), 0);
	for (size_t i = 0; i < area.size(); ++i)
	{
		for (size_t j = 0; j < area[i].size(); ++j)
		{
			if (area[i][j] != 0.0)
			{
				filter[j] = 1;
			}
		}
	}
	if (sci::anyFalse(filter))
	{
		for (size_t i = 0; i < area.size(); ++i)
			area[i] = sci::subvector(area[i], filter);
		for (size_t i = 0; i < zTemp.size(); ++i)
			zTemp[i] = sci::subvector(zTemp[i], filter);
		filter.push_back(1);
		for (size_t i = 0; i < xTemp.size(); ++i)
			xTemp[i] = sci::subvector(xTemp[i], filter);
	}

	std::swap(xTemp, xResampled);
	std::swap(yTemp, yResampled);
	std::swap(zTemp, zResampled);*/
}