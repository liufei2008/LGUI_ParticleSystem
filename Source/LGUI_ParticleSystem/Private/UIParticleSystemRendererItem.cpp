// Copyright 2021 LexLiu. All Rights Reserved.

#include "UIParticleSystemRendererItem.h"
#include "Engine.h"
#include "Core/LGUIMesh/UIDrawcallMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UIParticleSystem.h"


#define LOCTEXT_NAMESPACE "UIParticleSystemRendererItem"

UUIParticleSystemRendererItem::UUIParticleSystemRendererItem(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UUIParticleSystemRendererItem::BeginPlay()
{
	Super::BeginPlay();
}
void UUIParticleSystemRendererItem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


#if WITH_EDITOR
void UUIParticleSystemRendererItem::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UUIParticleSystemRendererItem::ApplyUIActiveState()
{
	if (UIDrawcallMesh.IsValid())
	{
		UIDrawcallMesh->SetUIMeshVisibility(this->IsUIActiveInHierarchy());
	}
	Super::ApplyUIActiveState();
}
void UUIParticleSystemRendererItem::SetClipType(ELGUICanvasClipType clipType)
{
	switch (clipType)
	{
	case ELGUICanvasClipType::None:
	{
		bool materialSet = false;
		if (Manager.IsValid())
		{
			if (auto NormalOriginMaterial = Manager->GetNormalMaterial(Material.Get()))
			{
				UIDrawcallMesh->SetMaterial(0, NormalOriginMaterial);
				materialSet = true;
			}
		}
		if (!materialSet)
		{
			UIDrawcallMesh->SetMaterial(0, Material.Get());
		}
	}
	break;
	case ELGUICanvasClipType::Rect:
	{
		if (!IsValid(RectClipDynamicMaterial) && Manager.IsValid())
		{
			if (auto RectClipOriginMaterial = Manager->GetRectClipMaterial(Material.Get()))
			{
				auto CreatedMat = UMaterialInstanceDynamic::Create(RectClipOriginMaterial, this);
				UIDrawcallMesh->SetMaterial(0, CreatedMat);
				RectClipDynamicMaterial = CreatedMat;
			}
		}
	}
	break;
	case ELGUICanvasClipType::Texture:
	{
		if (!IsValid(TextureClipDynamicMaterial) && Manager.IsValid())
		{
			if (auto TextureClipOriginMaterial = Manager->GetRectClipMaterial(Material.Get()))
			{
				auto CreatedMat = UMaterialInstanceDynamic::Create(TextureClipOriginMaterial, this);
				UIDrawcallMesh->SetMaterial(0, CreatedMat);
				TextureClipDynamicMaterial = CreatedMat;
			}
		}
	}
	break;
	}
}
void UUIParticleSystemRendererItem::SetRectClipParameter(const FVector4& OffsetAndSize, const FVector4& Feather)
{
	if (IsValid(RectClipDynamicMaterial))
	{
		RectClipDynamicMaterial->SetVectorParameterValue(FName("RectClipOffsetAndSize"), FLinearColor(OffsetAndSize));
		RectClipDynamicMaterial->SetVectorParameterValue(FName("RectClipFeather"), FLinearColor(Feather));
	}
}
void UUIParticleSystemRendererItem::SetTextureClipParameter(UTexture* ClipTex, const FVector4& OffsetAndSize)
{
	if (IsValid(TextureClipDynamicMaterial))
	{
		TextureClipDynamicMaterial->SetTextureParameterValue(FName("ClipTexture"), ClipTex);
		TextureClipDynamicMaterial->SetVectorParameterValue(FName("TextureClipOffsetAndSize"), FLinearColor(OffsetAndSize));
	}
}

AUIParticleSystemRendererItemActor::AUIParticleSystemRendererItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UIParticleSystemRendererItem = CreateDefaultSubobject<UUIParticleSystemRendererItem>(TEXT("UIParticleSystemRendererItem"));
	RootComponent = UIParticleSystemRendererItem;
}

#undef LOCTEXT_NAMESPACE
