#include "vrpch.h"
#include "vrTexture.h"

#include "vrRenderer.h"

#include "platform/openGL/vrOpenGLTexture.h"

namespace tinyvr {
	vrRef<vrTexture1D> vrTexture1D::Create(uint32_t width, vrTextureFormat format, vrTextureType type)
	{
		switch (vrRendererAPI::GetAPI())
		{
		case vrRendererAPI::API::None:		TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");	return nullptr;
		case vrRendererAPI::API::OpenGL:	return vrOpenGLTexture1D::CreateTexture({ width, 1, 1 }, format, type);
		}
	}

	vrRef<vrTexture2D> vrTexture2D::Create(uint32_t width, uint32_t height, vrTextureFormat format, vrTextureType type)
	{
		switch (vrRendererAPI::GetAPI())
		{
		case vrRendererAPI::API::None:		TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");	return nullptr;
		case vrRendererAPI::API::OpenGL:	return vrOpenGLTexture2D::CreateTexture({ width, height, 1 }, format, type);
		}
	}

	vrRef<vrTexture3D> vrTexture3D::Create(uint32_t width, uint32_t height, uint32_t depth, vrTextureFormat format, vrTextureType type)
	{
		switch (vrRendererAPI::GetAPI())
		{
		case vrRendererAPI::API::None:		TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");	return nullptr;
		case vrRendererAPI::API::OpenGL:	return vrOpenGLTexture3D::CreateTexture({ width, height, depth }, format, type);
		}
	}
}