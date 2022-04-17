#include "layers/GMMlayer.h"

#include "tinyVR/core/vrEntrypoint.h"

class GMM_APP : public tinyvr::vrApplication
{
public:
	GMM_APP()
	{
		glm::uvec3 brickRes = { 4, 3, 2 };
		uint32_t blockSize = 8;

		//std::string gmmFileDir = "resources/data/alter/alter-output-8-1-sgmm";
		//glm::uvec3 res = { 500, 500, 100 };
		//glm::vec2 valueRange(0.0f, 255.0f);
		//uint32_t numIntervals = 256;

		//std::string gmmFileDir = "resources/data/stone/stone-output-8-1-stone-sgmm";
		//glm::uvec3 res = { 300, 300, 300 };
		//glm::vec2 valueRange(0.0f, 255.0f);
		//uint32_t numIntervals = 256;

		//std::string gmmFileDir = "resources/data/stone/stone-output-8-1-stone2-sgmm";
		//glm::uvec3 res = { 300, 300, 300 };
		//glm::vec2 valueRange(0.0f, 255.0f);
		//uint32_t numIntervals = 256;

		//std::string gmmFileDir = "resources/data/plane/plane-output-8-1-plane-sgmm";
		//glm::uvec3 res = { 512, 512, 512 };
		//glm::vec2 valueRange(0.0f, 255.0f);
		//uint32_t numIntervals = 256;

		brickRes = { 3, 4, 2 };
		std::string gmmFileDir = "resources/data/lap3d/lap3d-output-8-1-lap3d-sgmm";
		glm::uvec3 res = { 256, 256, 512 };
		glm::vec2 valueRange(0.0f, 255.0f);
		uint32_t numIntervals = 256;

		//brickRes = { 2, 3, 4 };
		//std::string gmmFileDir = "resources/data/shock/shock-output-8-1-shock-sgmm";
		//glm::uvec3 res = { 400, 400, 400 };
		//glm::vec2 valueRange(0.0f, 255.0f);
		//uint32_t numIntervals = 256;

		// SGMM By myself
		//brickRes = { 4, 3, 2 };
		//std::string gmmFileDir = "S:/SGMM Data/snd";
		//glm::uvec3 res = { 300, 300, 300 };
		//glm::vec2 valueRange{ 43.0011, 185475.390625 };
		//uint32_t numIntervals = 256;

		//brickRes = { 3, 4, 2 };
		//std::string gmmFileDir = "S:/SGMM Data/lap3d";
		//glm::uvec3 res = { 256, 256, 512 };
		//glm::vec2 valueRange{ 0.0, 256.0 };
		//uint32_t numIntervals = 256;

		PushLayer(new gmm::vrGMMLayer(gmmFileDir, brickRes, blockSize, res, valueRange, numIntervals));
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