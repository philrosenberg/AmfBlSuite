#pragma once

#include<vector>

struct HplHeader;
class HplProfile;
class wxWindow;
class ProgressReporter;

void plotProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double maxRange, ProgressReporter &progressReporter, wxWindow *parent);
void plotProcessedWindProfile(const std::vector<double> &height, const std::vector<double> &degrees, const std::vector<double> &speed, const std::string &filename, double maxRange, ProgressReporter& progressReporter, wxWindow *parent);

