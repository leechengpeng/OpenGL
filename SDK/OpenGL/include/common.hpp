#pragma once
#include <glad/glad.h>

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
}