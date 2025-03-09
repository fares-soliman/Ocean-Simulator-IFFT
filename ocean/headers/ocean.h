#pragma once
#include <glad/glad.h> 
#include <glm/glm.hpp>

#include <vector>
#include "../headers/shader.h"
#include "../headers/camera.h"
#include "../headers/sim_params.h"

#define NUMBER_OF_VERTICIES_IN_A_SQUARE 6
#define VERTICIES 3
#define VERTICIES_INDEX 5

class Ocean
{
public:
	Ocean();
	~Ocean();
	void draw(glm::mat4 projection, glm::mat4 view, glm::mat4 model, GLuint height_texture, GLuint normal_texture, GLuint foam_texture, glm::vec3 cameraPos, GLuint skyboxTex);
	void initBuffer();
	void initShader();
	void initUniform();

private:
	GLuint vbo, vao, ibo;
	struct GridVertex
	{
		glm::vec3 position;
		glm::vec2 tex_coords;
	};
	std::vector<GLfloat> vertices;
	std::vector<unsigned int> indices;
	Shader* shader;

};

