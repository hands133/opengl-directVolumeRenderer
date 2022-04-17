#pragma once

#include "vrTrackBall.h"

namespace tinyvr {

	class vrTrackBall_CHEN : public vrTrackBall
	{
		using Pos = glm::dvec2;

	public:
		vrTrackBall_CHEN(const Pos& p);
		~vrTrackBall_CHEN() = default;

		void mousePressed() override;

		bool IsMousePressed() { return this->m_Pressed; }
		void mouseMove() { this->dragMat = rotateStrategy(); }

	private:
		glm::fvec3 V1, V2;

		static float z(const Pos& p);
		static float f(float v);

		glm::mat4 rotateStrategy() override;
	};
}

