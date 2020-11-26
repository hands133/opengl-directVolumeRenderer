#pragma once

#include <glad/glad.h>

#include <iostream>
#include <string>

// class for framebuffer
class frameBuffer{

public:

    frameBuffer(const std::string& str);
    ~frameBuffer();

    unsigned int getID() { return ID; }

    void bind();
    void unbind();
    void checkStatus();
    void bindTexture2d(GLenum attachment, GLint texID, GLint level);

private:
    unsigned int ID;

    std::string descrip;

};