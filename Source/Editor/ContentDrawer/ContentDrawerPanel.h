#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
class UWorld;

class FContentDrawerPanel : public IEditorPanel
{
public:
	bool Init() override;
	void Tick(float DeltaTime) override;
	void OnRender() override;
	const char* GetPanelName() const override { return "Content Drawer"; }
	void RenderFolderTree(const fs::path& Path, fs::path& SelectedPath);

	void SetWorld(UWorld* InWorld) { World = InWorld; }

	void StartRenaming(const fs::path& InPath);
	void CommitRename();
	void CancelRename();

private:
	void CreateNewScript(const fs::path& Directory);
	void CreateNewScene(const fs::path& Directory);

	UWorld* World = nullptr;
	std::vector<fs::path> RootFolders = { "Assets", "Scripts", "Scenes" };
	fs::path CurrentPath = "Assets";
	fs::path SelectedEntryPath;
	fs::path RenamingPath;
	char RenameBuffer[256] = {};
	bool bIsRenaming = false;
	bool bFocusRenameInput = false;

	const FString FolderIconPath = "Assets/Icons/Folder.png";
	const FString FileIconPath = "Assets/Icons/File.png";
};
