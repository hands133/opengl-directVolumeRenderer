#include "vrpch.h"
#include "vrFrameBuffer.h"

#include "tinyVR/renderer/vrRenderer.h"
#include "platform/openGL/vrOpenGLFrameBuffer.h"

namespace tinyvr {
	
	vrRef<vrFrameBuffer> vrFrameBuffer::Create(const vrFrameBufferSpecification& spec)
	{
		switch (vrRenderer::GetAPI())
		{
		case vrRendererAPI::API::None:		TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");	return nullptr;
		case vrRendererAPI::API::OpenGL:	return CreateRef<vrOpenGLFrameBuffer>(spec);
		}
	}

	uint32_t vrFrameBuffer::ReadPixelUI(const vrRef<vrTexture2D>& tex, const glm::ivec2& p)
	{
		switch (vrRenderer::GetAPI())
		{
		case vrRendererAPI::API::None:		TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");	return -1;
		case vrRendererAPI::API::OpenGL:	return vrOpenGLFrameBuffer::GetPixelUI(tex, p);
		}
	}

	float vrFrameBuffer::ReadPixelF(const vrRef<vrTexture2D>& tex, const glm::ivec2& p)
	{
		switch (vrRenderer::GetAPI())
		{
		case vrRendererAPI::API::None:		TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");	return -1;
		case vrRendererAPI::API::OpenGL:	return vrOpenGLFrameBuffer::GetPixelF(tex, p);
		}
	}

	std::vector<uint32_t> vrFrameBuffer::ReadPixelsUI(const vrRef<vrTexture2D>& tex, const glm::ivec2& P0, const glm::ivec2& P1)
	{
		switch (vrRenderer::GetAPI())
		{
		case vrRendererAPI::API::None:		TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");	return {};
		case vrRendererAPI::API::OpenGL:	return vrOpenGLFrameBuffer::GetPixelsUI(tex, P0, P1);
		}
	}
}