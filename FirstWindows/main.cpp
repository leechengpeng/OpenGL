#include "engine.hpp"
#include "shader.hpp"

namespace gl
{
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
	gl::Engine engine;
	engine.Init(1280, 720, framebuffer_size_callback);

	gl::DrawTriangle DrawTriangle;
	engine.AddPass(&DrawTriangle);
	engine.Render();

	return 0;
}