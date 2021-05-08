#include "texture.h"

GLenum Texture::Init_TEX_ID = GL_TEXTURE0;

Texture::Texture(const std::string &str, GLenum dim, GLuint lvl, GLenum borderParam, GLenum filterParam, GLboolean genMipMap) : texDim(dim), level(lvl), descrip(str)
{
    glGenTextures(1, &ID);
    this->texID = Init_TEX_ID;
    Init_TEX_ID++;
    initParam(borderParam, filterParam, genMipMap);
}

Texture::~Texture()
{
    unbind();
    glDeleteTextures(1, &ID);
}

void Texture::bind() const
{
    glBindTexture(texDim, getID());
}

void Texture::unbind() const
{
    // glBindTexture(GL_TEXTURE_2D, 0);
}

// Set data, if data == 0 reserve empty memory
void Texture::setData(GLint internalFormat, glm::ivec3 coverSpan, GLint border, GLenum format, GLenum type, const void *data)
{
    bind();
    switch (texDim)
    {
    case GL_TEXTURE_1D:
        glTexImage1D(texDim, level, internalFormat, coverSpan.x, border, format, type, data);
        break;
    case GL_TEXTURE_2D:
        glTexImage2D(texDim, level, internalFormat, coverSpan.x, coverSpan.y, border, format, type, data);
        break;
    case GL_TEXTURE_3D:
        glTexImage3D(texDim, level, internalFormat, coverSpan.x, coverSpan.y, coverSpan.z, border, format, type, data);
    default:
        break;
    }
    unbind();
}

// 2D texture by default
void Texture::initParam(GLenum borderParam, GLenum filterParam, GLboolean genMipMap)
{
    bind();

	glTexParameteri(texDim, GL_TEXTURE_MAG_FILTER, filterParam);
	glTexParameteri(texDim, GL_TEXTURE_MIN_FILTER, filterParam);
    
    if(texDim == GL_TEXTURE_1D)
        glTexParameteri(texDim, GL_TEXTURE_WRAP_S, borderParam);
    else if(texDim == GL_TEXTURE_2D)
    {
        glTexParameteri(texDim, GL_TEXTURE_WRAP_S, borderParam);
	    glTexParameteri(texDim, GL_TEXTURE_WRAP_T, borderParam);
    }
    else if(texDim == GL_TEXTURE_3D)
    {
        glTexParameteri(texDim, GL_TEXTURE_WRAP_S, borderParam);
        glTexParameteri(texDim, GL_TEXTURE_WRAP_T, borderParam);
        glTexParameteri(texDim, GL_TEXTURE_WRAP_R, borderParam);
    }
    else
    {
        std::cerr << "[TEXTURE]::" << descrip << " dimension error!" << std::endl;
        unbind();
    }

    if(genMipMap)   glGenerateMipmap(texDim);
    unbind();
}

