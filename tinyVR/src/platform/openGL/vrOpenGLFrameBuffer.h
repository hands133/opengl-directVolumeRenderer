#pragma once

#include "tinyVR/renderer/vrFrameBuffer.h"

#include "tinyVR/renderer/vrTexture.h"

namespace tinyvr {

	class vrOpenGLFrameBuffer : public vrFrameBuffer
	{
	public:
		vrOpenGLFrameBuffer(const vrFrameBufferSpecification& spec);
		~vrOpenGLFrameBuffer();

		void Bind() override;
		void Unbind() override;
		
		void CheckStatus() override;
		
		void Resize(uint32_t width, uint32_t height) override;
		
		uint32_t GetColorAttachmentRendererID(uint32_t idx = 0) const override;
		const vrRef<vrTexture2D>& GetColorAttachment(uint32_t idx) const {
			TINYVR_CORE_ASSERT(idx < m_MaxColorAttachments, "Idx = {0} out of range!", idx);
			return m_ColorAttachments[idx]; 
		}
		
		vrRef<vrTexture> GetDepthAttachment() const override
		{	return m_DepthAttachment;	}

		const vrFrameBufferSpecification& GetSpecification() const { return m_Specification; }

		void BindTexture(vrFBAttach attach, const vrRef<vrTexture2D>& tex, uint32_t idx = 0) override;
		
		static uint32_t GetPixelUI(const vrRef<vrTexture2D>& tex, const glm::ivec2 p);
		static float GetPixelF(const vrRef<vrTexture2D>& tex, const glm::ivec2 p);

		static std::vector<uint32_t> GetPixelsUI(const vrRef<vrTexture2D>& tex, const glm::ivec2& P0, const glm::ivec2& P1);

	private:
		void Init(const vrFrameBufferSpecification& spec) override;

	private:
		int m_MaxColorAttachments;

		uint32_t m_RendererID;
		vrFrameBufferSpecification m_Specification;

		vrRef<vrTexture2D> m_DepthAttachment;
		std::vector<vrRef<vrTexture2D>> m_ColorAttachments;
	};

}

