#include "EnginePCH.h"
#include "Scripting/ScriptWatcher.h"

FScriptWatcher& FScriptWatcher::Get()
{
	static FScriptWatcher Instance;
	return Instance;
}

FScriptWatchHandle FScriptWatcher::Register(const std::filesystem::path& InPath, FOnScriptFileChanged InCallback)
{
	if (InPath.empty() || !InCallback)
	{
		HTR_LOG(Warning, "[ScriptWatcher] Register: empty path or callback");
		return InvalidScriptWatchHandle;
	}

	const std::filesystem::path NormalizedPath = NormalizePath(InPath);
	if (NormalizedPath.empty())
	{
		return InvalidScriptWatchHandle;
	}

	const FScriptWatchHandle Handle = NextHandle++;

	// 자료구조에 (경로, 핸들, 콜백) 저장 + 등록 시점의 수정 시간 기록
	const std::wstring Key = MakeKey(NormalizedPath);

	FWatchSubscriber Subscriber{
	.Handle = Handle,
	.Callback = std::move(InCallback),
	};

	if (auto File = FileMap.Find(Key))
	{
		File->Subscribers.Add(std::move(Subscriber));
	}
	else
	{
		std::error_code ErrorCode;
		FWatchedFile& NewWatchedFile = FileMap[Key];
		NewWatchedFile.Path = NormalizedPath;
		NewWatchedFile.AppliedWriteTime = fs::last_write_time(NormalizedPath, ErrorCode);
		NewWatchedFile.bIsTimeValid = !ErrorCode;
		NewWatchedFile.Subscribers.Add(std::move(Subscriber));
	}
	HandleMap.Add(Handle, Key);

	HTR_LOG(Info, "[ScriptWatcher] Register #{}: {} (subscribers {}, files {})", Handle, reinterpret_cast<const char*>(NormalizedPath.u8string().c_str()), FileMap[Key].Subscribers.Num(), FileMap.Num());
	return Handle;
}

void FScriptWatcher::Unregister(FScriptWatchHandle InHandle)
{
	if (InHandle == InvalidScriptWatchHandle)
	{
		return;
	}

	// 핸들로 구독자 제거, 구독자가 0명이 된 파일은 목록에서 삭제
	if (const auto Path = HandleMap.Find(InHandle))
	{
		if (auto File = FileMap.Find(*Path))
		{
			for (int32 i = 0; i < File->Subscribers.Num(); ++i)
			{
				if (File->Subscribers[i].Handle == InHandle)
				{
					File->Subscribers.RemoveAtSwap(i);
					break;
				}
			}
			if (File->Subscribers.Num() <= 0)
			{
				FileMap.Remove(*Path);
			}
		}
		HandleMap.Remove(InHandle);

		HTR_LOG(Info, "[ScriptWatcher] Unregister #{} (files {}, handles {})", InHandle, FileMap.Num(), HandleMap.Num());
	}
}

void FScriptWatcher::Tick(float InRealDeltaTime)
{
	(void)InRealDeltaTime;

	// TODO(3단계): 누적 시간이 CheckInterval을 넘을 때만 검사
	// TODO(4단계): last_write_time(error_code 버전)으로 변경 판정 (!= 비교)
	// TODO(5단계): 디바운스 - 다음 검사에서도 시간이 같으면 확정 후 콜백 호출
}

std::filesystem::path FScriptWatcher::NormalizePath(const std::filesystem::path& InPath)
{
	if (InPath.empty())
	{
		return {};
	}

	std::error_code ErrorCode;
	std::filesystem::path AbsolutePath = std::filesystem::absolute(InPath, ErrorCode);
	if (ErrorCode)
	{
		HTR_LOG(Error, "[ScriptWatcher] NormalizePath failed ({})", ErrorCode.message());
		return {};
	}

	return AbsolutePath.lexically_normal();
}

// path를 받아 소문자화 된 wstring을 반환
std::wstring FScriptWatcher::MakeKey(const fs::path& InPath)
{
	std::wstring Key = InPath.native();
	if (Key.empty())
	{
		return Key;
	}
	CharLowerBuffW(Key.data(), static_cast<DWORD>(Key.size()));
	return Key;
}