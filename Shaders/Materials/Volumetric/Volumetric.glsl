#ifndef VOLUMETRICMATERIAL_GLSL
#define VOLUMETRICMATERIAL_GLSL

// https://www.alanzucconi.com/2017/10/10/atmospheric-scattering-1/
// https://www.pbrt.org/fileformat-v3.html#media-world
// This material definition follows the pbrt v3 heterogeneous participating media definition
struct Material {
    // The density grid
    sampler3D density;
    // Absorption coefficient, default to (0.0011, 0.0024, 0.014, 1)
    vec4 sigma_a;
    // Scattering coefficient, default to (2.55, 3.21, 3.77, 1)
    vec4 sigma_s;
    // phase function assymetry factor, default to 0
    float g;
    // Scale factor applied to the absorption and scattering coefficients
    float scale;
    // step size for ray marching
    float stepsize;
    // Transformlation matrix to the caconical [0, 1]^3 domain
    mat4 modelToDensity;
};

// Some usefull constant
float Inv4Pi = 0.07957747154594766788;

float VanderCorput( uint bits ) {
    bits = ( bits << 16u ) | ( bits >> 16u );
    bits = ( ( bits & 0x55555555u ) << 1u ) | ( ( bits & 0xAAAAAAAAu ) >> 1u );
    bits = ( ( bits & 0x33333333u ) << 2u ) | ( ( bits & 0xCCCCCCCCu ) >> 2u );
    bits = ( ( bits & 0x0F0F0F0Fu ) << 4u ) | ( ( bits & 0xF0F0F0F0u ) >> 4u );
    bits = ( ( bits & 0x00FF00FFu ) << 8u ) | ( ( bits & 0xFF00FF00u ) >> 8u );
    return float( bits ) * 2.3283064365386963e-10; // / 0x100000000
}

// The Henyey-Greenstein phase function
float phaseHG( float cosTheta, float g ) {
    float denom = 1 + g * g + 2 * g * cosTheta;
    return Inv4Pi * ( 1 - g * g ) / ( denom * sqrt( denom ) );
}
/* --------------------------------------------------------------------------------------------------------------
 */

#define SHADOW_RAY_SUBSAMPLING 4

/* --------------------------------------------------------------------------------------------------------------
 */
bool getTr( Material volume, vec3 p, vec3 dir, out vec3 tr ) {
    tr = vec3( 1 );
    // accumulated optical thickness
    vec3 tau = vec3( 0 );
    // sigma_t for the volume, that will be modulated by the density.
    // Here, we precompute the quantum for the integration
    vec3 sigma_t =
        ( volume.sigma_a.rgb + volume.sigma_s.rgb ) * SHADOW_RAY_SUBSAMPLING * volume.stepsize;
    bool hit = false;
    p += dir * VanderCorput( uint( dot( p, p ) ) ) * volume.stepsize;
    for ( ;; ) {
        float density = texture( volume.density, p ).r;
        if ( density > 0 ) {
            hit = true;
            tau = density * sigma_t;
            tr *= exp( -tau );
        }
        // go to next point
        p += dir * SHADOW_RAY_SUBSAMPLING * volume.stepsize;
        // volume test
        if ( any( greaterThan( p, vec3( 1.0 ) ) ) || any( lessThan( p, vec3( 0.0 ) ) ) ||
             all( lessThan( tr, vec3( 0.00001 ) ) ) )
            break;
    }
    return hit;
}

#define WANNA_BURN_YOUR_GPU
/* The following two functions mut be adatped to the lighting process */
// compute the attenation of the light (miss the light intensity parameter ...
vec3 lightColor( Material volume, vec3 p, Light l ) {
    // vec3 lightColor = lightContributionFrom(l, p);
    vec3 lightColor = l.color.xyz;
#ifdef WANNA_BURN_YOUR_GPU
    vec3 Tr       = vec3( 1 );
    vec3 dirLight = getLightDirection( l, p );
    getTr( volume, p, dirLight, Tr );
#else
    vec3 Tr = vec3( 1 );
#endif
    return Tr * lightColor;
}

// compute the scattering at p
#define VOLUME_SHADING
vec3 inscatter( Material volume, vec3 p, vec3 dir, Light l ) {
#ifdef VOLUME_SHADING
    vec3 dirLight = getLightDirection( l, p );
    return Inv4Pi * volume.sigma_s.rgb * phaseHG( dot( dir, dirLight ), volume.g ) *
           lightColor( volume, p, l );
#else
    return Inv4Pi * volume.sigma_s.rgb;
#endif
}

/* --------------------------------------------------------------------------------------------------------------
 */
/*
Volumetric does not fullfill GLSL material interface. When compositing in other shader system,
specific tasks must be realized
*/
/* --------------------------------------------------------------------------------------------------------------
 */

// march along the ray.
// Return, in color, the color acculumated into the volume and, in tr, the final transmittance
// p is the starting point on the boundary of the volume
// dir is the (normalized) direction of the ray
// light is the light source
// This functions assume that p, dir and light are expressed in the unit cube frame [0,1]^3, that is
// the local frame of the volume

// Ray marching with piecewise constant density
bool raymarch( Material volume,
               inout vec3 p,
               vec3 dir,
               Light l,
               inout float tmax,
               out vec3 color,
               out vec3 tr ) {
    tr = vec3( 1 );
    // accumulated optical thickness
    vec3 tau = vec3( 0 );
    // sigma_t for the volume, that will be modulated by the density. Here, we precompute the
    // quantum for the integration
    vec3 sigma_t = ( volume.sigma_a.rgb + volume.sigma_s.rgb ) * volume.stepsize;
    // The ray marching loop
    bool hit = false;
    float t  = VanderCorput( uint( dot( p, p ) ) ) * volume.stepsize;
    p += dir * t;
    for ( ;; ) {
        float density = texture( volume.density, p ).r * volume.scale;
        if ( density > 0 ) {
            hit = true;
            // Compute Transmission
            tau = density * sigma_t;
            tr *= exp( -tau );
            // Compute scattering at point p
            color += tr * density * inscatter( volume, p, dir, l ) * volume.stepsize;
        }
        // go to next point
        p += dir * volume.stepsize;
        t += volume.stepsize;
        // volume test
        if ( t > tmax || any( greaterThan( p, vec3( 1.0 ) ) ) ||
             any( lessThan( p, vec3( 0.0 ) ) ) || all( lessThan( tr, vec3( 0.00001 ) ) ) ) {
            tmax = t;
            break;
        }
    }
    return hit;
}
uniform Material material;
#endif
