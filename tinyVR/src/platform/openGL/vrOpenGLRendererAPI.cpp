#include "vrpch.h"
#include "vrOpenGLRendererAPI.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace tinyvr {
	void vrOpenGLRendererAPI::Init()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
	}

	void vrOpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void vrOpenGLRendererAPI::SetClearDepth(float depth)
	{
		glClearDepth(depth);
	}

	void vrOpenGLRendererAPI::SetViewPort(int x, int y, int width, int height)
	{
		glViewport(x, y, width, height);
	}

	void vrOpenGLRendererAPI::Clear(bool stencil)
	{
		GLenum clearOP = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		if (stencil)
			clearOP |= GL_STENCIL_BUFFER_BIT;

		glClear(clearOP);
	}

	void vrOpenGLRendererAPI::ClearBuffer(vrAPIOPBufferType t, uint32_t idx, const glm::vec4& v)
	{
		GLenum bufferType = GL_COLOR;
		switch (t)
		{
		case tinyvr::vrAPIOPBufferType::BUFFERTYPE_COLOR:	bufferType = GL_COLOR;		break;
		case tinyvr::vrAPIOPBufferType::BUFFERTYPE_DEPTH:	bufferType = GL_DEPTH;		break;
		case tinyvr::vrAPIOPBufferType::BUFFERTYPE_STENCIL:	bufferType = GL_STENCIL;	break;
		}
		glClearBufferfv(bufferType, idx, glm::value_ptr(v));
	}

	void vrOpenGLRendererAPI::DepthTest(bool enable, vrAPIOPDepth depthOP, bool mask)
	{
		if (enable)		glEnable(GL_DEPTH_TEST);
		else			glDisable(GL_DEPTH_TEST);

		if (mask)		glDepthMask(GL_TRUE);
		else			glDepthMask(GL_FALSE);

		glDepthFunc(GetDepthOPFromEnum(depthOP));
	}

	void vrOpenGLRendererAPI::CullFace(bool enable, vrAPIOPFaceCull facecullOP)
	{
		if (enable)		glEnable(GL_CULL_FACE);
		else			glDisable(GL_CULL_FACE);

		glCullFace(GetFaceCullOPFromEnum(facecullOP));
	}

	void vrOpenGLRendererAPI::AlphaBlend(bool enable, vrAPIOPBlendSrc blendSrcOP, vrAPIOPBlendDst blendDstOP, uint32_t idx)
	{
		if (enable)		glEnable(GL_BLEND);
		else			glDisable(GL_BLEND);

		glBlendFunci(idx, GetBlendOPSrcFromEnum(blendSrcOP), GetBlendOPDstFromEnum(blendDstOP));
	}

	void vrOpenGLRendererAPI::Multisample(bool enable)
	{
		if (enable)		glEnable(GL_MULTISAMPLE);
		else			glDisable(GL_MULTISAMPLE);
	}

	void vrOpenGLRendererAPI::ShadeModel(vrAPIOPShadeModel shadeModel)
	{
		GLenum shadeModelParam = GL_SMOOTH;
		switch (shadeModel)
		{
		case tinyvr::vrAPIOPShadeModel::SMOOTH:	shadeModelParam = GL_SMOOTH;	break;
		case tinyvr::vrAPIOPShadeModel::FLAT:	shadeModelParam = GL_FLAT;		break;
		}
		glShadeModel(shadeModelParam);
	}

	void vrOpenGLRendererAPI::MapFBBuffer(uint32_t count)
	{
		if (count == 0)	return;

		std::vector<GLenum> bufferMap(count, GL_COLOR_ATTACHMENT0);
		for (uint32_t i = 1; i < count; ++i)
			bufferMap[i] = GL_COLOR_ATTACHMENT0 + i;

		glDrawBuffers(count, bufferMap.data());
	}

	void vrOpenGLRendererAPI::DrawIndexed(const vrRef<vrVertexArray>& vertexArray)
	{
		glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
	}

	unsigned int vrOpenGLRendererAPI::GetDepthOPFromEnum(vrAPIOPDepth depthOP)
	{
		GLenum depthOPParam = GL_LESS;
		switch (depthOP)
		{
		case tinyvr::vrAPIOPDepth::DEPTH_NEVER:		depthOPParam = GL_NEVER;	break;
		case tinyvr::vrAPIOPDepth::DEPTH_LESS:		depthOPParam = GL_LESS;		break;
		case tinyvr::vrAPIOPDepth::DEPTH_EQUAL:		depthOPParam = GL_EQUAL;	break;
		case tinyvr::vrAPIOPDepth::DEPTH_NEQUAL:	depthOPParam = GL_NOTEQUAL;	break;
		case tinyvr::vrAPIOPDepth::DEPTH_LEQUAL:	depthOPParam = GL_LEQUAL;	break;
		case tinyvr::vrAPIOPDepth::DEPTH_GEQUAL:	depthOPParam = GL_GEQUAL;	break;
		case tinyvr::vrAPIOPDepth::DEPTH_GREATER:	depthOPParam = GL_GREATER;	break;
		case tinyvr::vrAPIOPDepth::DEPTH_ALWAYS:	depthOPParam = GL_ALWAYS;	break;
		}
		return depthOPParam;
	}

	unsigned int vrOpenGLRendererAPI::GetFaceCullOPFromEnum(vrAPIOPFaceCull facecullOP)
	{
		GLenum facecullOPParam = GL_BACK;
		switch (facecullOP)
		{
		case tinyvr::vrAPIOPFaceCull::CULLFACE_FRONT:	facecullOPParam = GL_FRONT;	break;
		case tinyvr::vrAPIOPFaceCull::CULLFACE_BACK:	facecullOPParam = GL_BACK;	break;
		}
		return facecullOPParam;
	}

	unsigned int vrOpenGLRendererAPI::GetBlendOPSrcFromEnum(vrAPIOPBlendSrc blendSrcOP)
	{
		GLenum blendOPParam = GL_ONE;
		switch (blendSrcOP)
		{
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_ZERO:				blendOPParam = GL_ZERO;			break;
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_ONE:					blendOPParam = GL_ONE;			break;
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_DSTCOLOR:			blendOPParam = GL_DST_COLOR;	break;
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_ONE_MINUS_DSTCOLOR:	blendOPParam = GL_ONE_MINUS_DST_COLOR;	break;
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_SRCALPHA:			blendOPParam = GL_SRC_ALPHA;	break;
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_ONE_MINUS_SRCALPHA:	blendOPParam = GL_ONE_MINUS_SRC_ALPHA;	break;
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_DSTALPHA:			blendOPParam = GL_DST_ALPHA;	break;
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_ONE_MINUS_DSTALPHA:	blendOPParam = GL_ONE_MINUS_CONSTANT_ALPHA;		break;
		case tinyvr::vrAPIOPBlendSrc::SRCBLEND_SRCALHPA_SATURATE:	blendOPParam = GL_SRC_ALPHA_SATURATE;	break;
		}
		return blendOPParam;
	}

	unsigned int vrOpenGLRendererAPI::GetBlendOPDstFromEnum(vrAPIOPBlendDst blendDstOP)
	{
		GLenum blendOPParam = GL_ZERO;
		switch (blendDstOP)
		{
		case tinyvr::vrAPIOPBlendDst::DSTBLEND_ZERO:				blendOPParam = GL_ZERO;			break;
		case tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE:					blendOPParam = GL_ONE;			break;
		case tinyvr::vrAPIOPBlendDst::DSTBLEND_SRCCOLOR:			blendOPParam = GL_SRC_COLOR;	break;
		case tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE_MINUS_SRCCOLOR:	blendOPParam = GL_ONE_MINUS_SRC_COLOR;	break;
		case tinyvr::vrAPIOPBlendDst::DSTBLEND_SRCALPHA:			blendOPParam = GL_SRC_ALPHA;	break;
		case tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE_MINUS_SRCALPHA:	blendOPParam = GL_ONE_MINUS_SRC_ALPHA;	break;
		case tinyvr::vrAPIOPBlendDst::DSTBLEND_DSTALPHA:			blendOPParam = GL_DST_ALPHA;	break;
		case tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE_MINUS_DSTALPHA:	blendOPParam = GL_ONE_MINUS_DST_ALPHA;	break;
		}
		return blendOPParam;
	}

}