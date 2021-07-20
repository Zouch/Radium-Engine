vec3 getKd() {
    if ( material.tex.hasKd == 1 ) { return vec3( texture( material.tex.kd, in_texcoord.xy ) ); }

    return material.kd.xyz;
}

vec3 getKs() {
    if ( material.tex.hasKs == 1 ) { return vec3( texture( material.tex.ks, in_texcoord.xy ) ); }

    return material.ks.xyz;
}

float getNs() {
    float ns = material.ns;
    if ( material.tex.hasNs == 1 ) { ns = texture( material.tex.ns, in_texcoord.xy ).r; }

    return max( ns, 0.001 );
}

vec3 getNormal() {
    if ( material.tex.hasNormal == 1 ) {
        mat3 tbn;
        vec3 binormal = normalize( cross( in_normal, in_tangent ) );
        tbn[0]        = in_tangent;
        tbn[1]        = binormal;
        tbn[2]        = in_normal;

        vec3 n = normalize( vec3( texture( material.tex.normal, in_texcoord.xy ) ) * 2 - 1 );
        return normalize( tbn * n );
    }

    // return vec3(normalize(in_normal));
    return normalize( in_normal );
}

bool toDiscard() {
    if ( material.tex.hasAlpha == 1 ) {
        float alpha = texture( material.tex.alpha, in_texcoord.xy ).r;
        if ( alpha < 0.1 ) { return true; }
    }
    return false;
}
