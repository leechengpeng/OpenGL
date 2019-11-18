#pragma once
#include "glm/glm.hpp"

namespace gl
{
	// Unity
	struct STime
	{
		glm::vec4 _Time;
		glm::vec4 _DeltaTime;

		STime() : _Time(), _DeltaTime() {}

		void SetTime(float passedTime)
		{
			_Time.x = passedTime / 20.f;
			_Time.y = passedTime;
			_Time.z = passedTime * 2.f;
			_Time.w = passedTime * 3.f;
		}

		void SetDeltalTime(float deltaTime)
		{
			assert(deltaTime != 0.f);
			_DeltaTime.x = deltaTime;
			_DeltaTime.y = 1 / deltaTime;
		}
	};

	struct SContext
	{
		GLuint width;
		GLuint height;
	};

	class RenderPass
	{
	public:
		RenderPass() {}
		virtual ~RenderPass() {}

		virtual void Init(const SContext& context) = 0;
		virtual void Update(const SContext& context, const STime& time) = 0;
	};
}