#include"Ceilometer.h"
#include"AmfNc.h"

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

	//create the data arrays
	std::vector<sci::UtcTime> times(profiles.size());
	std::vector<std::vector<steradianPerKilometre>> backscatter(profiles.size());
	std::vector<metre> cloudBase1(profiles.size());
	std::vector<metre> cloudBase2(profiles.size());
	std::vector<metre> cloudBase3(profiles.size());
	std::vector<metre> cloudBase4(profiles.size());
	std::vector<int32_t> gates;
	metre resolution;
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
}