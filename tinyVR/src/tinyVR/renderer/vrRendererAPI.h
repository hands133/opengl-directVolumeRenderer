#pragma once

#include <glm/glm.hpp>

#include "vrVertexArray.h"

namespace tinyvr {

	enum class vrAPIOPBufferType
	{
		BUFFERTYPE_COLOR,
		BUFFERTYPE_DEPTH,
		BUFFERTYPE_STENCIL
	};

	enum class vrAPIOPDepth
	{
		DEPTH_NEVER,
		DEPTH_LESS,
		DEPTH_EQUAL,
		DEPTH_NEQUAL,
		DEPTH_LEQUAL,
		DEPTH_GEQUAL,
		DEPTH_GREATER,
		DEPTH_ALWAYS
	};

	enum class vrAPIOPFaceCull
	{
		CULLFACE_FRONT,
		CULLFACE_BACK
	};

	// alpha blend option
	// refer https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBlendFunc.xhtml
	enum class vrAPIOPBlendSrc
	{
		SRCBLEND_ZERO,
		SRCBLEND_ONE,
		SRCBLEND_DSTCOLOR,
		SRCBLEND_ONE_MINUS_DSTCOLOR,
		SRCBLEND_SRCALPHA,
		SRCBLEND_ONE_MINUS_SRCALPHA,
		SRCBLEND_DSTALPHA,
		SRCBLEND_ONE_MINUS_DSTALPHA,
		SRCBLEND_SRCALHPA_SATURATE
	};

	enum class vrAPIOPBlendDst
	{
		DSTBLEND_ZERO,
		DSTBLEND_ONE,
		DSTBLEND_SRCCOLOR,
		DSTBLEND_ONE_MINUS_SRCCOLOR,
		DSTBLEND_SRCALPHA,
		DSTBLEND_ONE_MINUS_SRCALPHA,
		DSTBLEND_DSTALPHA,
		DSTBLEND_ONE_MINUS_DSTALPHA
	};

	enum class vrAPIOPShadeModel
	{
		SMOOTH, FLAT
	};

	class vrRendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1
		};
	public:
		virtual void Init() = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void SetClearDepth(float depth = 1.0f) = 0;
		virtual void SetViewPort(int x, int y, int width, int height) = 0;
		virtual void Clear(bool stencil = false) = 0;
		virtual void ClearBuffer(vrAPIOPBufferType t = vrAPIOPBufferType::BUFFERTYPE_COLOR,
			uint32_t idx = 0, const glm::vec4& v = glm::vec4(1.0f)) = 0;

		virtual void DepthTest(bool enable = false, vrAPIOPDepth depthOP = vrAPIOPDepth::DEPTH_LESS, bool mask = true) = 0;
		virtual void CullFace(bool enable = false, vrAPIOPFaceCull facecullOP = vrAPIOPFaceCull::CULLFACE_BACK) = 0;
		virtual void AlphaBlend(bool enable = false,
			vrAPIOPBlendSrc blendSrcOP = vrAPIOPBlendSrc::SRCBLEND_ONE,
			vrAPIOPBlendDst blendDstOP = vrAPIOPBlendDst::DSTBLEND_ZERO, uint32_t idx = 0) = 0;
		virtual void Multisample(bool enable = false) = 0;
		virtual void ShadeModel(vrAPIOPShadeModel shadeModel = vrAPIOPShadeModel::SMOOTH) = 0;
		virtual void MapFBBuffer(uint32_t count) = 0;

		virtual void DrawIndexed(const vrRef<vrVertexArray>& vertexArray) = 0;

		static API GetAPI() { return s_API; }
	private:
		static API s_API;
	};
}

