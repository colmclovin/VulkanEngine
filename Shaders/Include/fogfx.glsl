#ifndef FOGFX_GLSL
#define FOGFX_GLSL

#include "noise.glsl"

const vec3 FOG_COLOR = vec3(0.78, 0.82, 0.88);

vec3 applyFog(vec3 lit, vec3 worldPos, float f, float time) {
    vec2 p = worldPos.xz * 0.045 + vec2(time * 0.15, time * 0.10);
    vec2 warp = vec2(fbm(p + 3.1, 2, 1.0, 2.0, 0.5), fbm(p + 7.7, 2, 1.0, 2.0, 0.5));
    float cloud = fbm(p + warp * 0.8, 4, 1.0, 2.0, 0.5) * 0.5 + 0.5;

    float unexplored = 1.0 - smoothstep(0.0, 0.5, f);
    float explored   = 1.0 - smoothstep(0.5, 1.0, f);

    float edge = 4.0 * unexplored * (1.0 - unexplored);   // 0 when fully clear or fully fogged, 1 at the edge
    float density = smoothstep(0.25, 0.75, unexplored + (cloud - 0.5) * 0.9 * edge);
    vec3 gray = vec3(dot(lit, vec3(0.299, 0.587, 0.114)));
    vec3 col = mix(lit, mix(gray, lit, 0.4), explored) * mix(1.0, 0.8, explored);   // desaturate and dim slightly
    return mix(col, FOG_COLOR * (0.8 + 0.2 * cloud), density);
}

#endif