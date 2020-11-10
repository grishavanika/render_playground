struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 WorldPos : POSITION;
};

cbuffer PSConstantBuffer0 : register(b0)
{
    float4 LightColor;     // Unused.
    float4 LightPosition;  // World Space.
    float4 ViewerPosition;
    float4 HasTexture;     // Unsued.
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float3 object_color = float3(1.0, 1.0, 1.0);
    float3 normal = input.Normal;
    return float4(normal, 1.0);

    //float3 d         = (float3)LightPosition - input.WorldPos;
    //float3 light_dir = normalize(d);
    //float  diff_k    = max(dot(normal, light_dir), 0.0);
    //float3 diffuse   = diff_k * (float3)LightColor;
    //float3 view_dir  = normalize((float3)ViewerPosition - input.WorldPos);
}
