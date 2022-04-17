#pragma once

#include "tinyVR/renderer/vrRendererAPI.h"

namespace tinyvr {

	class vrOpenGLRendererAPI : public vrRendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void SetClearDepth(float depth = 1.0f) override;
		virtual void SetViewPort(int x, int y, int width, int height) override;
		virtual void Clear(bool stencil = false) override;
		virtual void ClearBuffer(vrAPIOPBufferType t = vrAPIOPBufferType::BUFFERTYPE_COLOR,
			uint32_t idx = 0, const glm::vec4& v = glm::vec4(1.0f)) override;

		virtual void DepthTest(bool enable = false, vrAPIOPDepth depthOP = vrAPIOPDepth::DEPTH_LESS, bool mask = true) override;
		virtual void CullFace(bool enable = false, vrAPIOPFaceCull facecullOP = vrAPIOPFaceCull::CULLFACE_BACK) override;
		virtual void AlphaBlend(bool enable = false,
			vrAPIOPBlendSrc blendSrcOP = vrAPIOPBlendSrc::SRCBLEND_ONE,
			vrAPIOPBlendDst blendDstOP = vrAPIOPBlendDst::DSTBLEND_ZERO, uint32_t idx = 0) override;
		virtual void Multisample(bool enable = false) override;
		virtual void ShadeModel(vrAPIOPShadeModel shadeModel = vrAPIOPShadeModel::SMOOTH) override;
		virtual void MapFBBuffer(uint32_t count) override;

		virtual void DrawIndexed(const vrRef<vrVertexArray>& vertexArray) override;

	private:
		unsigned int GetDepthOPFromEnum(vrAPIOPDepth depthOP);
		unsigned int GetFaceCullOPFromEnum(vrAPIOPFaceCull facecullOP);
		unsigned int GetBlendOPSrcFromEnum(vrAPIOPBlendSrc blendSrcOP);
		unsigned int GetBlendOPDstFromEnum(vrAPIOPBlendDst blendDstOP);
	};

}

