#pragma once
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>

namespace gl
{
	class Shader
	{
	public:
		Shader();
		~Shader();

		void AttachShader(GLuint shader_type, const std::string& shader_path) const;
		void AttachVertexShader(const std::string& shader_path) const;
		void AttachFragmentShader(const std::string& shader_path) const;

		void Link() const;
		void Active() const;

		void SetValue(const std::string& name, bool value) const;
		void SetValue(const std::string& name, int value) const;
		void SetValue(const std::string& name, float value) const;
		void SetValue(const std::string& name, const glm::vec3& value) const;
		void SetMatrix(const std::string& name, const float* mat) const;

		GLuint program() const { return program_; }

	private:
		void __LoadShader(const std::string& shader_path, std::string& shader_source) const;

		GLuint program_;
	};

	Shader::Shader() : program_(0)
	{
		program_ = glCreateProgram();
	}

	Shader::~Shader()
	{
		program_ = 0;
	}

	void Shader::AttachShader(GLuint shader_type, const std::string& shader_path) const
	{
		assert(program_ != 0);
		assert(shader_type == GL_VERTEX_SHADER || shader_type == GL_FRAGMENT_SHADER || shader_type == GL_GEOMETRY_SHADER || 
			shader_type == GL_TESS_CONTROL_SHADER || shader_type == GL_TESS_EVALUATION_SHADER);

		std::string shader_source;
		__LoadShader(shader_path, shader_source);
		const char* pshader_source = shader_source.c_str();

		GLuint shader = glCreateShader(shader_type);
		glShaderSource(shader, 1, &pshader_source, NULL);
		glCompileShader(shader);

		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			char log_info[512];
			glGetShaderInfoLog(shader, 512, NULL, log_info);
			std::cerr << "Compile shader failed: " << log_info << std::endl;
		}

		glAttachShader(program_, shader);
		glDeleteShader(shader);
	}
	
	inline void Shader::AttachVertexShader(const std::string& shader_path) const
	{
		AttachShader(GL_VERTEX_SHADER, shader_path);
	}

	inline void Shader::AttachFragmentShader(const std::string& shader_path) const
	{
		AttachShader(GL_FRAGMENT_SHADER, shader_path);
	}

	inline void Shader::Link() const
	{
		assert(program_ != 0);
		glLinkProgram(program_);

		GLint status;
		glGetProgramiv(program_, GL_LINK_STATUS, &status);
		if (!status) {
			char log_info[512];
			glGetProgramInfoLog(program_, 512, NULL, log_info);
			std::cerr << "Link shader failed: " << log_info << std::endl;
		}
	}

	inline void Shader::Active() const
	{
		assert(program_ != 0);
		glUseProgram(program_);
	}

	inline void Shader::SetValue(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(program_, name.c_str()), static_cast<int>(value));
	}

	inline void Shader::SetValue(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(program_, name.c_str()), value);
	}

	inline void Shader::SetValue(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(program_, name.c_str()), value);
	}

	inline void Shader::SetValue(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(program_, name.c_str()), 1, &value[0]);
	}

	inline void Shader::SetMatrix(const std::string& name, const float* mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(program_, name.c_str()), 1, GL_FALSE, mat);
	}

	void Shader::__LoadShader(const std::string& shader_path, std::string& shader_source) const
	{
		std::ifstream input;
		// 保证ifstream对象可以抛出异常
		input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			input.open(shader_path);
			std::stringstream stream;
			stream << input.rdbuf();
			shader_source = stream.str();
			input.close();
		}
		catch (std::ifstream::failure e)
		{
			std::cerr << "Failed to load shader file: " << shader_path << std::endl;
		}
	}
}