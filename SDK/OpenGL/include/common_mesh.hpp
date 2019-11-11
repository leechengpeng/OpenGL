#pragma once
#include <glad/glad.h>

namespace gl
{
	class BaseMesh
	{
	public:
		BaseMesh() : mVao(0)
		{
		}

		virtual void Draw() const = 0;

	protected:
		GLuint mVao;
	};

	class PlaneMesh : public BaseMesh
	{
	public:
		PlaneMesh()
		{
			glGenVertexArrays(1, &mVao);
			glBindVertexArray(mVao);
			{
				float floor[] = {
					// vertex           // normal		// uv
					1.0f, 0.f,  1.0f,  0.f, 1.f, 0.f,  1.f, 1.f,
					-1.0f, 0.f,  1.0f,  0.f, 1.f, 0.f,  0.f, 1.f,
					-1.0f, 0.f, -1.0f,  0.f, 1.f, 0.f,  0.f, 0.f,

					1.0f, 0.f,  1.0f,  0.f, 1.f, 0.f,  1.f, 1.f,
					-1.0f, 0.f, -1.0f,  0.f, 1.f, 0.f,  0.f, 0.f,
					1.0f, 0.f, -1.0f,  0.f, 1.f, 0.f,  1.f, 0.f,
				};
				GLuint vbo;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(floor), &floor, GL_STATIC_DRAW);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
			}
			glBindVertexArray(0);
		}

		virtual void Draw() const override
		{
			glBindVertexArray(mVao);
			{
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
			glBindVertexArray(0);
		}

	private:
	};
}