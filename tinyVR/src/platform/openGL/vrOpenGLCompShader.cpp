#include "vrpch.h"
#include "vrOpenGLCompShader.h"

#include "platform/openGL/vrOpenGLTexture.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>

namespace tinyvr
{
	vrOpenGLCompShader::vrOpenGLCompShader(const std::string& compSrc)
		: m_RendererID(0)
	{
		GLuint compShader = glCreateShader(GL_COMPUTE_SHADER);

		const GLchar* source = static_cast<const GLchar*>(compSrc.c_str());
		glShaderSource(compShader, 1, &source, 0);

		glCompileShader(compShader);

		GLint isCompiled = 0;
		glGetShaderiv(compShader, GL_COMPILE_STATUS, &isCompiled);
		if (GL_FALSE == isCompiled)
		{
			GLint maxLength = 0;
			glGetShaderiv(compShader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(compShader, maxLength, &maxLength, infoLog.data());

			// We don't need the shader anymore
			glDeleteShader(compShader);

			TINYVR_ERROR("{0}", infoLog.data());
			TINYVR_ASSERT(false, "vrOpenGLCompShader compilation failure!");

			return;
		}

		m_RendererID = glCreateProgram();
		GLuint program = m_RendererID;

		glAttachShader(program, compShader);
		glLinkProgram(program);

		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (GL_FALSE == isLinked)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(m_RendererID, maxLength, &maxLength, infoLog.data());

			glDeleteProgram(program);

			glDeleteShader(compShader);

			TINYVR_ERROR("{0}", infoLog.data());
			TINYVR_ASSERT(false, "vrOpenGLCompShader link failure!");

			return;
		}

		glDetachShader(program, compShader);
		glDeleteShader(compShader);
	}

	vrOpenGLCompShader::~vrOpenGLCompShader()
	{
		glDeleteProgram(m_RendererID);
	}

	void vrOpenGLCompShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void vrOpenGLCompShader::Unbind() const
	{
		glUseProgram(0);
	}

	void vrOpenGLCompShader::SetBool(const std::string& name, bool value)
	{
		glUniform1i(glGetUniformLocation(m_RendererID, name.c_str()), (int)value);
	}

	void vrOpenGLCompShader::SetInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

	void vrOpenGLCompShader::SetInt2(const std::string& name, const glm::ivec2& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2i(location, values.x, values.y);
	}

	void vrOpenGLCompShader::SetInt3(const std::string& name, const glm::ivec3& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3i(location, values.x, values.y, values.z);
	}

	void vrOpenGLCompShader::SetInt4(const std::string& name, const glm::ivec4& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4i(location, values.x, values.y, values.z, values.w);
	}

	void vrOpenGLCompShader::SetInts(const std::string& name, int* valuePtr, size_t items)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1iv(location, items, valuePtr);
	}

	void vrOpenGLCompShader::SetFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1f(location, value);
	}

	void vrOpenGLCompShader::SetFloat2(const std::string& name, const glm::vec2& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2f(location, values.x, values.y);
	}

	void vrOpenGLCompShader::SetFloat3(const std::string& name, const glm::vec3& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, values.x, values.y, values.z);
	}

	void vrOpenGLCompShader::SetFloat4(const std::string& name, const glm::vec4& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, values.x, values.y, values.z, values.w);
	}

	void vrOpenGLCompShader::SetFloats(const std::string& name, float* valuePtr, size_t items)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1fv(location, items, valuePtr);
	}

	void vrOpenGLCompShader::SetMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void vrOpenGLCompShader::SetMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void vrOpenGLCompShader::Compute(const glm::uvec3& numWorkgroups) const
	{
		glDispatchCompute(numWorkgroups.x, numWorkgroups.y, numWorkgroups.z);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
}