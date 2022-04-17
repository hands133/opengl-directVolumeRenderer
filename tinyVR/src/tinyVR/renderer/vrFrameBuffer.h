#pragma once

#include "tinyVR.h"

namespace tinyvr {

	enum class vrFBAttach
	{
		COLOR_ATTACHMENT,		// default channel 0
		DEPTH_ATTACHMENT
	};

	struct vrFrameBufferSpecification
	{
		uint32_t Width, Height;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class vrFrameBuffer
	{
	public:
		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void CheckStatus() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t idx = 0) const = 0;
		virtual const vrRef<vrTexture2D>& GetColorAttachment(uint32_t idx = 0) const { return nullptr; }
		virtual vrRef<vrTexture> GetDepthAttachment() const = 0;

		virtual const vrFrameBufferSpecification& GetSpecification() const = 0;

		virtual void BindTexture(vrFBAttach attach, const vrRef<vrTexture2D>& tex, uint32_t idx = 0) = 0;

		static vrRef<vrFrameBuffer> Create(const vrFrameBufferSpecification& spec);

		static uint32_t ReadPixelUI(const vrRef<vrTexture2D>& tex, const glm::ivec2& p);
		static float ReadPixelF(const vrRef<vrTexture2D>& tex, const glm::ivec2& p);

		static std::vector<uint32_t> ReadPixelsUI(const vrRef<vrTexture2D>& tex, const glm::ivec2& P0, const glm::ivec2& P1);

	protected:
		virtual void Init(const vrFrameBufferSpecification& spec) = 0;
	};
}

