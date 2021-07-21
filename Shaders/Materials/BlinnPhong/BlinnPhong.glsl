#ifndef BLINNPHONGMATERIAL_GLSL
#define BLINNPHONGMATERIAL_GLSL

//------------------- VertexAttrib interface ---------------------
vec4 getPerVertexBaseColor();
//----------------------------------------------------------------

// Blinn-Phong specular and exponent :
// As this shader is built on Blinn-Phong NDF, that approximate Cook-Torrance,
// Specular parameters from the asset must be converted so that specular highlight look the same
// than with standard Phong BRDF (divided both Ks and Ns by Pi looks good). See
// http://www.thetenthplanet.de/archives/255 for intuitions
const float Pi         = 3.141592653589793f;
const float OneOver2Pi = 0.159154943091895f;

struct BlinnPhongTextures {
    int hasKd;
    int hasKs;
    int hasNs;
    int hasNormal;
    int hasAlpha;

    sampler2D kd;
    sampler2D ks;
    sampler2D ns;
    sampler2D normal;
    sampler2D alpha;
};

struct Material {
    vec4 kd;
    vec4 ks;

    float ns;
    float alpha;

    int hasPerVertexKd;
    int renderAsSplat;

    BlinnPhongTextures tex;
};

// Implementation of the emissivity interface.
// For now, BlinnPhong Material is not emissive
vec3 getEmissiveColor( Material material, vec3 textCoord ) {
    return vec3( 0 );
}

// Du to access to in_vertexColor this does not respect the "Material.glsl" interface as it access
// data outside the material
vec4 getDiffuseColor( Material material, vec3 texCoord ) {
    vec4 dc = vec4( material.kd.rgb, material.alpha );
    if ( material.hasPerVertexKd == 1 ) { dc.rgb = getPerVertexBaseColor().rgb; }
    if ( material.tex.hasKd == 1 ) { dc.rgb = texture( material.tex.kd, texCoord.xy ).rgb; }
    if ( material.tex.hasAlpha == 1 ) { dc.a *= texture( material.tex.alpha, texCoord.xy ).r; }
    if ( material.renderAsSplat == 1 ) { dc.a = ( dot( texCoord.xy, texCoord.xy ) > 1 ) ? 0 : 1; }
    return dc;
}

vec4 getBaseColor( Material material, vec3 texCoord ) {
    return getDiffuseColor( material, texCoord );
}

vec3 getSpecularColor( Material material, vec3 texCoord ) {
    if ( material.tex.hasKs == 1 ) { return vec3( texture( material.tex.ks, texCoord.xy ) ); }

    return material.ks.xyz / Pi; // Phong specular to blinn-phong exponent
}

float getNs( Material material, vec2 texCoord ) {
    float ns = material.ns;
    if ( material.tex.hasNs == 1 ) { ns = texture( material.tex.ns, texCoord ).r; }

    return max( ns, 0.001 ) / Pi; // Phong exponent to blinn-phong exponent
}

vec3 getNormal( Material material, vec3 texCoord, vec3 N, vec3 T, vec3 B ) {
    if ( material.tex.hasNormal == 1 ) {
        vec3 normalLocal = normalize( vec3( texture( material.tex.normal, texCoord.xy ) ) * 2 - 1 );
        mat3 tbn;
        tbn[0] = T;
        tbn[1] = B;
        tbn[2] = N;
        return normalize( tbn * normalLocal );
    }

    return normalize( N );
}

bool toDiscard( Material material, vec4 color ) {
    return color.a < 1;
}

vec3 diffuseBSDF( Material material, vec3 texC ) {
    return getDiffuseColor( material, texC ).rgb / Pi;
}

vec3 specularBSDF( Material material, vec3 texC, vec3 L, vec3 V, vec3 N ) {
    // http://www.thetenthplanet.de/archives/255
    // Minimalist Cook-Torrance using Blinn-Phong approximation
    vec3 Ks  = getSpecularColor( material, texC );
    float Ns = getNs( material, texC.xy );
    vec3 H   = V + L;
    if ( length( H ) < 0.001 ) { H = N; }
    else {
        H = normalize( H );
    }
    float D  = ( Ns + 1 ) * OneOver2Pi * pow( max( dot( N, H ), 0.0f ), Ns );
    float FV = 0.25f * pow( clamp( dot( L, H ), 0.001f, 1.f ), -3.f );
    return Ks * D * FV;
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
    specular = specularBSDF( material, texC, L, V, N );
    return 1;
}

// Blinn-Phong exponent to Roughness conversiont :
// https://computergraphics.stackexchange.com/questions/1515/what-is-the-accepted-method-of-converting-shininess-to-roughness-and-vice-versa
float getGGXRoughness( Material material, vec3 texC ) {
    float Ns = getNs( material, texC.xy );
    if ( Ns > 1 ) { Ns = Ns / 128; }
    float r = clamp( 1 - Ns, 0.04f, 0.96f );
    return pow( r, 0.5f );
}

// wi (light direction) and wo (view direction) are in local frame
// wi dot N is then wi.z ...
vec3 evaluateBSDF( Material material, vec3 texC, vec3 l, vec3 v ) {
    vec3 diff = diffuseBSDF( material, texC );
    vec3 spec = specularBSDF( material, texC, l, v, vec3( 0, 0, 1 ) );
    return ( diff + spec ) * max( l.z, 0.0f );
}

uniform Material material;

#endif // BLINNPHONGMATERIAL_GLSL
