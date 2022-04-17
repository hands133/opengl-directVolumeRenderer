#include "datFile.h"

#include <fstream>

namespace file {

	std::vector<uint8_t> datFile::m_TypeBytes = { 0, 1, 1, 2, 2, 4, 4, 4, 4, 4, 8, 8, 8 };

	std::map<std::string, datFile::datDataType> datFile::m_StrTypeMap =
	{
		{ "NONE",		datFile::datDataType::NONE		},
		{ "UCHAR",		datFile::datDataType::UCHAR		},
		{ "CHAR",		datFile::datDataType::CHAR		},
		{ "USHORT",		datFile::datDataType::USHORT	},
		{ "SHORT",		datFile::datDataType::SHORT		},
		{ "INT",		datFile::datDataType::INT		},
		{ "UINT",		datFile::datDataType::UINT		},
		{ "FLOAT",		datFile::datDataType::FLOAT		},
		{ "LONG",		datFile::datDataType::LONG		},
		{ "ULONG",		datFile::datDataType::ULONG		},
		{ "LLONG",		datFile::datDataType::LLONG		},
		{ "ULLONG",		datFile::datDataType::ULLONG	},
		{ "DOUBLE",		datFile::datDataType::DOUBLE	}
	};

	std::vector<std::string> datFile::m_TypeName = 
	{
		"NONE", "UCHAR", "CHAR", "USHORT", "SHORT",
		"INT", "UINT", "FLOAT", "LONG",
		"ULONG", "LLONG", "ULLONG", "DOUBLE"
	};

	// method
	datFile::datFile(const std::string& path)
		: m_DP(nullptr), m_Min(FLT_MAX), m_Max(FLT_MIN), m_ModelMat(1.0f), 
		m_RawType(datFile::datDataType::NONE),m_Path(path) { }

	bool datFile::Read()
	{
		std::string datPath = m_Path;

		bool readDatSucc = ReadDatFile(datPath, m_RawInfo);
		if (readDatSucc)
			m_ModelMat = CalModelMat(m_RawInfo.m_Res);
		else
			return false;

		size_t bytesRequired = numSamples(m_RawInfo.m_Res) * TypeBytes(m_RawType);
		m_DP = MemoryAlloc(bytesRequired);
		bool readRawSucc = ReadRawFile(m_RawInfo.m_RawPath, bytesRequired);
		return readDatSucc && (nullptr != m_DP) && readRawSucc;
	}

	std::ostream& datFile::operator<<(std::ostream& out) const
	{
		out << "=====================================" << std::endl;
		out << m_RawInfo;
		out << "Data Type : \t" << m_TypeName[static_cast<size_t>(m_RawType)] << std::endl;
		out << "Element size : \t" << numSamples(m_RawInfo.m_Res) << " Items" << std::endl;

		if (m_DP == nullptr)
			out << "[WARNING] Data description file and raw data have not been loaded!\n";
		else
		{
			out << "raw file loaded, total " << numSamples(m_RawInfo.m_Res) * TypeBytes(m_RawType) << " Bytes.\n";
			out << "data locates at " << m_DP.get() << std::endl;
			out << "value range : \t[ " << m_Min << " , " << m_Max << " ]\n";
		}

		out << "=====================================" << std::endl;
		return out;
	}

	std::pair<glm::vec3, glm::vec3> datFile::SpatialSpan()
	{
		auto sS = m_RawInfo.m_SliceThick * glm::vec3(m_RawInfo.m_Res - glm::uvec3(1));
		auto sMid = sS / glm::vec3(2.0);
		auto mm = -sMid;
		auto MM = sMid;

		return std::pair<glm::vec3, glm::vec3>(mm, MM);
	}

