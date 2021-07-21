layout( location = 0 ) in vec3 in_position;
layout( location = 1 ) in vec3 in_normal;

struct Transform {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 mvp;
    mat4 modelView;
    mat4 worldNormal;
    mat4 viewNormal;
};

uniform Transform transform;
uniform int drawFixedSize;

out vec4 vColor;

void main() {
    mat4 mvp    = transform.proj * transform.view * transform.model;
    gl_Position = mvp * vec4( in_position, 1.0 );

    float l     = length( inNormal );
    vec4 normal = vec4( inNormal, 0.0 );
    if ( l != 0.0 ) {
        normal = normalize( normal );
        normal = ( 0.5 * normal ) + vec4( 0.5, 0.5, 0.5, 0.0 );
    }

    vColor = normal;
}
