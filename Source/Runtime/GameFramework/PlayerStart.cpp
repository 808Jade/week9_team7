#include "EnginePCH.h"
#include "PlayerStart.h"
#include "Asset/AssetManager.h"

APlayerStart::APlayerStart()
{
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("PlayerStartSprite");
	BillboardComponent->SetupAttachment(GetRootComponent());
    BillboardComponent->SetEditorIcon(true);    
    BillboardComponent->SetHiddenInGame(true);  
    BillboardComponent->SetIconSize(1.5f);      
    BillboardComponent->bCastShadow = false;   
    BillboardComponent->bReceiveShadow = false;
    UMaterial* BaseMat = UAssetManager::GetAssetByPath<UMaterial>("BillboardMaterial");
    UTexture2D* IconTex = UAssetManager::Get().LoadTexture("Assets/Icons/S_Player.png");
    if (BaseMat && IconTex)
    {
        UMaterial* IconMat = UMaterial::CreateInstance(BaseMat);
        IconMat->Textures[0] = IconTex;
        BillboardComponent->SetMaterial(0, IconMat);
    }
}
