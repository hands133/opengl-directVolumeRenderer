#pragma once

#include <glad/glad.h>

#include <iostream>

// class for framebuffer
class frameBuffer{

public:

    frameBuffer();
    ~frameBuffer();

    unsigned int getID() { return ID; }

    void bind();
    void unbind();
    void checkStatus();

private:
    unsigned int ID;

};