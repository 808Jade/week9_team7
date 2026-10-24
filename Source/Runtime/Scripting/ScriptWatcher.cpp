#include "EnginePCH.h"
#include "Scripting/ScriptWatcher.h"

FScriptWatcher& FScriptWatcher::Get()
{
	static FScriptWatcher Instance;
	return Instance;
}

FDelegateHandle FScriptWatcher::Register(const std::filesystem::path& InPath, FOnScriptFileChanged::FFuncType InCallback)
{
	if (!InCallback)
	{
		HTR_LOG(Warning, "[ScriptWatcher] Register: empty callback");
		return FDelegateHandle{};
	}

	std::wstring Key;
	bool bCreatedFile = false;
	FWatchedFile* File = FindOrAddFile(InPath, Key, bCreatedFile);
	if (!File)
	{
		return FDelegateHandle{};
	}

	const FDelegateHandle Handle = File->Subscribers.Add(std::move(InCallback));
	return CommitRegister(Key, Handle, bCreatedFile);
}

FScriptWatcher::FWatchedFile* FScriptWatcher::FindOrAddFile(const std::filesystem::path& InPath, std::wstring& OutKey, bool& bOutCreated)
{
	bOutCreated = false;

	if (InPath.empty())
	{
		HTR_LOG(Warning, "[ScriptWatcher] Register: empty path");
		return nullptr;
	}

	const std::filesystem::path NormalizedPath = NormalizePath(InPath);
	if (NormalizedPath.empty())
	{
		return nullptr;
	}

	OutKey = MakeKey(NormalizedPath);

	if (FWatchedFile* Existing = FileMap.Find(OutKey))
	{
		// 이미 감시 중인 파일: 수정 시간은 건드리지 않는다 (대기 중인 변경을 덮어쓰지 않기 위해)
		return Existing;
	}

	// 새 파일: 등록 시점의 수정 시간을 기준으로 기록. 파일이 없거나 읽기 실패면 '시간 무효'
	FWatchedFile& NewFile = FileMap[OutKey];
	NewFile.Path = NormalizedPath;

	std::error_code ErrorCode;
	NewFile.AppliedWriteTime = std::filesystem::last_write_time(NormalizedPath, ErrorCode);
	NewFile.bIsTimeValid = !ErrorCode;
	NewFile.bIsReadFailing = static_cast<bool>(ErrorCode); // 여기서 이미 경고를 찍었으므로 CheckFiles가 다시 찍지 않게
	if (ErrorCode)
	{
		HTR_LOG(Warning, "[ScriptWatcher] Cannot read write time yet: {} ({})", reinterpret_cast<const char*>(NormalizedPath.u8string().c_str()), ErrorCode.message());
	}

	bOutCreated = true;
	return &NewFile;
}

FDelegateHandle FScriptWatcher::CommitRegister(const std::wstring& InKey, FDelegateHandle InHandle, bool bCreatedFile)
{
	if (!InHandle.IsValid())
	{
		HTR_LOG(Warning, "[ScriptWatcher] Register: invalid callback or instance");
		if (bCreatedFile)
		{
			FileMap.Remove(InKey);
		}
		return FDelegateHandle{};
	}

	HandleMap.Add(InHandle.ID, InKey);

	const FWatchedFile* File = FileMap.Find(InKey);
	HTR_LOG(Info, "[ScriptWatcher] Register #{}: {} (files {}, handles {})",
		InHandle.ID,
		File ? reinterpret_cast<const char*>(File->Path.u8string().c_str()) : "?",
		FileMap.Num(),
		HandleMap.Num());

	return InHandle;
}

