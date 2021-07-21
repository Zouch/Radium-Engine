#ifndef SPOTLIGHT_GLSL
#define SPOTLIGHT_GLSL

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

vec3 lightDirection( Light light ) {
    return normalize( light.spot.direction );
}

vec3 lightContributionFrom( Light light,
                            vec3 position,
                            Material material,
                            vec2 T,
                            vec3 V,
                            vec3 N,
                            vec3 X,
                            vec3 Y ) {
    float d = length( light.spot.direction );

    float attenuation = light.spot.attenuation.constant + light.spot.attenuation.linear * d +
                        light.spot.attenuation.quadratic * d * d;

    vec3 l             = normalize( light.spot.position - position );
    float cosRealAngle = dot( l, lightDirection( light ) );
    float cosSpotOuter = cos( light.spot.innerAngle / 2.0 );
    float radialAttenuation =
        pow( clamp( ( cosRealAngle - cosSpotOuter ) / ( 1.0 - cosSpotOuter ), 0.0, 1.0 ), 1.6 );

    vec3 materialColor =
        computeMaterialInternal( material, T, lightDirection( light ), V, N, X, Y );

    return radialAttenuation / attenuation * light.color.xyz * materialColor;
}

uint lightCount() {
    return uint( 1 );
}

uniform Light light;

#endif
