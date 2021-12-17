#version 330 core
//
// Pass through Vertex shader.  
// Passes vertex information through without changing it.
//  Being used for debugging purposes.
// 
uniform mat4 MVMatrix;
uniform mat4 MVPMatrix;
uniform mat3 normalMatrix;

in vec4 vColor;
in vec4 vPosition;
in vec3 vNormal;

out vec4 Color;
out vec3 Normal;
out vec4 Position;

void main()
{
	Color = vColor;
	Normal = normalize(normalMatrix * vNormal);
	Position = MVMatrix * vPosition;
    gl_Position = MVPMatrix * vPosition;
}
