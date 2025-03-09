#version 440 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DArray tex;
uniform float layer;

void main()
{             
    vec4 texCol = texture(tex, vec3(TexCoords, layer)).rgba;      
    FragColor = texCol;
}