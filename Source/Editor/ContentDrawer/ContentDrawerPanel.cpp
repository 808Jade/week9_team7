#include "EnginePCH.h"
#include "Editor/ContentDrawer/ContentDrawerPanel.h"
#include "Asset/AssetManager.h"
#include "Text/Font.h"
#include "Editor/HitoriEd/EditorDragDrop.h"
#include "Editor/HitoriEd/EditorFileUtils.h"
#include "Editor/HitoriEd/LuaScriptUtils.h"
#include "Input/InputSystem.h"
#include "Engine/World.h"
#include <fstream>

bool FContentDrawerPanel::Init()
{
	return false;
}

void FContentDrawerPanel::Tick(float DeltaTime)
{
}

void FContentDrawerPanel::OnRender()
{
	ImGui::Begin("Content Drawer");
	std::error_code Error;

	for (const auto& Root : RootFolders)
	{
		if (!fs::exists(Root, Error))
		{
			fs::create_directories(Root, Error);
		}
	}
	Error.clear();

	if (!fs::is_directory(CurrentPath, Error))
	{
		CurrentPath = RootFolders.empty() ? fs::path("Assets") : RootFolders[0];
	}

	float treeViewWidth = 200.0f;
	ImGui::BeginChild("FolderTreeView", ImVec2(treeViewWidth, 0));
	{
		for (const auto& Root : RootFolders)
		{
			ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;
			if (CurrentPath == Root)
			{
				Flags |= ImGuiTreeNodeFlags_Selected;
			}

			bool bIsOpen = ImGui::TreeNodeEx(Root.string().c_str(), Flags);
			if (ImGui::IsItemClicked())
			{
				CurrentPath = Root;
			}
			if (bIsOpen)
			{
				RenderFolderTree(Root, CurrentPath);
				ImGui::TreePop();
			}
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("AssetGridView");
	{
		// F2 및 Escape 단축키 처리
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput)
		{
			if ((ImGui::IsKeyPressed(ImGuiKey_F2) || FInputSystem::IsKeyPressed(EKeyCode::F2)) && !SelectedEntryPath.empty() && !bIsRenaming)
			{
				std::error_code EC;
				if (fs::exists(SelectedEntryPath, EC))
				{
					StartRenaming(SelectedEntryPath);
				}
			}
		}

		if (bIsRenaming && (ImGui::IsKeyPressed(ImGuiKey_Escape) || FInputSystem::IsKeyPressed(EKeyCode::Escape)))
		{
			CancelRename();
		}

		// 상단 이동 및 현재 경로 표시
		if (ImGui::Button(".. Up"))
		{
			if (CurrentPath.has_parent_path() && CurrentPath != CurrentPath.root_path())
			{
				fs::path Parent = CurrentPath.parent_path();
				if (!Parent.empty())
				{
					CurrentPath = Parent;
				}
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Path: %s", CurrentPath.generic_string().c_str());
		ImGui::Separator();

		fs::path ItemToDelete;

		// 빈 공간 우클릭 팝업 메뉴: Create -> Lua Script / Scene
		if (ImGui::BeginPopupContextWindow("ContentDrawerBlankContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Lua Script"))
				{
					CreateNewScript(CurrentPath);
				}
				if (ImGui::MenuItem("Scene"))
				{
					CreateNewScene(CurrentPath);
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}

		// 그리드 레이아웃 계산
		float padding = 16.0f;
		float thumbnailSize = 80.0f;
		float cellSize = thumbnailSize + padding;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int ColumnCount = static_cast<int>(panelWidth / cellSize);
		if (ColumnCount < 1) ColumnCount = 1;

		// 테이블 기반 그리드 시작
		if (ImGui::BeginTable("AssetGridTable", ColumnCount))
		{
			std::error_code IterateError;
			fs::directory_iterator It(CurrentPath, IterateError);
			const fs::directory_iterator End;

			for (; !IterateError && It != End; It.increment(IterateError))
			{
				const fs::path Path = It->path();

				std::error_code TypeError;
				const bool bIsDirectory = fs::is_directory(Path, TypeError);

				if (TypeError)
					continue;

				ImGui::TableNextColumn();

				std::string PathString = Path.generic_string();
				std::string FilenameString = Path.filename().string();
				std::string ExtString = Path.extension().generic_string();

				ImGui::PushID(PathString.c_str());
				UTexture2D* Thumbnail = nullptr;
				UTexture2D* TextureAsset = nullptr;
				UFont* FontAsset = nullptr;

				if (bIsDirectory)
				{
					Thumbnail = UAssetManager::GetAssetByPath<UTexture2D>(FolderIconPath);
				}
				else
				{
					URenderAsset* Asset = UAssetManager::GetAssetByPath<URenderAsset>(PathString);
					if (Asset && Asset->IsA<UTexture2D>())
					{
						TextureAsset = Cast<UTexture2D>(Asset);
						Thumbnail = TextureAsset;
					}
					else if (Asset && Asset->IsA<UFont>())
					{
						FontAsset = Cast<UFont>(Asset);
						Thumbnail = FontAsset->AtlasTexture;
					}

					if (Thumbnail == nullptr)
					{
						Thumbnail = UAssetManager::GetAssetByPath<UTexture2D>(FileIconPath);
					}
				}

				const bool bIsSelected = (SelectedEntryPath == Path);
				if (bIsSelected)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.8f, 0.5f));
					ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.65f, 1.0f, 1.0f));
				}

				if (Thumbnail)
				{
					ImGui::ImageButton(PathString.c_str(), (ImTextureID)Thumbnail->GetResource()->GetSRV(), { thumbnailSize, thumbnailSize });
				}

				if (bIsSelected)
				{
					ImGui::PopStyleColor(2);
				}

				if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				{
					SelectedEntryPath = Path;
				}
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					SelectedEntryPath = Path;
				}

				// 아이템 우클릭: Rename / Delete 팝업
				if (ImGui::BeginPopupContextItem())
				{
					SelectedEntryPath = Path;
					if (ImGui::MenuItem("Rename", "F2"))
					{
						StartRenaming(Path);
					}
					if (ImGui::MenuItem("Delete", "Del"))
					{
						ItemToDelete = Path;
					}
					ImGui::EndPopup();
				}

				// 텍스처 드래그 소스
				if (TextureAsset && ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload(EditorDragDrop::Texture, &TextureAsset, sizeof(UTexture2D*));
					ImGui::Image((ImTextureID)TextureAsset->GetResource()->GetSRV(), { thumbnailSize, thumbnailSize });
					ImGui::TextUnformatted(FilenameString.c_str());
					ImGui::EndDragDropSource();
				}

				// 폰트 드래그 소스
				if (FontAsset && ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload(EditorDragDrop::Font, &FontAsset, sizeof(UFont*));
					ImGui::TextUnformatted(FilenameString.c_str());
					ImGui::EndDragDropSource();
				}

				// 루아 스크립트 드래그 소스
				if (ExtString == ".lua" && ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload(EditorDragDrop::LuaScript, PathString.c_str(), PathString.size() + 1);
					if (Thumbnail)
					{
						ImGui::Image((ImTextureID)Thumbnail->GetResource()->GetSRV(), { thumbnailSize, thumbnailSize });
					}
					ImGui::TextUnformatted(FilenameString.c_str());
					ImGui::EndDragDropSource();
				}

				// 더블 클릭 이벤트 처리
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (bIsDirectory)
					{
						CurrentPath = Path; // 하위 폴더로 이동
					}
					else if (ExtString == ".scene")
					{
						if (World)
						{
							FEditorFileUtils::LoadSceneFromPath(World, PathString);
						}
					}
					else if (ExtString == ".lua")
					{
						FLuaScriptUtils::OpenScriptPath(PathString);
					}
				}

				// 파일 이름 표시 또는 인라인 이름 변경 입력창
				if (bIsRenaming && Path == RenamingPath)
				{
					ImGui::PushItemWidth(thumbnailSize + padding);
					if (bFocusRenameInput)
					{
						ImGui::SetKeyboardFocusHere();
						bFocusRenameInput = false;
					}
					if (ImGui::InputText("##RenameGridItem", RenameBuffer, sizeof(RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
					{
						CommitRename();
					}
					if (ImGui::IsItemDeactivated() && !ImGui::IsItemDeactivatedAfterEdit())
					{
						CancelRename();
					}
					ImGui::PopItemWidth();
				}
				else
				{
					ImGui::TextWrapped("%s", FilenameString.c_str());
				}

				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		if (!ItemToDelete.empty())
		{
			std::error_code DeleteError;
			fs::remove_all(ItemToDelete, DeleteError);
			if (SelectedEntryPath == ItemToDelete)
			{
				SelectedEntryPath.clear();
			}
			if (RenamingPath == ItemToDelete)
			{
				CancelRename();
			}
			ItemToDelete.clear();
		}
	}
	ImGui::EndChild();
	ImGui::End();
}

void FContentDrawerPanel::RenderFolderTree(const fs::path& Path, fs::path& SelectedPath)
{
	std::error_code IterateError;
	fs::directory_iterator It(Path, IterateError);
	const fs::directory_iterator End;

	for (; !IterateError && It != End; It.increment(IterateError))
	{
		const fs::path Entry = It->path();

		std::error_code TypeError;
		const bool bIsDirectory =
			fs::is_directory(Entry, TypeError);

		if (TypeError || !bIsDirectory)
			continue;

		const FString FilenameString =
			Entry.filename().string();

		const bool bIsRenamingThis = (bIsRenaming && Entry == RenamingPath);
		if (bIsRenamingThis)
		{
			ImGui::SetNextItemWidth(150.0f);
			if (bFocusRenameInput)
			{
				ImGui::SetKeyboardFocusHere();
				bFocusRenameInput = false;
			}
			if (ImGui::InputText("##RenameTreeFolder", RenameBuffer, sizeof(RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
			{
				CommitRename();
			}
			if (ImGui::IsItemDeactivated() && !ImGui::IsItemDeactivatedAfterEdit())
			{
				CancelRename();
			}
		}
		else
		{
			ImGuiTreeNodeFlags Flags =
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_DrawLinesToNodes;

			if (Entry == SelectedPath)
				Flags |= ImGuiTreeNodeFlags_Selected;

			const bool bIsOpen =
				ImGui::TreeNodeEx(FilenameString.c_str(), Flags);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				SelectedPath = Entry;
				SelectedEntryPath = Entry;
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				SelectedEntryPath = Entry;
			}

			// 폴더 우클릭 팝업
			if (ImGui::BeginPopupContextItem())
			{
				SelectedEntryPath = Entry;
				if (ImGui::MenuItem("Rename", "F2"))
				{
					StartRenaming(Entry);
				}
				if (ImGui::MenuItem("Delete", "Del"))
				{
					std::error_code DeleteError;
					fs::remove_all(Entry, DeleteError);
					if (SelectedEntryPath == Entry)
					{
						SelectedEntryPath.clear();
					}
					if (CurrentPath == Entry)
					{
						CurrentPath = Entry.parent_path();
					}
				}
				ImGui::EndPopup();
			}

			if (bIsOpen)
			{
				RenderFolderTree(Entry, SelectedPath);
				ImGui::TreePop();
			}
		}
	}

	if (IterateError)
	{
		ImGui::TextUnformatted("Could not read this folder.");
	}
}

void FContentDrawerPanel::StartRenaming(const fs::path& InPath)
{
	std::error_code EC;
	if (!fs::exists(InPath, EC)) return;

	RenamingPath = InPath;
	SelectedEntryPath = InPath;
	std::string Name = InPath.filename().string();
	strncpy_s(RenameBuffer, sizeof(RenameBuffer), Name.c_str(), _TRUNCATE);
	bIsRenaming = true;
	bFocusRenameInput = true;
}

void FContentDrawerPanel::CancelRename()
{
	bIsRenaming = false;
	bFocusRenameInput = false;
	RenamingPath.clear();
	memset(RenameBuffer, 0, sizeof(RenameBuffer));
}

void FContentDrawerPanel::CommitRename()
{
	if (!bIsRenaming || RenamingPath.empty())
	{
		CancelRename();
		return;
	}

	std::string NewNameStr = RenameBuffer;
	// 앞뒤 공백 제거
	size_t first = NewNameStr.find_first_not_of(" \t\r\n");
	size_t last = NewNameStr.find_last_not_of(" \t\r\n");
	if (first == std::string::npos || last == std::string::npos)
	{
		CancelRename();
		return;
	}
	NewNameStr = NewNameStr.substr(first, last - first + 1);

	fs::path ParentDir = RenamingPath.parent_path();
	fs::path NewPath;

	std::error_code EC;
	bool bIsDir = fs::is_directory(RenamingPath, EC);

	if (bIsDir)
	{
		NewPath = ParentDir / NewNameStr;
	}
	else
	{
		fs::path NewNamePath(NewNameStr);
		// 사용자가 확장자를 생략했을 경우 기존 확장자 자동 유지
		if (!NewNamePath.has_extension() && RenamingPath.has_extension())
		{
			NewPath = ParentDir / (NewNameStr + RenamingPath.extension().string());
		}
		else
		{
			NewPath = ParentDir / NewNameStr;
		}
	}

	if (NewPath != RenamingPath)
	{
		if (fs::exists(NewPath, EC))
		{
			HTR_LOG(Warning, "[ContentDrawer] Cannot rename: '{}' already exists!", NewPath.filename().string());
		}
		else
		{
			fs::rename(RenamingPath, NewPath, EC);
			if (EC)
			{
				HTR_LOG(Error, "[ContentDrawer] Rename failed: {}", EC.message());
			}
			else
			{
				HTR_LOG(Info, "[ContentDrawer] Renamed '{}' -> '{}'", RenamingPath.filename().string(), NewPath.filename().string());
				if (SelectedEntryPath == RenamingPath)
				{
					SelectedEntryPath = NewPath;
				}
				if (CurrentPath == RenamingPath)
				{
					CurrentPath = NewPath;
				}
			}
		}
	}

	CancelRename();
}

void FContentDrawerPanel::CreateNewScript(const fs::path& Directory)
{
	std::error_code EC;
	fs::path TargetPath = Directory / "NewScript.lua";
	int Counter = 1;
	while (fs::exists(TargetPath, EC))
	{
		TargetPath = Directory / ("NewScript_" + std::to_string(Counter++) + ".lua");
	}

	fs::path TemplatePath = FLuaScriptUtils::MakeAbsolutePath(FLuaScriptUtils::TemplatePath);
	if (!TemplatePath.empty() && fs::exists(TemplatePath, EC))
	{
		fs::copy_file(TemplatePath, TargetPath, fs::copy_options::overwrite_existing, EC);
	}
	else
	{
		std::ofstream Out(TargetPath);
		if (Out.is_open())
		{
			Out << "function BeginPlay()\n";
			Out << "    print(\"[BeginPlay] \" .. obj.UUID)\n";
			Out << "end\n\n";
			Out << "function Tick(dt)\n";
			Out << "end\n\n";
			Out << "function EndPlay()\n";
			Out << "end\n\n";
			Out << "function OnOverlap(OtherActor)\n";
			Out << "end\n\n";
			Out << "function OnHit(OtherActor)\n";
			Out << "end\n";
			Out.close();
		}
	}
}

void FContentDrawerPanel::CreateNewScene(const fs::path& Directory)
{
	std::error_code EC;
	fs::path TargetPath = Directory / "NewScene.scene";
	int Counter = 1;
	while (fs::exists(TargetPath, EC))
	{
		TargetPath = Directory / ("NewScene_" + std::to_string(Counter++) + ".scene");
	}

	std::ofstream Out(TargetPath);
	if (Out.is_open())
	{
		Out << "{\n";
		Out << "    \"Version\": 4,\n";
		Out << "    \"World\": {\n";
		Out << "        \"PersistentLevel\": {\n";
		Out << "            \"Actors\": []\n";
		Out << "        }\n";
		Out << "    }\n";
		Out << "}\n";
		Out.close();
	}
}
