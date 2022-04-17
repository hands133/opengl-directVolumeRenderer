#include "vrpch.h"
#include "vrRenderCommand.h"

#include "platform/openGL/vrOpenGLRendererAPI.h"

namespace tinyvr {

	vrRendererAPI* vrRenderCommand::s_RendererAPI = new vrOpenGLRendererAPI();
}