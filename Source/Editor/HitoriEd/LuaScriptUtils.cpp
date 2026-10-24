#include "EnginePCH.h"
#include <shellapi.h>

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

// 템플릿 복사 + 컴포넌트 등록
bool FLuaScriptUtils::GenerateScript(AActor* InActor)
{
	if (!InActor)
	{
		HTR_LOG(Warning, "GenerateScript: Actor is null");
		return false;
	}

	// 에디터 월드에서만 허용 (PIE 액터는 복제본이라 Play 종료 시 변경이 사라진다)
	UWorld* World = InActor->GetWorld();
	if (!World || World->GetWorldType() != EWorldType::Editor)
	{
		HTR_LOG(Warning, "GenerateScript: Scripts can only be created in the editor world (not during PIE)");
		return false;
	}

	ULuaScriptComponent* Comp = GetScriptComponent(InActor);

	// ---------- 1. 경로 결정 (액터는 아직 건드리지 않는다) ----------
	const bool bHasStoredPath = Comp && Comp->HasScriptPath();
	const FString ScriptPath = bHasStoredPath ? Comp->GetScriptPath() : MakeScriptPath(InActor);
	if (ScriptPath.empty())
	{
		return false;
	}

	const fs::path AbsolutePath = MakeAbsolutePath(ScriptPath);
	if (AbsolutePath.empty())
	{
		HTR_LOG(Error, "GenerateScript: Invalid script path: {}", ScriptPath);
		return false;
	}

	// ---------- 2. 파일 작업 ----------
	std::error_code EC;
	const bool bFileExists = fs::exists(AbsolutePath, EC);
	if (EC)
	{
		HTR_LOG(Error, "GenerateScript: Failed to check file: {} ({})", FileToFString(AbsolutePath), EC.message());
		return false;
	}

	bool bCreatedFile = false;
	if (bFileExists)
	{
		// 같은 이름의 폴더 등이 있으면 스크립트로 쓸 수 없다
		if (!fs::is_regular_file(AbsolutePath, EC) || EC)
		{
			HTR_LOG(Error, "GenerateScript: Path exists but is not a file: {}", FileToFString(AbsolutePath));
			return false;
		}
		// 이미 있는 파일은 절대 덮어쓰지 않는다 (작성 중인 스크립트 보호)
	}
	else
	{
		const fs::path AbsoluteTemplatePath = MakeAbsolutePath(TemplatePath);
		if (AbsoluteTemplatePath.empty() || !fs::is_regular_file(AbsoluteTemplatePath, EC))
		{
			HTR_LOG(Error, "GenerateScript: Template not found: {}", AbsoluteTemplatePath.empty() ? TemplatePath : FileToFString(AbsoluteTemplatePath));
			return false;
		}

		fs::create_directories(AbsolutePath.parent_path(), EC);
		if (EC)
		{
			HTR_LOG(Error, "GenerateScript: Failed to create folder: {} ({})", FileToFString(AbsolutePath.parent_path()), EC.message());
			return false;
		}

		fs::copy_file(AbsoluteTemplatePath, AbsolutePath, fs::copy_options::none, EC);
		if (EC)
		{
			HTR_LOG(Error, "GenerateScript: Failed to copy template to {} ({})", FileToFString(AbsolutePath), EC.message());
			return false;
		}

		bCreatedFile = true;
	}

	// ---------- 3. 액터 수정 (파일 작업이 모두 성공한 뒤) ----------
	if (!Comp)
	{
		Comp = Cast<ULuaScriptComponent>(InActor->AddComponent(ULuaScriptComponent::StaticClass(), NAME_None));
		if (!Comp)
		{
			// 파일은 이미 만들어졌지만, 다시 Create를 누르면 '기존 파일 연결'로 처리된다
			HTR_LOG(Error, "GenerateScript: Failed to add ULuaScriptComponent to {}", InActor->GetName());
			return false;
		}
	}

	if (!bHasStoredPath)
	{
		Comp->SetScriptPath(ScriptPath);
	}

	// ---------- 4. 결과 로그 ----------
	if (bCreatedFile && bHasStoredPath)
	{
		HTR_LOG(Info, "GenerateScript: Restored missing script from template: {}", ScriptPath);
	}
	else if (bCreatedFile)
	{
		HTR_LOG(Info, "GenerateScript: Created: {}", ScriptPath);
	}
	else if (!bHasStoredPath)
	{
		HTR_LOG(Info, "GenerateScript: Linked existing file: {}", ScriptPath);
	}
	else
	{
		HTR_LOG(Info, "GenerateScript: Already linked: {}", ScriptPath);
	}

	if (!bHasStoredPath)
	{
		HTR_LOG(Info, "GenerateScript: Save the scene to keep the script path.");
	}

	return true;
}

