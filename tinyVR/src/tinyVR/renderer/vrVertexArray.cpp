#include "vrpch.h"
#include "vrVertexArray.h"

#include "vrRenderer.h"
#include "platform/openGL/vrOpenGLVertexArray.h"

namespace tinyvr {

	vrRef<vrVertexArray> vrVertexArray::Create()
	{
		switch (vrRenderer::GetAPI())
		{
			case vrRendererAPI::API::None:
				//TINYVR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
				TINYVR_ASSERT(false, "RendererAPI::None is currently not supported!");
				return nullptr;
			case vrRendererAPI::API::OpenGL:	return CreateRef<vrOpenGLVertexArray>();
		}
		//TINYVR_CORE_ASSERT(false, "Unknown RendererAPI");
		TINYVR_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}