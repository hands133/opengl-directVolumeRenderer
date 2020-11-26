#include "frameBuffer.h"

frameBuffer::frameBuffer()
{
    glGenFramebuffers(1, &ID);
}

frameBuffer::~frameBuffer()
{
    unbind();
    glDeleteFramebuffers(1, &ID);
}

void frameBuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, ID);
}

void frameBuffer::unbind()
{
    // note that unbind === bind to main buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void frameBuffer::checkStatus()
{
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status == GL_FRAMEBUFFER_COMPLETE)
        std::cout << "frame buffer ready." << std::endl;
    else
        std::cout << "frame buffer not ready!" << std::endl;
}