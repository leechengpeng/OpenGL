#include "engine.hpp"
#include "shader.hpp"
#include "common.hpp"
#include "controller.hpp"

namespace gl
{
	class Water : public RenderPass
	{
	public:

		virtual void Init() override
		{

		}

		virtual void Update() override
		{

		}
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

	gl::Water water;
	engine.AddPass(&water);
	engine.Render();

	return 0;
}