#pragma once

#include <glad/glad.h>
#include <glfw3.h>

#include <iostream>

void CheckGLError(const char* file, int line);
#define  GL_CALL(x) do{x;CheckGLError(__FILE__,__LINE__);}while(0)


//.cpp
inline void CheckGLError(const char* file, int line)
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		switch (error)
		{
		case GL_INVALID_ENUM:
			printf("GL Error: GL_INVALID_ENUM %s : %d \n", file, line);
			break;
		case GL_INVALID_VALUE:
			printf("GL Error: GL_INVALID_VALUE %s : %d \n", file, line);
			break;
		case GL_INVALID_OPERATION:
			printf("GL Error: GL_INVALID_OPERATION %s : %d \n", file, line);
			break;
		//case GL_STACK_OVERFLOW:
		//	printf("GL Error: GL_STACK_OVERFLOW %s : %d \n", file, line);
		//	break;
		//case GL_STACK_UNDERFLOW:
		//	printf("GL Error: GL_STACK_UNDERFLOW %s : %d \n", file, line);
		//	break;
		case GL_OUT_OF_MEMORY:
			printf("GL Error: GL_OUT_OF_MEMORY %s : %d \n", file, line);
			break;
		default:
			printf("GL Error: 0x%x %s : %d \n", error, file, line);
			break;
		}
	}
}