#version 440 core
out vec4 FragColor;
in vec2 TexCoord;
in vec3 FragPos;

uniform sampler2DArray NormalMap;
uniform sampler2DArray FoamMap;
uniform vec3 cameraPos;
uniform int u_resolution;
uniform samplerCube u_skybox;
uniform vec3 u_lengths;
uniform mat4 model;

uniform vec3 u_water_scatter_color;
uniform vec3 u_light_color;
uniform vec3 u_air_bubbles_color;
uniform vec3 u_light_position;
uniform float u_ambient_factor;
uniform float u_reflectivity_factor;
uniform float u_sss_factor1;
uniform float u_sss_factor2;
uniform float u_air_bubble_density;

void main()
{
    vec3 lightPosNormal = normalize(u_light_position);
    vec3 lightDir = normalize(u_light_position- FragPos);
    vec2 scaledTexCoord1 = fract(TexCoord * (float(u_resolution) / u_lengths[0]));
    vec2 scaledTexCoord2 = fract(TexCoord * (float(u_resolution) / u_lengths[1]));
    vec2 scaledTexCoord3 = fract(TexCoord * (float(u_resolution) / u_lengths[2]));

    vec3 normal1 = textureLod(NormalMap, vec3(scaledTexCoord1, 0.0f), 0.0).rgb * 2.0 - 1.0; // Convert from [0,1] to [-1,1]
    vec3 normal2 = textureLod(NormalMap, vec3(scaledTexCoord2, 1.0f), 0.0).rgb * 2.0 - 1.0;
    vec3 normal3 = textureLod(NormalMap, vec3(scaledTexCoord3, 2.0f), 0.0).rgb * 2.0 - 1.0;

    vec3 normal = normalize(normal1 + normal2 + normal3); 

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 worldNormal = normalize(normalMatrix * normal);

    vec3 V = normalize(cameraPos - FragPos);
    vec3 R = normalize(reflect(-lightDir, worldNormal));
    vec3 R_test = normalize(reflect(-lightPosNormal, worldNormal));
    vec3 H = normalize(lightDir + V);

    float fresnel = pow((1.0 - max(dot(V, worldNormal), 0.1)), 5.0);

    //ambient//////////////
    vec3 ambient = (u_ambient_factor * dot(worldNormal, vec3(0,1,0)) * u_water_scatter_color * u_light_color) + (u_air_bubble_density * u_air_bubbles_color * u_light_color);
    ///////////////////////

    //specular/////////////
    float spec = pow(max(dot(V, R_test), 0.0), 50);
    vec3 specular = u_light_color * spec * fresnel;
    ///////////////////////

    //env reflection///////
    vec3 reflectedSkybox = vec3(texture(u_skybox, reflect(-V, worldNormal)).rgb);
    vec3 env_reflection = fresnel * reflectedSkybox * u_reflectivity_factor;
    ///////////////////////

    //sub surface scattering
    float part1 = u_sss_factor1 * max(0, FragPos.y) * pow(max(dot(lightPosNormal, -V), 0.0f), 4.0f) * pow(0.5f - 0.5f * dot(lightPosNormal, worldNormal), 3.0f);
    float part2 = u_sss_factor2 * pow(max(dot(V, worldNormal), 0.0f), 2.0f);        
    vec3 scatter = (1 - fresnel) * (part1 + part2) * u_water_scatter_color * u_light_color;
    ////////////////////////

    vec3 finalOutput = ambient + specular + env_reflection + scatter;

    //foam//////////////////
    float foam = texture(FoamMap, vec3(scaledTexCoord1, 0.0f)).r;
    foam += texture(FoamMap, vec3(scaledTexCoord2, 1.0f)).r;
    foam += texture(FoamMap, vec3(scaledTexCoord3, 2.0f)).r;

    if (foam > 0.0f)
    {
        vec3 foamColor = vec3(foam, foam, foam);
        finalOutput += foamColor;
    }
    /////////////////////////

    // Apply lighting
    FragColor = vec4(finalOutput, 1.0); // Apply blue ocean shading
} 