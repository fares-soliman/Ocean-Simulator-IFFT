#include "scene.h"

#include <iostream>
#include <random>


Scene::Scene() : camera(glm::vec3(30.0f, 0.0f, 3.0f))
{

}

void Scene::Init()
{
	initalSpectrum = new ComputeShader(".\\shaders\\initialspectrum.comp");
	negativeInitalSpectrum = new ComputeShader(".\\shaders\\negativeInitialspectrum.comp");
	timeSpectrum = new ComputeShader(".\\shaders\\timespectrum.comp");
	turkeyVertFFT = new ComputeShader(".\\shaders\\turkeyvertical.comp");
	turkeyHorizFFT = new ComputeShader(".\\shaders\\turkeyhorizontal.comp");
	shadableTexOutput = new ComputeShader(".\\shaders\\shadeableTexOutput.comp");
	butterfly = new ComputeShader(".\\shaders\\butterflytex.comp");

	ocean = new Ocean();
	skybox = new Skybox({ ".\\res\\Epic_BlueSunset_Cam_2_Left+X.png",
				".\\res\\Epic_BlueSunset_Cam_3_Right-X.png",
				".\\res\\Epic_BlueSunset_Cam_4_Up+Y.png",
				".\\res\\Epic_BlueSunset_Cam_5_Down-Y.png",
				".\\res\\Epic_BlueSunset_Cam_0_Front+Z.png",
				".\\res\\Epic_BlueSunset_Cam_1_Back-Z.png"
			   });

	initInitialSpectrum();
	genInitialSpectrum();
	initTimeSpectrum();
	initTempTex();
	initShadingTex();
	genButterflyTex();

#if DEBUG
	screenQuad = new Shader(".\\shaders\\quad.vert", ".\\shaders\\quad.frag");
	debugTex = displacementTex;
#endif
}

void Scene::Update(float lastframe, float deltaTime)
{
	timeSpectrumUpdate(lastframe, deltaTime);

	turkeyFFT(turkeyHorizFFT, heightTex);
	turkeyFFT(turkeyVertFFT, heightTex);

	turkeyFFT(turkeyHorizFFT, dxTex);
	turkeyFFT(turkeyVertFFT, dxTex);

	turkeyFFT(turkeyHorizFFT, dzTex);
	turkeyFFT(turkeyVertFFT, dzTex);

	turkeyFFT(turkeyHorizFFT, jacobianXxZzTex);
	turkeyFFT(turkeyVertFFT, jacobianXxZzTex);

	turkeyFFT(turkeyHorizFFT, jacobianXzTex);
	turkeyFFT(turkeyVertFFT, jacobianXzTex);

	genShadingTex(deltaTime);
}

void Scene::Draw()
{
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	glm::mat4 view = camera.GetViewMatrix();

	skybox->draw(projection, camera);
	ocean->draw(projection, view, glm::mat4(1.0), displacementTex, normalTex, foamTex, camera.Position, skybox->tbo);

#if DEBUG
	renderQuad();
#endif
}

