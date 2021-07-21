layout( location = 0 ) out vec4 f_Accumulation;
layout( location = 1 ) out vec4 f_Revealage;

#include "Structs.glsl"

uniform Material material;
uniform Light light;

layout( location = 0 ) in vec3 in_position;
layout( location = 1 ) in vec3 in_texcoord;
layout( location = 2 ) in vec3 in_color;

void main() {
    if ( ( material.tex.hasAlpha == 1 && texture( material.tex.alpha, in_texcoord.st ).r < 0.1 ) ||
         material.alpha < 0.01 ) {
        discard;
    }

    float a = material.alpha;
    float z = -in_position.z;

    float va  = ( a + 0.01f );
    float va2 = va * va;
    float va4 = va2 * va2; // Pow4

    float vz  = abs( z ) / 200.0f;
    float vz2 = vz * vz;
    float vz4 = vz2 * vz2;

    float w = va4 + clamp( 0.3f / ( 0.00001f + vz4 ), 0.01, 3000.0 );

    vec3 color;
    if ( material.tex.hasKd == 1 ) { color = texture( material.tex.kd, in_texcoord.st ).rgb; }
    else {
        color = in_color;
    }

    f_Accumulation = vec4( color * a, a ) * w;
    f_Revealage    = vec4( a );
}
