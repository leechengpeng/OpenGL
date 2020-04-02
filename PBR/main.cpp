#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "controller.hpp"
#include "common_mesh.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gl
{
	class PBR : public RenderPass
	{
	public:
		PBR() : mRows(7), mColumns(7), mSpacing(2.5), mAlbedoMap(), mNormalMap(), mMetallicMap(), mRoughnessMap(), mAoMap()
		{

		}

		virtual void Init(const SContext& context) override
		{
			glEnable(GL_DEPTH_TEST);

			// Load textures
			mAlbedoMap = LoadTexture("../Resource/pbr_rustediron/rustediron2_basecolor.png");
			mNormalMap = LoadTexture("../Resource/pbr_rustediron/rustediron2_normal.png");
			mMetallicMap = LoadTexture("../Resource/pbr_rustediron/rustediron2_metallic.png");
			mRoughnessMap = LoadTexture("../Resource/pbr_rustediron/rustediron2_roughness.png");
			//mAoMap = LoadTexture("../Resource/pbr_rustediron/");

			// Init shader pbr
			mShaderPBR.AttachShader(GL_VERTEX_SHADER, "Shaders/pbr_vs.glsl");
			mShaderPBR.AttachShader(GL_FRAGMENT_SHADER, "Shaders/pbr_fs.glsl");
			mShaderPBR.Link();
			mShaderPBR.Active();
			//mShaderPBR.SetValue("ao", 1.0f);
			//mShaderPBR.SetValue("albedo", glm::vec3(0.5f, 0.0f, 0.0f));
			mShaderPBR.SetValue("albedoMap", 0);
			mShaderPBR.SetValue("normalMap", 1);
			mShaderPBR.SetValue("metallicMap", 2);
			mShaderPBR.SetValue("roughnessMap", 3);
			//mShaderPBR.SetValue("aoMap", 4);
			

			// Set light attributes
			mLightPos.push_back(glm::vec3(-10.0f, 10.0f, 10.0f));
			mLightPos.push_back(glm::vec3(10.0f, 10.0f, 10.0f));
			mLightPos.push_back(glm::vec3(-10.0f, -10.0f, 10.0f));
			mLightPos.push_back(glm::vec3(10.0f, -10.0f, 10.0f));
			mLightColor.push_back(glm::vec3(300.0f, 300.0f, 300.0f));
			mLightColor.push_back(glm::vec3(300.0f, 300.0f, 300.0f));
			mLightColor.push_back(glm::vec3(300.0f, 300.0f, 300.0f));
			mLightColor.push_back(glm::vec3(300.0f, 300.0f, 300.0f));

			Controller::Instance()->ResetCamera(glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f, -1.f, 0.f));
		}

		virtual void Update(const SContext& context, const STime& time) override
		{
			auto& camera = Controller::Instance()->GetCamera();

			glm::mat4 modelMat = glm::mat4(1.0f);
			glm::mat4 viewMat  = camera.GetViewMatrix();
			glm::mat4 projMat  = glm::perspective(glm::radians(camera.Zoom), (float)context.width / (float)context.height, 0.1f, 100.0f);

			// lighting
			mShaderPBR.Active();
			mShaderPBR.SetValue("camPos", camera.Position);
			mShaderPBR.SetMatrix("view", &viewMat[0][0]);
			mShaderPBR.SetMatrix("projection", &projMat[0][0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mAlbedoMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, mNormalMap);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, mMetallicMap);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, mRoughnessMap);

			// render rows*column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
			for (int row = 0; row < mRows; ++row)
			{
				mShaderPBR.SetValue("metallic", (float)row / (float)mRows);
				for (int col = 0; col < mColumns; ++col)
				{
					// we clamp the roughness to 0.025 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
					// on direct lighting.
					mShaderPBR.SetValue("roughness", glm::clamp((float)col / (float)mColumns, 0.05f, 1.0f));

					modelMat = glm::mat4(1.0f);
					modelMat = glm::translate(modelMat, glm::vec3(
						(col - (mColumns / 2)) * mSpacing,
						(row - (mRows / 2)) * mSpacing,
						0.0f
					));
					mShaderPBR.SetMatrix("model", &modelMat[0][0]);
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
				mShaderPBR.SetValue("uLightPos[" + std::to_string(i) + "]", newPos);
				mShaderPBR.SetValue("uLightColor[" + std::to_string(i) + "]", mLightColor[i]);

				modelMat = glm::mat4(1.0f);
				modelMat = glm::translate(modelMat, newPos);
				modelMat = glm::scale(modelMat, glm::vec3(0.5f));
				mShaderPBR.SetMatrix("model", &modelMat[0][0]);
				mSphere.Draw();
			}
		}

	private:
		GLint   mRows;
		GLint   mColumns;
		GLfloat mSpacing;

		GLint   mAlbedoMap;
		GLint   mNormalMap;
		GLint   mMetallicMap;
		GLint   mRoughnessMap;
		GLint   mAoMap;

		Sphere  mSphere;
		Shader  mShaderPBR;

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

	gl::PBR pbr;
	engine.AddPass(&pbr);
	engine.Render();

	return 0;
}