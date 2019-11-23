#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "common_mesh.hpp"
#include "controller.hpp"
#include <array>

namespace gl
{
	struct LightInfo
	{
		glm::vec3 pos;
		glm::vec3 color;

		LightInfo(const glm::vec3& pos, const glm::vec3& color) : pos(pos), color(color) {}
	};

	class Bloom : public RenderPass
	{
	public:
		Bloom() : mExposure(1.f)
		{

		}

		virtual void Init(const SContext& context) override
		{
			// 浮点帧缓冲区：颜色缓冲区格式被显示定义为GL_RGB16F、GL_RGBA16F、GL_RGB32F或者GL_RGBA32F，默认为RGB（8位）
			glGenFramebuffers(1, &mFloatFrameBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mFloatFrameBuffer);
			{
				glGenTextures(1, &mNormalColorBuffer);
				glBindTexture(GL_TEXTURE_2D, mNormalColorBuffer);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, context.width, context.height, 0, GL_RGBA, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mNormalColorBuffer, 0);

				glGenTextures(1, &mBrighterColorBuffer);
				glBindTexture(GL_TEXTURE_2D, mBrighterColorBuffer);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, context.width, context.height, 0, GL_RGBA, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mBrighterColorBuffer, 0);

				// 告知OpenGL我们正在通过glDrawBuffers渲染到多个颜色缓冲，否则OpenGL只会渲染到帧缓冲的第一个颜色附件，而忽略所有其他的
				GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
				glDrawBuffers(2, attachments);

				GLuint depthBuffer;
				glGenRenderbuffers(1, &depthBuffer);
				glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, context.width, context.height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glGenFramebuffers(2, &mBlurFrameBuffers[0]);
			glGenTextures(2, &mBlurColorBuffers[0]);
			for (size_t i = 0; i < 2; ++i)
			{
				glBindFramebuffer(GL_FRAMEBUFFER, mBlurFrameBuffers[i]);
				glBindTexture(GL_TEXTURE_2D, mBlurColorBuffers[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, context.width, context.height, 0, GL_RGB, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBlurColorBuffers[i], 0);
				// also check if framebuffers are complete (no need for depth buffer)
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
					std::cout << "Framebuffer not complete!" << std::endl;
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			// Init Lights
			mLightInfos.push_back(LightInfo(glm::vec3(0.0f, 0.5f, 1.5f), glm::vec3(5.0f, 5.0f, 5.0f)));
			mLightInfos.push_back(LightInfo(glm::vec3(-4.0f, 0.5f, -3.0f), glm::vec3(10.0f, 0.0f, 0.0f)));
			mLightInfos.push_back(LightInfo(glm::vec3(3.0f, 0.5f, 1.0f), glm::vec3(0.0f, 0.0f, 15.0f)));
			mLightInfos.push_back(LightInfo(glm::vec3(-.8f, 2.4f, -1.0f), glm::vec3(0.0f, 5.0f, 0.0f)));

			// Init Tex
			mWoodTex = LoadTexture("../Resource/Texture/wood.png");

			// Init Shaders
			mLighting.AttachShader(GL_VERTEX_SHADER, "Shaders/lighting_vs.glsl");
			mLighting.AttachShader(GL_FRAGMENT_SHADER, "Shaders/lighting_fs.glsl");
			mLighting.Link();
			mLighting.Active();
			mLighting.SetValue("diffuseTexture", 0);

			mRenderLights.AttachShader(GL_VERTEX_SHADER, "Shaders/renderlights_vs.glsl");
			mRenderLights.AttachShader(GL_FRAGMENT_SHADER, "Shaders/renderlights_fs.glsl");
			mRenderLights.Link();

			mBlur.AttachShader(GL_VERTEX_SHADER, "Shaders/blur_vs.glsl");
			mBlur.AttachShader(GL_FRAGMENT_SHADER, "Shaders/blur_fs.glsl");
			mBlur.Link();
			mRenderToScreen.Active();
			mRenderToScreen.SetValue("image", 0);

			mRenderToScreen.AttachShader(GL_VERTEX_SHADER, "Shaders/render_to_screen_vs.glsl");
			mRenderToScreen.AttachShader(GL_FRAGMENT_SHADER, "Shaders/render_to_screen_fs.glsl");
			mRenderToScreen.Link();
			mRenderToScreen.Active();
			mRenderToScreen.SetValue("scene", 0);
			mRenderToScreen.SetValue("bloomBlur", 1);

			// Init Camera
			Controller::Instance()->ResetCamera(glm::vec3(0.f, 1.f, 5.f), glm::vec3(0.0f, 1.0f, 0.0f));

			glEnable(GL_DEPTH_TEST);
		}

		virtual void Update(const SContext& context, const STime& time) override
		{
			auto& camera = Controller::Instance()->GetCamera();

			// 1. Render scene into floating point framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, mFloatFrameBuffer);
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glm::mat4 model = glm::mat4(1.0f);
				glm::mat4 view  = camera.GetViewMatrix();
				glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)context.width / (GLfloat)context.width, 0.1f, 100.0f);

