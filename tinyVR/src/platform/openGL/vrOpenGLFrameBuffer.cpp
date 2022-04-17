#include "vrpch.h"
#include "vrOpenGLFrameBuffer.h"

#include "platform/openGL/vrOpenGLTexture.h"

#include "platform/openGL/vrOpenGLErrorHandle.h"

#include <glad/glad.h>

namespace tinyvr {

	const static uint32_t s_MaxFrameBufferSize = 8192;

	vrOpenGLFrameBuffer::vrOpenGLFrameBuffer(const vrFrameBufferSpecification& spec)
		: m_RendererID(0), m_Specification(spec), m_MaxColorAttachments(0)
	{
		glCreateFramebuffers(1, &m_RendererID);
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_MaxColorAttachments);

		m_ColorAttachments.resize(m_MaxColorAttachments, nullptr);

		Init(spec);

		CheckStatus();
	}

	vrOpenGLFrameBuffer::~vrOpenGLFrameBuffer()
	{
		glDeleteFramebuffers(1, &m_RendererID);
	}

	void vrOpenGLFrameBuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
	}

	void vrOpenGLFrameBuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void vrOpenGLFrameBuffer::CheckStatus()
	{
		Bind();

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			switch (status)
			{
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			TINYVR_CORE_FATAL("Frame buffer attachments error!");		break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	TINYVR_CORE_FATAL("Frame buffer attachments missing!");		break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:			TINYVR_CORE_FATAL("Frame buffer draw call error!");			break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:			TINYVR_CORE_FATAL("Frame buffer read error!");				break;
			}
			TINYVR_ASSERT(false, "Frame-buffer not ready");
		}

		Unbind();
	}

	void vrOpenGLFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || width > s_MaxFrameBufferSize || height > s_MaxFrameBufferSize)
		{
			TINYVR_CORE_WARN("Attempted to resize framebufferto {0}, {1}", width, height);
			return;
		}

		m_Specification.Width = width;
		m_Specification.Height = height;

		glDeleteFramebuffers(1, &m_RendererID);
		glCreateFramebuffers(1, &m_RendererID);

		Bind();

		for (int i = 0; i < m_ColorAttachments.size(); ++i)
		{
			auto Cptr = m_ColorAttachments[i];
			if (Cptr)
			{
				Cptr->Resize(m_Specification.Width, m_Specification.Height);
				BindTexture(vrFBAttach::COLOR_ATTACHMENT, Cptr, i);
			}
		}

		m_DepthAttachment->Resize(m_Specification.Width, m_Specification.Height);
		BindTexture(vrFBAttach::DEPTH_ATTACHMENT, m_DepthAttachment);

		Unbind();

		CheckStatus();
	}

	uint32_t vrOpenGLFrameBuffer::GetColorAttachmentRendererID(uint32_t idx) const
	{
		TINYVR_CORE_ASSERT(idx < m_MaxColorAttachments, "idx = {0}, exceed maximum color attachments!", idx);
		
		auto colorAttachPtr = m_ColorAttachments[idx];
		TINYVR_CORE_ASSERT(colorAttachPtr, "idx = {0}, color attachment is empty!", idx);

		return m_ColorAttachments[idx]->GetTextureID();
	}

	void vrOpenGLFrameBuffer::BindTexture(vrFBAttach attach, const vrRef<vrTexture2D>& tex, uint32_t idx)
	{
		Bind();

		GLenum attachParam = GL_COLOR_ATTACHMENT0;
		switch (attach)
		{
		case tinyvr::vrFBAttach::COLOR_ATTACHMENT:
			attachParam += idx;
			m_ColorAttachments[idx] = tex;
			break;
		case tinyvr::vrFBAttach::DEPTH_ATTACHMENT:
			attachParam = GL_DEPTH_ATTACHMENT;
			m_DepthAttachment = tex;
			break;
		}

		GLuint texID = 0;
		GLint texLVL = 0;

		if (tex)
		{
			texID = tex->GetTextureID();
			texLVL = tex->GetLevel();
		}
		else
		{
			texID = 0;
			texLVL = 0;
		}

		switch (tex->GetDim())
		{
		case vrTextureDim::TEXTURE_DIM_1D:
			glFramebufferTexture1D(GL_FRAMEBUFFER, attachParam, GL_TEXTURE_1D, texID, texLVL);
			break;
		case vrTextureDim::TEXTURE_DIM_2D:
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachParam, GL_TEXTURE_2D, texID, texLVL);
			break;
		case vrTextureDim::TEXTURE_DIM_3D:
			glFramebufferTexture3D(GL_FRAMEBUFFER, attachParam, GL_TEXTURE_3D, texID, texLVL, 0);
			TINYVR_INFO("3D Texture only supports layer offset by z-index(=0)\n");
			break;
		}

		Unbind();
	}

	uint32_t vrOpenGLFrameBuffer::GetPixelUI(const vrRef<vrTexture2D>& tex, const glm::ivec2 p)
	{
		GLuint fbID;
		glGenFramebuffers(1, &fbID);
		glBindFramebuffer(GL_FRAMEBUFFER, fbID);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->GetTextureID(), 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		TINYVR_ASSERT(status == GL_FRAMEBUFFER_COMPLETE, "failed to make complete framebuffer object");

		glViewport(0, 0, tex->GetWidth(), tex->GetHeight());

		uint32_t data = 0;
		glBindFramebuffer(GL_FRAMEBUFFER, fbID);
		glReadPixels(p.x, p.y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &data);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbID);

		return data;
	}

	float vrOpenGLFrameBuffer::GetPixelF(const vrRef<vrTexture2D>& tex, const glm::ivec2 p)
	{
		GLuint fbID;
		glGenFramebuffers(1, &fbID);
		glBindFramebuffer(GL_FRAMEBUFFER, fbID);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->GetTextureID(), 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		TINYVR_ASSERT(status == GL_FRAMEBUFFER_COMPLETE, "failed to make complete framebuffer object");

		glViewport(0, 0, tex->GetWidth(), tex->GetHeight());

		float data = 0;
		glBindFramebuffer(GL_FRAMEBUFFER, fbID);
		glReadPixels(p.x, p.y, 1, 1, GL_RED, GL_FLOAT, &data);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbID);

		return data;
	}

	std::vector<uint32_t> vrOpenGLFrameBuffer::GetPixelsUI(const vrRef<vrTexture2D>& tex, const glm::ivec2& P0, const glm::ivec2& P1)
	{
		uint32_t size = (P1.y - P0.y + 1) * (P1.x - P0.x + 1);
		std::vector<uint32_t> buffer(size, 0);

		GLuint fbID;
		glGenFramebuffers(1, &fbID);
		glBindFramebuffer(GL_FRAMEBUFFER, fbID);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->GetTextureID(), 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		TINYVR_ASSERT(status == GL_FRAMEBUFFER_COMPLETE, "failed to make complete framebuffer object");

		glViewport(0, 0, tex->GetWidth(), tex->GetHeight());

		glBindFramebuffer(GL_FRAMEBUFFER, fbID);
		glReadPixels(P0.x, P0.y, P1.x - P0.x + 1, P1.y - P0.y + 1, GL_RED_INTEGER, GL_UNSIGNED_INT, buffer.data());

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbID);

		return buffer;
	}

	void vrOpenGLFrameBuffer::Init(const vrFrameBufferSpecification& spec)
	{
		m_ColorAttachments[0] = vrTexture2D::Create(spec.Width, spec.Height, tinyvr::vrTextureFormat::TEXTURE_FMT_RGBA);
		m_DepthAttachment = vrTexture2D::Create(spec.Width, spec.Height, vrTextureFormat::TEXTURE_FMT_DEPTH);

		m_ColorAttachments[0]->SetData(0, nullptr);
		m_DepthAttachment->SetData(0, nullptr);

		BindTexture(vrFBAttach::COLOR_ATTACHMENT, m_ColorAttachments[0], 0);
		BindTexture(vrFBAttach::DEPTH_ATTACHMENT, m_DepthAttachment);
	}
}