#pragma once

//all constants and parameters that alter the simulation

//Detail and Resolution
constexpr int SCR_WIDTH                         = 1920;
constexpr int SCR_HEIGHT                        = 1080;
constexpr int TEXTURE_DIM                       = 256;
constexpr int WORKGROUP_SIZE                    = 16;
constexpr int RESOLUTION_DIM                    = 256;
constexpr int NUMBER_OF_CASCADES                = 3;

// Wave Spectrum Constants
const glm::vec3 LENGTHS                         = glm::vec3(512,     256,     64);
const glm::vec3 LOWWAVENUMBERS                  = glm::vec3(0,       1,       2);
const glm::vec3 HIGHWAVENUMBERS                 = glm::vec3(1,       2,       9999);

//Initial Spectrum
constexpr float WIND_X                          = 7.0f;
constexpr float WIND_Y                          = 7.0f;
constexpr float FETCH                           = 400000.0f;
constexpr float DEPTH                           = 500.0f;
constexpr float SWELL                           = 0.5f;
constexpr float WIND_ANGLE                      = 45.0f;

//Time Spectrum
constexpr float CHOPPINESS                      = 1.0f;

//Shading params
const glm::vec3 WATERSCATTERCOLOR               = glm::vec3(0.424f,  0.812f,  0.741f);
const glm::vec3 LIGHTCOLOR                      = glm::vec3(0.920f,  0.799f,  0.55379f);
const glm::vec3 AIRBUBBLECOLOR                  = glm::vec3(0.0f,    0.25f,   1.0f);
const glm::vec3 LIGHTPOSITION                   = glm::vec3(-30.0f,    5.0f,    130.0f);

constexpr float AMBIENT_FACTOR                  = -0.1f;
constexpr float REFLECTIVITY_FACTOR             = 0.41f;
constexpr float SSS_FACTOR1                     = 0.40f;
constexpr float SSS_FACTOR2                     = 0.10f;
constexpr float AIRBUBBLEDENSITY                = 0.55f;

constexpr float FOAM_AMOUNT                     = 0.72f;
constexpr float FOAM_DECAY                      = 0.055f;

//DEBUG
#define         DEBUG                             0;