				// set uniform
				mLighting.Active();
				mLighting.SetMatrix("view", &view[0][0]);
				mLighting.SetMatrix("projection", &projection[0][0]);
				mLighting.SetValue("viewPos", camera.Position);
				for (GLuint i = 0; i < mLightInfos.size(); ++i)
				{
					mLighting.SetValue(("lights[" + std::to_string(i) + "].Position").c_str(), mLightInfos[i].pos);
					mLighting.SetValue(("lights[" + std::to_string(i) + "].Color").c_str(), mLightInfos[i].color);
				}
				// set texture
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mWoodTex);

				// create one large cube that acts as the floor
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0));
				model = glm::scale(model, glm::vec3(12.5f, 0.5f, 12.5f));
				mLighting.SetMatrix("model", &model[0][0]);
				mBox.Draw();
				// then create multiple cubes as the scenery
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
				model = glm::scale(model, glm::vec3(0.5f));
				mLighting.SetMatrix("model", &model[0][0]);
				mBox.Draw();

				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
				model = glm::scale(model, glm::vec3(0.5f));
				mLighting.SetMatrix("model", &model[0][0]);
				mBox.Draw();

				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(-1.0f, -1.0f, 2.0));
				model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
				mLighting.SetMatrix("model", &model[0][0]);
				mBox.Draw();

				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.0f, 2.7f, 4.0));
				model = glm::rotate(model, glm::radians(23.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
				model = glm::scale(model, glm::vec3(1.25));
				mLighting.SetMatrix("model", &model[0][0]);
				mBox.Draw();

				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -3.0));
				model = glm::rotate(model, glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
				mLighting.SetMatrix("model", &model[0][0]);
				mBox.Draw();

				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0));
				model = glm::scale(model, glm::vec3(0.5f));
				mLighting.SetMatrix("model", &model[0][0]);
				mBox.Draw();

				mRenderLights.Active();
				mRenderLights.SetMatrix("view", &view[0][0]);
				mRenderLights.SetMatrix("projection", &projection[0][0]);
				for (auto& lightInfo : mLightInfos)
				{
					model = glm::mat4(1.0f);
					model = glm::translate(model, lightInfo.pos);
					model = glm::scale(model, glm::vec3(0.25f));
					mRenderLights.SetMatrix("model", &model[0][0]);
					mRenderLights.SetValue("lightColor", lightInfo.color);
					mBox.Draw();
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			// 2. blur bright fragments with two-pass Gaussian Blur
			bool horizontal = true, first_iteration = true;
			mBlur.Active();
			{
				for (unsigned int i = 0; i < 10; ++i)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, mBlurFrameBuffers[horizontal]);
					mBlur.SetValue("horizontal", horizontal);
					glBindTexture(GL_TEXTURE_2D, first_iteration ? mBrighterColorBuffer : mBlurColorBuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
					mQuad.Draw();
					horizontal = !horizontal;
					if (first_iteration)
						first_iteration = false;
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}


			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			mRenderToScreen.Active();
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mNormalColorBuffer);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, mBlurColorBuffers[!horizontal]);
				mRenderToScreen.SetValue("exposure", mExposure);
				mQuad.Draw();
			}
		}

	private:
		GLuint		mFloatFrameBuffer;
		GLuint		mNormalColorBuffer;
		GLuint		mBrighterColorBuffer;

		CubeMesh	mBox;
		QuadMesh	mQuad;
		GLuint		mWoodTex;
		GLfloat		mExposure;
		
		Shader		mLighting;
		Shader		mRenderLights;
		Shader		mBlur;
		Shader		mRenderToScreen;

		std::array<GLuint, 2>  mBlurFrameBuffers;
		std::array<GLuint, 2>  mBlurColorBuffers;
		std::vector<LightInfo> mLightInfos;
	};
}

int main()
{
	gl::Engine engine;
	engine.Init(1280, 720);
	engine.SetFrameBufferSizeCallback(gl::FramebufferSizeCallback);
	engine.SetCursorPosCallback(gl::MouseCallback);

	gl::Bloom bloom;
	engine.AddPass(&bloom);
	engine.Render();

	return 0;
}