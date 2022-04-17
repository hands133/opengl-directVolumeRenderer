#pragma once

#include "tinyVR/core/vrBase.h"
#include "tinyVR/core/vrLog.h"

#ifdef TINYVR_ENABLE_ASSERTS

	#define TINYVR_ASSERT(x, ...) { if(!(x)) { TINYVR_ERROR("Assertion Failed : {0}", __VA_ARGS__); __debugbreak(); } }
	#define TINYVR_CORE_ASSERT(x, ...) { if(!(x)) { TINYVR_CORE_ERROR("Assertion Failed : {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define TINYVR_ASSERT(x, ...) { (x); }
	#define TINYVR_CORE_ASSERT(x, ...) { (x); }
#endif