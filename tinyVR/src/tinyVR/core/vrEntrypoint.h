#pragma once

#ifdef TINYVR_PLATFORM_WINDOWS

extern tinyvr::vrApplication* tinyvr::CreateApplication();

int main(int argc, char* argv[])
{
	tinyvr::vrLog::Init();

	auto app = tinyvr::CreateApplication();
	app->Run();
	delete app;

	//spdlog::shutdown();

	return 0;
}

#else
	#error No Main function defined!
#endif