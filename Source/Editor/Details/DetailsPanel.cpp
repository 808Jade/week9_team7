#include "EnginePCH.h"
#include "Editor/Details/DetailsPanel.h"

#include "imgui_internal.h"
#include "Editor/HitoriEd/EditorDragDrop.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ParticleSubUVComponent.h"
#include "Components/SpotLightComponent.h"
#include "Editor/EditorUI/ImGuiRenderer.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/LuaScriptComponent.h"

#include "Components/RotatingMovementComponent.h"
#include "Components/ProjectileMovementComponent.h"
#include "Components/FireBallComponent.h"
#include "Components/LuaScriptComponent.h"
#include "Asset/AssetManager.h"
#include "Render/Material.h"
#include "Render/Texture2D.h"
#include "Text/Font.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor.h"
#include "Core/EngineLog.h"
#include "Editor/HitoriEd/LuaScriptUtils.h"
#include "Editor/HitoriEd/EditorFileUtils.h"


namespace
{
	bool DrawAxisControl(const FString& _label, float& _value, float _speed, float _minValue, float _maxValue, float _resetValue, FVector4 _color)
	{
		bool isValueChanged = false;

		float lineHeight = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec4 color(_color.X, _color.Y, _color.Z, _color.W);
		ImVec2 buttonSize = { lineHeight + 2.0f , lineHeight };

		// 媛?踰꾪듉???됱긽 ?ㅼ젙
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ color.x + 0.1f, color.y + 0.1f, color.z + 0.1f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ color.x - 0.1f, color.y - 0.1f, color.z - 0.1f, 1.0f });

		// 援듭? ?고듃 ?곸슜
		if (ImGui::Button(_label.c_str(), buttonSize))
		{
			_value = _resetValue;
			isValueChanged = true;
		}
		ImGui::PopStyleColor(3);

		// ?쒕옒洹??щ씪?대뜑
		ImGui::SameLine();
		std::string dragID = "##" + _label;
		isValueChanged |= ImGui::DragFloat(dragID.c_str(), &_value, _speed, _minValue, _maxValue, "%.2f");

		return isValueChanged;

	}

	bool DrawVector3Controller(const FString& _label, float* _values, float _resetValue, float _columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable;

		bool isVectorChanged = false;

		ImGui::PushID(_label.c_str());
		if (ImGui::BeginTable(_label.c_str(), 2, flags)) // 怨좎쑀 ID, ??2媛? ?뚮옒洹?
		{
			// ImGuiTableColumnFlags_WidthFixed: 珥덇린 ?덈퉬 怨좎젙
			// ImGuiTableColumnFlags_WidthStretch: 李??ш린???곕씪 ?덈퉬 議곗젅 (湲곕낯媛?
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, _columnWidth);
			ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text(_label.c_str()); // ?쇱そ ?? ?덉씠釉?

			ImGui::TableSetColumnIndex(1);
			// [而⑦듃濡ㅻ윭 UI 肄붾뱶] // ?ㅻⅨ履??? 而⑦듃濡ㅻ윭
			ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 2,0 });
			ImGui::PushFont(boldFont);

			isVectorChanged |= DrawAxisControl("X", _values[0], 0.1f, 0.0f, 0.0f, _resetValue, { 0.8f, 0.1f, 0.1f, 1.0f });

			ImGui::PopItemWidth();

			ImGui::SameLine();
			isVectorChanged |= DrawAxisControl("Y", _values[1], 0.1f, 0.0f, 0.0f, _resetValue, { 0.1f, 0.8f, 0.1f, 1.0f });

			ImGui::PopItemWidth();

			ImGui::SameLine();
			isVectorChanged |= DrawAxisControl("Z", _values[2], 0.1f, 0.0f, 0.0f, _resetValue, { 0.1f, 0.1f, 0.8f, 1.0f });

			ImGui::PopItemWidth();

			ImGui::PopFont();
			ImGui::PopStyleVar();
			ImGui::EndTable();
		}

		ImGui::Columns(1);

		ImGui::PopID();

		return isVectorChanged;
	}

	// FRotator??V??(Pitch, Yaw, Roll) ?쒖꽌吏留??붿쭊? Roll=X, Pitch=Y, Yaw=Z異뺤씠誘濡?
	// X/Y/Z 移몄씠 ?ㅼ젣 ?뚯쟾異뺢낵 留욌룄濡??щ같?댄빐 ?몄쭛?쒕떎.
	bool DrawRotatorAsXYZ(const FString& _label, FRotator& _rotator)
	{
		float Euler[3] = { _rotator.Roll, _rotator.Pitch, _rotator.Yaw };
		const bool isChanged = DrawVector3Controller(_label, Euler, 0.0f, 55.0f);
		if (isChanged)
		{
			_rotator.Roll = Euler[0];
			_rotator.Pitch = Euler[1];
			_rotator.Yaw = Euler[2];
		}
		return isChanged;
	}

	void DrawTextureSlot(UMaterial** MaterialPtr, int32 SlotIndex)
	{
		const float ThumbnailSize = 64.0f;

		UMaterial* Material = *MaterialPtr;
		UTexture2D* Texture = nullptr;
		if (Material && SlotIndex < static_cast<int32>(Material->Textures.Num()))
		{
			Texture = Material->Textures[SlotIndex];
		}

		ImGui::PushID(SlotIndex);

		if (Texture && Texture->GetResource())
		{
			ImGui::Image(
				(ImTextureID)Texture->GetResource()->GetSRV(),
				{ ThumbnailSize, ThumbnailSize });
		}
		else
		{
			ImGui::Button("Empty", { ThumbnailSize, ThumbnailSize });
		}

		// 諛붾줈 ???꾩젽???쒕∼??諛쏅뒗??
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(EditorDragDrop::Texture))
			{
				UTexture2D* DroppedTexture = *static_cast<UTexture2D**>(Payload->Data);

				// 癒명떚由ъ뼹???놁쑝硫?湲곕낯 癒명떚由ъ뼹?먯꽌 ?쒖옉?쒕떎
				if (!Material)
				{
					Material = UMaterial::CreateInstance(UAssetManager::GetAssetByPath<UMaterial>("DefaultMaterial"));
					*MaterialPtr = Material;
				}
				// 怨듭쑀 ?먯뀑?대㈃ 吏湲?蹂듭젣?쒕떎.
				// ???쒖젏源뚯? 誘몃（硫??먮?吏 ?딆? ?ㅻ툕?앺듃??怨꾩냽 怨듭쑀 ?먯뀑???대떎.
				else if (Material->bIsInstance == false)
				{
					Material = UMaterial::CreateInstance(Material);
					*MaterialPtr = Material;
				}

				if (Material)
				{
					// ?щ’??諛곗뿴 踰붿쐞 諛뽰씠硫?洹??먮━源뚯? ?섎젮 以??
					while (static_cast<int32>(Material->Textures.Num()) <= SlotIndex)
					{
						Material->Textures.Add(nullptr);
					}

					Material->Textures[SlotIndex] = DroppedTexture;
					Texture = DroppedTexture;
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::Text("Slot %d", SlotIndex);

		if (Texture)
		{
			ImGui::TextWrapped("%s", Texture->GetPath().c_str());
		}
		else
		{
			ImGui::TextDisabled("(鍮꾩뼱 ?덉쓬)");
		}
		ImGui::EndGroup();

		ImGui::PopID();
	}

	FString GetAssetDisplayName(const FString& Path)
	{
		fs::path AssetPath(Path);
		if (AssetPath.stem() == "Atlas")
		{
			return AssetPath.parent_path().filename().string();
		}
		return AssetPath.stem().string();
	}

	void DrawFontSlot(UObject* Owner, UFont** FontPtr)
	{
		UFont* Current = *FontPtr;

		FString CurrentPath = Current ? Current->GetPath() : "";
		FString CurrentFileName = Current ? GetAssetDisplayName(CurrentPath) : "None";

		ImGui::SetNextItemWidth(-1.0f);

		bool bOpen = ImGui::BeginCombo("##Font", CurrentFileName.c_str());
		if (ImGui::IsItemHovered() && Current)
		{
			ImGui::SetTooltip("%s", CurrentPath.c_str());
		}

		if (bOpen)
		{
			for (TObjectIterator<UFont> Itr; Itr; ++Itr)
			{
				bool bSelected = (*Itr == Current);

				FString ItemPath = Itr->GetPath();
				FString ItemFileName = GetAssetDisplayName(ItemPath);

				ImGui::PushID(*Itr);

				if (ImGui::Selectable(ItemFileName.c_str(), bSelected))
				{
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("%s", ItemPath.c_str());
					}

					// 硫붾え由щ? 吏곸젒 諛붽씀吏 ?딄퀬 SetFont瑜?嫄곗튇??
					if (UTextRenderComponent* TextComponent = Cast<UTextRenderComponent>(Owner))
						TextComponent->SetFont(*Itr);
					else
						*FontPtr = *Itr;
				}
				ImGui::PopID();
				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	void DrawMeshSlot(UObject* Owner, UStaticMesh** StaticMeshPtr)
	{
		UStaticMesh* Current = *StaticMeshPtr;

		FString CurrentPath = Current ? Current->GetPath() : "";
		FString CurrentFileName = Current ? GetAssetDisplayName(CurrentPath) : "None";

		ImGui::SetNextItemWidth(-1.0f);

		bool bOpen = ImGui::BeginCombo("##StaticMesh", CurrentFileName.c_str());
		if (ImGui::IsItemHovered() && Current)
		{
			ImGui::SetTooltip("%s", CurrentPath.c_str());
		}

		if (bOpen)
		{
			for (TObjectIterator<UStaticMesh> Itr; Itr; ++Itr)
			{
				bool bSelected = (*Itr == Current);

				FString ItemPath = Itr->GetPath();
				FString ItemFileName = GetAssetDisplayName(ItemPath);

				ImGui::PushID(*Itr);

				if (ImGui::Selectable(ItemFileName.c_str(), bSelected))
				{
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("%s", ItemPath.c_str());
					}

					// 硫붾え由щ? 吏곸젒 諛붽씀吏 ?딄퀬 SetFont瑜?嫄곗튇??
					if (UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Owner))
						MeshComponent->SetStaticMesh(*Itr);
					else
						*StaticMeshPtr = *Itr;
				}
				ImGui::PopID();
				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	UMaterial* EnsureMaterialOverride(UMeshComponent* MeshComponent, int32 Slot, UMaterial* Effective, UMaterial* Override)
	{
		// ??而댄룷?뚰듃 ?꾩슜 ?몄뒪?댁뒪媛 ?꾨땲硫?吏湲??곸슜 以묒씤 癒명떚由ъ뼹??蹂듭젣?댁꽌 ??뼱?곌린濡?留뚮뱺??
		// 硫붿떆??癒명떚由ъ뼹? 媛숈? 硫붿떆瑜??곕뒗 紐⑤뱺 ?≫꽣媛 怨듭쑀?섎?濡?吏곸젒 怨좎튂硫????쒕떎.
		if (Override && Override->bIsInstance)	return Override;

		UMaterial* Source = Effective ? Effective : UAssetManager::GetAssetByPath<UMaterial>("DefaultMaterial");

		Override = UMaterial::CreateInstance(Source);
		MeshComponent->SetMaterial(Slot, Override);

		return Override;
	}

	bool SetMaterialTexture(UMeshComponent* MeshComponent, int32 MaterialSlot, int32 TextureSlot, UTexture2D* Texture, UMaterial*& Effective, UMaterial*& Override) {
		UTexture2D* Current{ Effective && Effective->Textures.IsValidIndex(TextureSlot) ? Effective->Textures[TextureSlot] : nullptr };
		if (Current == Texture) {
			return false;
		}

		Override = EnsureMaterialOverride(MeshComponent, MaterialSlot, Effective, Override);
		if (!Override) {
			return false;
		}

		while (!Override->Textures.IsValidIndex(TextureSlot)) {
			Override->Textures.Add(nullptr);
		}
		Override->Textures[TextureSlot] = Texture;
		Effective = Override;
		return true;
	}

	void DrawMaterialTextureSlot(UMeshComponent* MeshComponent, int32 MaterialSlot, int32 TextureSlot, const char* Label, UMaterial*& Effective, UMaterial*& Override) {
		const float ThumbnailSize{ 64.0f };
		const float ItemThumbnailSize{ 24.0f };
		UTexture2D* Texture{ Effective && Effective->Textures.IsValidIndex(TextureSlot) ? Effective->Textures[TextureSlot] : nullptr };

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(Label);
		ImGui::TableSetColumnIndex(1);
		ImGui::PushID(TextureSlot);

		if (Texture && Texture->GetResource()) {
			ImGui::Image((ImTextureID)Texture->GetResource()->GetSRV(), { ThumbnailSize, ThumbnailSize });
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", Texture->GetPath().c_str());
			}
		} else {
			ImGui::Button("Empty", { ThumbnailSize, ThumbnailSize });
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* Payload{ ImGui::AcceptDragDropPayload(EditorDragDrop::Texture) }) {
				UTexture2D* DroppedTexture{ *static_cast<UTexture2D**>(Payload->Data) };
				if (SetMaterialTexture(MeshComponent, MaterialSlot, TextureSlot, DroppedTexture, Effective, Override)) {
					Texture = DroppedTexture;
				}
			}
			ImGui::EndDragDropTarget();
		}

		const FString CurrentPath{ Texture ? Texture->GetPath() : "" };
		const FString CurrentFileName{ Texture ? GetAssetDisplayName(CurrentPath) : "None" };
		ImGui::SetNextItemWidth(-1.0f);
		const bool Open{ ImGui::BeginCombo("##Texture", CurrentFileName.c_str()) };
		if (ImGui::IsItemHovered() && Texture) {
			ImGui::SetTooltip("%s", CurrentPath.c_str());
		}

		if (Open) {
			if (ImGui::Selectable("None", Texture == nullptr)) {
				SetMaterialTexture(MeshComponent, MaterialSlot, TextureSlot, nullptr, Effective, Override);
			}

			for (TObjectIterator<UTexture2D> Iterator{}; Iterator; ++Iterator) {
				UTexture2D* ItemTexture{ *Iterator };
				if (!ItemTexture->GetResource() || ItemTexture->GetPath().empty()) {
					continue;
				}

				const bool Selected{ ItemTexture == Texture };
				const FString ItemPath{ ItemTexture->GetPath() };
				const FString ItemFileName{ GetAssetDisplayName(ItemPath) };
				ImGui::PushID(ItemTexture);
				ImGui::Image((ImTextureID)ItemTexture->GetResource()->GetSRV(), { ItemThumbnailSize, ItemThumbnailSize });
				ImGui::SameLine();
				if (ImGui::Selectable(ItemFileName.c_str(), Selected, ImGuiSelectableFlags_None, { 0.0f, ItemThumbnailSize })) {
					SetMaterialTexture(MeshComponent, MaterialSlot, TextureSlot, ItemTexture, Effective, Override);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", ItemPath.c_str());
				}
				if (Selected) {
					ImGui::SetItemDefaultFocus();
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}

		ImGui::PopID();
	}

	// 硫붿떆 而댄룷?뚰듃??癒명떚由ъ뼹 ?щ’????以꾩뵫 洹몃┛??
	// ?띿뒪泥섎? ?쒕∼?섎㈃ 洹??щ’????뼱?곌린??諛섏쁺?섍퀬, 硫붿떆 ?먯뀑??癒명떚由ъ뼹? 嫄대뱶由ъ? ?딅뒗??
	void DrawMaterialSlots(UMeshComponent* MeshComponent)
	{
		const int32 NumSlots = MeshComponent->GetNumMaterials();
		if (NumSlots <= 0)
		{
			return;
		}

		if (!ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		// 硫붿돩 ?댁쓽 留ㅽ꽣由ъ뼹 蹂?Slot ?쒗쉶
		for (int32 Slot = 0; Slot < NumSlots; ++Slot)
		{
			ImGui::PushID(Slot);

			FString SlotLabel = std::format("[{}] {}", Slot, MeshComponent->GetMaterialSlotName(Slot));
			bool bOpen = ImGui::TreeNode(SlotLabel.c_str());

			UMaterial* Override = MeshComponent->GetOverrideMaterial(Slot);
			UMaterial* Effective = MeshComponent->GetMaterial(Slot);

			ImGui::SameLine();

			if (Override)
			{
				ImGui::TextDisabled("Override");
				ImGui::SameLine();

				if (ImGui::SmallButton("Reset"))
				{
					MeshComponent->SetMaterial(Slot, nullptr);
					Override = nullptr;
					Effective = MeshComponent->GetMaterial(Slot);
				}
			}
			else
			{
				ImGui::TextDisabled("Default");
			}

			if (bOpen)
			{
				if (ImGui::BeginTable("MaterialProperties", 2))
				{
					ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

					DrawMaterialTextureSlot(MeshComponent, Slot, 0, "Texture", Effective, Override);
					DrawMaterialTextureSlot(MeshComponent, Slot, 1, "Normal Map", Effective, Override);

					ImGui::TableNextRow();

					// 3: Base Color
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Base Color");

					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-1.0f);

					FVector4 TempColor = Effective ? Effective->BaseColor : FVector4(1.0f, 1.0f, 1.0f, 1.0f);
					if (ImGui::ColorEdit4("##BaseColor", &TempColor.X))
					{
						Override = EnsureMaterialOverride(MeshComponent, Slot, Effective, Override);
						if (Override)
						{
							Override->BaseColor = TempColor;
							Effective = Override;
						}
					}

					ImGui::TableNextRow();
					
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Shininess");
					
					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-1.0f);
					
					float TempShininess =Effective ? Effective->Shininess : 32.0f;
					if (ImGui::DragFloat("##Shininess", &TempShininess, 0.5f, 1.0f, 256.0f))
					{
						Override = EnsureMaterialOverride(MeshComponent, Slot, Effective, Override);
						if (Override)
						{
							Override->Shininess = TempShininess;
							Effective = Override;
						}
					}

					ImGui::TableNextRow();
					
					// 4: UV Scroll Speed
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("UV Scroll");

					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-1.0f);

					FVector2 TempUVScrollSpeed = Effective ? Effective->UVScrollSpeed : FVector2();
					if (ImGui::DragFloat2("##UVScrollSpeed", &TempUVScrollSpeed.X, 0.01f))
					{
						Override = EnsureMaterialOverride(MeshComponent, Slot, Effective, Override);
						if (Override)
						{
							Override->UVScrollSpeed = TempUVScrollSpeed;
							Effective = Override;
						}
					}

					ImGui::TableNextRow();

					// 5: Sampler State
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Sampler");

					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-1.0f);

					const char* SamplerItems[] = { "Linear Clamp", "Linear Wrap" };
					int SamplerIndex = Effective ? static_cast<int>(Effective->SamplerState) : static_cast<int>(ESamplerState::LinearClamp);
					if (ImGui::BeginCombo("##SamplerState", SamplerItems[SamplerIndex]))
					{
						for (int i = 0; i < static_cast<int>(ESamplerState::Count); ++i)
						{
							if (ImGui::Selectable(SamplerItems[i], SamplerIndex == i))
							{
								Override = EnsureMaterialOverride(MeshComponent, Slot, Effective, Override);
								if (Override)
								{
									Override->SamplerState = static_cast<ESamplerState>(i);
									Effective = Override;
								}
							}
						}
						ImGui::EndCombo();
					}

					ImGui::TableNextRow();

					// 6: Blend State
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Blend Mode");

					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-1.0f);

					const char* BlendItems[] = { "Opaque", "Alpha Blend" };
					int BlendIndex = Effective ? static_cast<int>(Effective->BlendState) : static_cast<int>(EBlendState::Opaque);
					if (ImGui::BeginCombo("##BlendState", BlendItems[BlendIndex]))
					{
						for (int i = 0; i < IM_ARRAYSIZE(BlendItems); ++i)
						{
							if (ImGui::Selectable(BlendItems[i], BlendIndex == i))
							{
								Override = EnsureMaterialOverride(MeshComponent, Slot, Effective, Override);
								if (Override)
								{
									Override->BlendState = static_cast<EBlendState>(i);
									Effective = Override;
								}
							}
						}
						ImGui::EndCombo();
					}

					ImGui::EndTable();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();

			ImGui::Separator();
		}
	}

	// UClass???깅줉???꾨줈?쇳떚瑜???낆뿉 留욌뒗 ?꾩젽?쇰줈 洹몃┛??
	void DrawProperty(UObject* Object, const FProperty& Property, ImFont* CustomFont)
	{
		void* ValuePtr = reinterpret_cast<char*>(Object) + Property.Offset;
		const FString Label = "##" + Property.Name;

		const char* DisplayName = Property.Name.c_str();

		// ?⑤꼸???볦쑝硫??쇰꺼 | 媛믪쓣 ??以꾩뿉, 醫곴굅???쇰꺼??湲몃㈃ ?쇰꺼 ?꾨옒 以꾩뿉 媛믪쓣 ?볥뒗??
		const float StartX = ImGui::GetCursorPosX();
		const float AvailableWidth = ImGui::GetContentRegionAvail().x;
		const float LabelWidth = AvailableWidth * 0.45f;

		const bool bInline = Property.Type != EPropertyType::Transform && AvailableWidth >= 260.0f && ImGui::CalcTextSize(DisplayName).x < LabelWidth;

		ImGui::AlignTextToFramePadding();
		ImGui::TextWrapped("%s", DisplayName);

		if (bInline)
			ImGui::SameLine(StartX + LabelWidth);

		ImGui::SetNextItemWidth(-1.0f);

		bool bChanged = false;

		switch (Property.Type)
		{
		case EPropertyType::Float:
			bChanged = ImGui::DragFloat(Label.c_str(), static_cast<float*>(ValuePtr), 0.1f);
			break;
		case EPropertyType::Int:
			bChanged = ImGui::DragInt(Label.c_str(), static_cast<int*>(ValuePtr), 1.0f);
			break;
		case EPropertyType::Bool:
			bChanged = ImGui::Checkbox(Label.c_str(), static_cast<bool*>(ValuePtr));
			break;
		case EPropertyType::Vector:
		{
			FVector* Value = static_cast<FVector*>(ValuePtr);
			bChanged = ImGui::DragFloat3(Label.c_str(), Value->V, 0.1f);
			break;
		}
		case EPropertyType::Rotator:
		{
			FRotator* Value = static_cast<FRotator*>(ValuePtr);
			// Transform??Rotation怨?媛숈? X/Y/Z 異??쒖꽌濡?蹂댁뿬以??
			float Euler[3] = { Value->Roll, Value->Pitch, Value->Yaw };
			if (ImGui::DragFloat3(Label.c_str(), Euler, 0.1f))
			{
				Value->Roll = Euler[0];
				Value->Pitch = Euler[1];
				Value->Yaw = Euler[2];
			}
			break;
		}


		case EPropertyType::Vector4:
		{
			FVector4* Value = static_cast<FVector4*>(ValuePtr);
			bChanged = ImGui::DragFloat4(Label.c_str(), &Value->X, 0.1f);
			break;
		}

		case EPropertyType::Color:
		{
			FColor* C = static_cast<FColor*>(ValuePtr);
			float V[4] = { C->R / 255.f, C->G / 255.f, C->B / 255.f, C->A / 255.f };
			if (ImGui::ColorEdit4(Label.c_str(), V))
			{
				*C = FColor(uint8(V[0] * 255.f + 0.5f), uint8(V[1] * 255.f + 0.5f),
					uint8(V[2] * 255.f + 0.5f), uint8(V[3] * 255.f + 0.5f));
				bChanged = true;
			}
			break;
		}

		case EPropertyType::LinearColor:
		{
			// ??낆? Vector4? 媛숆퀬 ?꾩젽留??됱긽 ?좏깮湲곕떎
			FLinearColor* Value = static_cast<FLinearColor*>(ValuePtr);
			bChanged = ImGui::ColorEdit4(Label.c_str(), &Value->R);
			break;
		}
		case EPropertyType::String:
		{
			FString* Value = static_cast<FString*>(ValuePtr);

			char Buffer[256] = {};
			strncpy_s(Buffer, Value->c_str(), sizeof(Buffer) - 1);
			if (CustomFont) ImGui::PushFont(CustomFont);
			if (ImGui::InputText(Label.c_str(), Buffer, sizeof(Buffer)))
			{
				*Value = Buffer;
			}
			if (CustomFont) ImGui::PopFont();

			if (Property.Name == "ScriptPath")
			{
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					FLuaScriptUtils::OpenScriptPath(*Value);
				}
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(EditorDragDrop::LuaScript))
					{
						const char* DroppedScriptPath = static_cast<const char*>(Payload->Data);
						if (DroppedScriptPath)
						{
							*Value = DroppedScriptPath;
							if (ULuaScriptComponent* LuaComp = Cast<ULuaScriptComponent>(Object))
							{
								LuaComp->SetScriptPath(*Value);
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
			}
			break;
		}
		case EPropertyType::Transform:
		{
			FTransform* Value = static_cast<FTransform*>(ValuePtr);

			ImGui::NewLine();

			FTransform Transform = *Value;
			bool bChange = false;

			bChanged |= DrawVector3Controller("Location", Value->Location.V, 0.0f, 55.0f);
			bChanged |= DrawRotatorAsXYZ("Rotation", Value->Rotation);
			bChanged |= DrawVector3Controller("Scale", Value->Scale.V, 1.0f, 55.0f);

			//if (bChange)
			//{
			//	// ??而댄룷?뚰듃硫?SetTransform??嫄곗튂怨? ?꾨땲硫?媛믪쓣 吏곸젒 ?대떎
			//	if (USceneComponent* SceneComponent = Cast<USceneComponent>(Object))
			//	{
			//		SceneComponent->SetTransform(Transform);
			//	}
			//	else
			//	{
			//		*Value = Transform;
			//	}
			//}
			break;
		}
		case EPropertyType::Object:
		{
			UObject** ObjPtr = static_cast<UObject**>(ValuePtr);

			if (Property.Class == UMaterial::StaticClass())
			{
				DrawTextureSlot(reinterpret_cast<UMaterial**>(ObjPtr), 0);
			}
			else if (Property.Class == UFont::StaticClass())
			{
				DrawFontSlot(Object, reinterpret_cast<UFont**>(ObjPtr));
			}
			else if (Property.Class == UStaticMesh::StaticClass())
			{
				DrawMeshSlot(Object, reinterpret_cast<UStaticMesh**>(ObjPtr));
			}
			break;
		}
		default:
			ImGui::TextDisabled("(Unsupported)");
			break;
		}

		if (bChanged)
		{
			Object->PostEditChangeProperty(Property);
		}
	}

	// ?대옒??怨꾩링???곕씪 ?щ씪媛硫?媛??④퀎???꾨줈?쇳떚瑜??쒖떆
	void DrawProperties(UObject* Object, ImFont* CustomFont)
	{
		if (!Object)
		{
			return;
		}

		TArray<UClass*> ClassChain;
		for (UClass* Class = Object->GetClass(); Class; Class = Class->Super)
		{
			ClassChain.Add(Class);
		}

		// 湲곕컲 ?대옒?ㅻ????쒖떆
		for (auto It = ClassChain.rbegin(); It != ClassChain.rend(); ++It)
		{
			UClass* Class = *It;

			// ?몄텧???꾨줈?쇳떚媛 ?섎굹???놁쑝硫??ㅻ뜑??洹몃━吏 ?딅뒗??
			bool bHasVisibleProperty = false;
			for (const FProperty& Property : Class->GetProperties())
			{
				if (Property.HasAnyFlags(CPF_Edit))
				{
					bHasVisibleProperty = true;
					break;
				}
			}
			if (!bHasVisibleProperty)
			{
				continue;
			}

			ImGui::PushID(Class->Name.c_str());
			if (ImGui::CollapsingHeader(Class->Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (const FProperty& Property : Class->GetProperties())
				{
					if (!Property.HasAnyFlags(CPF_Edit))
					{
						continue;
					}

					const bool bReadOnly = Property.HasAnyFlags(CPF_EditConst);
					if (bReadOnly) ImGui::BeginDisabled();
					DrawProperty(Object, Property, CustomFont);
					if (bReadOnly) ImGui::EndDisabled();
				}
			}
			ImGui::PopID();
		}
	}

	// 컴포넌트 프로퍼티 아래에 버튼 1개 + "Script File" 칸
	// 스크립트가 없으면 [Create Script], 있으면 [Edit Script] (같은 자리에서 라벨이 바뀐다)
	void DrawScriptSection(AActor* Actor, UActorComponent*& OutSelectedComponent)
	{
		if (!Actor) { return; }

		// ---------- 1. 이번 프레임 상태 계산 ----------
		// BeginDisabled/EndDisabled 짝이 어긋나지 않도록 조건은 여기서 한 번만 계산해 고정한다.
		ULuaScriptComponent* ScriptComp = FLuaScriptUtils::GetScriptComponent(Actor);
		const bool bHasScriptPath = ScriptComp && ScriptComp->HasScriptPath();

		// GenerateScript와 같은 기준(에디터 월드에서만 생성 허용)
		UWorld* World = Actor->GetWorld();
		const bool bIsEditorWorld = World && World->GetWorldType() == EWorldType::Editor;
		const bool bIsSceneSaved = !FEditorFileUtils::GetCurrentScenePath().empty();

		ImGui::PushID("LuaScriptSection");

		// ---------- 2. 버튼 1개: Create Script / Edit Script ----------
		if (!bHasScriptPath)
		{
			const bool bCanCreate = bIsEditorWorld && bIsSceneSaved;

			ImGui::BeginDisabled(!bCanCreate);
			if (ImGui::Button("Create Script"))
			{
				// 실패 이유는 FLuaScriptUtils가 로그로 남기므로 UI에서는 분기하지 않는다
				if (FLuaScriptUtils::GenerateScript(Actor))
				{
					// Add Component와 동일하게 스크립트 컴포넌트를 바로 선택
					if (ULuaScriptComponent* CreatedComp = FLuaScriptUtils::GetScriptComponent(Actor))
					{
						OutSelectedComponent = CreatedComp;
					}
				}
			}
			ImGui::EndDisabled();
			// AllowWhenDisabled가 없으면 비활성 버튼에서는 툴팁이 뜨지 않는다
			if (!bCanCreate && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("%s", !bIsEditorWorld ? "Not available during PIE. Stop PIE first." : "Save the scene first.");
			}
		}
		else
		{
			// PIE 중에도 허용 (Play 중 수정 -> Hot Reload)
			if (ImGui::Button("Edit Script"))
			{
				// 버튼이 하나뿐이라 '파일 복구'도 여기서 처리한다
				if (bIsEditorWorld)
				{
					FLuaScriptUtils::GenerateScript(Actor);
				}
				FLuaScriptUtils::OpenScript(Actor);
			}
		}

		// ---------- 3. Script File: 파일 이름만 읽기 전용으로 표시 ----------
		char FileNameBuffer[256] = {};
		if (bHasScriptPath)
		{
			// 저장 경로는 항상 '/' 구분자 (예: Scripts/Room_AStaticMeshActor_2.lua)
			const FString& ScriptPath = ScriptComp->GetScriptPath();
			const size_t SlashIndex = ScriptPath.find_last_of("/\\");
			const FString FileName = (SlashIndex == FString::npos) ? ScriptPath : ScriptPath.substr(SlashIndex + 1);
			strncpy_s(FileNameBuffer, sizeof(FileNameBuffer), FileName.c_str(), _TRUNCATE);
		}

		// ?ㅻⅨ UI? 媛숈? 湲곕낯 ?고듃濡??쒖떆
		UTexture2D* ScriptIcon = UAssetManager::GetAssetByPath<UTexture2D>("Assets/Icons/LuaScript.png");
		if (!ScriptIcon) ScriptIcon = UAssetManager::Get().LoadTexture("Assets/Icons/LuaScript.png", false);
		if (ScriptIcon && ScriptIcon->GetResource() && ScriptIcon->GetResource()->GetSRV())
		{
			ImGui::Image((ImTextureID)ScriptIcon->GetResource()->GetSRV(), ImVec2(18.0f, 18.0f));
			ImGui::SameLine();
		}

		ImGui::InputTextWithHint("Script File", "None", FileNameBuffer, sizeof(FileNameBuffer), ImGuiInputTextFlags_ReadOnly);

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (bHasScriptPath)
			{
				FLuaScriptUtils::OpenScript(Actor);
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(EditorDragDrop::LuaScript))
			{
				const char* DroppedScriptPath = static_cast<const char*>(Payload->Data);
				if (DroppedScriptPath && strlen(DroppedScriptPath) > 0)
				{
					FLuaScriptUtils::AssignScript(Actor, DroppedScriptPath);
					if (ULuaScriptComponent* NewComp = FLuaScriptUtils::GetScriptComponent(Actor))
					{
						OutSelectedComponent = NewComp;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		// ?꾩껜 寃쎈줈??留덉슦?ㅻ? ?щ졇?????댄똻?쇰줈
		if (bHasScriptPath && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s\n(Double-click to open in editor)", ScriptComp->GetScriptPath().c_str());
		}

		// ?꾨줈?앺듃 Scripts/ ?대뜑 ??猷⑥븘 ?ㅽ겕由쏀듃 ?쒕∼?ㅼ슫 紐⑸줉
		const char* CurrentComboLabel = (FileNameBuffer[0] != '\0') ? FileNameBuffer : "Select from Scripts/...";
		if (ImGui::BeginCombo("Select Script", CurrentComboLabel))
		{
			std::error_code EC;
			if (std::filesystem::exists("Scripts", EC))
			{
				for (const auto& Entry : std::filesystem::recursive_directory_iterator("Scripts", EC))
				{
					if (Entry.is_regular_file() && Entry.path().extension() == ".lua")
					{
						std::string RelativePath = Entry.path().generic_string();
						std::string FileName = Entry.path().filename().string();
						bool bSelected = (bHasScriptPath && ScriptComp->GetScriptPath() == RelativePath);

						if (ImGui::Selectable(FileName.c_str(), bSelected))
						{
							FLuaScriptUtils::AssignScript(Actor, RelativePath);
							if (ULuaScriptComponent* NewComp = FLuaScriptUtils::GetScriptComponent(Actor))
							{
								OutSelectedComponent = NewComp;
							}
						}
						if (bSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("%s", RelativePath.c_str());
						}
					}
				}
			}
			ImGui::EndCombo();
		}

		ImGui::PopID();
	}

	void DrawActorHeader(AActor* Actor, UActorComponent*& OutSelectedComponent)
	{
		if (!Actor) { return; }

		static char ComponentNameInputBuf[256] = {};

		ImGui::Text("Actor: %s", Actor->GetName().c_str());
		ImGui::Text("UUID: %u", Actor->GetUUID());

		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup"))
		{
			ImGui::Text("Component Name:");
			ImGui::SameLine();
			ImGui::InputText("##Component Name", ComponentNameInputBuf, sizeof(ComponentNameInputBuf));
			TArray<UClass*> Classes;
			GetDerivedClasses(UActorComponent::StaticClass(), Classes);
			// ?대쫫???뺣젹?섎㈃ 硫붾돱媛 ?덉젙??
			for (UClass* Class : Classes)
			{
				//if (!IsSpawnableComponent(Class)) continue;
				if (ImGui::MenuItem(Class->Name.c_str()))
				{
					OutSelectedComponent = Actor->AddComponent(Class, FString(ComponentNameInputBuf));
					ComponentNameInputBuf[0] = '\0';
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

	}
}

bool FDetailsPanel::Init()
{
	ImGuiIO& io = ImGui::GetIO();

	io.Fonts->AddFontDefault();

	const char* fontPath = "HMKMRHD.ttf";
	float fontSize = 15.0f;
	const ImWchar* koreanRanges = io.Fonts->GetGlyphRangesKorean();

	CustomFont = io.Fonts->AddFontFromFileTTF(fontPath, fontSize, nullptr, koreanRanges);
	assert(CustomFont != nullptr);

	return true;
}

void FDetailsPanel::Tick(float DeltaTime)
{
}

const ULightComponent* FDetailsPanel::GetSelectedLight() const
{
	UActorComponent* Component = TargetComponent;
	while (Component && Component->IsHiddenInDetails())
	{
		USceneComponent* SceneComponent = Cast<USceneComponent>(Component);
		Component = SceneComponent ? SceneComponent->GetAttachParent() : nullptr;
	}
	return Cast<ULightComponent>(Component);
}

void FDetailsPanel::DrawShadowPreview()
{
	const ULightComponent* Light = GetSelectedLight();
	if (!Light || !QueryShadowPreview || !ImGuiRenderer) return;
	if (!ImGui::CollapsingHeader("Shadow Maps", ImGuiTreeNodeFlags_DefaultOpen)) return;
	FTexture2DArray* Atlas = nullptr;
	TArray<FLightShadowRenderData> Shadows;
	if (!QueryShadowPreview(Light, Atlas, Shadows) || !Atlas || !Atlas->GetWidth() || !Atlas->GetHeight())
	{
		ImGui::TextDisabled("No shadow map available for this light in the active view.");
		return;
	}
	ImGui::SliderFloat("Preview Contrast", &ShadowPreviewContrast, 1.0f, 256.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
	const float Width = float(Atlas->GetWidth());
	const float Height = float(Atlas->GetHeight());
	const float DisplayWidth = ImGui::GetContentRegionAvail().x;
	if (DisplayWidth <= 0) return;
	static const char* PointFaces[] = { "+X", "-X", "+Z", "-Z", "+Y", "-Y" };
	for (int32 i = 0; i < Shadows.Num(); ++i)
	{
		const auto& Shadow = Shadows[i];
		const FRect& Rect = Shadow.AtlasRect;
		if (Rect.Width <= 0 || Rect.Height <= 0) continue;
		ImGui::PushID(i);
		if (Light->GetLightType() == ELightType::Point)
			ImGui::Text("Face %s", PointFaces[i % 6]);
		else if (Light->GetLightType() == ELightType::Directional)
			ImGui::Text("Cascade %d", i + 1);
		ImGui::TextDisabled("Slice %u | %.0f x %.0f", Shadow.AtlasIndex, Rect.Width, Rect.Height);
		
		const ImVec2 UV0(Rect.X / Width, Rect.Y / Height);
		const ImVec2 UV1((Rect.X + Rect.Width) / Width, (Rect.Y + Rect.Height) / Height);
		
		if (!ImGuiRenderer->DrawShadowMap(Atlas->GetSRV(), Shadow.AtlasIndex,
			ImVec2(DisplayWidth, DisplayWidth * Rect.Height / Rect.Width), UV0, UV1, ShadowPreviewContrast))
			ImGui::TextDisabled("Shadow preview shader is unavailable.");
		ImGui::PopID();
	}
}


void FDetailsPanel::OnRender()
{
	ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);

	ImGui::Begin("Details");

	if (TargetActor)
	{
		// 鍮뚮낫?쒖쿂???뷀뀒?쇱뿉 ?④꺼吏?而댄룷?뚰듃媛 ?좏깮?섎㈃ ?몄텧?섎뒗 遺紐⑤줈 ?щ┛??
		// (?? ?쇱씠?몃뒗 鍮뚮낫?쒕? ?대┃?댁꽌 怨좊Ⅴ吏留??섏튂??SpotLight 履쎌뿉 ?덈떎)
		while (TargetComponent && TargetComponent->IsHiddenInDetails())
		{
			USceneComponent* HiddenComponent = Cast<USceneComponent>(TargetComponent);
			TargetComponent = HiddenComponent ? HiddenComponent->GetAttachParent() : nullptr;
		}

		UActorComponent* SelectCandidateComponent = TargetComponent;

		DrawActorHeader(TargetActor, TargetComponent);
		DrawCompoenetList(TargetActor, TargetComponent);				
		ImGui::Separator();

		// ?≫꽣 -> 而댄룷?뚰듃 ?쒖쑝濡? ?대옒?ㅻ퀎 ?꾨줈?쇳떚 ?쒖떆
		ImGui::PushID(TargetActor);
		DrawProperties(TargetActor, CustomFont);
		ImGui::PopID();

		if (TargetComponent)
		{
			ImGui::PushID(TargetComponent);
			ImGui::SeparatorText("Component Properties");
			DrawProperties(TargetComponent, CustomFont);

			// 명세 위치: 선택된 컴포넌트 프로퍼티 바로 아래에 [Create/Edit Script] + Script File
			DrawScriptSection(TargetActor, TargetComponent);

			DrawShadowPreview();

			if(SelectCandidateComponent != TargetComponent)
			{
				SelectComponent(TargetComponent);
			}
			if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(TargetComponent))
			{
				DrawMaterialSlots(MeshComponent);
			}
			ImGui::PopID();

			if (PendingDeleteComponent)
			{
				if (DeleteCallback)
				{
					DeleteCallback(PendingDeleteComponent);
				}

				PendingDeleteComponent = nullptr;
			}			
		}		
	}	
	ImGui::End();
}


void FDetailsPanel::SelectComponent(UActorComponent* Component)
{
	if (!Component)
	{
		TargetComponent = nullptr;
		EditingNameComponent = nullptr;
		bFocusNameEdit = false;

		if (Callback)
			Callback(nullptr);

		return;
	}

	TargetComponent = Component;

	if (Callback)
		Callback(TargetComponent);
}

void FDetailsPanel::DrawCompoenetList(AActor* SelectedActor, UActorComponent*& OutSelectedComponent)
{
	if (!SelectedActor) { return; }

	if (PendingDragComponent && PendingDroppedComponent)
	{
		TryReparent(PendingDroppedComponent, PendingDragComponent);
		PendingDragComponent = nullptr;
		PendingDroppedComponent = nullptr;
	}

	USceneComponent* Root = SelectedActor->GetRootComponent();

	if (Root && ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawSceneComponentNode(Root, Root, OutSelectedComponent);
	}

	ImGui::Separator();

	// Draw ActorComponent
	for (UActorComponent* ActorComp : SelectedActor->GetComponents())
	{
		if (!ActorComp || Cast<USceneComponent>(ActorComp))
		{
			continue;
		}

		ImGui::PushID(ActorComp);
		ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (ActorComp == OutSelectedComponent)
		{
			Flags |= ImGuiTreeNodeFlags_Selected;
		}
		
		FString Label = ActorComp->GetName();
		const ImVec2 RowStart = ImGui::GetCursorScreenPos();
		const float RowWidth = ImGui::GetContentRegionAvail().x;

		ImGui::TreeNodeEx("ActorComponentNode", Flags, "%s", Label.c_str());

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			OutSelectedComponent = ActorComp;
		}

		if (ImGui::BeginPopupContextItem("Component Context"))
		{
			if (ImGui::MenuItem("Delete", nullptr, false, ActorComp != Root))
				PendingDeleteComponent = ActorComp;
			ImGui::EndPopup();
		}

		const FString ClassName = std::format("({})", ActorComp->GetClass()->Name);
		const float TextX = RowStart.x + RowWidth - ImGui::CalcTextSize(ClassName.c_str()).x;
		ImGui::GetWindowDrawList()->AddText(ImVec2(TextX, RowStart.y), IM_COL32(200, 200, 200, 255), ClassName.c_str());		

		ImGui::PopID();
	}
}

void FDetailsPanel::DrawSceneComponentNode(USceneComponent* Component, USceneComponent* Root, UActorComponent*& OutSelectedComponent)
{
	// 鍮뚮낫??媛숈? ?먮뵒???쒖떆??而댄룷?뚰듃???몃━???몄텧?섏? ?딅뒗??
	if (!Component || Component->IsHiddenInDetails())
		return;

	// ?④꺼吏??먯떇留??덈뒗 ?몃뱶媛 ?쇱묠 ?붿궡?쒕? 媛뽰? ?딅룄濡??몄텧?섎뒗 ?먯떇留??쇰떎
	TArray<USceneComponent*> Children;
	for (USceneComponent* Child : Component->GetAttachChildren())
	{
		if (Child && !Child->IsHiddenInDetails())
		{
			Children.Add(Child);
		}
	}

	const bool bIsLeaf = Children.IsEmpty();
	bIsEditingName = (EditingNameComponent == Component);

	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
	if (Component == OutSelectedComponent)
	{
		Flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (bIsLeaf)
	{
		Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	FString Label = Component->GetName();
	if (Component == Root)
		Label += " (Root)";

	ImGui::PushID(Component);
	const ImVec2 RowStart = ImGui::GetCursorScreenPos();
	const float RowWidth = ImGui::GetContentRegionAvail().x;
	const bool bOpen = ImGui::TreeNodeEx("ComponentNode", Flags, "%s", bIsEditingName ? "" : Label.c_str());

	USceneComponent* DragComponent = Component;

	// source: Root ?쒖쇅, ?꾩옱 Component瑜?payload???댁쓬
	if (Component != Root && ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload(EditorDragDrop::Component, &Component, sizeof(USceneComponent*));
		ImGui::EndDragDropSource();
	}
	// target: 紐⑤뱺 ?됱씠 媛?? payload瑜?諛쏆븘 ?꾩옱 Component瑜?遺紐⑤줈 ?ъ슜
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(EditorDragDrop::Component))
		{
			USceneComponent* DroppedComponent = *static_cast<USceneComponent**>(Payload->Data);
			PendingDroppedComponent = DroppedComponent;
			PendingDragComponent = DragComponent;
		}
		ImGui::EndDragDropTarget();
	}

	if (!bIsEditingName)
	{
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			OutSelectedComponent = Component;

		if (Component != Root && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			EditingNameComponent = Component;
			strncpy_s(ComponentNameEditBuffer, sizeof(ComponentNameEditBuffer), Component->GetName().c_str(), _TRUNCATE);
			bFocusNameEdit = true;
		}

		if (ImGui::BeginPopupContextItem("Component Context"))
		{
			if (ImGui::MenuItem("Delete", nullptr, false, Component != Root))
				PendingDeleteComponent = Component;
			ImGui::EndPopup();
		}

		const FString ClassName = std::format("({})", Component->GetClass()->Name);
		const float TextX = RowStart.x + RowWidth - ImGui::CalcTextSize(ClassName.c_str()).x;
		ImGui::GetWindowDrawList()->AddText(ImVec2(TextX, RowStart.y), IM_COL32(200, 200, 200, 255), ClassName.c_str());
	}
	else
	{
		ImGui::SameLine();
		if (bFocusNameEdit)
		{
			ImGui::SetKeyboardFocusHere();
			bFocusNameEdit = false;
		}

		const bool bConfirmed = ImGui::InputText("Rename", ComponentNameEditBuffer,
			sizeof(ComponentNameEditBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
		const bool bCancelled = ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape);
		const bool bFinished = bConfirmed || ImGui::IsItemDeactivatedAfterEdit();
		if (bCancelled || bFinished)
		{
			if (!bCancelled && ComponentNameEditBuffer[0] != '\0')
			{
				FName NewName(ComponentNameEditBuffer);
				AActor* Owner = Component->GetOwner();

				UActorComponent* Existing = Owner->FindComponentByName(NewName);
				if (!Existing)
				{
					Component->SetName(FName(ComponentNameEditBuffer));
				}
				else
				{
					HTR_LOG(Warning, "A component with this name already exists in this Actor.");
				}
			}
			EditingNameComponent = nullptr;
		}
	}

	if (bOpen && !bIsLeaf)
	{
		for (USceneComponent* Child : Children)
		{
			DrawSceneComponentNode(Child, Root, OutSelectedComponent);
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
}

void FDetailsPanel::TryReparent(USceneComponent* DroppedComponent, USceneComponent* DragComponent)
{
	if (DroppedComponent != nullptr 
		&& DroppedComponent->GetAttachParent() != DragComponent
		&& DroppedComponent != DragComponent)
	{
		DroppedComponent->SetupAttachment(DragComponent, EAttachmentRule::KeepWorld);
	}
}


FDetailsPanel::~FDetailsPanel()
{
	//delete transform;
}
