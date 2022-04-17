#include "layers/vrRCLayer.h"

#include "tinyVR/core/vrEntrypoint.h"

class tinyVR_App : public tinyvr::vrApplication
{
public:
	tinyVR_App()
	{
		//std::string datPath = "resources/volume data/Bucky_32x32x32_uint8.dat";
		//std::string datPath = "resources/volume data/fuel_64x64x64_uint8.dat";
		std::string datPath = "resources/volume data/neghip_64x64x64_uint8.dat";
		//std::string datPath = "resources/volume data/silicium_98_34_34_uint8.dat";
		//std::string datPath = "resources/volume data/tooth_103x94x161_uint8.dat";
		PushLayer(new rc::vrRCLayer(datPath));
	}

	~tinyVR_App()
	{

	}

private:

};

tinyvr::vrApplication* tinyvr::CreateApplication()
{
	return new tinyVR_App();
}