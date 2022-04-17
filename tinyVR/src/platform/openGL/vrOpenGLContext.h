#pragma once

#include "tinyVR/renderer/vrGraphicsContext.h"

struct GLFWwindow;

namespace tinyvr {

	class vrOpenGLContext : public vrGraphicsContext
	{
	public:
		vrOpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle;
	};
}

