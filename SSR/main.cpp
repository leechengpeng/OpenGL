#include "engine.hpp"
#include "shader.hpp"
#include "model.hpp"
#include "common.hpp"
#include "controller.hpp"
#include "common_mesh.hpp"
#include <vector>
#include <random>

GLuint SCR_WIDTH = 1280;
GLuint SCR_HEIGHT = 720;

namespace gl
{
	// TODO 优化引擎架构：添加前向渲染、延迟渲染的引擎架构
	GLuint gBuffer = 0;
	GLuint gPos, gNormal, gAlbedoSpec;

	class GeometryPass : public RenderPass
	{
	public:
		GeometryPass() : mShader(), mModel("../Resource/Model/Nanosuit/nanosuit.obj")
		{

		}

		virtual void Init() override
		{
			glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
			glEnable(GL_DEPTH_TEST);

			// G-Buffer, 3 texture:
			// 1. Position (RGB, 16F)
			// 2. Normals (RGB, 16F)
			// 3. Color (RGB) + Specular(A)
			glGenFramebuffers(1, &gBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
			{
				// Position Buffer
				glGenTextures(1, &gPos);
				glBindTexture(GL_TEXTURE_2D, gPos);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPos, 0);
				// Normal Buffer
				glGenTextures(1, &gNormal);
				glBindTexture(GL_TEXTURE_2D, gNormal);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
				// Color & Specular color buffer
				glGenTextures(1, &gAlbedoSpec);
				glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

				// Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
				GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
				glDrawBuffers(3, attachments);

				// Depth buffer?
				GLuint rboDepth;
				glGenRenderbuffers(1, &rboDepth);
				glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

				// Check the framebuffer status
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					throw std::exception("Framebuffer not complete!");
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Init shader
			mShader.AttachShader(GL_VERTEX_SHADER, "Shaders/g_buffer.vs.glsl");
			mShader.AttachShader(GL_FRAGMENT_SHADER, "Shaders/g_buffer.fs.glsl");
			mShader.Link();

			// Init model position
			mModelPositions.push_back(glm::vec3(-3.0, -3.0, -3.0));
			mModelPositions.push_back(glm::vec3(0.0, -3.0, -3.0));
			mModelPositions.push_back(glm::vec3(3.0, -3.0, -3.0));
			mModelPositions.push_back(glm::vec3(-3.0, -3.0, 0.0));
			mModelPositions.push_back(glm::vec3(0.0, -3.0, 0.0));
			mModelPositions.push_back(glm::vec3(3.0, -3.0, 0.0));
			mModelPositions.push_back(glm::vec3(-3.0, -3.0, 3.0));
			mModelPositions.push_back(glm::vec3(0.0, -3.0, 3.0));
			mModelPositions.push_back(glm::vec3(3.0, -3.0, 3.0));

			Controller::Instance()->ResetCamera(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		virtual void Update() override
		{
			auto& camera = Controller::Instance()->GetCamera();

			// Geometry Pass: render scene's geometry/color data into gbuffer
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glm::mat4 view = camera.GetViewMatrix();
				glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
				mShader.Active();
				mShader.SetMatrix("view", &view[0][0]);
				mShader.SetMatrix("projection", &projection[0][0]);

				for (auto& pos : mModelPositions)
				{
					glm::mat4 model = glm::mat4(1.0f);
					model = glm::translate(model, pos);
					model = glm::scale(model, glm::vec3(0.25f));
					mShader.SetMatrix("model", &model[0][0]);

					mModel.Draw(mShader);
				}
				mModel.Draw(mShader);

				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.f, -3.f, 0.f));
				model = glm::scale(model, glm::vec3(10.f));
				mShader.SetMatrix("model", &model[0][0]);
				mFloor.Draw();
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

	private:
		Model		mModel;
		Shader		mShader;
		PlaneMesh	mFloor;
		std::vector<glm::vec3> mModelPositions;
	};

	class DeferredLightingPass : public RenderPass
	{
	public:
		virtual void Init() override
		{
			glGenVertexArrays(1, &mQuadVao);
			glBindVertexArray(mQuadVao);
			{
				GLfloat quadVertices[] = {
					// Positions        // Texture Coords
					-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
					-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
					1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
				};

				GLuint vbo;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
			}
			glBindVertexArray(0);

			// Init shader
			mShader.AttachShader(GL_VERTEX_SHADER, "Shaders/deferred_shading.vs.glsl");
			mShader.AttachShader(GL_FRAGMENT_SHADER, "Shaders/deferred_shading.fs.glsl");
			mShader.Link();

			// Set samplers
			mShader.Active();
			mShader.SetValue("gPosition", 0);
			mShader.SetValue("gNormal", 1);
			mShader.SetValue("gAlbedoSpec", 2);

			// Init Light
			constexpr GLuint NUM_LIGHTS = 32;
			Lights.resize(NUM_LIGHTS);

			srand(13);
			for (auto& light : Lights)
			{
				// Cal slightly ramdom offsets
				GLfloat x = ((rand() % 100) / 100.0) * 6.0 - 3.0;
				GLfloat y = ((rand() % 100) / 100.0) * 6.0 - 4.0;
				GLfloat z = ((rand() % 100) / 100.0) * 6.0 - 3.0;
				GLfloat r = ((rand() % 100) / 200.0) + 0.5;
				GLfloat g = ((rand() % 100) / 200.0) + 0.5;
				GLfloat b = ((rand() % 100) / 200.0) + 0.5;
				light = std::make_pair<glm::vec3, glm::vec3>(glm::vec3(x, y, z), glm::vec3(r, g, b));
			}
		}

		virtual void Update() override
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			auto& camera = Controller::Instance()->GetCamera();

			mShader.Active();
			mShader.SetValue("viewPos", camera.Position);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gPos);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

			// Set lights
			for (GLuint i = 0; i < Lights.size(); ++i)
			{
				mShader.SetValue(("lights[" + std::to_string(i) + "].Position").c_str(), Lights[i].first);
				mShader.SetValue(("lights[" + std::to_string(i) + "].Color").c_str(), Lights[i].second);

				const GLfloat constant = 1.0; // Note that we don't send this to the shader, we assume it is always 1.0 (in our case)
				const GLfloat linear = 0.7;
				const GLfloat quadratic = 1.8;
				mShader.SetValue(("lights[" + std::to_string(i) + "].Linear").c_str(), linear);
				mShader.SetValue(("lights[" + std::to_string(i) + "].Quadratic").c_str(), quadratic);
			}

			glBindVertexArray(mQuadVao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Write to default framebuffer
			glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

	private:
		GLuint mQuadVao;
		Shader mShader;

		std::vector<std::pair<glm::vec3, glm::vec3>> Lights;
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
	engine.Init(SCR_WIDTH, SCR_HEIGHT);
	engine.SetFrameBufferSizeCallback(framebuffer_size_callback);
	engine.SetCursorPosCallback(mouse_callback);

	gl::GeometryPass GeometryPass;
	engine.AddPass(&GeometryPass);

	gl::DeferredLightingPass DeferredLightingPass;
	engine.AddPass(&DeferredLightingPass);

	engine.Render();

	return 0;
}