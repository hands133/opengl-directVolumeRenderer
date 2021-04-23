#include "shader.h"
#include "texture.h"

Shader::Shader(const GLchar *vertexPath, const GLchar *fragmentPath)
{
	// get vertex shader / fragment shader from pat
	std::string vertCodeSrc;
	std::string fragCodeSrc;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;
	// throw exception
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		// open file
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// read from file to buffer
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file
		vShaderFile.close();
		fShaderFile.close();
		// convert to string
		vertCodeSrc = vShaderStream.str();
		fragCodeSrc = fShaderStream.str();
	}
	catch (const std::ifstream::failure& e)
	{
		std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n" << e.what() << '\n';
		return;
	}

	const char *vShaderCode = vertCodeSrc.c_str();
	const char *fShaderCode = fragCodeSrc.c_str();

	// compile shader
	unsigned int vertex, fragment;
	int success;
	char infoLog[512] = { '\0' };

	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	checkCompileErrors(vertex, SHADER_TYPE::VERTEX);

	// fragment shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	checkCompileErrors(fragment, SHADER_TYPE::FRAGMENT);

	// shader program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	// print error
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		return;
	}

	// delete shader
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::use()
{
	glUseProgram(ID);
}

void Shader::setBool(const std::string &name, bool value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const
{
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec3(const std::string &name, float v0, float v1, float v2) const
{
	glUniform3f(glGetUniformLocation(ID, name.c_str()), v0, v1, v2);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &vec) const
{
	glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setMat4(const std::string &name, const glm::mat4x4 &trans) const
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(trans));
}

void Shader::setTexture(const std::string &name, const Texture& tex) const
{
	this->setInt(name, tex.getID() - 1);
}

void Shader::checkCompileErrors(unsigned int shader, SHADER_TYPE type)
{
	int success;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	std::string shaderTypeStr = "";
	switch (type)
	{
	case SHADER_TYPE::VERTEX:
		shaderTypeStr = "VERTEX";
		break;
	case SHADER_TYPE::FRAGMENT:
		shaderTypeStr = "FRAGMENT";
		break;
	}

	if (!success)
	{
		glGetShaderInfoLog(shader, infoLength, NULL, infoLog);
		std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << shaderTypeStr << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		return;
	}
}