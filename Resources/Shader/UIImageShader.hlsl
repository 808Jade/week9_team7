#pragma pack_matrix(row_major)

// UI 파라미터 버퍼 (MeshPassProcessor에서 b1 슬롯에 바인딩)
cbuffer UIConstants : register(b1)
{
    float4 Rect; // (PosX, PosY, Width, Height) - 화면 좌상단 기준 픽셀 좌표
    float4 ViewportSize; // (ViewportWidth, ViewportHeight, 0, 0)
    float4 Color; // (R, G, B, A) Tint & Opacity
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

Texture2D UITexture : register(t0);
SamplerState UISampler : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // UV(0.0 ~ 1.0)를 기준으로 화면 픽셀 좌표 계산
    float pixelX = Rect.x + input.UV.x * Rect.z;
    float pixelY = Rect.y + input.UV.y * Rect.w;

    // 픽셀 좌표를 정규화된 디바이스 좌표(NDC: -1.0 ~ +1.0)로 변환
    // NDC X: 화면 왼쪽(-1.0) ~ 오른쪽(+1.0)
    // NDC Y: 화면 위쪽(+1.0) ~ 아래쪽(-1.0)
    float ndcX = (pixelX / ViewportSize.x) * 2.0f - 1.0f;
    float ndcY = 1.0f - (pixelY / ViewportSize.y) * 2.0f;

    output.Position = float4(ndcX, ndcY, 0.0f, 1.0f);
    output.UV = input.UV;
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 texColor = UITexture.Sample(UISampler, input.UV);
    return texColor * Color;
}