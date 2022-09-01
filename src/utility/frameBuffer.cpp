#include "frameBuffer.h"

frameBuffer::frameBuffer(const std::string& str) : descrip(str)
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
    bind();
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status == GL_FRAMEBUFFER_COMPLETE)
        std::cout << "[FRAME BUFFER]::" << descrip << " ready." << std::endl;
    else
        std::cout << "[FRAME BUFFER]::" << descrip << " not ready!" << std::endl;
    unbind();
}

void frameBuffer::bindTexture2d(GLenum attachment, GLint texID, GLint level)
{
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texID, level);
    unbind();
}