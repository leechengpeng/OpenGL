#pragma once
#include "camera.hpp"

namespace gl
{
	class Controller
	{
	public:
		virtual ~Controller() 
		{

		}

		static Controller* Instance()
		{
			if (!mInstance)
			{
				mInstance = new Controller();
			}
			return mInstance;
		}

		void MouseCallback(GLdouble xpos, GLdouble ypos)
		{
			if (mFirstMouse)
			{
				mLastX = xpos;
				mLastY = ypos;
				mFirstMouse = false;
			}

			float xoffset = xpos - mLastX;
			float yoffset = mLastY - ypos; // reversed since y-coordinates go from bottom to top

			mLastX = xpos;
			mLastY = ypos;

			mCamera.ProcessMouseMovement(xoffset, yoffset);
		}

		void ProcessInput(GLFWwindow* window, GLfloat tDeltaTime)
		{
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				mCamera.ProcessKeyboard(FORWARD, tDeltaTime);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				mCamera.ProcessKeyboard(BACKWARD, tDeltaTime);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				mCamera.ProcessKeyboard(LEFT, tDeltaTime);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				mCamera.ProcessKeyboard(RIGHT, tDeltaTime);
		}

		const Camera& GetCamera() const { return mCamera; }

	protected:
		Controller() : mCamera(glm::vec3(0.0f, 0.0f, 0.0f)), mFirstMouse(true) {}

	private:
		GLdouble	mLastX;
		GLdouble	mLastY;
		GLboolean	mFirstMouse;
		Camera		mCamera;

		static Controller* mInstance;
	};

	Controller* Controller::mInstance = nullptr;
}