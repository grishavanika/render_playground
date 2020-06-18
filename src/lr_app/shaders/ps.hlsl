Texture2D txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float3 light_dir = float3(0., 1., 0.);
    float3 light_color = float3(1., 1., 1.);
    float3 color = saturate(dot(light_dir, input.Norm)) * light_color;

#if (0)
    return txDiffuse.Sample(samLinear, input.Tex) * float4(color, 1.);
#elif (0)
    return txDiffuse.Sample(samLinear, input.Tex);
#else
    return float4(color, 1.);
#endif
}
