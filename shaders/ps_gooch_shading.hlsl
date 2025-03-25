#include "common_gooch_shading.hlsl"

struct PointLight
{
    float4 LightColor; // Unused.
    float4 LightPosition; // World Space.
};

#define MAX_LIGHTS_COUNT 16

cbuffer PSConstantBuffer0 : register(b0)
{
    PointLight Lights[MAX_LIGHTS_COUNT];
    float4 ViewerPosition;
    // x = has texture
    // y = lights count
    float4 Parameters; // Unused.
};

// Shading Models, page 105, Real-Time rendering.
// Implementing Shading Models, page 123.
float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float4 c_surface   = float4(1.0, 0.65, 0.0, 1.0); // orange
    float4 c_cool      = float4(0.0, 0.0, 0.55, 1.0) + 0.25 * c_surface;
    float4 c_warn      = float4(0.3, 0.3, 0.0, 1.0)  + 0.25 * c_surface;
    float4 c_highlight = float4(2.0, 2.0, 2.0, 2.0);

    float4 c_unlit = c_cool / 2;

    float3 n = normalize(input.Normal);
    float3 v = normalize(ViewerPosition.xyz - input.WorldPos);

    float4 out_color = c_unlit;

    for (uint i = 0; i < uint(Parameters.y); ++i)
    {
        float3 l = normalize(Lights[0].LightPosition.xyz - input.WorldPos);
        float NdL = clamp(dot(n, l), 0.0, 1.0);
        float3 r_l = reflect(-l, n);
        float s = clamp (100.0 * dot(r_l , v) - 97.0 , 0.0 , 1.0);
        float4 lit = lerp(c_warn, c_highlight, s);
        out_color.rgb += NdL * Lights[i].LightColor.rgb * lit.rgb;
    }
    // out_color = c_unlit;
    return out_color;
}
