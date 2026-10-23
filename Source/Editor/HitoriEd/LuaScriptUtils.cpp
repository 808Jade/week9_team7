#include "EnginePCH.h"
#include "Editor/HitoriEd/LuaScriptUtils.h"

#include "Editor/HitoriEd/EditorFileUtils.h"
#include "GameFramework/Actor.h"
#include "Components/LuaScriptComponent.h"
#include "UObject/Casts.h"

ULuaScriptComponent* FLuaScriptUtils::GetScriptComponent(AActor* InActor)
{
	if (!InActor)
	{
		return nullptr;
	}
	return InActor->FindComponentByClass<ULuaScriptComponent>();
}

FString FLuaScriptUtils::MakeScriptPath(AActor* InActor)
{
	if (!InActor)
	{
		HTR_LOG(Warning, "MakeScriptPath: Actor is null");
		return {};
	}

	// CurrentScenePath는 ANSI 문자열이다.
	// fs::path은 Windows에서 ANSI로 해석하므로
	// 씬 경로만큼은 그대로 path에 넣는 것이 맞다.
	const FString ScenePathAnsi = FEditorFileUtils::GetCurrentScenePath();
	if (ScenePathAnsi.empty())
	{
		HTR_LOG(Warning, "MakeScriptPath: Scene is not saved yet. Save the scene before creating a script.");
		return {};
	}

	const fs::path ScenePath(ScenePathAnsi);

	// path 내부는 wide 문자열이므로 FileToFString으로 UTF-8로 꺼낸다 (한글 유지)
	const FString SceneName = SanitizeFileName(FileToFString(ScenePath.stem()));

	// 액터 이름은 엔진 내부 문자열이라 UTF-8
	const FString ActorName = SanitizeFileName(InActor->GetName());

	// 구분자는 항상 '/' (상대 경로, 프로젝트 루트 기준)
	const FString Result = ScriptFolderPath + SceneName + "_" + ActorName + ".lua";

	HTR_LOG(Info, "MakeScriptPath: {}", Result);
	return Result;
}

bool FLuaScriptUtils::GenerateScript(AActor* InActor)
{
	// TODO(4단계): 템플릿 복사 + 컴포넌트 등록
	HTR_LOG(Warning, "GenerateScript: Not implemented yet");
	return false;
}

bool FLuaScriptUtils::OpenScript(AActor* InActor)
{
	// TODO(5단계): ShellExecuteW로 외부 편집기 열기
	HTR_LOG(Warning, "OpenScript: Not implemented yet");
	return false;
}

std::filesystem::path FLuaScriptUtils::MakeAbsolutePath(const FString& InRelativePath)
{
	if (InRelativePath.empty())
	{
		return {};
	}

	std::error_code ErrorCode;
    fs::path AbsolutePath = fs::absolute(FStringToFile(InRelativePath), ErrorCode);
	if (ErrorCode)
	{
		HTR_LOG(Error, "MakeAbsolutePath failed: {} ({})", InRelativePath, ErrorCode.message());
		return {};
	}

	return AbsolutePath.lexically_normal();
}

FString FLuaScriptUtils::SanitizeFileName(const FString& InName)
{
	FString Result;
	Result.reserve(InName.size());

	for (char Ch : InName)
	{
		const unsigned char UCh = static_cast<unsigned char>(Ch);

		// 0x80 이상은 UTF-8 멀티바이트의 일부이므로 그대로 유지
		const bool bInvalid =
			UCh < 0x20 ||
			Ch == '\\' || Ch == '/' || Ch == ':' || Ch == '*' ||
			Ch == '?' || Ch == '"' || Ch == '<' || Ch == '>' ||
			Ch == '|' || Ch == ' ';

		Result.push_back(bInvalid ? '_' : Ch);
	}

	// Windows는 파일 이름 끝의 '.'을 허용하지 않는다
	while (!Result.empty() && Result.back() == '.')
	{
		Result.pop_back();
	}

	if (Result.empty())
	{
		Result = "Unnamed";
	}

	return Result;
}

std::filesystem::path FLuaScriptUtils::FStringToFile(const FString& InString)
{
	if (InString.empty())
	{
		return {};
	}

	const int32 SrcLen = static_cast<int32>(InString.size());
	const int32 WideLen = MultiByteToWideChar(CP_UTF8, 0, InString.data(), SrcLen, nullptr, 0);
	if (WideLen <= 0)
	{
		HTR_LOG(Error, "FStringToFile: invalid UTF-8 string: {}", InString);
		return {};
	}

	std::wstring Wide(WideLen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, InString.data(), SrcLen, Wide.data(), WideLen);

	return std::filesystem::path(Wide);
}

FString FLuaScriptUtils::FileToFString(const std::filesystem::path& InPath)
{
	const std::wstring& Wide = InPath.native();
	if (Wide.empty())
	{
		return {};
	}

	const int32 SrcLen = static_cast<int32>(Wide.size());
	const int32 Utf8Len = WideCharToMultiByte(CP_UTF8, 0, Wide.data(), SrcLen, nullptr, 0, nullptr, nullptr);
	if (Utf8Len <= 0)
	{
		return {};
	}

	FString Result(Utf8Len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, Wide.data(), SrcLen, Result.data(), Utf8Len, nullptr, nullptr);

	return Result;
}
