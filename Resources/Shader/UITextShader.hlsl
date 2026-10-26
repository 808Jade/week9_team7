#pragma pack_matrix(row_major)

cbuffer UIConstants : register(b1)
{
    float4 Rect; // (PosX, PosY, 0, 0) - 화면 좌상단 기준 픽셀 좌표
    float4 ViewportSize; // (ViewportWidth, ViewportHeight, DistanceRange, 0)
    float4 Color; // (R, G, B, A) 텍스트 틴트 색상 및 투명도
};

struct VS_INPUT
{
    float3 Position : POSITION; // 글자별 픽셀 오프셋 (X, Y)
    float2 UV : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // 글자의 로컬 픽셀 오프셋에 UI 위치(Rect.xy)를 더해 화면 픽셀 좌표 계산
    float pixelX = Rect.x + input.Position.x;
    float pixelY = Rect.y + input.Position.y;

    // 픽셀 좌표를 정규화된 디바이스 좌표(NDC: -1.0 ~ +1.0)로 변환
    float ndcX = (pixelX / ViewportSize.x) * 2.0f - 1.0f;
    float ndcY = 1.0f - (pixelY / ViewportSize.y) * 2.0f;

    output.Position = float4(ndcX, ndcY, 0.0f, 1.0f);
    output.UV = input.UV;
    return output;
}

float Median(float3 msdf)
{
    return max(min(msdf.r, msdf.g), min(max(msdf.r, msdf.g), msdf.b));
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 Sample = FontAtlas.Sample(FontSampler, input.UV).rgb;
    float SignedDist = Median(Sample) - 0.5f;

    float TexWidth, TexHeight;
    FontAtlas.GetDimensions(TexWidth, TexHeight);
    
    // MSDF 디스턴스 필드를 화면 픽셀 기준 날카로운 외곽선으로 복원
    float PxRange = ViewportSize.z > 0.0f ? ViewportSize.z : 2.0f;
    float2 UnitRange = float2(PxRange, PxRange) / float2(TexWidth, TexHeight);
    float2 ScreenTexSize = float2(1.0, 1.0) / fwidth(input.UV);
    float ScreenPxDistance = max(0.5 * dot(UnitRange, ScreenTexSize), 1.0) * SignedDist;
    float Opacity = saturate(ScreenPxDistance + 0.5f);

    clip(Opacity - 0.001f);

    // 전달받은 Color(RGB)와 폰트 알파(Opacity)를 곱해서 최종 출력!
    return float4(Color.rgb, Color.a * Opacity);
}