#include "layers/GMMlayer.h"

#include "tinyVR/core/vrEntrypoint.h"

class GMM_APP : public tinyvr::vrApplication
{
public:
	GMM_APP()
	{
		glm::uvec3 res = { 1, 1, 1 };
		glm::vec2 valueRange = { 0, 256 };
		uint32_t numIntervals = 256;
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
		res = { 500, 500, 100 };
		numIntervals = 256;

		{	// CLOUDf07
			//SGMMDataVarName = "CLOUDf07";
			//valueRange = glm::vec2(-0.001, 0.0025382);
			//rho = 0.7;
			//rho = 0.9;
			 //rho = 0.96;
			 //rho = 0.999;
			//e0 = 0;
		}
		{	// Pf07
			//SGMMDataVarName = "Pf07";
			//valueRange = glm::vec2(-6000, 2261.767578125);
			//rho = 0.72;
			//rho = 0.75;
			//rho = 0.8;
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
			//rho = 0.62;
			//rho = 0.68;
			//rho = 0.74;
			//e0 = 0;
		}
		{	// alter
			//SGMMDataVarName = "alter";
			//valueRange = glm::vec2(0, 256);
			//rho = 0.60;
			//rho = 0.67;
			//rho = 0.72;
			//rho = 0.95;
			//e0 = 1.3;

			//rho = 0.11;
			//rho = 0.135;
			//rho = 0.2;
			//e0 = 1.2;
		}

		// ================ Deep Water Impact ================
		//brickRes = { 4, 3, 2 };
		//res = { 300, 300, 300 };
		//numIntervals = 256;

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
			//rho = 0.95;
			//e0 = 0;
		}
		{
			//SGMMDataVarName = "v02";
			//valueRange = glm::vec2(0, 1);
			//rho = 0.6;
			//rho = 0.78;
			//rho = 0.95;
			//e0 = 0;
		}
		{	// stone 1
			//SGMMDataVarName = "stone1";
			//valueRange = glm::vec2(0, 256);
			//rho = 0.6;
			//rho = 0.72;
			//rho = 0.85;
			//e0 = 0;
		}
		{	// stone 2
			//SGMMDataVarName = "stone2";
			//valueRange = glm::vec2(0, 256);
			//rho = 0.6;
			//rho = 0.72;
			//rho = 0.85;

			//e0 = 0;
		}

		// ================ lap3d ================
		{
			//brickRes = { 3, 4, 2 };
			//res = { 256, 256, 512 };
			//numIntervals = 256;

			//SGMMDataVarName = "lap3d";
			//blockSize = 8;
			//valueRange = glm::vec2(0, 255);
		}
		{
			//brickRes = { 3, 4, 2 };
			//res = { 256, 256, 512 };
			//numIntervals = 256;

			//SGMMDataVarName = "lap3dF32";
			//blockSize = 8;
			//valueRange = glm::vec2(9.393370624568124e-20, 0.44742774963378906);
		}
		{
			//brickRes = { 3, 4, 2 };
			//res = { 256, 256, 512 };
			//numIntervals = 256;

			//SGMMDataVarName = "lap3dF32_b6";
			//blockSize = 6;
			//valueRange = glm::vec2(9.393370624568124e-20, 0.44742774963378906);

			//rho = 1.0;

			//rho = 0.65;

			//rho = 0.9;
			//rho = 0.88;
			//rho = 0.82;
		}
		{
			brickRes = { 4, 4, 7 };
			res = { 350, 350, 700 };
			numIntervals = 256;

			blockSize = 6;
			valueRange = glm::vec2(7.471811320796687e-19, 0.3604208827018738);
			SGMMDataVarName = "lap3dF32_700_b6";

			rho = 1.0;

			//rho = 0.65;

			//rho = 0.82;
			//rho = 0.88;
			//rho = 0.9;
			//rho = 1.0;
		}

		//brickRes = { 4, 4, 8 };
		//glm::uvec3 res = { 256, 256, 512 };
		//glm::vec2 valueRange = glm::vec2(0.0, 256.0);
		//uint32_t numIntervals = 256;

		//blockSize = 4;
		//valueRange = glm::vec2(7.471811320796687e-19, 0.3604208827018738);
		//SGMMDataVarName = "lap3dF32_512_b4";
		
		//rho = 0.5;
		//rho = 0.65;
		//rho = 0.8;
		// Fuck
		//rho = 0.9;
		//rho = 1.0;
		
		// ================ shock ================
		//brickRes = { 2, 3, 4 };
		//res = { 400, 400, 400 };
		//valueRange = glm::vec2(0.0, 256.0);
		//numIntervals = 256;

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