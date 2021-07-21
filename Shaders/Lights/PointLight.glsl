#ifndef POINTLIGHT_GLSL
#define POINTLIGHT_GLSL

struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};

struct DirectionalLight {
    vec3 direction;
};

struct PointLight {
    vec3 position;
    Attenuation attenuation;
};

struct SpotLight {
    vec3 position;
    vec3 direction;

    Attenuation attenuation;

    float innerAngle;
    float outerAngle;
};

struct Light {
    int type;
    vec4 color;

    DirectionalLight directional;
    PointLight point;
    SpotLight spot;
};

vec3 lightDirection( Light light, vec3 position ) {
    return light.point.position - position;
}

vec3 lightContributionFrom( Light light,
                            vec3 position,
                            Material material,
                            vec2 T,
                            vec3 V,
                            vec3 N,
                            vec3 X,
                            vec3 Y ) {
    float d = length( light.point.position - position );
    vec3 materialColor =
        computeMaterialInternal( material, T, lightDirection( light ), V, N, X, Y );

    return light.color.xyz / ( d * d ) * materialColor;
}

uint lightCount() {
    return uint( 1 );
}

uniform Light light;

#endif // POINTLIGHT_GLSL
