#pragma once

#include<vector>

struct HplHeader;
class HplProfile;
class wxWindow;
class ProgressReporter;

void plotProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, double maxRange, ProgressReporter &progressReporter, wxWindow *parent);


