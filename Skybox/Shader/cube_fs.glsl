#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 camPos;
uniform samplerCube skybox;

void main()
{    
    vec3 v = normalize(Position - camPos);
    vec3 r = reflect(v, normalize(Normal));
    FragColor = vec4(texture(skybox, r).rgb, 1.0);
}