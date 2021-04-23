#pragma once

#include <glad/glad.h>

#include <iostream>
#include <string>

#include <glm/glm.hpp>

// class for texture
class Texture
{
public:
    Texture(const std::string &str, GLenum dim, GLuint lvl = 0, GLenum borderParam = GL_CLAMP_TO_EDGE, GLenum filterParam = GL_LINEAR, GLboolean genMipmap = false);
    ~Texture();

    void bind();
    void unbind();

    void setData(GLint internalFormat, glm::ivec3 coverSpan, GLint border, GLenum format, GLenum type, const void *data);

    // ADT method
    unsigned int getID() const { return ID; }
    GLenum getDim() const { return texDim; }
    unsigned int getLvl() const { return level; }
    std::string getDescrip() const { return descrip; }
private:
    void initParam(GLenum borderParam = GL_CLAMP_TO_EDGE, GLenum filterParam = GL_LINEAR, GLboolean genMipmap = false);

    GLenum texDim;  // GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D
    unsigned int ID;
    unsigned int level;
    std::string descrip;
};