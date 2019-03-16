#include "ArcTools.h"
#include <random>

namespace Arc_Engine {
	std::string ArcTools::_currentPath = "";
	GLuint ArcTools::quadVAO = 0;
	GLuint ArcTools::quadVBO = 0;
	GLuint ArcTools::debugDepthShaderProgram = -1;
	GLuint ArcTools::PostEffectProgram = -1;
	
	GLuint ArcTools::noiseTexture = 0;
	GLuint ArcTools::g_buffer = 0;
	std::vector<glm::vec3> ArcTools::ssaoNoise;

	std::string ArcTools::debug_depth_vert_shader_path = "\\shadowShader\\shadow_vert.vert";
	std::string ArcTools::debug_depth_frag_shader_path = "\\shadowShader\\shadow_frag.frag";

	const std::string quad_vert_shader_path = "\\QuadShader\\quad_vert.vert";
	const std::string quad_frag_shader_path = "\\QuadShader\\quad_frag.frag";

	std::string ArcTools::getCurrentPath() {
		if (_currentPath.compare("") == 0) {
			char buf[80];
			_getcwd(buf, sizeof(buf)); //将当前工作目录的绝对路径复制到参数buffer所指的内存空间中
			std::string path1 = std::string(buf);
			std::size_t found = path1.find_last_of("/\\");
			_currentPath = path1.substr(0, found);
		}
		return _currentPath;
	}

	void ArcTools::drawDebugQuad(GLuint texture) {
		if (debugDepthShaderProgram == -1) {
			debugDepthShaderProgram = Arc_Engine::ArcMaterial::loadShaderAndCreateProgram(getCurrentPath() + shader_path + debug_depth_vert_shader_path, getCurrentPath() + shader_path + debug_depth_frag_shader_path);
		}
		glViewport(0, 0, WIDTH, HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(debugDepthShaderProgram);
		glUniform1f(glGetUniformLocation(debugDepthShaderProgram, "near_plane"), 1.0f);
		glUniform1f(glGetUniformLocation(debugDepthShaderProgram, "far_plane"), 7.5f);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		RenderQuad();
		glUseProgram(0);
	}
	
	void ArcTools::drawPostEffectQuad(GLuint texture, GLuint gbuffer, const glm::mat4 proj) {
		if (PostEffectProgram == -1) {
			PostEffectProgram = Arc_Engine::ArcMaterial::loadShaderAndCreateProgram(getCurrentPath() + shader_path + quad_vert_shader_path, getCurrentPath() + shader_path + quad_frag_shader_path);
		}
		glViewport(0, 0, WIDTH * ANTI_AILASING_MULTY_TIME, HEIGHT * ANTI_AILASING_MULTY_TIME);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(PostEffectProgram);
		glUniform1f(glGetUniformLocation(PostEffectProgram, "near_plane"), 1.0f);
		glUniform1f(glGetUniformLocation(PostEffectProgram, "far_plane"), 7.5f);

		GLint texLocation = glGetUniformLocation(PostEffectProgram, "screenTexture");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(texLocation, 0);

		GLint texLocation1 = glGetUniformLocation(PostEffectProgram, "gbufferTexture");
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gbuffer);
		glUniform1i(texLocation1, 1);

		generateSSAONoiceTextures();
		GLint texLocation2 = glGetUniformLocation(PostEffectProgram, "texNoise");
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glUniform1i(texLocation2, 2);

		GLint modelMatLocation = glGetUniformLocation(PostEffectProgram, "projection");
		glUniformMatrix4fv(modelMatLocation, 1, GL_FALSE, glm::value_ptr(proj));

		//std::cout << proj << std::endl;

		RenderQuad();
		glUseProgram(0);
	}

	void ArcTools::RenderQuad()
	{
		if (quadVAO == 0)
		{
			GLfloat quadVertices[] = {
				// Positions        // Texture Coords
				-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
			};
			// Setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}



	GLuint ArcTools::generateSSAONoiceTextures() {

		std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // 随机浮点数，范围0.0 - 1.0
		std::default_random_engine generator;
		std::vector<glm::vec3> ssaoKernel;
		for (GLuint i = 0; i < 64; ++i)
		{
			glm::vec3 sample(
				randomFloats(generator) * 2.0 - 1.0,
				randomFloats(generator) * 2.0 - 1.0,
				randomFloats(generator)
			);
			sample = glm::normalize(sample);
			sample *= randomFloats(generator);
			GLfloat scale = GLfloat(i) / 64.0;
			scale = lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;
			ssaoKernel.push_back(sample);
		}

		for (GLuint i = 0; i < 16; i++)
		{
			glm::vec3 noise(
				randomFloats(generator) * 2.0 - 1.0,
				randomFloats(generator) * 2.0 - 1.0,
				0.0f);
			ssaoNoise.push_back(noise);
		}

		glGenTextures(1, &noiseTexture);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		return noiseTexture;
	}
	
	GLfloat ArcTools::lerp(GLfloat a, GLfloat b, GLfloat f)
	{
		return a + f * (b - a);
	}
}