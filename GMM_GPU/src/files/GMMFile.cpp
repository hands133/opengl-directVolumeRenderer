#include "GMMFile.h"

#include <fstream>

#include "tinyVR.h"

namespace gmm
{
	GMMFile::GMMFile(int blockSide, int numInterval, int numBricks)
		: m_BlockSide(blockSide), m_numInterval(numInterval),
		m_numBricks(numBricks), m_maxKernelNumPerBin(4),
		m_maxBinNumPerBrick(0), m_resolution(glm::uvec3(0))
	{
		m_dataList.resize(m_numBricks);
	}

	GMMFile::~GMMFile() { }

	bool GMMFile::read(const std::string& baseDir)
	{
		std::string baseInfoPath = baseDir + "/" + std::to_string(m_numBricks) + "-blocks-info.txt";
		std::ifstream file(baseInfoPath, std::ios::in);
		if (!file)
		{
			TINYVR_FATAL("Open File {0} Failed!", baseInfoPath);
			TINYVR_ASSERT(false, "Open File Failed!");
			return false;
		}

		TINYVR_INFO("Open File {0}", baseInfoPath);

		std::vector<std::thread> tPool;
		for (int i = 0; i < m_numBricks; ++i)
		{
			std::string x, y, z;
			std::string dx, dy, dz;

			file >> x >> y >> z;
			file >> dx >> dy >> dz;

			auto O = glm::uvec3(std::stoi(x), std::stoi(y), std::stoi(z));
			auto R = glm::uvec3(std::stoi(dx), std::stoi(dy), std::stoi(dz));

			//ReadParts(baseDir, i, O, R);
			tPool.emplace_back(&GMMFile::ReadParts, this,
				std::cref(baseDir), i, O, R);
		}
		for (auto& t : tPool)	t.join();

		const auto& back = m_dataList.back();
		m_resolution = back.first.origin + back.first.span;

		return true;
	}

	const void GMMFile::WriteToDir(const std::filesystem::path& dirPath)
	{
		if (!std::filesystem::exists(dirPath))
			std::filesystem::create_directory(dirPath);
		// brick info
		auto brickInfoFilePath = dirPath / fmt::format("{0}-blocks-Info.txt", m_numBricks);
		std::ofstream oBrickInfoFile(brickInfoFilePath);

		for (auto& info : m_dataList)
		{
			auto& brickInfo = info.first;
			oBrickInfoFile << fmt::format("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\n",
				brickInfo.origin.x, brickInfo.origin.y, brickInfo.origin.z,
				brickInfo.span.x, brickInfo.span.y, brickInfo.span.z);
		}
		oBrickInfoFile.close();

		// SGMM info
		for (int i = 0; i < m_numBricks; ++i)
		{
			auto& sgmmBrickInfo = m_dataList[i].second;
			auto& sgmmBlockInfo = sgmmBrickInfo.BlocksListPerBlock;
			auto& sgmmBlockCoeffs = sgmmBrickInfo.GMMListPerBlock;

			auto sgmmCoeffsFilePath = dirPath / ("spatialGMM-" + std::to_string(i) + "-0.txt");
			auto sgmmBlocksInfoPath = dirPath / ("blockInfo-" + std::to_string(i) + "-8.txt");

			std::ofstream sgmmCoeffsFile(sgmmCoeffsFilePath);
			std::ofstream sgmmBlocksFile(sgmmBlocksInfoPath);
			// file header
			sgmmCoeffsFile << "block\tbin\tprobability\tgaussian number\tgmm\n";
			// each block
			uint32_t numBrickBlocks = sgmmBlockCoeffs.size();
			for (int blockIdx = 0; blockIdx < numBrickBlocks; ++blockIdx)
			{
				sgmmCoeffsFile << blockIdx << ' ';
				auto& sgmmCoeffsEachBlock = sgmmBlockCoeffs[blockIdx];
				auto& sgmmInfoEachBlock = sgmmBlockInfo[blockIdx];

				sgmmBlocksFile << fmt::format("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\n",
					sgmmInfoEachBlock[0], sgmmInfoEachBlock[1], sgmmInfoEachBlock[2],
					sgmmInfoEachBlock[3], sgmmInfoEachBlock[4], sgmmInfoEachBlock[5]);

				for (auto& binPair : sgmmCoeffsEachBlock)
				{
					int binIdx = binPair.first;
					auto& bin = binPair.second;

					sgmmCoeffsFile << fmt::format("{0} {1} {2} ", binIdx, bin.GetProb(), bin.GetKernelNum());
					for (int k = 0; k < bin.GetKernelNum(); ++k)
					{
						auto& kernel = bin.GetKernel(k);
						sgmmCoeffsFile << fmt::format("{0} {1} {2} {3} {4} {5} {6} ",
							kernel.weight,
							kernel.funcs.x.mean, kernel.funcs.y.mean, kernel.funcs.z.mean,
							kernel.funcs.x.var, kernel.funcs.y.var, kernel.funcs.z.var);
					}
					sgmmCoeffsFile << '\n';
				}
			}
			sgmmCoeffsFile.close();
			sgmmBlocksFile.close();
		}

		return void();
	}

	// save brick res and block info, delete gmm coeffs
	void GMMFile::ReleaseDataBuffer()
	{
		for (auto& p : m_dataList)
		{
			auto& gmmPerBrick = p.second.GMMListPerBlock;
			gmmPerBrick.swap(std::vector<std::vector<std::pair<uint32_t, GMMBin>>>());
		}
	}

