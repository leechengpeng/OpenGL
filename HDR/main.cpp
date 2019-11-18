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

		virtual void Init(const SContext& context) override
		{
			// Init
			glGenFramebuffers(1, &mHDRFrameBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mHDRFrameBuffer);
			{
				// color
				glGenTextures(1, &mHDRColorTex);
				glBindTexture(GL_TEXTURE_2D, mHDRColorTex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, context.width, context.height, 0, GL_RGBA, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mHDRColorTex, 0);
				// depth
				GLuint rboDepth;
				glGenRenderbuffers(1, &rboDepth);
				glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, context.width, context.height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
				// check
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					std::cout << "Framebuffer not complete!" << std::endl;
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Init Light info
			mLightInfo.push_back(std::make_pair(glm::vec3(0.0f, 0.0f, 49.5f), glm::vec3(200.0f, 200.0f, 200.0f)));
			mLightInfo.push_back(std::make_pair(glm::vec3(-1.4f, -1.9f, 9.0f), glm::vec3(0.1f, 0.0f, 0.0f)));
			mLightInfo.push_back(std::make_pair(glm::vec3(0.0f, -1.8f, 4.0f), glm::vec3(0.0f, 0.0f, 0.2f)));
			mLightInfo.push_back(std::make_pair(glm::vec3(0.8f, -1.7f, 6.0f), glm::vec3(0.0f, 0.1f, 0.0f)));

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
					for (GLuint i = 0; i < mLightInfo.size(); ++i)
					{
						mShaderLighting.SetValue(("lights[" + std::to_string(i) + "].Position").c_str(), mLightInfo[i].first);
						mShaderLighting.SetValue(("lights[" + std::to_string(i) + "].Color").c_str(), mLightInfo[i].second);
					}
					mShaderLighting.SetValue("viewPos", camera.Position);

					glm::mat4 model = glm::mat4();
					model = glm::translate(model, glm::vec3(0.0f, 0.0f, 25.0));
					model = glm::scale(model, glm::vec3(5.0f, 5.0f, 55.0f));
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
				glBindTexture(GL_TEXTURE_2D, mHDRColorTex);
				mShaderToneMapping.SetValue("hdr", mHDR);
				mShaderToneMapping.SetValue("exposure", mExposure);
				mQuad.Draw();
			}
		}

	private:
		QuadMesh	mQuad;
		CubeMesh	mTunnel;

		GLuint		mHDRColorTex;
		GLuint		mHDRFrameBuffer;
		Shader		mShaderLighting;
		Shader		mShaderToneMapping;
		GLboolean	mHDR;
		GLfloat		mExposure;

		std::vector<std::pair<glm::vec3, glm::vec3>> mLightInfo;
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