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
		Skybox() : mSkyboxVao(), mCubemapTex()
		{

		}

		virtual void Init() override
		{
			glGenVertexArrays(1, &mCubeVao);
			glBindVertexArray(mCubeVao);
			{
				float cubeVertices[] = {
					// positions          // normals
					-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
					0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
					0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
					0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
					-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
					-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

					-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
					0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
					0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
					0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
					-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
					-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

					-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
					-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
					-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
					-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
					-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
					-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

					0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
					0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
					0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
					0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
					0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
					0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

					-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
					0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
					0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
					0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
					-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
					-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

					-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
					0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
					0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
					0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
					-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
					-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
				};

				unsigned int vbo;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
			}
			glBindVertexArray(0);

			glGenVertexArrays(1, &mSkyboxVao);
			glBindVertexArray(mSkyboxVao);
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
			mCubemapTex = LoadCubemap(faces);

			mShader.AttachShader(GL_VERTEX_SHADER, "Shader/skybox_vs.glsl");
			mShader.AttachShader(GL_FRAGMENT_SHADER, "Shader/skybox_fs.glsl");
			mShader.Link();

			mCubeShader.AttachShader(GL_VERTEX_SHADER, "Shader/cube_vs.glsl");
			mCubeShader.AttachShader(GL_FRAGMENT_SHADER, "Shader/cube_fs.glsl");
			mCubeShader.Link();

			glEnable(GL_DEPTH_TEST);
		}

		virtual void Update() override
		{
			auto& camera = Controller::Instance()->GetCamera();

			glm::mat4 modelMat = glm::mat4(1.0f);
			//glm::mat4 viewMat = glm::mat4(glm::mat3(camera.GetViewMatrix()));
			glm::mat4 viewMat = camera.GetViewMatrix();
			glm::mat4 projMat = glm::perspective(glm::radians(camera.Zoom), (float)1280 / (float)720, 0.1f, 100.0f);

			// 首先渲染天空盒再渲染物体，这样的效率比较低（模型可能会挡住大部分天空盒，但天空盒不可见的部分还是被渲染了一遍）
			// 因为上述原因，我们可以采用提前深度测试(Early Depth Testing)的方法来消除上述问题
			// cube
			{
				mCubeShader.Active();
				mCubeShader.SetMatrix("model", &modelMat[0][0]);
				mCubeShader.SetMatrix("view", &viewMat[0][0]);
				mCubeShader.SetMatrix("projection", &projMat[0][0]);
				glBindVertexArray(mCubeVao);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, mCubemapTex);
				glDrawArrays(GL_TRIANGLES, 0, 36);
				glBindVertexArray(0);
			}

			// skybox
			// change depth function so depth test passes when values are equal to depth buffer's content
			glDepthFunc(GL_LEQUAL);  
			{
				viewMat = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
				mShader.Active();
				mShader.SetMatrix("model", &modelMat[0][0]);
				mShader.SetMatrix("view", &viewMat[0][0]);
				mShader.SetMatrix("projection", &projMat[0][0]);

				glBindVertexArray(mSkyboxVao);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, mCubemapTex);
				glDrawArrays(GL_TRIANGLES, 0, 36);
				glBindVertexArray(0);
			}
			// set depth function back to default
			glDepthFunc(GL_LESS);
		}

	private:
		GLuint mCubeVao;
		GLuint mSkyboxVao;
		GLuint mCubemapTex;
		Shader mShader;
		Shader mCubeShader;
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