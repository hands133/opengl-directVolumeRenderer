#pragma once

#include "vrOpenGLTextureImpl.hpp"

#include <type_traits>
#include <algorithm>

#include <glm/gtc/type_precision.hpp>

namespace tinyvr
{
	template <typename __MappedType>
	inline static glm::vec2 __getMinMaxImpl(void* buffer, size_t size)
	{
		__MappedType* p = static_cast<__MappedType*>(buffer);
		auto mM = std::minmax_element(p, p + size);
		glm::vec2 minmax = { *(mM.first), *(mM.second) };
		return minmax;
	}

	template <typename __MappedType>
	inline static std::vector<float> __getHistogram(void* buffer, size_t size, uint32_t NumIntervals)
	{
		auto mM = __getMinMaxImpl<__MappedType>(buffer, size);
		__MappedType* p = static_cast<__MappedType*>(buffer);

		float dv = (mM.y - mM.x + 1) / float(NumIntervals);

		std::vector<float> hist(NumIntervals, 0.0f);
		for (size_t i = 0; i < size; ++i)
		{
			float v = static_cast<float>(p[i]);
			hist[static_cast<uint32_t>((v - mM.x) / dv)] += 1.0f;
		}

		for (auto& e : hist)	 e /= size;
		return hist;
	}

	struct TypeRegistry {
		//static constexpr size_t __numTypes = EnumElements<tinyvr::vrTextureType>();
		static constexpr size_t __numTypes = 7;
		static decltype(&__getMinMaxImpl<int>)	fp_minmax_table[__numTypes];
		static decltype(&__getHistogram<int>)	fp_hist_table[__numTypes];

		template <typename __MappedType>
		TypeRegistry(__MappedType, tinyvr::vrTextureType type)
		{
			fp_minmax_table[EnumNumber(type)] = &__getMinMaxImpl<__MappedType>;
			fp_hist_table[EnumNumber(type)] = &__getHistogram<__MappedType>;
		}
	};

	template <typename T>
	T Declval(std::true_type) { return T(); }
	template <typename T>
	T Declval(std::false_type) { return std::declval<T>(); }
	template <typename T>
	T Declval() { return Declval<T>(std::is_default_constructible<T>()); }

	glm::vec2 getMinMax(vrTextureType type, vrTextureFormat format, void* p, size_t size);
	std::vector<float> getHistogram(vrTextureType type, vrTextureFormat format, void* p, size_t size, uint32_t NumIntervals);
}


