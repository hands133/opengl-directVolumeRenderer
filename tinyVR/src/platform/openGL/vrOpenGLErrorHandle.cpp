#include "vrpch.h"
#include "vrOpenGLErrorHandle.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace tinyvr {

	void vrOpenGLErrorHandle::CheckGLError(const char* file, int line)
	{
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			switch (error)
			{
			case GL_INVALID_ENUM:
				TINYVR_ERROR("GL Error: GL_INVALID_ENUM {} : {}", file, line);
				TINYVR_ASSERT(false, "GL_INVALID_ENUM");
				break;
			case GL_INVALID_VALUE:
				TINYVR_ERROR("GL Error : GL_INVALID_VALUE {} : {}", file, line);
				TINYVR_ASSERT(false, "GL_INVLAID_VALUE");
				break;
			case GL_INVALID_OPERATION:
				TINYVR_ERROR("GL Error : GL_INVALID_OPERATION {} : {}", file, line);
				TINYVR_ASSERT(false, "GL_INVALID_OPERATION");
				break;
			case GL_STACK_OVERFLOW:
				TINYVR_ERROR("GL Error : GL_STACK_OVERFLOW {} : {}", file, line);
				TINYVR_ASSERT(false, "GL_STACK_OVERFLOW");
				break;
			case GL_STACK_UNDERFLOW:
				TINYVR_ERROR("GL Error : GL_STACK_UNDERFLOW {} : {}", file, line);
				TINYVR_ASSERT(false, "GL_STACK_UNDERFLOW");
				break;
			case GL_OUT_OF_MEMORY:
				TINYVR_ERROR("GL Error : GL_OUT_OF_MEMORY {} : {}", file, line);
				TINYVR_ASSERT(false, "GL_OUT_OF_MEMORY");
				break;
			default:
				TINYVR_ERROR("GL Error : NORMAL ERROR [{}] {} : {}", error, file, line);
				TINYVR_ASSERT(false, "GL_ERROR");
				break;
			}
		}
		else
			TINYVR_INFO("GL Error : No error {} : {}", file, line);
	}

}