	void GMMFile::ReadParts(const std::string& baseDir, int i, const glm::uvec3& O, const glm::uvec3& R)
	{
		auto& p = m_dataList[i];

		p.first.origin = O;
		p.first.span = R;

		auto blockR = R / glm::uvec3(m_BlockSide);
		p.first.numBlocks = blockR.x * blockR.y * blockR.z;
		if (p.first.numBlocks > m_maxBinNumPerBrick)
			m_maxBinNumPerBrick = p.first.numBlocks;

		std::string brickBlockFilePath = baseDir + "/blockInfo-" + std::to_string(i) + "-8.txt";
		std::string brickParamFilePath = baseDir + "/spatialGmm-" + std::to_string(i) + "-0.txt";
		bool GMMListPerBlockIsRead = ReadBlockPerBrick(brickBlockFilePath, p.first.origin, i);
		bool GMMParamPerBlockIsRead = ReadParamPerBrick(brickParamFilePath, p.first.origin, i);

		if (!(GMMListPerBlockIsRead && GMMParamPerBlockIsRead))
		{
			TINYVR_FATAL("Load parts {0} Failed!", ((!GMMListPerBlockIsRead) ? brickBlockFilePath : brickParamFilePath));
			TINYVR_ASSERT(false, "Load parts Failed!");
		}
	}

	bool GMMFile::ReadBlockPerBrick(const std::string& brickBlockPath, const glm::ivec3& offset, int index)
	{
		std::ifstream file(brickBlockPath, std::ios::in);
		if (!file)
		{
			TINYVR_FATAL("Read File {0} Failed", brickBlockPath);
			TINYVR_ASSERT(false, "Read File Failed!");
		}

		auto& p = m_dataList[index];
		auto& blockList = p.second.BlocksListPerBlock;
		blockList.resize(p.first.numBlocks);

		std::string x0, y0, z0;
		std::string dx, dy, dz;
		for (int i = 0; i < p.first.numBlocks; ++i)
		{
			file >> x0 >> y0 >> z0;
			file >> dx >> dy >> dz;

			blockList[i][0] = std::stoi(x0);
			blockList[i][1] = std::stoi(y0);
			blockList[i][2] = std::stoi(z0);
			blockList[i][3] = std::stoi(dx);
			blockList[i][4] = std::stoi(dy);
			blockList[i][5] = std::stoi(dz);
		}
		file.close();

		return true;
	}

	bool GMMFile::ReadParamPerBrick(const std::string& brickParamPath, const glm::ivec3& offset, int index)
	{
		std::ifstream file(brickParamPath, std::ios::in);
		if (!file)
		{
			TINYVR_FATAL("Read File {0} Failed!", brickParamPath);
			TINYVR_ASSERT(false, "Read File Failed!");
		}

		std::string str;
		std::getline(file, str);	// skip first line

		auto& p = m_dataList[index];
		auto& gmmList = p.second.GMMListPerBlock;
		gmmList.resize(p.first.numBlocks);

		std::string blockIdx, binIdx, binProb, paramGaussianN;
		std::string meansStr[3], varsStr[3];
		glm::dvec3 means, vars;
		std::string kernelWeight;

		uint32_t numKernels = 0;

		for (auto& item : gmmList)	item.reserve(1);

		while (!file.eof())
		{
			file >> blockIdx >> binIdx >> binProb >> paramGaussianN;
			unsigned int numGaussian = std::stoi(paramGaussianN);

			numKernels += numGaussian;

			GMMBin bin;
			bin.SetProb(std::stod(binProb));
			for (unsigned int i = 0; i < numGaussian; ++i)
			{
				file >> kernelWeight;

				file >> meansStr[0] >> meansStr[1] >> meansStr[2];
				file >> varsStr[0] >> varsStr[1] >> varsStr[2];

				means.x = std::stod(meansStr[0]);
				means.y = std::stod(meansStr[1]);
				means.z = std::stod(meansStr[2]);

				vars.x = std::stod(varsStr[0]);
				vars.y = std::stod(varsStr[1]);
				vars.z = std::stod(varsStr[2]);

				GaussianKernel kernel(std::stod(kernelWeight), means, vars);

				bin.Push(kernel);
			}
			gmmList[std::stoi(blockIdx)].emplace_back(
				std::make_pair(std::stoi(binIdx), bin));
		}
		m_dataList[index].first.numKernels = numKernels;

		file.close();

		return true;
	}

	std::ostream& operator<<(std::ostream& in, const GMMFile& file)
	{
		TINYVR_INFO("{1:=^{2}} {0} {1:=^{2}}", "GMMFile", "", 20);

		TINYVR_INFO("Total bricks : {0}", file.m_numBricks);
		TINYVR_INFO("Max block numbers : {0}", file.GetmaxBlockNum());
		TINYVR_INFO("Max kernel number in each bin : {0}", file.GetmaxKernelNum());
		TINYVR_INFO("For each brick, it contains kernels like");

		for (int i = 0; i < file.m_numBricks; ++i)
		{
			const auto& info = file.m_dataList[i].first;
			TINYVR_INFO("Brick {0:>3} has {1:>4} blocks, from [{2:>4}, {3:>4}, {4:>4}] to [{5:>4}, {6:>4}, {7:>4}], total {8:>6} kernels",
				i, info.numBlocks,
				info.origin.x, info.origin.y, info.origin.z,
				info.origin.x + info.span.x - 1,
				info.origin.y + info.span.y - 1,
				info.origin.z + info.span.z - 1,
				info.numKernels);
		}

		return in;
	}
}