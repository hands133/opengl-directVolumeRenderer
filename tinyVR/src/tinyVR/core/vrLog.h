#pragma once

#include <memory>

#include "tinyVR/core/vrBase.h"
#include "spdlog/spdlog.h"

namespace tinyvr {

	class TINYVR_API vrLog
	{
	public:
		static void Init();

		static vrRef<spdlog::logger>& GetClientLogger() { return s_ClientLogger; };
		static vrRef<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; };
			   
	private:   
		static vrRef<spdlog::logger> s_ClientLogger, s_CoreLogger;
	};
}

// Core log macros
#define TINYVR_CORE_TRACE(...)	::tinyvr::vrLog::GetCoreLogger()->trace(__VA_ARGS__)
#define TINYVR_CORE_INFO(...)	::tinyvr::vrLog::GetCoreLogger()->info(__VA_ARGS__)
#define TINYVR_CORE_WARN(...)	::tinyvr::vrLog::GetCoreLogger()->warn(__VA_ARGS__)
#define TINYVR_CORE_ERROR(...)	::tinyvr::vrLog::GetCoreLogger()->error(__VA_ARGS__)
#define TINYVR_CORE_FATAL(...)	::tinyvr::vrLog::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define TINYVR_TRACE(...)		::tinyvr::vrLog::GetClientLogger()->trace(__VA_ARGS__)
#define TINYVR_INFO(...)		::tinyvr::vrLog::GetClientLogger()->info(__VA_ARGS__)
#define TINYVR_WARN(...)		::tinyvr::vrLog::GetClientLogger()->warn(__VA_ARGS__)
#define TINYVR_ERROR(...)		::tinyvr::vrLog::GetClientLogger()->error(__VA_ARGS__)
#define TINYVR_FATAL(...)		::tinyvr::vrLog::GetClientLogger()->critical(__VA_ARGS__)