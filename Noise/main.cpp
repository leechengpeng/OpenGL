#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "common_mesh.hpp"
#include "controller.hpp"

namespace gl
{
	// ²Î¿¼£ºhttps://blog.csdn.net/linjf520/article/details/99647624
	class WaterNoise : public RenderPass
	{
	public:
		virtual void Init(const SContext& context) override
		{
			// Init shader
			mShader.AttachShader(GL_VERTEX_SHADER, "Shaders/water_vs.glsl");
			mShader.AttachShader(GL_FRAGMENT_SHADER, "Shaders/water_fs.glsl");
			mShader.Link();

			mTex = LoadTexture("../Resource/Skybox/right.jpg");
			mTexNoise = LoadTexture("../Resource/Texture/noise.jpg", GL_REPEAT);

			// Init Camera
			Controller::Instance()->ResetCamera(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		virtual void Update(const SContext& context, const STime& time) override
		{
			mShader.SetValue("_time", time._Time.x);

			auto& camera = Controller::Instance()->GetCamera();
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::rotate(model, 1.5708f, glm::vec3(1.f, 0.f, 0.f));
			glm::mat4 view = camera.GetViewMatrix();
			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)1280 / (float)720, 0.1f, 100.0f);
			mShader.SetMatrix("model", &model[0][0]);
			mShader.SetMatrix("view", &view[0][0]);
			mShader.SetMatrix("projection", &projection[0][0]);

			mShader.Active();
			mShader.SetValue("tex", 0);
			mShader.SetValue("tex_noise", 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mTex);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, mTexNoise);

			mPlane.Draw();
		}

	private:
		GLuint mTex;
		GLuint mTexNoise;

		Shader	  mShader;
		PlaneMesh mPlane;
	};
}

int main()
{
	gl::Engine engine;
	engine.Init(1280, 720);
	engine.SetFrameBufferSizeCallback(gl::FramebufferSizeCallback);
	engine.SetCursorPosCallback(gl::MouseCallback);

	gl::WaterNoise water;
	engine.AddPass(&water);
	engine.Render();

	return 0;
}