#version 330 core

in vec3 vertex;
in vec3 normal;
in vec3 color;
in vec2 texcoord0;

out vec4 frag;

void main()
{
	frag = vec4(1,1,1, 1.0);
}
