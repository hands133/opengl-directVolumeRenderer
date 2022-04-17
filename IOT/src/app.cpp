#include "layers/vrIOTLayer.h"

#include "tinyVR/core/vrEntrypoint.h"

class tinyVR_App : public tinyvr::vrApplication
{
public:
	tinyVR_App()
	{
		std::string path = "resources/models/obj/nanosuit.obj";
		//std::string path = "resources/models/obj/su_27.obj";
		//std::string path = "resources/models/obj/tank.obj";
		//std::string path = "resources/models/trent1000.obj";
		PushLayer(new iot::IOTLayer(path));
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