#version 440 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 FragPos;

uniform sampler2DArray u_displacement_map;
uniform int u_resolution;
uniform vec3 u_lengths;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model; 

vec3 deformer(vec3 p0)
{
    vec2 scaledTexCoord1 = aTexCoord * (float(u_resolution) / u_lengths[0]);
    vec2 scaledTexCoord2 = aTexCoord * (float(u_resolution) / u_lengths[1]);
    vec2 scaledTexCoord3 = fract(aTexCoord * (float(u_resolution) / u_lengths[2]));
	
    p0 += vec3(textureLod(u_displacement_map, vec3(scaledTexCoord1, 0.0f), 0.0).rgb);
    p0 += vec3(textureLod(u_displacement_map, vec3(scaledTexCoord2, 1.0f), 0.0).rgb);
    p0 += vec3(textureLod(u_displacement_map, vec3(scaledTexCoord3, 2.0f), 0.0).rgb);

    return p0;
}

void main()
{
    vec3 displacedPos = deformer(aPos);
    FragPos = vec3(model * vec4(displacedPos, 1.0));
    vec3 vert = aPos;
    vert.x += displacedPos.x;
    vert.y += displacedPos.y;
    vert.z += displacedPos.z;
    gl_Position = projection * view * model * vec4(vert, 1.0);
    TexCoord = aTexCoord;
}