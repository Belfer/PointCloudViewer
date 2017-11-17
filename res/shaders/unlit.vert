#version 330 core

layout (location = 0) in vec3 POSITION;
layout (location = 1) in vec3 NORMAL;
layout (location = 2) in vec3 COLOR;
layout (location = 3) in vec2 TEXCOORD0;

out vec3 vertex;
out vec3 normal;
out vec3 color;
out vec2 texcoord0;

uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

void main() {
	gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(POSITION, 1.0);
	vertex      = POSITION;
	normal      = NORMAL;
	color       = COLOR;
	texcoord0   = TEXCOORD0;
}
