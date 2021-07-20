#include "TransformStructs.glsl"

layout( location = 0 ) in vec3 in_position;
layout( location = 1 ) in vec3 in_normal;

uniform int drawFixedSize;
uniform Transform transform;

layout( location = 0 ) out vec3 out_position;
layout( location = 1 ) out vec3 out_normal;
layout( location = 2 ) out vec3 out_eye;

void main() {
    mat4 mvp;
    if ( drawFixedSize > 0 ) {
        // distance to camera
        mat4 modelView = transform.view * transform.model;
        float d        = length( modelView[3].xyz );
        mat3 scale3    = mat3( d );
        mat4 scale     = mat4( scale3 );
        mat4 model     = transform.model * scale;
        mvp            = transform.proj * transform.view * model;
    }
    else {
        mvp = transform.proj * transform.view * transform.model;
    }

    gl_Position = mvp * vec4( in_position, 1.0 );

    vec4 pos = transform.model * vec4( in_position, 1.0 );
    pos /= pos.w;
    vec3 normal = mat3( transform.worldNormal ) * in_normal;
    vec3 eye    = -transform.view[3].xyz * mat3( transform.view );

    out_position = vec3( pos );
    out_normal   = normal;
    out_eye      = vec3( eye );
}
