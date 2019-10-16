#include "engine.hpp"
#include "shader.hpp"
#include "controller.hpp"
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace gl
{
	unsigned int loadCubemap(const std::vector<std::string>& tFaces)
	{
		unsigned int texID;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

		for (unsigned int i = 0; i < tFaces.size(); i++)
		{
			int width, height, nrChannels;
			auto data = stbi_load(tFaces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				//GL_TEXTURE_CUBE_MAP_POSITIVE_X	右
				//GL_TEXTURE_CUBE_MAP_NEGATIVE_X	左
				//GL_TEXTURE_CUBE_MAP_POSITIVE_Y	上
				//GL_TEXTURE_CUBE_MAP_NEGATIVE_Y	下
				//GL_TEXTURE_CUBE_MAP_POSITIVE_Z	后
				//GL_TEXTURE_CUBE_MAP_NEGATIVE_Z	前
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			}
			else
			{
				std::cout << "Cubemap texture failed to load at path: " << tFaces[i] << std::endl;
			}
			stbi_image_free(data);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		return texID;
	}

	class Skybox : public RenderPass
	{
	public:
		Skybox() : mVao(), mCubemapTex()
		{

		}

		virtual void Init() override
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

			glGenVertexArrays(1, &mVao);
			glBindVertexArray(mVao);
			{
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
			mCubemapTex = loadCubemap(faces);

			mShader.AttachShader(GL_VERTEX_SHADER, "Shader/skybox_vs.glsl");
			mShader.AttachShader(GL_FRAGMENT_SHADER, "Shader/skybox_fs.glsl");
			mShader.Link();
		}

		virtual void Update() override
		{
			auto& camera = Controller::Instance()->GetCamera();

			glm::mat4 model_mat = glm::mat4(1.0f);
			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)1280 / (float)720, 0.1f, 100.0f);
			//glm::mat4 view = camera.GetViewMatrix();
			glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));

			mShader.Active();
			mShader.SetMatrix("model", &model_mat[0][0]);
			mShader.SetMatrix("view", &view[0][0]);
			mShader.SetMatrix("projection", &projection[0][0]);

			glBindVertexArray(mVao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mCubemapTex);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
		}

	private:
		GLuint mCubemapTex;
		GLuint mVao;
		Shader mShader;
	};
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
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