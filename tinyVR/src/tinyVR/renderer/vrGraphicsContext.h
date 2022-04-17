#pragma once

namespace tinyvr {

	class vrGraphicsContext
	{
	public:
		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;
	};

}
