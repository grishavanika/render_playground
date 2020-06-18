Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer PSConstantBuffer0 : register(b0)
{
    float4 LightColor;
    float4 LightPosition; // World Space.
    float4 ViewerPosition;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
    float3 WorldPos : POSITION;
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float ambient_strength = 0.1;
    float3 ambient = ambient_strength;// * (float3)LightColor;

    float3 norm = input.Norm;
    float3 d = (float3)LightPosition - input.WorldPos;
    float3 light_dir = normalize(d);
    float diff_k = max(dot(norm, light_dir), 0.0);
    float3 diffuse = diff_k * (float3)LightColor;
    
    diffuse = diffuse / length(d);

    float specular_strength = 5;
    float shininess = 16;
    float3 view_dir = normalize((float3)ViewerPosition - input.WorldPos);
    float3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    float3 specular = specular_strength * spec * (float3)LightColor; 

    specular = specular / length(d);

#if (1)
    float3 object_color = (float3)txDiffuse.Sample(samLinear, input.Tex);
#else
    float3 object_color = float3(1.0, 1.0, 1.0);
#endif

    float3 final_color = object_color * (ambient + diffuse + specular);
#if (1)
    return float4(final_color, 1);
#else
    return float4(ambient, 1);
#endif
}
