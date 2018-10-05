#include"Ceilometer.h"
#include"AmfNc.h"



char CampbellCeilometerProfile::getProfileFlag() const
{
	if (m_profile.getMessageStatus() == cms_rawDataMissingOrSuspect)
		return 5;
	if (m_profile.getMessageStatus() == cms_someObscurationTransparent)
		return 4;
	if (m_profile.getMessageStatus() == cms_fullObscurationNoCloudBase)
		return 3;
	if (m_profile.getMessageStatus() == cms_noSignificantBackscatter)
		return 2;
	return 1;
}
std::vector<char> CampbellCeilometerProfile::getGateFlags() const
{
	//Depending upon how the ceilometer is set up it can replace data
	//that it believes is below the signal to noise threshold with 0.
	//Also data below 1e-7 sr m-1 is probably noise too so flag it

	std::vector<char> result(getBetas().size(), 1);
	sci::assign(result, getBetas() < steradianPerMetre(1e-7), char(2));
	return result;
}

void writeToNc(const HplHeader &header, const std::vector<CampbellCeilometerProfile> &profiles, sci::string directory,
	const PersonInfo &author, const ProcessingSoftwareInfo &processingSoftwareInfo, const ProjectInfo &projectInfo,
	const PlatformInfo &platformInfo, const sci::string &comment)
{
	InstrumentInfo ceilometerInfo;
	ceilometerInfo.name = sU("ncas-ceilometer-3");
	ceilometerInfo.manufacturer = sU("Campbell");
	ceilometerInfo.model = sU("CS135");
	ceilometerInfo.description = sU("");
	ceilometerInfo.operatingSoftware = sU("");
	ceilometerInfo.operatingSoftwareVersion = sU("");
	ceilometerInfo.serial = sU("");

	CalibrationInfo ceilometerCalibrationInfo;

	DataInfo dataInfo;

	//------create the data arrays-----

	//time array, holds time/date
	std::vector<sci::UtcTime> times(profiles.size());
	//backscatter vs time and height
	std::vector<std::vector<steradianPerKilometre>> backscatter(profiles.size());
	//cloud bases
	std::vector<metre> cloudBase1(profiles.size());
	std::vector<metre> cloudBase2(profiles.size());
	std::vector<metre> cloudBase3(profiles.size());
	std::vector<metre> cloudBase4(profiles.size());
	//the gates - just an index from 0 to n_gates-1
	std::vector<int32_t> gates;
	//flags - one flag for each profile and one flage for each
	//each gate in each profile
	std::vector<char> profileFlag(profiles.size());
	std::vector<std::vector<char>> gateFlags(profiles.size());
	//the vertical resolution of the profiles - we check this is the same for each profile
	metre resolution;
	//Some other housekeeping information
	std::vector<size_t> pulseQuantities(profiles.size());
	std::vector<megahertz> sampleRates(profiles.size());
	std::vector<percent> windowTransmissions(profiles.size());
	std::vector<percent> laserEnergies(profiles.size());
	std::vector<kelvin> laserTemperatures(profiles.size());
	std::vector<radian> tiltAngles(profiles.size());
	std::vector<millivolt> backgrounds(profiles.size());
	for (size_t i = 0; i < profiles.size(); ++i)
	{
		times[i] = profiles[i].getTime();
		backscatter[i] = profiles[i].getBetas();
		cloudBase1[i] = profiles[i].getCloudBase1();
		cloudBase2[i] = profiles[i].getCloudBase2();
		cloudBase3[i] = profiles[i].getCloudBase3();
		cloudBase4[i] = profiles[i].getCloudBase4();
		
		//assign gates for the first profile then check it is identical for the remaining profiles
		if (i == 0)
		{
			for (size_t j = 0; j < profiles[i].getGates().size(); ++j)
				gates[j] = (int32_t)profiles[i].getGates()[j];
			resolution = profiles[i].getResolution();
		}
		else
		{
			if(resolution != profiles[i].getResolution())
				throw("Found a change in the resolution during a day. Cannot process this day.");
			for (size_t j = 0; j < gates.size(); ++i)
				if (gates[j] != profiles[i].getGates()[j])
					throw("Found a change in the gates during a day. Cannot process this day.");
		}

		//assign the flags
		profileFlag[i] = profiles[i].getProfileFlag();
		gateFlags[i] = profiles[i].getGateFlags();

		//assign the housekeeping data
		pulseQuantities[i] = profiles[i].getPulseQuantity();
		sampleRates[i] = profiles[i].getSampleRate();
		windowTransmissions[i] = profiles[i].getWindowTransmission();
		laserEnergies[i] = profiles[i].getLaserPulseEnergy();
		laserTemperatures[i] = profiles[i].getLaserTemperature();
		tiltAngles[i] = profiles[i].getTiltAngle();
		backgrounds[i] = profiles[i].getBackground();
	}


	//according to the data procucts spreadsheet the instrument needs
	//to produce aerosol-backscatter and cloud-base products

	//The time dimension will be created automatically when we create our
	//output file, but we must specify that we need a height dimension too
	sci::NcDimension gateDimension(sU("gate"), gates.size());
	std::vector<sci::NcDimension*> nonTimeDimensions;
	nonTimeDimensions.push_back(&gateDimension);
	//create the file and dimensions. The time variable is added automatically
	OutputAmfNcFile ceilometerFile(directory, ceilometerInfo, author, processingSoftwareInfo, ceilometerCalibrationInfo, dataInfo,
		projectInfo, platformInfo, comment, times, nonTimeDimensions);

	//add the data variables

	//gates/heights
	std::vector<unitless> gatesPhysical;
	sci::convert(gatesPhysical, gates);
	ceilometerFile.write(AmfNcVariable<int32_t>(sU("gate-number"), ceilometerFile, gateDimension, sU(""), sU("1")), gates);
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-lower-height"), ceilometerFile, gateDimension, sU("")), AmfNcVariable<metre>::convertValues(gatesPhysical*resolution));
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-upper-height"), ceilometerFile, gateDimension, sU("")), AmfNcVariable<metre>::convertValues((gatesPhysical + unitless(1))*resolution));
	ceilometerFile.write(AmfNcVariable<metre>(sU("gate-mid-height"), ceilometerFile, gateDimension, sU("")), AmfNcVariable<metre>::convertValues((gatesPhysical + unitless(0.5))*resolution));
	
	//backscatter
	std::vector<sci::NcDimension*> backscatterDimensions{ &ceilometerFile.getTimeDimension(), &gateDimension };
	ceilometerFile.write(AmfNcVariable<steradianPerMetre>(sU("aerosol-backscatter"), ceilometerFile, backscatterDimensions, sU("aerosol-backscatter")), AmfNcVariable<steradianPerMetre>::convertValues(backscatter));

	//cloud bases
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-1"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), AmfNcVariable<metre>::convertValues(cloudBase1));
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-2"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), AmfNcVariable<metre>::convertValues(cloudBase2));
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-3"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), AmfNcVariable<metre>::convertValues(cloudBase3));
	ceilometerFile.write(AmfNcVariable<metre>(sU("cloud-base-4"), ceilometerFile, ceilometerFile.getTimeDimension(), sU("")), AmfNcVariable<metre>::convertValues(cloudBase4));

	//flags
	ceilometerFile.write(sci::NcVariable<char>(sU("profile-flag"), ceilometerFile, ceilometerFile.getTimeDimension()),profileFlag);
	ceilometerFile.write(sci::NcVariable<char>(sU("gate-flag"), ceilometerFile, backscatterDimensions),gateFlags);

	//housekeeping
	ceilometerFile.write(sci::NcVariable<size_t>(sU("pulse-quantity"), ceilometerFile, ceilometerFile.getTimeDimension()), pulseQuantities);
	ceilometerFile.write(sci::NcVariable<megahertz>(sU("sample-rate"), ceilometerFile, ceilometerFile.getTimeDimension()), sampleRates);
	ceilometerFile.write(sci::NcVariable<percent>(sU("window-transmission"), ceilometerFile, ceilometerFile.getTimeDimension()), windowTransmissions);
	ceilometerFile.write(sci::NcVariable<percent>(sU("laser-energy"), ceilometerFile, ceilometerFile.getTimeDimension()), laserEnergies);
	ceilometerFile.write(sci::NcVariable<kelvin>(sU("laser-temperature"), ceilometerFile, ceilometerFile.getTimeDimension()), laserTemperatures);
	ceilometerFile.write(sci::NcVariable<radian>(sU("tilt-angle"), ceilometerFile, ceilometerFile.getTimeDimension()), tiltAngles);
	ceilometerFile.write(sci::NcVariable<millivolt>(sU("background"), ceilometerFile, ceilometerFile.getTimeDimension()), backgrounds);
}