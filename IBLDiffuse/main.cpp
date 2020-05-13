/* 漫反射辐照度 */
#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "controller.hpp"
#include "common_mesh.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace gl
{
	class IBLDiffuse : public RenderPass
	{
	public:
		IBLDiffuse() : mEnvCubeMapSize(512), mIrradianceMapSize(32)
		{

		}

		virtual void Init(const SContext& context) override
		{
			// Configure global OpenGL state
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL); // set depth function to less than AND equal for skybox depth trick.

			// Init shader
			{
				mEquirectangularToCubemapShader.AttachShader(GL_VERTEX_SHADER,		"Shaders/cubemap.vs");
				mEquirectangularToCubemapShader.AttachShader(GL_FRAGMENT_SHADER,	"Shaders/equirectangular_to_cubemap.fs");
				mEquirectangularToCubemapShader.Link();

				mIrradianceShader.AttachShader(GL_VERTEX_SHADER,	"Shaders/cubemap.vs");
				mIrradianceShader.AttachShader(GL_FRAGMENT_SHADER,	"Shaders/irradiance_convolution.fs");
				mIrradianceShader.Link();
				mIrradianceShader.Active();
				mIrradianceShader.SetValue("environmentMap", 0);

				mBackgroundShader.AttachShader(GL_VERTEX_SHADER,	"Shaders/background.vs");
				mBackgroundShader.AttachShader(GL_FRAGMENT_SHADER,	"Shaders/background.fs");
				mBackgroundShader.Link();
				mBackgroundShader.Active();
				mBackgroundShader.SetValue("environmentMap", 0);
			}

			// Setup framebuffer
			unsigned int captureFBO;
			unsigned int captureRBO;
			glGenFramebuffers(1, &captureFBO);
			glGenRenderbuffers(1, &captureRBO);
			glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
			glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

			glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
			std::vector<glm::mat4> captureViews = {
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
			};

			// Convert HDR equirectangular environment map to cubemap equivalent
			mEquirectangularToCubemapShader.Active();
			{
				// Setup cubemap to render to and attach to framebuffer
				mEnvCubeMap = _CreateEmptyCubeMap(mEnvCubeMapSize);

				mEquirectangularToCubemapShader.SetValue("equirectangularMap", 0);
				mEquirectangularToCubemapShader.SetMatrix("projection", captureProjection);

				mHDRTex = LoadTextureHDR("../Resource/HDR/Newport_Loft_Ref.hdr");
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, mHDRTex);

				glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
				glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
				for (unsigned int i = 0; i < 6; ++i)
				{
					mEquirectangularToCubemapShader.SetMatrix("view", captureViews[i]);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mEnvCubeMap, 0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					mCube.Draw();
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
			// -----------------------------------------------------------------------------
			mIrradianceShader.Active();
			{
				// Create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
				mIrradianceMap = _CreateEmptyCubeMap(mIrradianceMapSize);

				glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
				glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

				mIrradianceShader.SetMatrix("projection", captureProjection);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvCubeMap);

				glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
				glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
				{
					for (unsigned int i = 0; i < 6; ++i)
					{
						mIrradianceShader.SetMatrix("view", captureViews[i]);
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mIrradianceMap, 0);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

						mCube.Draw();
					}
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			// 渲染之前，还原窗口的尺寸
			glViewport(0, 0, context.width, context.height);
		}

		virtual void Update(const SContext& context, const STime& time) override
		{
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			auto& camera = Controller::Instance()->GetCamera();

			glm::mat4 modelMat = glm::mat4(1.0f);
			glm::mat4 viewMat = camera.GetViewMatrix();
			glm::mat4 projMat = glm::perspective(glm::radians(camera.Zoom), (float)context.width / (float)context.height, 0.1f, 100.0f);

			// render skybox (render as last to prevent overdraw)
			mBackgroundShader.Active();
			mBackgroundShader.SetMatrix("view",			viewMat);
			mBackgroundShader.SetMatrix("projection",	projMat);
			glActiveTexture(GL_TEXTURE0);
			//glBindTexture(GL_TEXTURE_CUBE_MAP, mEnvCubeMap);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMap); // display irradiance map
			mCube.Draw();
		}

	private:
		GLuint _CreateEmptyCubeMap(GLuint size) const
		{
			GLuint mEmptyCubeMap;
			glGenTextures(1, &mEmptyCubeMap);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mEmptyCubeMap);
			for (unsigned int i = 0; i < 6; ++i)
			{
				// 数据先设置为空
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
			}
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			return mEmptyCubeMap;
		}

		GLint	mHDRTex;
		GLuint	mEnvCubeMap;
		GLuint	mIrradianceMap;

		GLuint	mEnvCubeMapSize;
		GLuint	mIrradianceMapSize;

		Shader mPBR;
		Shader mEquirectangularToCubemapShader;
		Shader mIrradianceShader;
		Shader mBackgroundShader;

		CubeMesh	mCube;
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

	gl::IBLDiffuse diffuse;
	engine.AddPass(&diffuse);
	engine.Render();

	return 0;
}