//initialize the intial spectrum and gaussian textures
void Scene::initInitialSpectrum()
{
	std::vector<float> gaussian_rnd(4 * RESOLUTION_DIM * RESOLUTION_DIM);
	std::random_device dev;
	std::mt19937 rng(dev());
	std::normal_distribution<float> dist(0.f, 1.f); //~N(0,1)
	for (int i = 0; i < (int)gaussian_rnd.size(); ++i)
		gaussian_rnd[i] = dist(rng);

	glGenTextures(1, &spectrumTex_array);
	glBindTexture(GL_TEXTURE_2D_ARRAY, spectrumTex_array);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	glGenTextures(1, &gaussTex);
	glBindTexture(GL_TEXTURE_2D, gaussTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, 0, GL_RGBA,
		GL_FLOAT, gaussian_rnd.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}

//populate initial spectrum texture 
void Scene::genInitialSpectrum()
{
	initalSpectrum->use();
	initalSpectrum->setInt("u_resolution", RESOLUTION_DIM);
	initalSpectrum->setVec2("u_wind", glm::vec2(WIND_X, WIND_Y));
	initalSpectrum->setFloat("u_fetch", FETCH);
	initalSpectrum->setFloat("u_depth", DEPTH);
	initalSpectrum->setFloat("u_swell", SWELL);
	initalSpectrum->setFloat("u_wind_angle", WIND_ANGLE);
	initalSpectrum->setVec3("u_lengths", LENGTHS);
	initalSpectrum->setVec3("u_low_cascades", LOWWAVENUMBERS);
	initalSpectrum->setVec3("u_high_cascades", HIGHWAVENUMBERS);

	glBindImageTexture(0, spectrumTex_array, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(1, gaussTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	initalSpectrum->dispatch((unsigned int)TEXTURE_DIM / WORKGROUP_SIZE);
	initalSpectrum->wait();

	negativeInitalSpectrum->use();
	negativeInitalSpectrum->setInt("u_resolution", RESOLUTION_DIM);
	glBindImageTexture(0, spectrumTex_array, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

	negativeInitalSpectrum->dispatch((unsigned int)TEXTURE_DIM / WORKGROUP_SIZE);
	negativeInitalSpectrum->wait();
}

//initialize the spectrum textures that get updated every tick
void Scene::initTimeSpectrum()
{
	glGenTextures(1, &heightTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, heightTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	glGenTextures(1, &dxTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dxTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	glGenTextures(1, &dzTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, dzTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	glGenTextures(1, &jacobianXxZzTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, jacobianXxZzTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	glGenTextures(1, &jacobianXzTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, jacobianXzTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

}

//initialize ping-ping texture used for turkeyFFT
void Scene::initTempTex()
{
	glGenTextures(1, &tempTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tempTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

//initialize textures applied to ocean mesh
void Scene::initShadingTex()
{
	glGenTextures(1, &displacementTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, displacementTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	glGenTextures(1, &normalTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, normalTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	glGenTextures(1, &foamTex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, foamTex);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, TEXTURE_DIM, TEXTURE_DIM, NUMBER_OF_CASCADES, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

unsigned int reverse(unsigned int i, unsigned int log_2_N) {
	unsigned int res = 0;
	for (int j = 0; j < log_2_N; j++) {
		res = (res << 1) + (i & 1);
		i >>= 1;
	}
	return res;
}

//gen butterfly texture needed for turkeyFFT
void Scene::genButterflyTex()
{
	int reversals_line[RESOLUTION_DIM];

	unsigned int log_2_N = log(RESOLUTION_DIM) / log(2);

	for (int i = 0; i < RESOLUTION_DIM; ++i) {
		reversals_line[i] = reverse(i, log_2_N);
	}

	butterfly->use();
	butterfly->setInt("N", RESOLUTION_DIM);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(reversals_line), reversals_line, GL_STATIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);

	glGenTextures(1, &butterflyTex);
	glBindTexture(GL_TEXTURE_2D, butterflyTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, log_2_N, TEXTURE_DIM, 0, GL_RGBA,
		GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindImageTexture(0, butterflyTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	butterfly->dispatch((unsigned int)TEXTURE_DIM);
	butterfly->wait();
}

//updates time spectrum on all frequency based textures
void Scene::timeSpectrumUpdate(float lastFrame, float deltaTime)
{
	timeSpectrum->use();
	timeSpectrum->setInt("u_resolution", RESOLUTION_DIM);
	timeSpectrum->setVec3("u_lengths", LENGTHS);
	timeSpectrum->setFloat("u_choppiness", CHOPPINESS);
	timeSpectrum->setFloat("u_time", lastFrame + deltaTime);

	glBindImageTexture(0, spectrumTex_array, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, heightTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, dxTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(3, dzTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(4, jacobianXxZzTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(5, jacobianXzTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	timeSpectrum->dispatch((unsigned int)TEXTURE_DIM / WORKGROUP_SIZE);
	timeSpectrum->wait();
}

//performs turkeyFFT given a frequency texture and a vert or horizontal shader
void Scene::turkeyFFT(ComputeShader* turkeyShader, GLuint& freqTex)
{
	turkeyShader->use();
	unsigned int log_2_N = log(RESOLUTION_DIM) / log(2);
	bool swap_temp = false;

	glBindImageTexture(0, butterflyTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	auto& tmp = tempTex;

	for (int stage = 0; stage < log_2_N; ++stage)
	{
		glBindImageTexture(1, freqTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(2, tmp, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		turkeyShader->setInt("stage", stage);

		turkeyShader->dispatch((unsigned int)TEXTURE_DIM / WORKGROUP_SIZE);
		turkeyShader->wait();
		std::swap(tmp, freqTex);
		swap_temp = !swap_temp;
	}
	if (swap_temp) std::swap(tmp, freqTex);
}

//outputs final texztures needed for ocean shading
void Scene::genShadingTex(float deltaTime)
{
	shadableTexOutput->use();
	glBindImageTexture(0, heightTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, dxTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(2, dzTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(3, jacobianXxZzTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(4, jacobianXzTex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(5, normalTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(6, displacementTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(7, foamTex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

	shadableTexOutput->setFloat("u_delta_time", deltaTime);
	shadableTexOutput->setFloat("u_choppiness", CHOPPINESS);
	shadableTexOutput->setFloat("u_foam_amount", FOAM_AMOUNT);
	shadableTexOutput->setFloat("u_foam_decay", FOAM_DECAY);

	shadableTexOutput->dispatch((unsigned int)TEXTURE_DIM / WORKGROUP_SIZE);
	shadableTexOutput->wait();
}

#if DEBUG
void Scene::renderQuad()
{
	screenQuad->use();
	screenQuad->setInt("tex", 0);
	screenQuad->setFloat("layer", 0.0);
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	//glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, debugTex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
#endif


