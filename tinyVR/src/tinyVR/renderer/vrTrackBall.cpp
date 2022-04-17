#include "vrpch.h"
#include "vrTrackBall.h"

namespace tinyvr {

	vrTrackBall::vrTrackBall(const Pos& p)
		: m_Pressed(false), m_CurrentPosRef(p),
		m_PressedPos(0.0), m_ReleasedPos(0.0),
		dragMat(1.0)
	{

	}

	void vrTrackBall::mousePressed()
	{
		m_Pressed = true;
		m_PressedPos = m_CurrentPosRef;
	}

	void vrTrackBall::mouseReleased()
	{
		m_Pressed = false;
		m_ReleasedPos = m_CurrentPosRef;

		dragMat = glm::mat4(1.0f);
	}
	
}