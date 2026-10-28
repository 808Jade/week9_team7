#pragma pack_matrix(row_major)

cbuffer PortalConstants : register(b0)
{
    matrix World;
    matrix ViewProjection;
    float4 PortalColor;
    float4 Params; // x = 포탈 텍스처 사용, y = 시간(초), z = NDC 깊이 바이어스
};

Texture2D<float4> PortalTexture : register(t0);

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    output.position = mul(mul(float4(input.position, 1.0f), World), ViewProjection);

    // 벽과의 Z-fighting 방지: 클립 z를 w에 비례해 당겨 NDC 깊이를 Params.z만큼 앞으로.
    output.position.z -= Params.z * output.position.w;
    output.uv = input.uv;
    return output;
}

static const float Inner = 0.86f; // 이 안쪽 = 포탈 너머, 바깥 = 테두리

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float2 p = (input.uv - 0.5f) * 2.0f; // 중심 (0,0), 가장자리 ±1
    float d = length(p); // 메시 비율이 1:2라 화면에선 세로 타원

    clip(1.0f - d); // 타원 밖은 버림 → 벽이 보임

    float Time = Params.y;
    float Angle = atan2(p.y, p.x);

    // 테두리
    if (d > Inner)
    {
        float t = (d - Inner) / (1.0f - Inner); // 0 = 안쪽 가장자리, 1 = 바깥
        float Wave = 0.75f + 0.25f * sin(Angle * 10.0f + Time * 5.0f)
                           * sin(Angle * 3.0f - Time * 2.0f); // 일렁임
        float3 Hot = lerp(float3(1, 1, 1), PortalColor.rgb, saturate(t * 2.0f)); // 안쪽은 하얗게
        float3 Rim = Hot * (1.6f - t) * Wave; // 바깥으로 갈수록 어둡게
        return float4(saturate(Rim), 1.0f);
    }

    // 포탈 안
    float3 View;
    if (Params.x > 0.5f)
        View = PortalTexture.Load(int3(int2(input.position.xy), 0)).rgb;
    else
        View = PortalColor.rgb * (0.25f + 0.1f * sin(d * 20.0f - Time * 3.0f)); // 짝 없음: 소용돌이 단색

    float EdgeGlow = smoothstep(Inner - 0.15f, Inner, d); // 테두리 근처 색 번짐
    View = lerp(View, PortalColor.rgb, EdgeGlow * 0.6f);
    return float4(View, 1.0f);
}