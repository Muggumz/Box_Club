#version 440

layout(location = 0) in vec3 inNormal;

uniform layout (binding=0) samplerCube s_Environment;

out vec4 frag_color;

void main() {
    vec3 norm = normalize(inNormal);

    frag_color = vec4(texture(s_Environment, norm).rgb, 1.0);
}