#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "controller.hpp"

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

			// Init shader
			mShader.AttachShader(GL_VERTEX_SHADER, "Shaders/g_buffer.vs.glsl");
			mShader.AttachShader(GL_FRAGMENT_SHADER, "Shaders/g_buffer.fs.glsl");
			mShader.Link();
		}

		virtual void Update() override
		{
			auto& camera = Controller::Instance()->GetCamera();

			// Geometry Pass: render scene's geometry/color data into gbuffer
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glm::mat4 model = glm::mat4(1.0f);
				glm::mat4 view = camera.GetViewMatrix();
				glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
				mShader.Active();
				mShader.SetMatrix("model", &model[0][0]);
				mShader.SetMatrix("view", &view[0][0]);
				mShader.SetMatrix("projection", &projection[0][0]);

				glBindVertexArray(mFloorVao);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);

			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

	private:
		GLuint mFloorVao;
		Shader mShader;
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