#include "vrpch.h"
#include "vrBuffer.h"

#include "vrRenderer.h"

#include "platform/openGL/vrOpenGLBuffer.h"

namespace tinyvr {

	vrRef<vrVertexBuffer> vrVertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (vrRenderer::GetAPI())
		{
		case vrRendererAPI::API::None:
			//TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			TINYVR_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case vrRendererAPI::API::OpenGL:	return CreateRef<vrOpenGLVertexBuffer>(vertices, size);
		}
		//TINYVR_CORE_ASSERT(false, "Unknown RendererAPI");
		TINYVR_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	vrRef<vrIndexBuffer> vrIndexBuffer::Create(uint32_t* indices, uint32_t size)
	{
		switch (vrRenderer::GetAPI())
		{
		case vrRendererAPI::API::None:	
			//TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); 
			TINYVR_ASSERT(false, "RendererAPI::None is currently not supported!"); 
			return nullptr;
		case vrRendererAPI::API::OpenGL:	return CreateRef<vrOpenGLIndexBuffer>(indices, size);
		}
		//TINYVR_CORE_ASSERT(false, "Unknown RendererAPI");
		TINYVR_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}