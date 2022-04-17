#pragma once

#include "tinyVR/utils/json.hpp"

#include "vrTransferFunctionNode.h"

namespace tinyvr
{
	class vrDefaultTransferFunctionImporter
	{
	public:
		vrDefaultTransferFunctionImporter() {};
		std::vector<std::string> Read(const std::string& dataFilePath);

		const std::vector<std::string>& GetDefaultTFNameList() { return m_DefaultTFNameList; }
		const std::vector<tfCNode>& GetDefaultTFByName(const std::string& key);
		const std::vector<tfCNode>& GetDefaultTFByIndex(uint32_t idx) { return m_DefaultTFParamList[idx]; }
	private:
		void InitTFNameListNData(const nlohmann::json& json);

	private:
		std::vector<std::vector<tfCNode>> m_DefaultTFParamList;
		std::vector<std::string> m_DefaultTFNameList;
	};
}