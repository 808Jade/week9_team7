#pragma once

// 화면 후처리 설정 묶음.
// 에디터 설정(FEditorSettings), 게임 엔진(UGameEngine), 씬 렌더 옵션(FSceneRenderOptions)이
// 같은 구조체를 통째로 주고받는다. 비네팅 같은 후처리 항목도 여기에 추가한다.
struct FPostProcessSettings
{
	// Show Flag: Gamma Correction. 끄면 ToneMap이 선형 값을 그대로 출력한다.
	bool bEnableGammaCorrection = true;
	// 디스플레이 감마. ToneMap에서 pow(x, 1/Gamma)
	float Gamma = 2.2f;

	bool bEnableVignette = false;
	float VignetteIntensity = 0.5f;
	float VignetteRadius = 0.4f;
	float VignetteSmoothness = 0.5f;
	FVector VignetteColor{ 0.f, 0.f, 0.f };
};
