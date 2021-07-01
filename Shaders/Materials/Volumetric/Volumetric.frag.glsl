// Fragment position in model space
layout( location = 0 ) in vec3 in_position;
// view position in model space
layout( location = 1 ) in vec3 in_eyeInModelSpace;

// The modeltoimage matrix
flat in mat4 invbiasmvp;
flat in mat4 world2model;

// the depth and color texture whre the volume is rendered.
uniform sampler2D imageColor;
uniform sampler2D imageDepth;

#include "DefaultLight.glsl"
#include "Volumetric.glsl"

out vec4 fragColor;

void main() {

    // get image informations
    ivec3 texSize        = textureSize( material.density, 0 );
    ivec2 colorImageSize = textureSize( imageColor, 0 );

    // Transform all geometries from model-space to the canonical grid space
    vec4 pos = material.modelToDensity * vec4( in_position, 1 );
    vec4 eye = material.modelToDensity * vec4( in_eyeInModelSpace, 1 );

    // normalize so that everything is in [0,1]^3 ( canonical space)
    pos.xyz /= texSize;
    eye.xyz /= texSize;
    // compute the ray dir in canonical space
    vec3 dir = pos.xyz - eye.xyz;
    float dt = length( dir );
    dir      = normalize( dir );

    // Compute the depth limit to an object inside the volume
    // compute limit point in screenspace
    vec3 fpos        = gl_FragCoord.xyz / vec3( colorImageSize, 1 );
    vec3 objectDepth = vec3( fpos.xy, texture( imageDepth, fpos.xy ).r );
    if ( objectDepth.z < 1 )
    {
        // Transform limit point to model space
        vec4 objPoint = invbiasmvp * vec4( objectDepth, 1 );
        objPoint /= objPoint.w;
        // Transform limit point to canonical space
        objPoint = material.modelToDensity * vec4( objPoint.xyz, 1 );
        objPoint.xyz /= texSize;
        vec3 dirObject = objPoint.xyz - eye.xyz;
        float tmax     = length( dirObject );
        // compute the depth limit
        dt = ( tmax - dt );
    }
    else
    {
        // no limit, set it to infinity
        dt = 2;
    }

    // compute the light information in canonical space
    Light l = transformLight( light, material.modelToDensity * world2model );
    l.color *= 100; // TODO understand why this is necessary : check units and physical properties of the volume
    switch ( light.type )
    {
    case 0:
        break;
    case 1:
        l.point.position /= texSize;
        break;
    case 2:
        l.spot.position /= texSize;
        break;
    default:
        break;
    }

    // raymarch on the volume
    vec3 p               = pos.xyz; // position that progress in the volume
    vec3 volColor        = vec3( 0 );
    vec3 volTransmitance = vec3( 1 );
    bool hit             = raymarch( material, p, dir, l, dt, volColor, volTransmitance );

    // compute final color
    if ( !hit )
    {
        discard; // no medium found
    }
    else
    {
        vec4 backColor = texture( imageColor, fpos.xy );
        fragColor      = vec4( volColor + backColor.rgb * volTransmitance, 1 );
    }
}
