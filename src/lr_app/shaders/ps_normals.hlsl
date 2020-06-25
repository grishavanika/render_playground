struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float3 light_color = float3(1., 1., 1.);
    float3 light_pos = float3(0., .0, -15);
    float3 view_pos = float3(0., 0., -15);
    
    float3 n = normalize(input.Normal);
    float3 light_dir = normalize(light_pos - (float3)input.Position);

#if (0)
    float3 object_colot = float3(1., 0., 0.);
    float3 color = saturate(dot(light_dir, n)) * object_colot + 0.1;
    return float4(color, 1.);
#endif

    float3 m_ambient = float3(1.0f, 0.5f, 0.31f);
    float3 m_diffuse = float3(1.0f, 0.5f, 0.31f);
    float m_shininess = 32.0f;
    float3 m_specular = float3(0.5f, 0.5f, 0.5f);

    // ambient
    float3 ambient = light_color * m_ambient;

    // diffuse
    float diff = max(dot(n, light_dir), 0.0);
    float3 diffuse = light_color * (diff * m_diffuse);

    // specular
    float3 view_dir = normalize(view_pos - (float3)input.Position);
    float3 reflect_dir = reflect(-light_dir, n);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), m_shininess);
    float3 specular = light_color * (spec * m_specular);

    float3 final_color = ambient + diffuse + specular;
    return float4(final_color, 1.0);
}

