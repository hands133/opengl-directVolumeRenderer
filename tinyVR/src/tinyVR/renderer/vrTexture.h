#pragma once

#include "tinyVR/core/vrBase.h"

namespace tinyvr {

	enum class vrTextureDim
	{
		TEXTURE_DIM_1D,
		TEXTURE_DIM_2D,
		TEXTURE_DIM_3D
	};

	enum class vrTextureBorder
	{
		TEXTURE_BORDER_CLAMP,
		TEXTURE_BORDER_CLAMP_EDGE,
		TEXTURE_BORDER_CLAMP_BORDER,
		TEXTURE_BORDER_REPEAT,
		TEXTURE_BORDER_MIRROR_REPEAT
	};

	enum class vrTextureInterp
	{
		TEXTURE_INTERP_LINEAR,
		TEXTURE_INTERP_NEAREST
	};

	enum class vrTextureType
	{
		// Unsigned type
		TEXTURE_TYPE_U8I,
		TEXTURE_TYPE_U16I,
		TEXTURE_TYPE_U32I,
		TEXTURE_TYPE_8I,
		TEXTURE_TYPE_16I,
		TEXTURE_TYPE_32I,
		TEXTURE_TYPE_FLT16,
		TEXTURE_TYPE_FLT32
	};

	enum class vrTextureFormat
	{
		TEXTURE_FMT_RED,        // (R)
		TEXTURE_FMT_RG,         // (R, G)
		TEXTURE_FMT_RGB,        // (R, G, B)
		TEXTURE_FMT_RGBA,       // (R, G, B, A)
		TEXTURE_FMT_DEPTH,      // (D), clamp to [0, 1]
		//TEXTURE_FMT_STENCIL,    // (D, S)
	};

	enum class vrTexImageAccess
	{
		TEXTURE_IMAGE_ACCESS_READONLY,
		TEXTURE_IMAGE_ACCESS_WRITEONLY,
		TEXTURE_IMAGE_ACCESS_READWRITE
	};


	class vrTexture
	{
	public:
		vrTexture() = default;
		virtual ~vrTexture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetDepth() const = 0;
		virtual uint32_t GetLevel() const = 0;
		virtual uint32_t GetTextureID() const = 0;

		virtual vrTextureDim GetDim() const = 0;

		virtual void SetLayout(vrTextureFormat format, vrTextureType type) = 0;
		virtual void SetData(uint32_t size = 0, const void* data = nullptr) = 0;

		virtual void Bind() const = 0;
		virtual void BindUnit(uint32_t unit) const = 0;
		virtual void BindImage(uint32_t idx, vrTexImageAccess access) const = 0;

		// Strange function
		virtual glm::vec2 GlobalMinMaxVal() const = 0;
		virtual std::vector<float> GetHistogram(uint32_t NumIntervals) const = 0;
		virtual bool GetData(void* buffer) const = 0;
	};

	class vrTexture1D : public vrTexture
	{
	public:
		static vrRef<vrTexture1D> Create(
			uint32_t width,
			vrTextureFormat format = vrTextureFormat::TEXTURE_FMT_RGBA, vrTextureType type = vrTextureType::TEXTURE_TYPE_FLT32);

		vrTexture1D() = default;
		~vrTexture1D() = default;

		uint32_t GetHeight() const override final { return 1; }
		uint32_t GetDepth() const override final { return 1; }

		vrTextureDim GetDim() const override final { return vrTextureDim::TEXTURE_DIM_1D; }

		virtual void Resize(uint32_t width) {}
	};

	class vrTexture2D : public vrTexture
	{
	public:
		static vrRef<vrTexture2D> Create(
			uint32_t width, uint32_t height,
			vrTextureFormat format = vrTextureFormat::TEXTURE_FMT_RGBA, vrTextureType type = vrTextureType::TEXTURE_TYPE_FLT32);

		uint32_t GetDepth() const override final { return 1; }

		vrTextureDim GetDim() const override final { return vrTextureDim::TEXTURE_DIM_2D; }

		virtual void Resize(uint32_t width, uint32_t height) {}
	};

	class vrTexture3D : public vrTexture
	{
	public:
		static vrRef<vrTexture3D> Create(
			uint32_t width, uint32_t height, uint32_t depth,
			vrTextureFormat format = vrTextureFormat::TEXTURE_FMT_RGBA, vrTextureType type = vrTextureType::TEXTURE_TYPE_FLT32);

		vrTextureDim GetDim() const override final { return vrTextureDim::TEXTURE_DIM_3D; }

		virtual void Resize(uint32_t width, uint32_t height, uint32_t depth) {}
	};
}