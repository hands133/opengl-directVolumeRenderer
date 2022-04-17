#pragma once

#include "tinyVR/renderer/vrTexture.h"
#include <glm/glm.hpp>

#include "vrTransferFunctionNode.h"

#include "vrDefaultTransferFunctionImporter.h"

template <typename T>
T Lerp(T l, T r, float t)
{
	return l + t * (r - l);
}

namespace tinyvr
{
	class vrTransferFunction
	{
	public:
		vrTransferFunction(vrRef<vrTexture1D>& tfTex, uint32_t nIsoVals);
		~vrTransferFunction() {}

		static vrRef<vrTransferFunction> Create(vrRef<vrTexture1D>& tfTex, uint32_t nIsoVals = 256)
		{
			return std::make_shared<vrTransferFunction>(tfTex, nIsoVals);
		}

		const vrRef<vrTexture1D>& GetTFTexture() { return m_TFTexture; }
		const std::vector<std::string>& GetTFDefaultNameList() { return m_DefaultTFImporter.GetDefaultTFNameList(); }
		void UpdateColorParameters(int index);	// for vrTransferFunctionWidget
		
		void SetIsoValueRange(const glm::uvec2& range);
		glm::uvec2 GetIsoValueRange() const { return m_IsoValRange; }

		const std::vector<tfCNode>& GetColorNodes() const { return m_ColorNodes; }
		const std::vector<tfANode>& GetAlphaNodes() const { return m_AlphaNodes; }

		const std::vector<glm::vec4>& GetColorBuffer() { return m_ColorBuffer; }

		glm::vec3 GetColor3(float x) const { 
			return GetColor3(Lerp(m_IsoValRange.x, m_IsoValRange.y, glm::clamp(x, 0.0f, 1.0f))); }
		glm::vec3 GetColor3(uint32_t isoValue) const;

		float GetAlpha(float x) const {
			return GetAlpha(Lerp(m_IsoValRange.x, m_IsoValRange.y, glm::clamp(x, 0.0f, 1.0f))); }
		float GetAlpha(uint32_t isoValue) const;

		glm::vec4 GetColor(float x) const {
			return GetColor(Lerp(m_IsoValRange.x, m_IsoValRange.y, glm::clamp(x, 0.0f, 1.0f))); }
		glm::vec4 GetColor(uint32_t isoValue) const;

		void UpdateColor3(float x, const glm::vec3& C)
			{ UpdateColor3(Lerp(m_IsoValRange.x, m_IsoValRange.y, glm::clamp(x, 0.0f, 1.0f)), C); }
		void UpdateColor3(uint32_t isoValue, const glm::vec3& C);

		void UpdateAlpha(uint32_t isoValue, float a);
		void UpdateAlphaByIdx(uint32_t idx, float a);

		void UpdateColorIsoValue(uint32_t oldIsoValue, uint32_t newIsoValue);
		void UpdateAlphaIsoValue(uint32_t oldIsoValue, uint32_t newIsoValue);

		void AddColorNode(uint32_t isoVal, glm::vec3 C);
		void AddAlphaNode(uint32_t isoVal, float a);

		void DeleteColorNode(uint32_t isoVal);
		void DeleteAlphaNode(uint32_t isoVal);

		void OnUpdate();	// Update Texture & Inner Color Buffer

	private:
		void UpdateColorNodes(const std::string& key);

		void UpdateColorBuffer();
	
		void UpdateTFWidth();
		void UpdateTFData();

		template <typename T>
		void LerpElement(glm::vec4& C, uint32_t isoValue, const std::vector<T>& cont) const;

		template <typename T>
		void LerpElements(std::vector<glm::vec4>& colorBuffer, const std::vector<T>& cont) const;

	private:
		vrRef<vrTexture1D>& m_TFTexture;
		glm::uvec2 m_IsoValRange;

		std::vector<tfCNode> m_ColorNodes;
		std::vector<tfANode> m_AlphaNodes;

		std::vector<glm::vec4> m_ColorBuffer;

		bool m_IsoValRangeChanged = false;
		bool m_ColorNodesChanged = false;
		bool m_AlphaNodesChanged = false;

		vrDefaultTransferFunctionImporter m_DefaultTFImporter;
		std::vector<std::string> m_DefaultTFKeys;

	private:
		template <typename T>
		struct LerpSelector {};

		template <>
		struct LerpSelector<tfCNode>
		{
			void operator()(glm::vec4& C, float t, const tfCNode& l, const tfCNode& r) {
				auto lerpC = Lerp(l.C, r.C, t);
				C.r = lerpC.r;
				C.g = lerpC.g;
				C.b = lerpC.b;
			}
		};

		template <>
		struct LerpSelector<tfANode>
		{
			void operator()(glm::vec4& C, float t, const tfANode& l, const tfANode& r) {
				C.a = Lerp(l.a, r.a, t);
			}
		};
	};

	template<typename T>
	inline void vrTransferFunction::LerpElement(glm::vec4& C, uint32_t isoValue, const std::vector<T>& cont) const
	{
		LerpSelector<T> __lerp{};

		auto l = cont.begin();
		auto r = cont.begin();
		r++;

		uint32_t lIsoV = l->isoVal;
		uint32_t rIsoV = r->isoVal;

		uint32_t isoV = glm::clamp(isoValue, m_IsoValRange.x, m_IsoValRange.y);
		while (rIsoV < isoV)
		{
			l++;
			r++;

			lIsoV = l->isoVal;
			rIsoV = r->isoVal;
		}

		float t = static_cast<float>(isoV - lIsoV) / (rIsoV - lIsoV);
		__lerp(C, t, *l, *r);
	}

	template<typename T>
	inline void vrTransferFunction::LerpElements(std::vector<glm::vec4>& colorBuffer, const std::vector<T>& cont) const
	{
		LerpSelector<T> __lerp{};

		auto l = cont.begin();
		auto r = cont.begin();
		r++;

		uint32_t lIsoV = l->isoVal;
		uint32_t rIsoV = r->isoVal;

		for (size_t isoV = 0; isoV < colorBuffer.size(); ++isoV)
		{
			while (rIsoV < isoV)
			{
				l++;
				r++;
				
				lIsoV = l->isoVal;
				rIsoV = r->isoVal;
			}

			float t = static_cast<float>(isoV - lIsoV) / (rIsoV - lIsoV);
			__lerp(colorBuffer[isoV], t, *l, *r);
		}
	}
}
