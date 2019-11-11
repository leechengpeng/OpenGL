#pragma once

namespace gl
{
	class RenderPass
	{
	public:
		RenderPass() {}
		virtual ~RenderPass() {}

		virtual void Init() = 0;
		virtual void Update(float passedTime, float deltaTime) = 0;
	};
}