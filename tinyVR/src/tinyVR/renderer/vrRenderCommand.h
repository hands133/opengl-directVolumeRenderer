#pragma once

#include "vrRendererAPI.h"

namespace tinyvr {

	class vrRenderCommand
	{
	public:

		static void Init()
		{
			s_RendererAPI->Init();
		}

		static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		static void Clear(bool stencil = false)
		{
			s_RendererAPI->Clear(stencil);
		}

		static void ClearBuffer(vrAPIOPBufferType t = vrAPIOPBufferType::BUFFERTYPE_COLOR, uint32_t idx = 0, const glm::vec4& v = glm::vec4(1.0f))
		{
			s_RendererAPI->ClearBuffer(t, idx, v);
		}

		static void SetClearDepth(float depth)
		{
			s_RendererAPI->SetClearDepth(depth);
		}

		static void SetViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewPort(x, y, width, height);
		}

		static void DrawIndexed(const vrRef<vrVertexArray>& vertexArray)
		{
			s_RendererAPI->DrawIndexed(vertexArray);
		}

		static void EnableDepthTest(vrAPIOPDepth depthOP = vrAPIOPDepth::DEPTH_LESS, bool mask = true)
		{
			s_RendererAPI->DepthTest(true, depthOP, mask);
		}

		static void DisableDepthTest()
		{
			s_RendererAPI->DepthTest(false);
		}

		static void EnableCullFace(vrAPIOPFaceCull facecullOP = vrAPIOPFaceCull::CULLFACE_BACK)
		{
			s_RendererAPI->CullFace(true, facecullOP);
		}

		static void DisableCullFace()
		{
			s_RendererAPI->CullFace(false);
		}

		static void EnableAlphaBlend(
			vrAPIOPBlendSrc blendSrcOP = vrAPIOPBlendSrc::SRCBLEND_ONE,
			vrAPIOPBlendDst blendDstOP = vrAPIOPBlendDst::DSTBLEND_ZERO, uint32_t idx = 0)
		{
			s_RendererAPI->AlphaBlend(true, blendSrcOP, blendDstOP, idx);
		}

		static void DisableAlphaBlend()
		{
			s_RendererAPI->AlphaBlend(false);
		}

		static void EnableMultiSample()
		{
			s_RendererAPI->Multisample(true);
		}

		static void DisableMultisample()
		{
			s_RendererAPI->Multisample(false);
		}

		static void ShadeModel(vrAPIOPShadeModel shadeModel = vrAPIOPShadeModel::SMOOTH)
		{
			s_RendererAPI->ShadeModel(shadeModel);
		}

		static void MapFBBuffer(uint32_t count)
		{
			s_RendererAPI->MapFBBuffer(count);
		}

	private:
		static vrRendererAPI* s_RendererAPI;
	};
}


