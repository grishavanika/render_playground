
// https://www.asawicki.info/news_1514_flat_shading_in_directx_11
// Enable `nointerpolation` for "flat shading".
struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    /*nointerpolation*/ float3 Normal : NORMAL;
    float3 WorldPos : POSITION;
};
