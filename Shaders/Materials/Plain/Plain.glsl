#ifndef GLSL_PLAIN_MATERIAL
#define GLSL_PLAIN_MATERIAL
// Assume that plain rendering is based on very simple "material"
// This material is rendered as a plain, flat color comming either fro a uniform color, a
// per-vefrtex color or a texture.
struct PlainTextures {
    int hasColor;
    int hasMask;

    sampler2D color;
    sampler2D mask;
};

struct Material {
    int perVertexColor;
    vec4 color;
    PlainTextures tex;
};

//------------------- VertexAttrib interface ---------------------
vec4 getPerVertexBaseColor();
vec3 getWorldSpaceNormal();
#define DONT_USE_INPUT_TANGENT

//----------------------------------------------------------------
// Implementation of the emissivity interface.
// Plain Material is not emissive
vec3 getEmissiveColor( Material material, vec3 textCoord ) {
    return vec3( 0 );
}

vec4 getBaseColor( Material material, vec3 texCoord ) {
    vec4 dc = vec4( material.color.rgb, 1 );

    if ( material.perVertexColor == 1 ) { dc.rgb = getPerVertexBaseColor().rgb; }
    if ( material.tex.hasColor == 1 ) { dc.rgb = texture( material.tex.color, texCoord.xy ).rgb; }

    if ( material.tex.hasMask == 1 && texture( material.tex.mask, texCoord.xy ).r < 0.1 ) {
        dc.a = 0;
    }
    return dc;
}

// diffuseColor is basecolor
vec4 getDiffuseColor( Material material, vec3 texCoord ) {
    return getBaseColor( material, texCoord );
}

// specular color is black
vec3 getSpecularColor( Material material, vec3 texCoord ) {
    return vec3( 0 );
}

// Return the world-space normal computed according to the microgeometry definition`
// As no normal map is defined, return N
vec3 getNormal( Material material, vec3 texCoord, vec3 N, vec3 T, vec3 B ) {
    return N;
}

// return true if the fragment must be condidered as transparent (either fully or partially)
bool toDiscard( Material material, vec4 color ) {
    return ( color.a < 0.1 );
}

vec3 diffuseBSDF( Material material, vec3 texC ) {
    return getDiffuseColor( material, texC ).rgb;
}

// Note that diffuse and specular must not be multiplied by cos(wi) as this will be done when using
// the BSDF
int getSeparateBSDFComponent( Material material,
                              vec3 texC,
                              vec3 L,
                              vec3 V,
                              vec3 N,
                              out vec3 diffuse,
                              out vec3 specular ) {
    diffuse  = diffuseBSDF( material, texC );
    specular = vec3( 0 );
    return 1;
}

float getGGXRoughness( Material material, vec3 texC ) {
    return 1.f;
}

// wi (light direction) and wo (view direction) are in local frame
// wi dot N is then wi.z ...
vec3 evaluateBSDF( Material material, vec3 texC, vec3 l, vec3 v ) {
    return diffuseBSDF( material, texC );
}

uniform Material material;

#endif
