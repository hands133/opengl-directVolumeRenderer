#pragma once

#include <chrono>
#include <string>

#include "tinyVR/core/vrLog.h"

namespace tinyvr {

	class vrTimer
	{
	public:
		vrTimer(const char* name)
			: m_Name(name)
		{
			m_StartTimepoint = std::chrono::steady_clock::now();
		}

		~vrTimer()
		{
			Stop();
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::steady_clock::now();
			auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

			TINYVR_TRACE("{0}\telapse time : {1} ms", m_Name, elapsedTime.count() / 1000.0f);
		}

	private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
	};


	namespace TimerUtils {

		template <size_t N>
		struct ChangeResult
		{
			char Data[N];
		};

		template <size_t N, size_t K>
		constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
		{
			ChangeResult<N> result = {};

			size_t srcIndex = 0;
			size_t dstIndex = 0;
			while (srcIndex < N)
			{
				size_t matchIndex = 0;
				while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
					matchIndex++;
				if (matchIndex == K - 1)
					srcIndex += matchIndex;
				result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
				srcIndex++;
			}
			return result;
		}
	}
}

#define TINYVR_PROFILE TRUE
#if TINYVR_PROFILE
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define TINYVR_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define TINYVR_FUNC_SIG __PRETTY_FUNCTION__
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define TINYVR_FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define TINYVR_FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define TINYVR_FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define TINYVR_FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define TINYVR_FUNC_SIG __func__
#else
#define TINYVR_FUNC_SIG "TINYVR_FUNC_SIG unknown!"
#endif

#define TINYVR_PROFILE_SCOPE_LINE2(name, line) constexpr auto fixedName##line = ::tinyvr::TimerUtils::CleanupOutputString(name, "__cdecl ");\
													::tinyvr::vrTimer timer##line(fixedName##line.Data)
#define TINYVR_PROFILE_SCOPE_LINE(name, line) TINYVR_PROFILE_SCOPE_LINE2(name, line)
#define TINYVR_PROFILE_SCOPE(name) TINYVR_PROFILE_SCOPE_LINE(name, __LINE__)
#define TINYVR_PROFILE_FUNCTION() TINYVR_PROFILE_SCOPE(TINYVR_FUNC_SIG)
#else
#define TINYVR_PROFILE_SCOPE_LINE2(name, line) 

#define TINYVR_PROFILE_SCOPE_LINE(name, line)
#define TINYVR_PROFILE_SCOPE(name)
#define TINYVR_PROFILE_FUNCTION()
#endif