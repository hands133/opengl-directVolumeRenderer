#pragma once

#include <functional>

#define GL_CALL(x) do { x; tinyvr::vrOpenGLErrorHandle::CheckGLError(__FILE__, __LINE__); } while(0)
#define GL_CHECK() { tinyvr::vrOpenGLErrorHandle::CheckGLError(__FILE__, __LINE__); }

namespace tinyvr {

	class vrOpenGLErrorHandle
	{
	public:
		static void CheckGLError(const char* file, int line);
	};

}