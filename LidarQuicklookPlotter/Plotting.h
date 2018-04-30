#pragma once

#include<vector>

struct HplHeader;
class HplProfile;
class wxWindow;

void plotProfiles(const HplHeader &header, const std::vector<HplProfile> &profiles, std::string filename, wxWindow *parent);


