#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "common_mesh.hpp"
#include "controller.hpp"

namespace gl
{
	// https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/6.hdr/hdr.cpp
	class HDR : public RenderPass
	{
	public:
		HDR() : mHDR(true), mExposure(1.f)
		{

		}

		struct LightInfo
		{
			glm::vec3 pos;
			glm::vec3 color;

			LightInfo(const glm::vec3& pos, const glm::vec3& color) : pos(pos), color(color) {}
		};

		virtual void Init(const SContext& context) override
		{
			// 浮点帧缓冲区：颜色缓冲区格式被显示定义为GL_RGB16F、GL_RGBA16F、GL_RGB32F或者GL_RGBA32F，默认为RGB（8位）
			glGenFramebuffers(1, &mHDRFrameBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mHDRFrameBuffer);
			{
				glGenTextures(1, &mFloatColorBuffer);
				glBindTexture(GL_TEXTURE_2D, mFloatColorBuffer);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, context.width, context.height, 0, GL_RGBA, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFloatColorBuffer, 0);
				GLuint depthBuffer;
				glGenRenderbuffers(1, &depthBuffer);
				glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, context.width, context.height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
				// check
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					std::cout << "Framebuffer not complete!" << std::endl;
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Init Light infos
			mLightInfos.push_back(LightInfo(glm::vec3(0.0f, 0.0f, 49.5f), glm::vec3(200.0f, 200.0f, 200.0f)));
			mLightInfos.push_back(LightInfo(glm::vec3(-1.4f, -1.9f, 9.0f), glm::vec3(0.1f, 0.0f, 0.0f)));
			mLightInfos.push_back(LightInfo(glm::vec3(0.0f, -1.8f, 4.0f), glm::vec3(0.0f, 0.0f, 0.2f)));
			mLightInfos.push_back(LightInfo(glm::vec3(0.8f, -1.7f, 6.0f), glm::vec3(0.0f, 0.1f, 0.0f)));

			// Init Shader
			mShaderLighting.AttachShader(GL_VERTEX_SHADER, "Shaders/lighting_vs.glsl");
			mShaderLighting.AttachShader(GL_FRAGMENT_SHADER, "Shaders/lighting_fs.glsl");
			mShaderLighting.Link();

			mShaderToneMapping.AttachShader(GL_VERTEX_SHADER, "Shaders/tone_mapping_vs.glsl");
			mShaderToneMapping.AttachShader(GL_FRAGMENT_SHADER, "Shaders/tone_mapping_fs.glsl");
			mShaderToneMapping.Link();
			mShaderToneMapping.Active();
			mShaderToneMapping.SetValue("hdrBuffer", 0);

			// Init Camera
			Controller::Instance()->ResetCamera(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		virtual void Update(const SContext& context, const STime& time) override
		{
			auto& camera = Controller::Instance()->GetCamera();

			// 1. Render scene into floating point framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, mHDRFrameBuffer);
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				mShaderLighting.Active();
				{
					for (GLuint i = 0; i < mLightInfos.size(); ++i)
					{
						mShaderLighting.SetValue(("lights[" + std::to_string(i) + "].Position").c_str(), mLightInfos[i].pos);
						mShaderLighting.SetValue(("lights[" + std::to_string(i) + "].Color").c_str(), mLightInfos[i].color);
					}
					mShaderLighting.SetValue("viewPos", camera.Position);

					glm::mat4 model = glm::mat4(1.f);
					model = glm::translate(model, glm::vec3(0.0f, 0.0f, 25.0));
					model = glm::scale(model, glm::vec3(5.f, 5.f, 55.f));
					glm::mat4 view = camera.GetViewMatrix();
					glm::mat4 projection = glm::perspective(camera.Zoom, (GLfloat)context.width / (GLfloat)context.width, 0.1f, 100.0f);
					mShaderLighting.SetMatrix("model", &model[0][0]);
					mShaderLighting.SetMatrix("view", &view[0][0]);
					mShaderLighting.SetMatrix("projection", &projection[0][0]);
					mTunnel.Draw();
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// 2. Now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			mShaderToneMapping.Active();
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mFloatColorBuffer);
				mShaderToneMapping.SetValue("hdr", mHDR);
				mShaderToneMapping.SetValue("exposure", mExposure);
				mQuad.Draw();
			}
		}

	private:
		QuadMesh	mQuad;
		CubeMesh	mTunnel;

		GLuint		mFloatColorBuffer;
		GLuint		mHDRFrameBuffer;
		Shader		mShaderLighting;
		Shader		mShaderToneMapping;
		GLboolean	mHDR;
		GLfloat		mExposure;

		std::vector<LightInfo> mLightInfos;
	};
}

int main()
{
	gl::Engine engine;
	engine.Init(1280, 720);
	engine.SetFrameBufferSizeCallback(gl::FramebufferSizeCallback);
	engine.SetCursorPosCallback(gl::MouseCallback);

	gl::HDR hdr;
	engine.AddPass(&hdr);
	engine.Render();

	return 0;
}