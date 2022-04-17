#pragma once

namespace tinyvr {

	class vrTimestep
	{
	public:
		vrTimestep(float time = 0.0f) : m_Time(time)
		{

		}

		operator float() const { return m_Time; }

		float GetSeconds() const { return m_Time; }
		float GetMilliseconds() const { return m_Time * 1000.0f; }

		void SetTimeEpoch(float time) { m_currentTime = time; }
		float GetTimeEpoch() { return m_currentTime; }

	private:
		float m_Time;
		float m_currentTime;
	};

}

