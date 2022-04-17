#include "vrpch.h"
#include "vrTrackBall_CHEN.h"

namespace tinyvr {

	vrTrackBall_CHEN::vrTrackBall_CHEN(const Pos& p)
		: vrTrackBall(p), V1(0.0), V2(0.0)
	{
	}

	void vrTrackBall_CHEN::mousePressed()
	{
		vrTrackBall::mousePressed();

		V1 = glm::vec3(m_PressedPos.x, m_PressedPos.y, vrTrackBall_CHEN::z(m_PressedPos));
		V1 = glm::normalize(V1);
	}

	float vrTrackBall_CHEN::z(const Pos& p)
	{
		float d2 = glm::dot(p, p);
		float r = 1.0f;

		if (d2 < r / 2.0f)      return glm::sqrt(r * r - d2);
		else                    return r / (2.0f * glm::sqrt(d2));
	}

	float vrTrackBall_CHEN::f(float v)
	{
		if (v <= 0.0f)      return 0.0f;
		else                return glm::min(v, 1.0f) * PI_F_DIV2;
	}

	glm::mat4 vrTrackBall_CHEN::rotateStrategy()
	{
		if (m_CurrentPosRef == m_PressedPos)	return glm::mat4(1.0);

		float theta = 0.0f;
		glm::vec3 rotateAxis = glm::vec3(1.0f);

		Pos dir = glm::normalize(m_CurrentPosRef - m_PressedPos);

		V2 = glm::vec3(m_CurrentPosRef.x, m_CurrentPosRef.y, vrTrackBall_CHEN::z(m_CurrentPosRef));
		V2 = glm::normalize(V2);

		theta = glm::acos(glm::dot(V1, V2));
		rotateAxis = glm::cross(V1, V2);

		return glm::rotate(glm::mat4(1.0), theta, rotateAxis);
	}
}