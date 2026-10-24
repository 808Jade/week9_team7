#pragma once

#include "Core/EngineString.h"
#include <cstdint>
#include <filesystem>

class AActor;
class ULuaScriptComponent;


class FLuaScriptUtils 
{
public:
	static inline const FString ScriptFolderPath = "Scripts/";
	static inline const FString TemplatePath = ScriptFolderPath + "template.lua";

	static ULuaScriptComponent* GetScriptComponent(AActor* InActor);
	static FString MakeScriptPath(AActor* InActor);
	static bool GenerateScript(AActor* InActor);
	static bool OpenScript(AActor* InActor);

	// 프로젝트 루트(작업 디렉터리) 기준 상대 경로 -> 정규화된 절대 경로. 실패 시 빈 path
	static std::filesystem::path MakeAbsolutePath(const FString& InRelativePath);

private: 
	// 씬 이름/액터 이름 각각의 최대 길이(바이트). 전체 경로가 MAX_PATH를 넘지 않게 한다
	static constexpr size_t MaxFileNameLength = 64;

	// 파일 이름에 쓸 수 없는 문자(\ / : * ? " < > | 공백, 제어 문자)를 '_'로 치환
	static FString SanitizeFileName(const FString& InName);

	// UTF-8 FString <-> std::filesystem::path 변환은 이 두 함수로만 한다
	static std::filesystem::path FStringToFile(const FString& InString);
	static FString FileToFString(const std::filesystem::path& InPath);

	// ShellExecuteW 실패 코드(32 이하)를 원인별 로그로 남긴다
	static void LogShellExecuteError(const char* InVerb, intptr_t InResult, const std::filesystem::path& InPath);
};
