#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <exception>
#include <vector>
#include <functional>
#include "renderpass.hpp"
#include "controller.hpp"

namespace gl
{
	using FrameBufferSizeFunc = void(*)(GLFWwindow*, int, int);
	using CursorPosCallFunc = void(*)(GLFWwindow*, double, double);

	class Engine
	{
	public:
		Engine();
		virtual ~Engine();

		void Init(GLuint width, GLuint height);
		void Render();

		void AddPass(RenderPass* tPass);
		void SetFrameBufferSizeCallback(FrameBufferSizeFunc tCallback);
		void SetCursorPosCallback(CursorPosCallFunc tCallback);

	private:
		GLfloat						mDeltaTime;
		GLfloat						mLastFrame;
		GLFWwindow*					mWindow;
		std::vector<RenderPass*>	mRenderPasses;
	};

	Engine::Engine() : mWindow(nullptr)
	{

	}

	Engine::~Engine()
	{

	}

	void Engine::Init(GLuint width, GLuint height)
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

		// glad: load all OpenGL function pointers
		// ---------------------------------------
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			throw std::exception("Failed to init GLAD...");
		}
	}

	void Engine::Render()
	{
		for (auto& pass : mRenderPasses)
		{
			pass->Init();
		}

		while (!glfwWindowShouldClose(mWindow))
		{
			auto currentFrame = glfwGetTime();
			mDeltaTime = currentFrame - mLastFrame;
			mLastFrame = currentFrame;
			Controller::Instance()->ProcessInput(mWindow, mDeltaTime);

			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			for (auto& pass : mRenderPasses)
			{
				pass->Update();
			}

			glfwSwapBuffers(mWindow);
			glfwPollEvents();
		}

		glfwTerminate();
	}

	inline void Engine::AddPass(RenderPass* pass)
	{
		mRenderPasses.emplace_back(pass);
	}


	inline void Engine::SetFrameBufferSizeCallback(FrameBufferSizeFunc tCallback)
	{
		if (!mWindow)
		{
			throw std::exception("Please call Engine::Init() first...");
		}
		glfwSetFramebufferSizeCallback(mWindow, tCallback);
	}

	inline void Engine::SetCursorPosCallback(CursorPosCallFunc tCallback)
	{
		if (!mWindow)
		{
			throw std::exception("Please call Engine::Init() first...");
		}
		glfwSetCursorPosCallback(mWindow, tCallback);
	}
}