#include "common_gooch_shading.hlsl"

cbuffer PSConstantBuffer0 : register(b0)
{
    float4 LightColor;     // Unused.
    float4 LightPosition;  // World Space.
    float4 ViewerPosition;
    float4 HasTexture;     // Unused.
};

// Shading Models, page 105, Real-Time rendering.
float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float4 c_surface   = float4(1.0, 0.65, 0.0, 1.0); // orange
    float4 c_cool      = float4(0.0, 0.0, 0.55, 1.0) + 0.25 * c_surface;
    float4 c_warn      = float4(0.3, 0.3, 0.0, 1.0)  + 0.25 * c_surface;
    float4 c_highlight = float4(1.0, 1.0, 1.0, 1.0);

    float3 light_dir = normalize((float3)LightPosition - input.WorldPos);
    float3 view_dir  = normalize((float3)ViewerPosition - input.WorldPos);
    float3 l = -1 * light_dir;
    float3 n = normalize(input.Normal);
    float3 v = view_dir;

    float t = (dot(n, l) + 1) / 2;
    float3 r = reflect(l, n);
    float s = clamp(100 * dot(r, v) - 97, 0, 1);

    float4 c = s * c_highlight + (1 - s) * lerp(c_warn, c_cool, t);
    return c;
}
