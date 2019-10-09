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

		const Camera& GetCamera() const { return mCamera; }

	protected:
		Controller() : mCamera(glm::vec3(0.0f, 0.0f, 3.0f)), mFirstMouse(true) {}

	private:
		GLdouble	mLastX;
		GLdouble	mLastY;
		GLboolean	mFirstMouse;
		Camera		mCamera;

		static Controller* mInstance;
	};

	Controller* Controller::mInstance = nullptr;
}