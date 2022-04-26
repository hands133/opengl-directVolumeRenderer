#include "layers/GMMlayer.h"

#include "tinyVR/core/vrEntrypoint.h"

class GMM_APP : public tinyvr::vrApplication
{
public:
	GMM_APP()
	{
		glm::uvec3 brickRes = { 4, 3, 2 };
		uint32_t blockSize = 8;

		float rho = 0.0;
		float e0 = 0;
		std::filesystem::path SGMMDataSetBaseDir = "S:/SGMM Data";
		std::filesystem::path SGMMDataSetPicDir = "S:/SGMM Pic";
		std::string SGMMDataVarName = "";

		// SGMM By myself
		// ================ Isabel ================
		brickRes = { 4, 3, 2 };
		glm::uvec3 res = { 500, 500, 100 };
		glm::vec2 valueRange(0.0f);
		uint32_t numIntervals = 256;

		{	// alter
			SGMMDataVarName = "alter";
			valueRange = glm::vec2(0, 256);
			rho = 0.7;
			rho = 0.9;
			rho = 0.999;
			e0 = 0;
		}

		{	// CLOUDf07
			//SGMMDataVarName = "CLOUDf07";
			//valueRange = glm::vec2(-0.001, 0.0025382);
			//rho = 0.7;
			//rho = 0.9;
			//rho = 0.999;
			//e0 = 0;
		}
		{	// Pf07
			//SGMMDataVarName = "Pf07";
			//valueRange = glm::vec2(-6000, 2261.767578125);
			//rho = 0.72;
			//rho = 0.75;
			//rho = 0.8;
			//rho = 1.0;
			//e0 = 0;
		}
		{	// QVAPORf07
			//SGMMDataVarName = "QVAPORf07";
			//valueRange = glm::vec2(-0.01, 0.022782223299);
			//rho = 0.5;
			//rho = 0.62;
			//rho = 0.69;
			//e0 = 0;
		}
		{	// TCf07
			//SGMMDataVarName = "TCf07";
			//valueRange = glm::vec2(-81.03565979, 35.01);
			//rho = 1.0;
			//rho = 0.62;
			//rho = 0.69;
			//e0 = 0;
		}

		// ================ Deep Water Impact ================
		//brickRes = { 4, 3, 2 };
		//glm::uvec3 res = { 300, 300, 300 };
		//glm::vec2 valueRange(0.0f);
		//uint32_t numIntervals = 256;

		{	// snd
			//SGMMDataVarName = "snd";
			//valueRange = glm::vec2(43.0011, 185475.390625);
			//rho = 0.6;
			//rho = 0.78;
			//rho = 0.85;
			//e0 = 0;
		}
		{	// tev
			//SGMMDataVarName = "tev";
			//valueRange = glm::vec2(0.018699018, 0.40897539258);
			//rho = 0.6;
			//rho = 0.78;
			//rho = 0.85;
			//e0 = 0;
		}
		{
			//SGMMDataVarName = "v02";
			//valueRange = glm::vec2(0, 1);
			//rho = 0.6;
			//rho = 0.78;
			//rho = 0.85;
			//e0 = 0;
		}

		// ================ lap3d ================
		//brickRes = { 3, 4, 2 };
		//glm::uvec3 res = { 256, 256, 512 };
		//glm::vec2 valueRange = glm::vec2(0.0, 256.0);
		//uint32_t numIntervals = 256;

		//SGMMDataVarName = "lap3d";
		//rho = 0.5;
		//rho = 0.65;
		//rho = 0.8;
		//rho = 1.0;
		
		// ================ shock ================
		//brickRes = { 2, 3, 4 };
		//glm::uvec3 res = { 400, 400, 400 };
		//glm::vec2 valueRange = glm::vec2(0.0, 256.0);
		//uint32_t numIntervals = 256;

		//SGMMDataVarName = "shock";
		//rho = 0.6;
		//rho = 0.75;
		//rho = 0.85;

		// ================ plane ================
		//brickRes = { 4, 3, 2 };
		//glm::uvec3 res = { 512, 512, 512 };
		//glm::vec2 valueRange = glm::vec2(0.0, 256.0);
		//uint32_t numIntervals = 256;
		//
		//SGMMDataVarName = "plane";
		//rho = 0.6;
		//rho = 0.75;
		//rho = 0.85;

		SGMMDataSetBaseDir = SGMMDataSetBaseDir / SGMMDataVarName;
		SGMMDataSetPicDir = SGMMDataSetPicDir / SGMMDataVarName;
		
		PushLayer(new gmm::vrGMMLayer(
			SGMMDataSetBaseDir.string(), 
			brickRes, blockSize, res, valueRange, numIntervals,
			rho, e0, SGMMDataSetPicDir));
	}

	~GMM_APP()
	{

	}

private:
};

tinyvr::vrApplication* tinyvr::CreateApplication()
{
	return new GMM_APP();
}