// ShellExecuteW로 외부 편집기 열기
bool FLuaScriptUtils::OpenScript(AActor* InActor)
{
	if (!InActor)
		return false;

	ULuaScriptComponent* Comp = GetScriptComponent(InActor);
	if (!Comp)
	{
		HTR_LOG(Warning, "OpenScript: No script component. Press Create Script first.");
		return false;
	}
	if (!Comp->HasScriptPath())
	{
		HTR_LOG(Warning, "OpenScript: Script path is empty. Press Create Script first.");
		return false;
	}

	fs::path AbsolutePath = MakeAbsolutePath(Comp->GetScriptPath());
	if (AbsolutePath.empty())
	{
		HTR_LOG(Error, "OpenScript: Invalid script path: {}", Comp->GetScriptPath());
		return false;
	}

	// 파일이 없을 때는 ErrorCode가 비어 있으므로 '확인 실패'와 '파일 없음'을 나눠서 처리
	std::error_code ErrorCode;
	const bool bExists = fs::exists(AbsolutePath, ErrorCode);
	if (ErrorCode)
	{
		HTR_LOG(Error, "OpenScript: Failed to check file: {} ({})", FileToFString(AbsolutePath), ErrorCode.message());
		return false;
	}
	if (!bExists)
	{
		HTR_LOG(Warning, "OpenScript: Cannot find script file: {} (Press Create Script to restore it from the template)", FileToFString(AbsolutePath));
		return false;
	}

	// exists는 폴더에도 true. 폴더를 넘기면 탐색기가 열린다
	if (!fs::is_regular_file(AbsolutePath, ErrorCode) || ErrorCode)
	{
		HTR_LOG(Error, "OpenScript: Path exists but is not a file: {}", FileToFString(AbsolutePath));
		return false;
	}

	HINSTANCE Output = ShellExecuteW(GetActiveWindow(), L"open", AbsolutePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	INT_PTR Result = reinterpret_cast<INT_PTR>(Output);
	if (Result > 32) // 32 초과이면 성공
	{
		HTR_LOG(Info, "OpenScript: Opened: {}", FileToFString(AbsolutePath));
		return true;
	}

	LogShellExecuteError("open", Result, AbsolutePath);

	// .lua에 연결된 프로그램이 없거나(31) 연결 정보가 불완전하면(27) 대체 수단을 차례로 시도
	if (Result == SE_ERR_NOASSOC || Result == SE_ERR_ASSOCINCOMPLETE)
	{
		// 1) '연결 프로그램 선택' 창 (Windows 10/11에서는 실패하기도 한다)
		Output = ShellExecuteW(GetActiveWindow(), L"openas", AbsolutePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		Result = reinterpret_cast<INT_PTR>(Output);
		if (Result > 32)
		{
			HTR_LOG(Info, "OpenScript: Opened with 'Open with' dialog: {}", FileToFString(AbsolutePath));
			return true;
		}
		LogShellExecuteError("openas", Result, AbsolutePath);

		// 2) 메모장. 경로에 공백이 있을 수 있으므로 큰따옴표로 감싼다
		const std::wstring QuotedPath = L"\"" + AbsolutePath.native() + L"\"";
		Output = ShellExecuteW(GetActiveWindow(), L"open", L"notepad.exe", QuotedPath.c_str(), nullptr, SW_SHOWNORMAL);
		Result = reinterpret_cast<INT_PTR>(Output);
		if (Result > 32)
		{
			HTR_LOG(Info, "OpenScript: Opened with Notepad: {}", FileToFString(AbsolutePath));
			return true;
		}
		LogShellExecuteError("notepad", Result, AbsolutePath);
	}

	// 3) 모든 방법 실패
	MessageBoxW(
		GetActiveWindow(),
		L"스크립트 파일을 열지 못했습니다.\n자세한 내용은 Output Log를 확인하세요.",
		L"Edit Script 실패",
		MB_OK | MB_ICONERROR
	);
	return false;
}

void FLuaScriptUtils::LogShellExecuteError(const char* InVerb, intptr_t InResult, const std::filesystem::path& InPath)
{
	const FString PathString = FileToFString(InPath);

	switch (InResult)
	{
	case SE_ERR_FNF:
	case SE_ERR_PNF:
		HTR_LOG(Error, "OpenScript({}): No File or Path such as: {}", InVerb, PathString);
		break;
	case SE_ERR_ACCESSDENIED:
		HTR_LOG(Error, "OpenScript({}): File Access Denied: {}", InVerb, PathString);
		break;
	case SE_ERR_NOASSOC:
	case SE_ERR_ASSOCINCOMPLETE:
		HTR_LOG(Warning, "OpenScript({}): No program is associated with .lua files (code {})", InVerb, InResult);
		break;
	default:
		HTR_LOG(Error, "OpenScript({}): ShellExecute failed (code {}): {}", InVerb, InResult, PathString);
		break;
	}
}

std::filesystem::path FLuaScriptUtils::MakeAbsolutePath(const FString& InRelativePath)
{
	if (InRelativePath.empty())
		return {};

	// 변환 실패 시 빈 path가 나오는데, absolute(빈 path)는 작업 디렉터리를 돌려주므로 여기서 막는다
	const fs::path RelativePath = FStringToFile(InRelativePath);
	if (RelativePath.empty())
		return {};

	std::error_code ErrorCode;
	fs::path AbsolutePath = fs::absolute(RelativePath, ErrorCode);
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

	// 길이 제한. UTF-8 연속 바이트(10xxxxxx) 중간에서 자르지 않도록 문자 경계까지 물러난다
	if (Result.size() > MaxFileNameLength)
	{
		size_t CutLength = MaxFileNameLength;
		while (CutLength > 0 && (static_cast<unsigned char>(Result[CutLength]) & 0xC0) == 0x80)
		{
			--CutLength;
		}
		Result.resize(CutLength);
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
		return {};

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
