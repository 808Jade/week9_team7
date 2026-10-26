#pragma pack_matrix(row_major)

cbuffer PortalConstants : register(b0)
{
    matrix World;
    matrix ViewProjection;
    float4 PortalColor; // 짝 포탈이 없을 때 보여줄 색
    float4 Params; // x = 1이면 포탈 텍스처 사용
};

Texture2D<float4> PortalTexture : register(t0);

// 정점 레이아웃과 맞추기 위해 모든 입력을 선언한다 (OutlineShader와 동일)
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
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    output.position = mul(mul(float4(input.position, 1.0f), World), ViewProjection);
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    if (Params.x < 0.5f)
        return float4(PortalColor.rgb, 1.0f);

    // 화면 픽셀 = 포탈 텍스처 픽셀 (2단계에서 같은 크기로 만들었음)
    int2 Pixel = int2(input.position.xy);
    return float4(PortalTexture.Load(int3(Pixel, 0)).rgb, 1.0f);
}