#pragma once

#include "GMM.h"
#include "tinyVR.h"

#include <string>

namespace gmm
{
	struct GMMBrickInfo
	{
		glm::uvec3 origin = glm::uvec3(0);
		glm::uvec3 span = glm::uvec3(0);
		unsigned int numBlocks = 0;
		unsigned int numKernels = 0;
	};

	struct GMMBrickData
	{
		std::vector<std::vector<GMMBin>> GMMListPerBlock;
		std::vector<std::array<int, 6>> BlocksListPerBlock;
	};

	// spatial GMM
	class GMMFile
	{
	public:
		GMMFile(int blockSide = 8, int numInterval = 256, int numBricks = 24);
		~GMMFile();

		bool read(const std::string& baseInfoDir);

		int GetmaxKernelNum() const { return m_maxKernelNumPerBin; }
		int GetmaxBlockNum() const { return m_maxBinNumPerBrick; }
		const std::pair<GMMBrickInfo, GMMBrickData>& getBrick(int index) { return m_dataList[index]; }

		const glm::ivec3 getBlockBrickCoord(uint32_t brickIdx, uint32_t brickBlockIdx) const
		{
			auto& blockInfo = m_dataList[brickIdx].second.BlocksListPerBlock[brickBlockIdx];
			return glm::ivec3(blockInfo[0], blockInfo[1], blockInfo[2]);
		}

		const glm::ivec3 getBlockSize(uint32_t brickIdx, uint32_t brickBlockIdx) const
		{
			auto& blockInfo = m_dataList[brickIdx].second.BlocksListPerBlock[brickBlockIdx];
			return glm::ivec3(blockInfo[3], blockInfo[4], blockInfo[5]);
		}

		void UpdateGMMBinCoeffs(uint32_t brickIdx, uint32_t blockIdx, const std::vector<GMMBin>& binList)
		{
			auto& gmmCoeffsList = m_dataList[brickIdx].second.GMMListPerBlock[blockIdx];
			TINYVR_ASSERT(binList.size() == gmmCoeffsList.size(), "List Size Doesn't match!");

			std::copy(binList.begin(), binList.end(), gmmCoeffsList.begin());
		}

		std::vector<std::pair<GMMBrickInfo, GMMBrickData>>::const_iterator begin()	const { return m_dataList.begin(); }
		std::vector<std::pair<GMMBrickInfo, GMMBrickData>>::const_iterator end()	const { return m_dataList.end(); }

		const std::pair<GMMBrickInfo, GMMBrickData>& operator[](size_t i) const { return m_dataList[i]; }
		std::pair<GMMBrickInfo, GMMBrickData>& operator[](size_t i) { return m_dataList[i]; }

		friend std::ostream& operator<<(std::ostream& in, const GMMFile& file);

		const void WriteToDir(const std::filesystem::path& dirPath);

	private:

		// function for reconstruction
		size_t idxBrickPos(const glm::uvec3& samplePoint) const;
		float evalAtPos(const glm::uvec3& samplePoint) const;

		void ReadParts(const std::string& baseDir, int i, const glm::uvec3& O, const glm::uvec3& R);
		bool ReadBlockPerBrick(const std::string& brickBlockPath, const glm::ivec3& offset, int index);
		bool ReadParamPerBrick(const std::string& brickParamPath, const glm::ivec3& offset, int index);

	private:
		int m_BlockSide, m_numInterval, m_numBricks;
		unsigned int m_maxKernelNumPerBin;
		unsigned int m_maxBinNumPerBrick;

		glm::uvec3 m_resolution;

		std::vector<std::pair<GMMBrickInfo, GMMBrickData>> m_dataList;
	};
}

