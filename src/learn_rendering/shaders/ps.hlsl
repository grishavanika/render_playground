struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float3 color = saturate(dot(float3(0., 1., 0.), input.Norm)) * float3(1., 0., 0.) + 0.1;

    return float4(color, 1.);
}
