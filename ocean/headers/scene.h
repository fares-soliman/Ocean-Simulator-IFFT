#pragma once

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <../headers/camera.h>
#include <../headers/skybox.h>
#include <../headers/ocean.h>
#include <../headers/compute.h>
#include <../headers/sim_params.h>


class Scene {

public:
	Scene();
	void Init();
	void Update(float lastframe, float deltaTime);
	void Draw();

	Camera camera;
#if DEBUG
	GLuint debugTex;
#endif

private:
	void initInitialSpectrum();
	void genInitialSpectrum();
	void initTimeSpectrum();
	void initTempTex();
	void initShadingTex();
	void genButterflyTex();
	void timeSpectrumUpdate(float lastFrame, float deltaTime);
	void turkeyFFT(ComputeShader* turkeyShader, GLuint& freqTex);
	void genShadingTex(float deltaTime);

	GLuint spectrumTex_array;
	GLuint gaussTex;
	GLuint heightTex;
	GLuint dxTex;
	GLuint dzTex;
	GLuint tempTex;
	GLuint displacementTex;
	GLuint normalTex;
	GLuint foamTex;
	GLuint butterflyTex;
	GLuint jacobianXxZzTex;
	GLuint jacobianXzTex;
	GLuint ssbo;

	ComputeShader* initalSpectrum;
	ComputeShader* negativeInitalSpectrum;
	ComputeShader* timeSpectrum;
	ComputeShader* turkeyVertFFT;
	ComputeShader* turkeyHorizFFT;
	ComputeShader* shadableTexOutput;
	ComputeShader* butterfly;

	Ocean* ocean;
	Skybox* skybox;

#if DEBUG
	unsigned int quadVAO = 0;
	unsigned int quadVBO;
	Shader* screenQuad;
	void renderQuad();
#endif
};

