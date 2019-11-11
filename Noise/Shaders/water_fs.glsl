#version 330 core
out vec4 FragColor;

uniform float _time;
uniform sampler2D tex;
uniform sampler2D tex_noise;

in vec2 TexCoord;

void main()
{
    float scale_x = 0.1;
    float scale_y = 0.1;
    float speed_x = 1.f;
    float speed_y = 1.f;

    float offset1 = texture(tex_noise, TexCoord + vec2(_time * speed_x, 0.f)).r;
    float offset2 = texture(tex_noise, TexCoord + vec2(0.f, _time * speed_y)).r;
    vec2 offset = vec2(offset1, offset2) - 0.35;
    offset *= vec2(scale_x, scale_y);

    vec3 base_col = texture(tex, TexCoord + offset).rgb;

    FragColor = vec4(base_col, 1.f);
    // FragColor = vec4(_time * 10, 0.f, 0.f, 1.f);
}