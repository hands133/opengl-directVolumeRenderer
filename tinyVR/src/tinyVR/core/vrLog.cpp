#include "vrpch.h"
#include "vrLog.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"

namespace tinyvr {

	vrRef<spdlog::logger> vrLog::s_CoreLogger;
	vrRef<spdlog::logger> vrLog::s_ClientLogger;

	void vrLog::Init()
	{
		//https://hub.fastgit.org/gabime/spdlog/wiki/3.-Custom-formatting
		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = spdlog::stdout_color_mt("TINYVR");
		s_CoreLogger->set_level(spdlog::level::level_enum::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::level_enum::trace);
	}
}