#version 330 core
// directional Fragment shader 
// Last update Sept 27, 2021
//
uniform vec4 ambientLight;
uniform vec3 directionalLightDirection;
uniform vec3 directionalLightColor;
uniform vec3 halfVector;
uniform float shininess;
uniform float strength;
uniform int   mode;

in vec4 Position;
in vec3 Normal;
in vec4 Color;

out vec4 FragColor;

void main()
{
	float diffuse = max(0.0, dot(Normal, directionalLightDirection));
	float specular = max (0.0, dot (Normal, halfVector));
	
	if (diffuse == 0.0)  {
		specular = 0.0;
	} else {
		specular = pow(specular, shininess);
	}

	vec3 scatteredLight = ambientLight.rgb + directionalLightColor * diffuse;
    vec3 reflectedLight = directionalLightColor * specular * strength;
    vec3 rgb = min(Color.rgb * scatteredLight +
 						reflectedLight, vec3(1.0));
//	if (diffuse == 0.0) {
//		FragColor = vec4(1.0, 0.0, 0.0, 1.0);
//	} else {
		FragColor = vec4(rgb, Color.a);
//	}
}