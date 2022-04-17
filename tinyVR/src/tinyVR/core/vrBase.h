#pragma once

#include <memory>

// a bunch of marcro defines here
#ifdef TINYVR_PLATFORM_WINDOWS
	#if TINYVR_DYNAMIC_LINK
		#ifdef TINYVR_BUILD_DLL
			#define TINYVR_API __declspec(dllexport)
		#else
			#define TINYVR_API __declspec(dllimport)
		#endif
	#else
		#define TINYVR_API
	#endif
#else
	#error tinyVR only support windows!
#endif // TINYVR_BUILD_DLL

#ifdef TINYVR_DEBUG
	#define TINYVR_ENABLE_ASSERTS
#endif

#ifdef TINYVR_ENABLE_ASSERTS
	#define TINYVR_ASSERT(x, ...) { if(!(x)) { TINYVR_ERROR("Assertion Failed : {0}", __VA_ARGS__); __debugbreak(); } } 
#else
	#define TINYVR_ASSERT(x, ...)
#endif

// 2^x
#define BIT(x) (1 << x)

// function bind
#define TINYVR_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

namespace tinyvr {

	template <typename T>
	using vrScope = std::unique_ptr<T>;
	template <typename T, typename ... Args>
	constexpr vrScope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	using vrRef = std::shared_ptr<T>;
	template <typename T, typename ... Args>
	constexpr vrRef<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
}

#include "tinyVR/core/vrLog.h"
#include "tinyVR/core/vrAssert.h"
#include "tinyVR/core/vrDefs.h"