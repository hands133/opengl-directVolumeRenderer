#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Texture;
class Shader
{
public:

	Shader(const GLchar *vertexPath, const GLchar *fragmentPath);       // constructor
	~Shader() { destroy(); }
	void destroy() { glDeleteProgram(ID); }
	unsigned int progID() { return ID; }
	void use();             // ʹ��/�������
	// uniform ���ߺ���
	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string &name, float value) const;
	void setVec3(const std::string &name, float x, float y, float z) const;
	void setVec3(const std::string &name, const glm::vec3 &vec) const;
	void setMat4(const std::string &name, const glm::mat4x4 &trans) const;
	void setTexture(const std::string &name, const Texture& tex) const;

private:
	unsigned int ID;        // shader ID

	enum class SHADER_TYPE { VERTEX, FRAGMENT };

	const int infoLength = 1024;
	char infoLog[1024];

	void checkCompileErrors(unsigned int shader, SHADER_TYPE type);
};