/* 漫反射辐照度 */
#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "controller.hpp"
#include "common_mesh.hpp"
#include <stb_image_write.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace gl
{
	class IBLDiffuse : public RenderPass
	{
	public:
		IBLDiffuse() : mEnvCubeMapSize(512), mIrradianceMapSize(32), mRows(7), mColumns(7), mSpacing(2.5)
		{

		}

		virtual void Init(const SContext& context) override
		{
			// Configure global OpenGL state
			glEnable(GL_DEPTH_TEST);
			// Set depth function to less than AND equal for skybox depth trick.
			glDepthFunc(GL_LEQUAL);
			// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

			// Init shader
			mPbrShader.AttachVertexShader("Shaders/pbr.vs");
			mPbrShader.AttachFragmentShader("Shaders/pbr.fs");
			mPbrShader.Link();
			mPbrShader.Active();
			mPbrShader.SetValue("irradianceMap", 0);
			mPbrShader.SetValue("prefilterMap", 1);
			mPbrShader.SetValue("brdfLUT", 2);
			mPbrShader.SetValue("albedo", glm::vec3(0.5f, 0.0f, 0.0f));
			mPbrShader.SetValue("ao", 1.0f);

			mBackgroundShader.AttachShader(GL_VERTEX_SHADER, "Shaders/background.vs");
			mBackgroundShader.AttachShader(GL_FRAGMENT_SHADER, "Shaders/background.fs");
			mBackgroundShader.Link();
			mBackgroundShader.Active();
			mBackgroundShader.SetValue("environmentMap", 0);

			mBRDFShader.AttachVertexShader("Shaders/brdf.vs");
			mBRDFShader.AttachFragmentShader("Shaders/brdf.fs");
			mBRDFShader.Link();

			// Setup framebuffer
			GLuint fbo;
			GLuint rbo;
			glGenFramebuffers(1, &fbo);
			glGenRenderbuffers(1, &rbo);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

			glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
			std::vector<glm::mat4> captureViews = {
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
			};

			// Preprocess
			mEnvCubeMap = _EquirectangularToCubemap(captureViews, captureProjection, fbo);
			mIrradianceMap = _CubemapToIrradianceMap(captureViews, captureProjection, fbo, rbo, mEnvCubeMap);
			mPrefilterMap = _CubemapToPrefilterMap(captureViews, captureProjection, fbo, rbo, mEnvCubeMap);
			mBRDFLutMap = _GenBRDFLUT(fbo, rbo);

			// Set light attributes
			mLightPos.push_back(glm::vec3(-10.0f, 10.0f, 10.0f));
			mLightPos.push_back(glm::vec3(10.0f, 10.0f, 10.0f));
			mLightPos.push_back(glm::vec3(-10.0f, -10.0f, 10.0f));
			mLightPos.push_back(glm::vec3(10.0f, -10.0f, 10.0f));
			mLightColor.push_back(glm::vec3(300.0f, 300.0f, 300.0f));
			mLightColor.push_back(glm::vec3(300.0f, 300.0f, 300.0f));
			mLightColor.push_back(glm::vec3(300.0f, 300.0f, 300.0f));
			mLightColor.push_back(glm::vec3(300.0f, 300.0f, 300.0f));

			Controller::Instance()->ResetCamera(glm::vec3(0.f, 0.f, 15.f), glm::vec3(0.f, 1.f, 0.f));

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

			// PBR lighting
			{
				mPbrShader.Active();
				mPbrShader.SetValue("camPos", camera.Position);
				mPbrShader.SetMatrix("view", viewMat);
				mPbrShader.SetMatrix("projection", projMat);

				// bind pre-computed IBL data
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMap);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilterMap);
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, mBRDFLutMap);

				// render rows*column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
				for (int row = 0; row < mRows; ++row)
				{
					mPbrShader.SetValue("metallic", (float)row / (float)mRows);
					for (int col = 0; col < mColumns; ++col)
					{
						// we clamp the roughness to 0.025 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
						// on direct lighting.
						mPbrShader.SetValue("roughness", glm::clamp((float)col / (float)mColumns, 0.05f, 1.0f));

						modelMat = glm::mat4(1.0f);
						modelMat = glm::translate(modelMat, glm::vec3(
							(col - (mColumns / 2)) * mSpacing,
							(row - (mRows / 2)) * mSpacing,
							0.0f
						));
						mPbrShader.SetMatrix("model", modelMat);
						mSphere.Draw();
					}
				}

				// render light source (simply re-render sphere at light positions)
				// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
				// keeps the codeprint small.
				for (unsigned int i = 0; i < mLightPos.size(); ++i)
				{
					glm::vec3 newPos = mLightPos[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
					newPos = mLightPos[i];
					mPbrShader.SetValue("lightPositions[" + std::to_string(i) + "]", newPos);
					mPbrShader.SetValue("lightColors[" + std::to_string(i) + "]", mLightColor[i]);

					modelMat = glm::mat4(1.0f);
					modelMat = glm::translate(modelMat, newPos);
					modelMat = glm::scale(modelMat, glm::vec3(0.5f));
					mPbrShader.SetMatrix("model", modelMat);
					mSphere.Draw();
				}
			}

			// render skybox (render as last to prevent overdraw)
			{
				mBackgroundShader.Active();
				mBackgroundShader.SetMatrix("view", viewMat);
				mBackgroundShader.SetMatrix("projection", projMat);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilterMap);
				mCube.Draw();
			}
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

		GLuint _EquirectangularToCubemap(const std::vector<glm::mat4>& cubeViews, const glm::mat4& proj, GLuint fbo) const
		{
			Shader equirectangularToCubemapShader("Shaders/cubemap.vs", "Shaders/equirectangular_to_cubemap.fs");
			equirectangularToCubemapShader.Link();

			auto cubeMap = _CreateEmptyCubeMap(mEnvCubeMapSize);
			// Convert HDR equirectangular environment map to cubemap equivalent
			equirectangularToCubemapShader.Active();
			{
				equirectangularToCubemapShader.SetValue("equirectangularMap", 0);
				equirectangularToCubemapShader.SetMatrix("projection", proj);

				auto hdr = LoadTextureHDR("../Resource/HDR/Newport_Loft_Ref.hdr");
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, hdr);

				// Don't forget to configure the viewport to the capture dimensions.
				glViewport(0, 0, mEnvCubeMapSize, mEnvCubeMapSize);
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mEnvCubeMapSize, mEnvCubeMapSize);
				for (unsigned int i = 0; i < cubeViews.size(); ++i)
				{
					equirectangularToCubemapShader.SetMatrix("view", cubeViews[i]);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap, 0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					mCube.Draw();
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			return cubeMap;
		}

		GLuint _CubemapToIrradianceMap(const std::vector<glm::mat4>& cubeViews, const glm::mat4& proj, GLuint fbo, GLuint rbo, GLuint envMap) const
		{
			Shader irradianceShader("Shaders/cubemap.vs", "Shaders/irradiance_convolution.fs");
			irradianceShader.Link();

			auto irradianceMap = _CreateEmptyCubeMap(mIrradianceMapSize);
			// Solve diffuse integral by convolution to create an irradiance (cube)map.
			irradianceShader.Active();
			{
				irradianceShader.SetValue("environmentMap", 0);
				irradianceShader.SetMatrix("projection", proj);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, envMap);

				glViewport(0, 0, mIrradianceMapSize, mIrradianceMapSize);
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mIrradianceMapSize, mIrradianceMapSize);
				for (unsigned int i = 0; i < cubeViews.size(); ++i)
				{
					irradianceShader.SetMatrix("view", cubeViews[i]);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					mCube.Draw();
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			return irradianceMap;
		}

		// Specular
		GLuint _CubemapToPrefilterMap(const std::vector<glm::mat4>& cubeViews, const glm::mat4& proj, GLuint fbo, GLuint rbo, GLuint envMap) const
		{
			Shader prefilterShader("Shaders/cubemap.vs", "Shaders/prefilter.fs");
			prefilterShader.Link();

			GLuint prefilterMapSize = 128;
			auto prefilterMap = _CreateEmptyCubeMap(prefilterMapSize);

			// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
			prefilterShader.Active();
			{
				prefilterShader.SetValue("environmentMap", 0);
				prefilterShader.SetMatrix("projection", proj);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, envMap);

				glBindFramebuffer(GL_FRAMEBUFFER, fbo);
				unsigned int maxMipLevels = 5;
				for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
				{
					// reisze framebuffer according to mip-level size.
					unsigned int mipWidth = 128 * std::pow(0.5, mip);
					unsigned int mipHeight = 128 * std::pow(0.5, mip);
					glBindRenderbuffer(GL_RENDERBUFFER, rbo);
					glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
					glViewport(0, 0, mipWidth, mipHeight);

					float roughness = (float)mip / (float)(maxMipLevels - 1);
					prefilterShader.SetValue("roughness", roughness);
					for (unsigned int i = 0; i < 6; ++i)
					{
						prefilterShader.SetMatrix("view", cubeViews[i]);
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						mCube.Draw();
					}
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			return prefilterMap;
		}

		GLuint _GenBRDFLUT(GLuint fbo, GLuint rbo)
		{
			GLuint brdfLUT;
			glGenTextures(1, &brdfLUT);

			// pre-allocate enough memory for the LUT texture.
			glBindTexture(GL_TEXTURE_2D, brdfLUT);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
			// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT, 0);

			glViewport(0, 0, 512, 512);
			mBRDFShader.Active();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			mQuad.Draw();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return brdfLUT;
		}

		GLuint	mEnvCubeMap;
		GLuint	mIrradianceMap;
		GLuint	mPrefilterMap;
		GLuint  mBRDFLutMap;

		GLuint	mEnvCubeMapSize;
		GLuint	mIrradianceMapSize;

		Shader mPbrShader;
		Shader mBRDFShader;
		Shader mBackgroundShader;

		CubeMesh	mCube;
		QuadMesh	mQuad;

		// PBR
		GLint	mRows;
		GLint	mColumns;
		GLfloat	mSpacing;
		Sphere	mSphere;

		std::vector<glm::vec3> mLightPos;
		std::vector<glm::vec3> mLightColor;
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
