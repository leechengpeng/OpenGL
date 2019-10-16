#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "controller.hpp"
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gl
{
	class Skybox : public RenderPass
	{
	public:
		Skybox() : mVao(), mCubemapTex()
		{

		}

		virtual void Init() override
		{
			float skyboxVertices[] = {        
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				1.0f,  1.0f, -1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				1.0f, -1.0f,  1.0f
			};

			glGenVertexArrays(1, &mVao);
			glBindVertexArray(mVao);
			{
				GLuint vbo;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			}
			glBindVertexArray(0);

			std::vector<std::string> faces
			{
				"../Resource/Skybox/right.jpg",
				"../Resource/Skybox/left.jpg",
				"../Resource/Skybox/top.jpg",
				"../Resource/Skybox/bottom.jpg",
				"../Resource/Skybox/front.jpg",
				"../Resource/Skybox/back.jpg"
			};
			mCubemapTex = loadCubemap(faces);

			mShader.AttachShader(GL_VERTEX_SHADER, "Shader/skybox_vs.glsl");
			mShader.AttachShader(GL_FRAGMENT_SHADER, "Shader/skybox_fs.glsl");
			mShader.Link();
		}

		virtual void Update() override
		{
			auto& camera = Controller::Instance()->GetCamera();

			glm::mat4 modelMat = glm::mat4(1.0f);
			glm::mat4 viewMat = glm::mat4(glm::mat3(camera.GetViewMatrix()));
			glm::mat4 projMat = glm::perspective(glm::radians(camera.Zoom), (float)1280 / (float)720, 0.1f, 100.0f);

			mShader.Active();
			mShader.SetMatrix("model", &modelMat[0][0]);
			mShader.SetMatrix("view", &viewMat[0][0]);
			mShader.SetMatrix("projection", &projMat[0][0]);

			glBindVertexArray(mVao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mCubemapTex);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
		}

	private:
		GLuint mCubemapTex;
		GLuint mVao;
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

	gl::Skybox skybox;
	engine.AddPass(&skybox);
	engine.Render();

	return 0;
}