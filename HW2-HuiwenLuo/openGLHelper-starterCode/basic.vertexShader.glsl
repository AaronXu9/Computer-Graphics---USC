#version 150

uniform mat4 projectionMatrix; 
uniform mat4 modelViewMatrix; 
uniform mat4 normalMatrix;

in vec3 position; 
in vec2 texCoord; 
in vec3 normal; 

out vec2 fragTexCoord; 
out vec3 fragNormal;
out vec3 fragPosition; 

void main() {
    fragTexCoord = texCoord;
	fragPosition = vec3(modelViewMatrix * vec4(position, 1.0f));
	fragNormal = normalize((normalMatrix*vec4(normal, 0.0f)).xyz);
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
}