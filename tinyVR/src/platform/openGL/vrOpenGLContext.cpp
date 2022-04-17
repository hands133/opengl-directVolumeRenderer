#include "vrpch.h"
#include "vrOpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace tinyvr {

	vrOpenGLContext::vrOpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		TINYVR_ASSERT(windowHandle, "Window handle is null!");
	}

	void vrOpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

		TINYVR_ASSERT(status, "Failed to initialize Glad!");

		TINYVR_INFO("OpenGL Info:");
		TINYVR_INFO("\tVendor : {0}", glGetString(GL_VENDOR));
		TINYVR_INFO("\tRenderer : {0}", glGetString(GL_RENDERER));
		TINYVR_INFO("\tVersion : {0}", glGetString(GL_VERSION));

		TINYVR_INFO("OpenGL Data Limitations");
		int i, j, k;

		// Maximum texture size
		i = j = k = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &i);
		TINYVR_INFO("\tMaximum texture size : {0}", i);

		// Maximum compute size
		i = j = k = 0;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &i);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &j);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &k);
		TINYVR_INFO("\tMaximum compute size : [{0}, {1}, {2}]", i, j, k);

		// Maximum local work group size
		i = j = k = 0;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &i);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &j);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &k);
		GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS;
		TINYVR_INFO("\tMaximum local work group size : [{0}, {1}, {2}]", i, j, k);

		GLint v;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &v);
		TINYVR_INFO("\tMaximum color attachments of frame buffer : {0}", v);
	}

	void vrOpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}


}