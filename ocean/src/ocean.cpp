#include "../headers\ocean.h"

Ocean::Ocean()
{
    initShader();
    initBuffer();
    initUniform();
}

Ocean::~Ocean()
{
}

void Ocean::draw(glm::mat4 projection, glm::mat4 view, glm::mat4 model, GLuint height_texture, GLuint normal_texture, GLuint foam_texture, glm::vec3 cameraPos, GLuint skyboxTex)
{
    shader->use();

    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setMat4("model", model);
    shader->setVec3("cameraPos", cameraPos);

    shader->setVec3("u_water_scatter_color", WATERSCATTERCOLOR);
    shader->setVec3("u_light_color", LIGHTCOLOR);
    shader->setVec3("u_air_bubbles_color", AIRBUBBLECOLOR);
    shader->setVec3("u_light_position", LIGHTPOSITION);
    shader->setFloat("u_ambient_factor", AMBIENT_FACTOR);
    shader->setFloat("u_reflectivity_factor", REFLECTIVITY_FACTOR);
    shader->setFloat("u_sss_factor1", SSS_FACTOR1);
    shader->setFloat("u_sss_factor2", SSS_FACTOR2);
    shader->setFloat("u_air_bubble_density", AIRBUBBLEDENSITY);

    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D_ARRAY, height_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, normal_texture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D_ARRAY, foam_texture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

}

void Ocean::initBuffer()
{
    int vertex_count = RESOLUTION_DIM;
    vertices.resize(vertex_count * vertex_count * VERTICIES_INDEX);
    indices.resize((RESOLUTION_DIM-1) * (RESOLUTION_DIM-1) * NUMBER_OF_VERTICIES_IN_A_SQUARE);

    unsigned int idx = 0;
    for (int z = -RESOLUTION_DIM / 2; z < RESOLUTION_DIM / 2; ++z)
    {
        for (int x = -RESOLUTION_DIM / 2; x < RESOLUTION_DIM / 2; ++x)
        {
            vertices[idx++] = float(x);
            vertices[idx++] = -3.f;
            vertices[idx++] = float(z);

            float u = ((float)x + (RESOLUTION_DIM / 2)) / (RESOLUTION_DIM);
            float v = ((float)z + (RESOLUTION_DIM / 2)) / (RESOLUTION_DIM);
            if (x == (RESOLUTION_DIM / 2) - 1)
                u = 1.f;
            if (z == (RESOLUTION_DIM / 2) - 1)
                v = 1.f;
            vertices[idx++] = u;
            vertices[idx++] = v;
        }
    }
    assert(idx == vertices.size());

    // counter-clockwise winding
    idx = 0;
    for (unsigned int y = 0; y < RESOLUTION_DIM-1; ++y)
    {
        for (unsigned int x = 0; x < RESOLUTION_DIM-1; ++x)
        {
            indices[idx++] = (vertex_count * y) + x;
            indices[idx++] = (vertex_count * (y + 1)) + x;
            indices[idx++] = (vertex_count * y) + x + 1;

            indices[idx++] = (vertex_count * y) + x + 1;
            indices[idx++] = (vertex_count * (y + 1)) + x;
            indices[idx++] = (vertex_count * (y + 1)) + x + 1;
        }
    }
    assert(idx == indices.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);

    //vertices
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //texture coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);

    //reset binding of buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Ocean::initShader()
{
    shader = new Shader(".\\shaders\\ocean.vert", ".\\shaders\\ocean.frag");
}

void Ocean::initUniform()
{
    shader->use();
    shader->setInt("u_displacement_map", 0);
    shader->setInt("NormalMap", 1);
    shader->setInt("FoamMap", 4);
    shader->setInt("u_skybox", 3);
    shader->setInt("u_resolution", RESOLUTION_DIM);
    shader->setVec3("u_lengths", LENGTHS);
}
