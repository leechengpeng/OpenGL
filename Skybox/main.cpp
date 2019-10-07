#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <exception>
#include <vector>

#include "shader.hpp"

namespace gl
{
	class RenderPass
	{
	public:
		RenderPass() {}
		virtual ~RenderPass() {}

		virtual void Init() = 0;
		virtual void Update() = 0;

	private:
	};

	class DrawTriangle : public RenderPass
	{
	public:
		DrawTriangle() : mVao(0), mVbo(0), mShader() {}
		virtual ~DrawTriangle() {}

		virtual void Init() override;
		virtual void Update() override;

	private:
		GLuint mVao;
		GLuint mVbo;
		Shader mShader;
	};

	void DrawTriangle::Init()
	{
		float vertices[] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.0f,  0.5f, 0.0f
		};

		glGenVertexArrays(1, &mVao);
		glBindVertexArray(mVao);
		{
			glGenBuffers(1, &mVbo);
			glBindBuffer(GL_ARRAY_BUFFER, mVbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		glBindVertexArray(0);

		mShader.AttachShader(GL_VERTEX_SHADER, "Shader/triangle_vs.glsl");
		mShader.AttachShader(GL_FRAGMENT_SHADER, "Shader/triangle_fs.glsl");
		mShader.Link();
	}

	void DrawTriangle::Update()
	{
		mShader.Active();
		glBindVertexArray(mVao);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	class Renderer
	{
	public:
		Renderer();
		virtual ~Renderer();

		void Init(GLuint width, GLuint height, void (*framebufferSizeCallback)(GLFWwindow*, int, int));
		void Render();

		void AddPass(RenderPass* pass);

	private:
		GLFWwindow* mWindow;
		std::vector<RenderPass*> mRenderPasses;
	};

	Renderer::Renderer() : mWindow(nullptr)
	{

	}

	Renderer::~Renderer()
	{

	}

	void Renderer::Init(GLuint width, GLuint height, void (*framebufferSizeCallback)(GLFWwindow*, int, int))
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

		mWindow = glfwCreateWindow(width, height, "LearnOpenGL", NULL, NULL);
		if (mWindow == nullptr)
		{
			glfwTerminate();
			throw std::exception("Failed to init GLFW...");
		}

		glfwMakeContextCurrent(mWindow);
		glfwSetFramebufferSizeCallback(mWindow, framebufferSizeCallback);

		// glad: load all OpenGL function pointers
		// ---------------------------------------
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			throw std::exception("Failed to init GLAD...");
		}
	}

	void Renderer::Render()
	{
		for (auto& pass : mRenderPasses)
		{
			pass->Init();
		}

		while (!glfwWindowShouldClose(mWindow))
		{
			// processInput(mWindow);
			for (auto& pass : mRenderPasses)
			{
				pass->Update();
			}

			glfwSwapBuffers(mWindow);
			glfwPollEvents();
		}

		glfwTerminate();
	}

	void Renderer::AddPass(RenderPass* pass)
	{
		mRenderPasses.emplace_back(pass);
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

int main()
{
	gl::Renderer Renderer;
	Renderer.Init(1280, 720, framebuffer_size_callback);

	gl::DrawTriangle DrawTriangle;
	Renderer.AddPass(&DrawTriangle);

	Renderer.Render();

	return 0;
}