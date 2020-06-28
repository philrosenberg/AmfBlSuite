#pragma once
#include<vector>

void resample(const std::vector<std::vector<double>>& x, const std::vector<std::vector<double>>& y, const std::vector<std::vector<double>>& z, double xResolution, double yResolution,
	std::vector<std::vector<double>>& xResampled, std::vector<std::vector<double>>& yResampled, std::vector<std::vector<double>>& zResampled);
void resample(const std::vector<double>& x, const std::vector<std::vector<double>>& y, const std::vector<std::vector<double>>& z, double xResolution, double yResolution,
	std::vector<std::vector<double>>& xResampled, std::vector<std::vector<double>>& yResampled, std::vector<std::vector<double>>& zResampled);
void resample(const std::vector<std::vector<double>>& x, const std::vector<double>& y, const std::vector<std::vector<double>>& z, double xResolution, double yResolution,
	std::vector<std::vector<double>>& xResampled, std::vector<std::vector<double>>& yResampled, std::vector<std::vector<double>>& zResampled);