	std::vector<float> datFile::GetData()
	{
		int N = numSamples(m_RawInfo.m_Res);
		std::vector<float>  bufferLP(N, 0.0f);
		auto* d = m_DP.get();

		switch (m_RawType)
		{
			case file::datFile::datDataType::NONE:
				bufferLP.clear();
				break;
			case file::datFile::datDataType::UCHAR:
			{
				auto* puc = static_cast<unsigned char*>(d);
				std::copy(puc, puc + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::CHAR:
			{
				auto* pc = static_cast<char*>(d);
				std::copy(pc, pc + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::USHORT:
			{
				auto* pus = static_cast<unsigned short*>(d);
				std::copy(pus, pus + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::SHORT:
			{
				auto* ps = static_cast<short*>(d);
				std::copy(ps, ps + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::INT:
			{
				auto* pi = static_cast<int*>(d);
				std::copy(pi, pi + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::UINT:
			{
				auto* pui = static_cast<unsigned int*>(d);
				std::copy(pui, pui + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::FLOAT:
			{
				auto* pf = static_cast<float*>(d);
				std::copy(pf, pf + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::LONG:
			{
				auto* pl = static_cast<long*>(d);
				std::copy(pl, pl + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::ULONG:
			{
				auto* pul = static_cast<unsigned long*>(d);
				std::copy(pul, pul + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::LLONG:
			{
				auto* pll = static_cast<long long*>(d);
				std::copy(pll, pll + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::ULLONG:
			{
				auto* pull = static_cast<unsigned long long*>(d);
				std::copy(pull, pull + N, bufferLP.begin());
			}
			break;
			case file::datFile::datDataType::DOUBLE:
			{
				auto* pd = static_cast<double*>(d);
				std::copy(pd, pd + N, bufferLP.begin());
			}
		}

		return bufferLP;
	}

	bool datFile::ReadDatFile(const std::string& datPath, rawFileInfo& info)
	{
		std::ifstream file(datPath, std::ios::in);
		if (!file)
		{
			file.close();
			std::cerr << "[ERROR] Open Data Description File " << datPath << " Failed!\n";
			return false;
		}

		std::string item;	// iter

		// Raw file name
		file >> item;
		file >> info.m_RawPath;
		// Tagged file name
		file >> item;
		file >> info.m_TagPath;
		// Resolution
		file >> item;
		file >> item;
		info.m_Res.x = std::stoi(item);
		file >> item;
		info.m_Res.y = std::stoi(item);
		file >> item;
		info.m_Res.z = std::stoi(item);
		// Sliche thickness
		file >> item;
		file >> item;
		info.m_SliceThick.x = std::stof(item);
		file >> item;
		info.m_SliceThick.y = std::stof(item);
		file >> item;
		info.m_SliceThick.z = std::stof(item);
		// Format
		file >> item;
		file >> item;

		auto typeIter = m_StrTypeMap.find(item);
		if (typeIter == m_StrTypeMap.end())
		{
			file.close();
			std::cerr << "[ERROR] Invalid typename of key 'Format' : " << item << std::endl;
			return false;
		}
		m_RawType = typeIter->second;

		file.close();

		info.m_RawPath = datPath.substr(0,
			std::min(datPath.rfind('\\'), datPath.rfind('/')) + 1) + info.m_RawPath;

		return true;
	}

	bool datFile::ReadRawFile(const std::string& rawPath, size_t bytes)
	{
		std::ifstream file(rawPath, std::ios::binary | std::ios::in);
		if (!file)
		{
			std::cerr << "[ERROR] Open Data Raw File " << rawPath << " failed!\n";
			file.close();
			return false;
		}

		try
		{
			file.read(static_cast<char*>(m_DP.get()), bytes);
			auto [m, M] = CalMinMax(m_RawType, numSamples(m_RawInfo.m_Res));
			m_Min = m;
			m_Max = M;
		}
		catch (const std::exception&)
		{
			std::cerr << "[ERROR] Load data from raw file " << rawPath << " Failed!\n";
			file.close();
			return false;
		}

		file.close();

		return true;
	}

	std::pair<float, float> datFile::CalMinMax(datFile::datDataType type, size_t numItems)
	{
		auto* dp = m_DP.get();
		std::pair<float, float> mM(FLT_MAX, FLT_MIN);
		switch (type)
		{
		case file::datFile::datDataType::NONE:
			mM = { 0.0f, 0.0f };
			break;
		case file::datFile::datDataType::UCHAR:
			mM = GetMinMax<uint8_t>(dp, numItems);
			break;
		case file::datFile::datDataType::CHAR:
			mM = GetMinMax<int8_t>(dp, numItems);
			break;
		case file::datFile::datDataType::USHORT:
			mM = GetMinMax<uint16_t>(dp, numItems);
			break;
		case file::datFile::datDataType::SHORT:
			mM = GetMinMax<int16_t>(dp, numItems);
			break;
		case file::datFile::datDataType::INT:
			mM = GetMinMax<int32_t>(dp, numItems);
			break;
		case file::datFile::datDataType::UINT:
			mM = GetMinMax<uint32_t>(dp, numItems);
			break;
		case file::datFile::datDataType::FLOAT:
			mM = GetMinMax<float>(dp, numItems);
			break;
		case file::datFile::datDataType::LONG:
			mM = GetMinMax<int32_t>(dp, numItems);
			break;
		case file::datFile::datDataType::ULONG:
			mM = GetMinMax<uint32_t>(dp, numItems);
			break;
		case file::datFile::datDataType::LLONG:
			mM = GetMinMax<int64_t>(dp, numItems);
			break;
		case file::datFile::datDataType::ULLONG:
			mM = GetMinMax<uint64_t>(dp, numItems);
			break;
		case file::datFile::datDataType::DOUBLE:
			mM = GetMinMax<double>(dp, numItems);
			break;
		}

		return mM;
	}

	glm::mat4 datFile::CalModelMat(const glm::uvec3 res)
	{
		int maxAxis = std::max({ m_RawInfo.m_Res.x, m_RawInfo.m_Res.y,  m_RawInfo.m_Res.z });
		auto Scale = glm::vec3(m_RawInfo.m_Res) / glm::vec3(maxAxis);

		auto mat = glm::scale(glm::mat4(1.0f), Scale);
		return mat;
	}

	datFile::memPtr datFile::MemoryAlloc(size_t size)
	{
		auto freeFunc = [](void* p) { free(p); };
		void* p = nullptr;
		try
		{
			p = malloc(size);
		}
		catch (const std::exception& e)
		{
			std::cerr << "[ERROR] Memory alloc failed : " << e.what() << "\n";
			free(p);
			p = nullptr;
		}
		return memPtr(p, freeFunc);
	}

	size_t datFile::TypeBytes(datDataType type)
	{
		return m_TypeBytes[static_cast<int>(type)];
	}

	std::ostream& operator<<(std::ostream& out, const rawFileInfo& info)
	{
		out << "raw file path: \t" << info.m_RawPath << std::endl;
		out << "tag file path: \t" << info.m_TagPath << std::endl;
		out << "m_Res : \t[ " << info.m_Res.x << ", " << info.m_Res.y << ", " << info.m_Res.z << " ]" << std::endl;
		out << "slice thick : \t[ " << info.m_SliceThick.x << ", " << info.m_SliceThick.y << ", " << info.m_SliceThick.z << " ]" << std::endl;
		return out;
	}
}