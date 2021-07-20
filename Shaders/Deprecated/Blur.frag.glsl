in vec2 varTexcoord;
out vec4 fragColor;

uniform sampler2D color;
uniform vec2 offset;

const float weight[5] = float[]( 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 );

void main() {
    vec3 c = texture( color, varTexcoord ).rgb * weight[0];

    for ( int i = 1; i < 5; ++i ) {
        c += texture( color, varTexcoord + i * offset ).rgb * weight[i];
        c += texture( color, varTexcoord - i * offset ).rgb * weight[i];
    }

    fragColor = vec4( c, 1.0 );
}
