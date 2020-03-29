#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "controller.hpp"

namespace gl
{
	class Lighting : public RenderPass
	{
	public:
		virtual void Init(const SContext& context) override
		{
			// floor data buffer
			glGenVertexArrays(1, &mFloorVao);
			glBindVertexArray(mFloorVao);
			{
				float floor[] = {
					 10.0f, -0.5f,  10.0f,
					-10.0f, -0.5f,  10.0f,
					-10.0f, -0.5f, -10.0f,

					 10.0f, -0.5f,  10.0f,
					-10.0f, -0.5f, -10.0f,
					 10.0f, -0.5f, -10.0f,
				};
				GLuint vbo;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(floor), &floor, GL_STATIC_DRAW);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			}
			glBindVertexArray(0);

			mShader.AttachShader(GL_VERTEX_SHADER, "Shader/lighting_vs.glsl");
			mShader.AttachShader(GL_FRAGMENT_SHADER, "Shader/lighting_fs.glsl");
			mShader.Link();

			//Controller::Instance()->ResetCamera(glm::vec3(0.f, 1.f, 2.f), glm::vec3(0.f, 1.f, 0.f));
		}

		virtual void Update(const SContext& context, const STime& time) override
		{
			auto& camera = Controller::Instance()->GetCamera();

			glm::mat4 modelMat = glm::mat4(1.0f);
			glm::mat4 viewMat = camera.GetViewMatrix();
			glm::mat4 projMat = glm::perspective(glm::radians(camera.Zoom), (float)1280 / (float)720, 0.1f, 100.0f);

			// lighting
			mShader.Active();
			mShader.SetMatrix("model", &modelMat[0][0]);
			mShader.SetMatrix("view", &viewMat[0][0]);
			mShader.SetMatrix("projection", &projMat[0][0]);
			mShader.SetValue("camPosition", camera.Position);
			// render
			glBindVertexArray(mFloorVao);
			{
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
			glBindVertexArray(0);
		}

	private:
		GLuint mFloorVao;
		Shader mShader;
	};
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	gl::Controller::Instance()->MouseCallback(xpos, ypos);
}

int main()
{
	gl::Engine engine;
	engine.Init(1280, 720);
	engine.SetFrameBufferSizeCallback(framebuffer_size_callback);
	engine.SetCursorPosCallback(mouse_callback);

	gl::Lighting lighting;
	engine.AddPass(&lighting);
	engine.Render();

	return 0;
}