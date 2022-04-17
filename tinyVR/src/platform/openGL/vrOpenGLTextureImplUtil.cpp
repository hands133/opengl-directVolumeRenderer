#pragma once
#include "vrpch.h"

#include "vrOpenGLTextureImplUtil.h"

namespace tinyvr {

	decltype(&__getMinMaxImpl<int>) TypeRegistry::fp_minmax_table[TypeRegistry::__numTypes];
	decltype(&__getHistogram<int>) TypeRegistry::fp_hist_table[TypeRegistry::__numTypes];

#define REGISTER_TYPE(TYPE, __MAPPEDTYPE)						\
	static TypeRegistry register_minmax_##TYPE(					\
		Declval<__MAPPEDTYPE>(), tinyvr::vrTextureType::TYPE);	\
	static TypeRegistry register_hist_##TYPE(					\
		Declval<__MAPPEDTYPE>(), tinyvr::vrTextureType::TYPE);

	REGISTER_TYPE(TEXTURE_TYPE_U8I, uint8_t);
	REGISTER_TYPE(TEXTURE_TYPE_U16I, uint16_t);
	REGISTER_TYPE(TEXTURE_TYPE_U32I, uint32_t);
	REGISTER_TYPE(TEXTURE_TYPE_8I, int8_t);
	REGISTER_TYPE(TEXTURE_TYPE_16I, int16_t);
	REGISTER_TYPE(TEXTURE_TYPE_32I, int32_t);
	REGISTER_TYPE(TEXTURE_TYPE_FLT32, float);

	glm::vec2 getMinMax(vrTextureType type, vrTextureFormat format, void* p, size_t size)
	{
		TINYVR_CORE_ASSERT(format == tinyvr::vrTextureFormat::TEXTURE_FMT_RED, "Sorry, single channel only!");
		return TypeRegistry::fp_minmax_table[EnumNumber(type)](p, size);
	}

	std::vector<float> getHistogram(vrTextureType type, vrTextureFormat format, void* p, size_t size, uint32_t NumIntervals)
	{
		TINYVR_CORE_ASSERT(format == tinyvr::vrTextureFormat::TEXTURE_FMT_RED, "Sorry, single channel only!");
		return TypeRegistry::fp_hist_table[EnumNumber(type)](p, size, NumIntervals);
	}
}