#version 150

//light setting
uniform vec4 lightAmbient;
uniform vec4 lightDiffuse;
uniform vec4 lightSpecular;
uniform vec3 lightPosition;

//material setting
uniform sampler2D materialTex;
uniform float matKa;
uniform float matKd;
uniform float matKs;
uniform float matKsExp;

uniform vec3 cameraPosition;

in vec2 fragTexCoord; 
in vec3 fragNormal;
in vec3 fragPosition; 

out vec4 finalColor;

void main() {
	//finalColor = texture(materialTex, fragTexCoord);
	vec4 surfaceColor = texture(materialTex, fragTexCoord);
	vec3 lightdir = normalize(lightPosition - fragPosition);
	vec3 eyedir = normalize(cameraPosition - fragPosition);
	vec3 reflectdir = -reflect(lightdir, fragNormal);

	//phong lighting
	float kd = max(dot(lightdir, fragNormal), 0.0f);
	float ks = max(dot(reflectdir, eyedir), 0.0f);
	vec4 ambient = matKa * lightAmbient * surfaceColor;
	vec4 diffuse = matKd * kd * lightDiffuse * surfaceColor;
	vec4 specular = matKs * pow(ks, matKsExp) * lightSpecular * surfaceColor;

	finalColor = ambient + diffuse + specular;
}