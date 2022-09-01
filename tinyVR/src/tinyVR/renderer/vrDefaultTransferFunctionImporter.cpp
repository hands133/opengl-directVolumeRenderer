#include "vrpch.h"
#include "vrDefaultTransferFunctionImporter.h"

#include <fstream>
#include <filesystem>

namespace tinyvr
{
	std::vector<std::string> vrDefaultTransferFunctionImporter::Read(const std::string& dataFilePath)
	{
		if (!std::filesystem::exists(dataFilePath))
		{
			TINYVR_WARN("Default Color Table Parameters File {0} is missing!", dataFilePath);
			return {};
		}

		std::ifstream dataFile(dataFilePath);
		nlohmann::json data;
		dataFile >> data;
		InitTFNameListNData(data);

		return m_DefaultTFNameList;
	}

	const std::vector<tfCNode>& vrDefaultTransferFunctionImporter::GetDefaultTFByName(const std::string& key)
	{
		auto iter = std::find(m_DefaultTFNameList.begin(), m_DefaultTFNameList.end(), key);
		if (iter == m_DefaultTFNameList.end())	return {};

		return m_DefaultTFParamList[std::distance(m_DefaultTFNameList.begin(), iter)];
	}

	void vrDefaultTransferFunctionImporter::InitTFNameListNData(const nlohmann::json& json)
	{
		std::vector<glm::vec4> buffer;
		for (auto& e : json)
		{
			try
			{
				glm::vec2 XRange(std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
				auto pBuf = e.at("RGBPoints").get<nlohmann::json::array_t>();

				int flts = pBuf.size();
				int nodes = flts / 4;

				buffer.resize(nodes);
				for (size_t i = 0; i < nodes; ++i)
				{
					buffer[i].a = pBuf[i * 4 + 0].get<float>();
					buffer[i].r = pBuf[i * 4 + 1].get<float>();
					buffer[i].g = pBuf[i * 4 + 2].get<float>();
					buffer[i].b = pBuf[i * 4 + 3].get<float>();

					if (buffer[i].a < XRange.x)	XRange.x = buffer[i].a;
					if (buffer[i].a > XRange.y)	XRange.y = buffer[i].a;
				}

				std::vector<tfCNode> ColorNodes(nodes);
				std::transform(buffer.begin(), buffer.end(), ColorNodes.begin(), [&](const glm::vec4& v)
					{
						float range = XRange.y - XRange.x;
						uint32_t isoVal = (v.a - XRange.x) / range * 511.999;
						return tfCNode{ isoVal, { v.r, v.g, v.b } }; 
					});

				m_DefaultTFParamList.emplace_back(ColorNodes);

				std::string name;
				name = e.at("Name").get<std::string>();
				m_DefaultTFNameList.emplace_back(name);
			}
			catch (const std::exception&) {	continue; }
		}
		TINYVR_INFO("Read Default Parameters Finished, total {0:>4} sets", m_DefaultTFNameList.size());
	}
}