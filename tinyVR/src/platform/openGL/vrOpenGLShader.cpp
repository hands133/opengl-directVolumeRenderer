#include "vrpch.h"
#include "vrOpenGLShader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace tinyvr {

	vrOpenGLShader::vrOpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc)
		: m_RendererID(0)
	{
		// Create an empty vertex shader handle
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

		// Send the vertex shader source code to GL
		// Note that std::string's .c_str is NULL character terminated
		const GLchar* source = static_cast<const GLchar*>(vertexSrc.c_str());
		glShaderSource(vertexShader, 1, &source, 0);

		// Compile the vertex shader
		glCompileShader(vertexShader);

		GLint isCompiled = 0;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
		if (GL_FALSE == isCompiled)
		{
			GLint maxLength = 0;
			glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(vertexShader, maxLength, &maxLength, infoLog.data());

			// We don't need the shader anymore
			glDeleteShader(vertexShader);

			TINYVR_ERROR("{0}", infoLog.data());
			TINYVR_ASSERT(false, "Vertex shader compilation failure!");
			
			return;
		}

		// Create an empty fragment shader handle
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		// Send the fragment shader source code to GL
		// Note that std::string's .c_str is NULL character terminated
		source = static_cast<const GLchar*>(fragmentSrc.c_str());
		glShaderSource(fragmentShader, 1, &source, 0);

		// Compile the fragment shader
		glCompileShader(fragmentShader);

		isCompiled = 0;
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
		if (GL_FALSE == isCompiled)
		{
			GLint maxLength = 0;
			glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, infoLog.data());

			// We don't need the shader anymore
			glDeleteShader(fragmentShader);

			TINYVR_ERROR("{0}", infoLog.data());
			TINYVR_ASSERT(false, "Fragment shader compilation failure!");
			
			return;
		}

		// Get a program object
		m_RendererID = glCreateProgram();
		GLuint program = m_RendererID;

		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);

		// Link our program
		glLinkProgram(program);

		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (GL_FALSE == isLinked)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());

			glDeleteProgram(program);

			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			TINYVR_ERROR("{0}", infoLog.data());
			TINYVR_ASSERT(false, "vrOpenGLShader link failure!");
			
			return;
		}

		glDetachShader(program, vertexShader);
		glDetachShader(program, fragmentShader);

		// check maximum texture units
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_MaxTextureUnit);
	}

	vrOpenGLShader::~vrOpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	void vrOpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void vrOpenGLShader::Unbind() const
	{
		glUseProgram(0);
	}

	void vrOpenGLShader::SetBool(const std::string& name, bool value)
	{
		glUniform1i(glGetUniformLocation(m_RendererID, name.c_str()), (int)value);
	}

	void vrOpenGLShader::SetInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

	void vrOpenGLShader::SetInt2(const std::string& name, const glm::ivec2& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2i(location, values.x, values.y);
	}

	void vrOpenGLShader::SetInt3(const std::string& name, const glm::ivec3& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3i(location, values.x, values.y, values.z);
	}

	void vrOpenGLShader::SetInt4(const std::string& name, const glm::ivec4& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4i(location, values.x, values.y, values.z, values.w);
	}

	void vrOpenGLShader::SetInts(const std::string& name, int* valuePtr, size_t items)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1iv(location, items, valuePtr);
	}

	void vrOpenGLShader::SetFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1f(location, value);
	}

	void vrOpenGLShader::SetFloat2(const std::string& name, const glm::vec2& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform2f(location, values.x, values.y);
	}

	void vrOpenGLShader::SetFloat3(const std::string& name, const glm::vec3& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, values.x, values.y, values.z);
	}

	void vrOpenGLShader::SetFloat4(const std::string& name, const glm::vec4& values)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform4f(location, values.x, values.y, values.z, values.w);
	}

	void vrOpenGLShader::SetFloats(const std::string& name, float* valuePtr, size_t items)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1fv(location, items, valuePtr);
	}

	void vrOpenGLShader::SetMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void vrOpenGLShader::SetMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void vrOpenGLShader::SetTexture(const std::string& name, const vrRef<vrTexture>& tex)
	{
		// NOTE : what is this?
		// When using shaders, you set your sampler uniforms to a texture image unit
		//		(glUniform1i(samplerLoc, i), where i is the image unit).
		//		That represents the number you used with glActiveTexture.
		//		The sampler will pick the target based on the sampler type.
		//		So a sampler2D will pick from the GL_TEXTURE_2D target.
		//		This is one reason why samplers have different types.
		glActiveTexture(GL_TEXTURE0 + tex->GetTextureID());
		tex->Bind();
		SetInt(name, tex->GetTextureID());
	}
}