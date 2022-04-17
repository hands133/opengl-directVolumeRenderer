#include "vrpch.h"
#include "vrTransferFunction.h"

namespace tinyvr
{
	vrTransferFunction::vrTransferFunction(vrRef<vrTexture1D>& tfTex, uint32_t nIsoVals = 256)
		: m_TFTexture(tfTex), m_IsoValRange(0, nIsoVals - 1)
	{
		// Prepare
		// 0. Preset Nodes
		std::string dataPath = "resources/builtIn/ColorMap.json";
		m_DefaultTFKeys = m_DefaultTFImporter.Read(dataPath);
		
		// 1. Color Nodes
		UpdateColorNodes("Cool to Warm");

		m_AlphaNodes.emplace_back(tfANode{ m_IsoValRange.x, 0.0f });
		m_AlphaNodes.emplace_back(tfANode{ m_IsoValRange.y, 1.0f });

		m_AlphaNodesChanged = true;
		m_IsoValRangeChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::UpdateColorParameters(int index)
	{
		auto& colorParams = m_DefaultTFImporter.GetDefaultTFByIndex(index);
		m_ColorNodes.resize(colorParams.size());
		std::copy(colorParams.begin(), colorParams.end(), m_ColorNodes.begin());

		m_ColorNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::SetIsoValueRange(const glm::uvec2& range)
	{
		m_IsoValRange = range;
		m_IsoValRangeChanged = true;
	}

	glm::vec3 vrTransferFunction::GetColor3(uint32_t isoValue) const
	{
		glm::vec4 C(0.0f);
		LerpElement(C, isoValue, m_ColorNodes);
		return glm::vec3(C.x, C.y, C.z);
	}

	float vrTransferFunction::GetAlpha(uint32_t isoValue) const
	{
		glm::vec4 C(0.0f);
		LerpElement(C, isoValue, m_AlphaNodes);
		return C.a;
	}

	glm::vec4 vrTransferFunction::GetColor(uint32_t isoValue) const
	{
		glm::vec4 C(0.0f);
		LerpElement(C, isoValue, m_ColorNodes);
		LerpElement(C, isoValue, m_AlphaNodes);
		return C;
	}

	void vrTransferFunction::UpdateColor3(uint32_t isoValue, const glm::vec3& C)
	{
		auto targetIter = std::find(m_ColorNodes.begin(), m_ColorNodes.end(), isoValue);
		if (targetIter != m_ColorNodes.end())	targetIter->C = glm::clamp(C, glm::vec3(0.0f), glm::vec3(1.0f));

		m_ColorNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::UpdateAlpha(uint32_t isoValue, float a)
	{
		auto targetIter = std::find(m_AlphaNodes.begin(), m_AlphaNodes.end(), isoValue);
		if (targetIter != m_AlphaNodes.end())	targetIter->a = glm::clamp(a, 0.0f, 1.0f);

		m_AlphaNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::UpdateAlphaByIdx(uint32_t idx, float a)
	{
		if (idx >= m_AlphaNodes.size())	return;
		m_AlphaNodes[idx].a = glm::clamp(a, 0.0f, 1.0f);

		m_AlphaNodesChanged = true;
		
		OnUpdate();
	}

	void vrTransferFunction::UpdateColorIsoValue(uint32_t oldIsoValue, uint32_t newIsoValue)
	{
		if (newIsoValue == oldIsoValue)		return;
		if (oldIsoValue == m_IsoValRange.x || oldIsoValue == m_IsoValRange.y)	return;

		auto targetIter = std::find(m_ColorNodes.begin(), m_ColorNodes.end(), oldIsoValue);
		if (targetIter == m_ColorNodes.end())	return;

		auto targetL = targetIter - 1;
		auto targetR = targetIter + 1;

		newIsoValue = glm::clamp(newIsoValue, targetL->isoVal + 1, targetR->isoVal - 1);
		targetIter->isoVal = newIsoValue;

		m_ColorNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::UpdateAlphaIsoValue(uint32_t oldIsoValue, uint32_t newIsoValue)
	{
		if (newIsoValue == oldIsoValue)		return;
		if (oldIsoValue == m_IsoValRange.x || oldIsoValue == m_IsoValRange.y)	return;

		auto targetIter = std::find(m_AlphaNodes.begin(), m_AlphaNodes.end(), oldIsoValue);
		if (targetIter == m_AlphaNodes.end())	return;

		auto targetL = targetIter - 1;
		auto targetR = targetIter + 1;

		newIsoValue = glm::clamp(newIsoValue, targetL->isoVal + 1, targetR->isoVal - 1);
		targetIter->isoVal = newIsoValue;

		m_AlphaNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::AddColorNode(uint32_t isoVal, glm::vec3 C)
	{
		auto searchIter = std::upper_bound(m_ColorNodes.begin(), m_ColorNodes.end(), isoVal,
			[](uint32_t isoVal, const tfCNode& rhs) { return isoVal < rhs.isoVal; });
		
		if ((searchIter - 1)->isoVal == isoVal)		(searchIter - 1)->C = C;
		else m_ColorNodes.insert(searchIter, tfCNode{ isoVal, C });

		m_ColorNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::AddAlphaNode(uint32_t isoVal, float a)
	{
		auto searchIter = std::upper_bound(m_AlphaNodes.begin(), m_AlphaNodes.end(), isoVal,
			[](uint32_t isoVal, const tfANode& rhs) { return isoVal < rhs.isoVal; });
		
		if ((searchIter - 1)->isoVal == isoVal)		(searchIter - 1)->a = a;
		else m_AlphaNodes.insert(searchIter, tfANode{ isoVal, a });

		m_AlphaNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::DeleteColorNode(uint32_t isoVal)
	{
		auto targetIter = std::find(m_ColorNodes.begin(), m_ColorNodes.end(), isoVal);
		if (targetIter != m_ColorNodes.end())	m_ColorNodes.erase(targetIter);

		m_ColorNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::DeleteAlphaNode(uint32_t isoVal)
	{
		auto targetIter = std::find(m_AlphaNodes.begin(), m_AlphaNodes.end(), isoVal);
		if (targetIter != m_AlphaNodes.end())	m_AlphaNodes.erase(targetIter);

		m_AlphaNodesChanged = true;

		OnUpdate();
	}

	void vrTransferFunction::OnUpdate()
	{
		UpdateTFWidth();
		UpdateTFData();
	}

	void vrTransferFunction::UpdateColorNodes(const std::string& key)
	{
		auto buffer = m_DefaultTFImporter.GetDefaultTFByName(key);
		m_ColorNodes.resize(buffer.size());
		std::copy(buffer.begin(), buffer.end(), m_ColorNodes.begin());

		m_ColorNodesChanged = true;
	}

	void vrTransferFunction::UpdateColorBuffer()
	{
		if (m_ColorNodesChanged)	LerpElements(m_ColorBuffer, m_ColorNodes);
		if (m_AlphaNodesChanged)	LerpElements(m_ColorBuffer, m_AlphaNodes);

		m_ColorNodesChanged = false;
		m_AlphaNodesChanged = false;
	}

	void vrTransferFunction::UpdateTFWidth()
	{
		uint32_t nIsoVals = m_IsoValRange.y - m_IsoValRange.x + 1;

		if (m_TFTexture == nullptr)
			m_TFTexture = vrTexture1D::Create(nIsoVals,
				vrTextureFormat::TEXTURE_FMT_RGBA, vrTextureType::TEXTURE_TYPE_FLT32);

		if (m_IsoValRangeChanged)
		{
			m_ColorBuffer.resize(nIsoVals, glm::vec4(0.0f));
			m_TFTexture->Resize(nIsoVals);
		}

		m_IsoValRangeChanged = false;
	}

	void vrTransferFunction::UpdateTFData()
	{
		UpdateColorBuffer();
		m_TFTexture->SetData(m_ColorBuffer.size(), m_ColorBuffer.data());
	}
}