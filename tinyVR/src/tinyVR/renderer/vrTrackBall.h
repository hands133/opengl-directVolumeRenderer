#pragma once

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace tinyvr {

	class vrTrackBall
	{
		using Pos = glm::dvec2;

	public:
		vrTrackBall(const Pos& p);
		virtual ~vrTrackBall() {}

		virtual void mousePressed();
		virtual void mouseReleased();

		glm::mat4 getDragMat() { return dragMat; }

	protected:

		bool m_Pressed;
		const Pos& m_CurrentPosRef;
		Pos m_PressedPos, m_ReleasedPos;

		glm::mat4 dragMat;

		virtual glm::mat4 rotateStrategy() = 0;
	};
}