void FScriptWatcher::Unregister(FDelegateHandle InHandle)
{
	if (!InHandle.IsValid())
	{
		return;
	}

	// 이미 해제된 핸들이면 조용히 무시 (소멸자 안전망에서 두 번 올 수 있음)
	const std::wstring* KeyPtr = HandleMap.Find(InHandle.ID);
	if (!KeyPtr)
	{
		return;
	}

	// HandleMap.Remove 후에도 키를 써야 하므로 복사해 둔다
	const std::wstring Key = *KeyPtr;
	HandleMap.Remove(InHandle.ID);

	// 핸들로 구독자 제거, 구독자가 0명이 된 파일은 목록에서 삭제
	if (FWatchedFile* File = FileMap.Find(Key))
	{
		File->Subscribers.Remove(InHandle);
		if (!File->Subscribers.IsBound())
		{
			FileMap.Remove(Key);
		}
	}

	HTR_LOG(Info, "[ScriptWatcher] Unregister #{} (files {}, handles {})", InHandle.ID, FileMap.Num(), HandleMap.Num());
}

void FScriptWatcher::Tick(float InRealDeltaTime)
{
	if (!(InRealDeltaTime >= 0))
	{
		return;
	}

	// FileMap이 비어있으면 return. 검사할게 없음
	if (FileMap.IsEmpty())
	{
		AccumulatedTime = 0.f;
		return;
	}

	AccumulatedTime += InRealDeltaTime;
	// 누적 시간이 CheckInterval을 넘을 때만 검사
	if (AccumulatedTime < CheckInterval)
	{
		return;
	}
	AccumulatedTime = 0.f;
	
	CheckFiles();
}

// last_write_time 변경 판정
void FScriptWatcher::CheckFiles()
{
	TArray<std::wstring> ConfirmedList;

	std::error_code ErrorCode;
	for (auto& [Key, File] : FileMap)
	{
		fs::file_time_type LastWriteTime = fs::last_write_time(File.Path, ErrorCode);
		if (ErrorCode)
		{
			// 읽기 실패: 다음 검사에서 다시 본다
			if (!File.bIsReadFailing)
			{
				File.bIsReadFailing = true;
				HTR_LOG(Warning, "[ScriptWatcher] Cannot read: {} ({})",
					reinterpret_cast<const char*>(File.Path.u8string().c_str()), ErrorCode.message());
			}
			continue;
		}

		// 실패 -> 성공으로 돌아온 순간에만
		if (File.bIsReadFailing)
		{
			File.bIsReadFailing = false;
			HTR_LOG(Info, "[ScriptWatcher] Available again: {}",
				reinterpret_cast<const char*>(File.Path.u8string().c_str()));
		}

		const bool bIsSame = File.bIsTimeValid && LastWriteTime == File.AppliedWriteTime;

		// continue -> 아직 파일이 변경되었다고 확정하지 않고 대기한다.
		if (!File.bIsWaiting)
		{
			if (bIsSame)
			{
				continue;
			}
			File.bIsWaiting = true;
			File.PendingWriteTime = LastWriteTime;
			continue;
		}
		if (bIsSame)
		{
			File.bIsWaiting = false;
			continue;
		}
		if (LastWriteTime != File.PendingWriteTime)
		{
			File.PendingWriteTime = LastWriteTime;
			continue;
		}
		const uintmax_t FileSize = fs::file_size(File.Path, ErrorCode);
		if (ErrorCode || FileSize == 0)
		{
			continue;
		}

		File.AppliedWriteTime = LastWriteTime;
		File.bIsTimeValid = true;
		File.bIsWaiting = false;
		ConfirmedList.Add(Key);
	}

	BroadcastUpdates(ConfirmedList);
}

// Delegate로 파일 업데이트 broadcast
void FScriptWatcher::BroadcastUpdates(const TArray<std::wstring>& InConfrimedList)
{
	for (const auto& String : InConfrimedList)
	{
		if (FWatchedFile* File = FileMap.Find(String))
		{
			fs::path CopiedPath = File->Path;
			HTR_LOG(Info, "[ScriptWatcher] Changed {}", reinterpret_cast<const char*>(File->Path.u8string().c_str()));
			File->Subscribers.Broadcast(CopiedPath);
		}
	}
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
