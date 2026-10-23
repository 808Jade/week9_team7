#pragma once

#include "Core/EngineString.h"

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
	// ShellExecute, Hot Reload(파일 감시)에서 공용으로 사용
	static fs::path MakeAbsolutePath(const FString& InRelativePath);

private: 
	// 파일 이름에 쓸 수 없는 문자(\ / : * ? " < > | 공백, 제어 문자)를 '_'로 치환
	// UTF-8 멀티바이트(한글 등)는 그대로 둔다
	static FString SanitizeFileName(const FString& InName);

	// UTF-8 FString <-> std::filesystem::path 변환은 반드시 이 두 함수로만 한다
	// (std::string을 path에 그대로 넣으면 Windows가 ANSI(CP949)로 해석해 한글이 깨짐)
	static fs::path FStringToFile(const FString& InString);
	static FString FileToFString(const fs::path& InPath);
};
