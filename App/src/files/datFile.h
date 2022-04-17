#pragma once

// class for reading raw file and dat file
// dat file:
//          ObjectFileName: xxxxxx.raw
//          TaggedFilename: ---
//          m_Res: X Y Z
//          SliceThickness: [x y z]
//          Format:         UCHAR

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iterator>
#include <memory>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace file {

	struct rawFileInfo
	{
		std::string m_RawPath;
		std::string m_TagPath;		// useless
		glm::uvec3 m_Res;
		glm::vec3 m_SliceThick;
	};

	// output stream operator overload for rawFileInfo
	inline std::ostream& operator<<(std::ostream& out, const rawFileInfo& info);

	class datFile
	{
	public:

		enum class datDataType : uint8_t
		{
			NONE, UCHAR, CHAR, USHORT, SHORT, INT, UINT,
			FLOAT, LONG, ULONG, LLONG, ULLONG, DOUBLE
		};

		using memPtr = std::unique_ptr<void, std::function<void(void*)>>;

		datFile(const std::string& path);
		~datFile() {}

		// ADT method
		bool Read();
		std::ostream& operator<<(std::ostream& out) const;
		std::vector<float> GetData();

		std::pair<glm::vec3, glm::vec3> SpatialSpan();
		glm::vec2 ValueSpan() { return glm::vec2(m_Min, m_Max); }
		datDataType GetType() { return m_RawType; }
		std::string GetTypeName() { return m_TypeName[static_cast<int>(m_RawType)]; }

		glm::mat4 GetModelMat() { return m_ModelMat; }
		std::string GetDatPath() { return m_Path; }
		std::string GetRawPath() { return m_RawInfo.m_RawPath; }

		glm::uvec3 GetResolution() { return m_RawInfo.m_Res; }
		
	private:

		bool ReadDatFile(const std::string& datPath, rawFileInfo& info);
		bool ReadRawFile(const std::string& rawPath, size_t bytes);
		std::pair<float, float> CalMinMax(datDataType type, size_t numItems);
		glm::mat4 CalModelMat(const glm::uvec3 res);

		static memPtr MemoryAlloc(size_t size);
		static size_t TypeBytes(datDataType type);

		static size_t numSamples(glm::uvec3 res) { return 1ull * res.x * res.y * res.z; }

		template <typename T>
		static std::pair<float, float> GetMinMax(void* p, size_t numItems);

	private:
		std::string m_Path;

		memPtr m_DP;
		float m_Min, m_Max;
		glm::mat4 m_ModelMat;

		datDataType m_RawType;
		rawFileInfo m_RawInfo;

		// auxiliary variables
		static std::map<std::string, datDataType> m_StrTypeMap;
		static std::vector<uint8_t> m_TypeBytes;
		static std::vector<std::string> m_TypeName;
	};
}

template<typename T>
inline std::pair<float, float> file::datFile::GetMinMax(void* dp, size_t numItems)
{
	auto p = static_cast<T*>(dp);
	auto mM = std::minmax_element(p, p + numItems);
	return std::make_pair(*(mM.first), *(mM.second));
}

inline std::ostream& operator<<(std::ostream& out, const file::datFile& file)
{
	file.operator<<(out);
	return out;
}