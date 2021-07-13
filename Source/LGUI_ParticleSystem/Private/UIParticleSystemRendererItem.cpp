// Copyright 2021 LexLiu. All Rights Reserved.

#include "UIParticleSystemRendererItem.h"
#include "Engine.h"
#include "Core/LGUIMesh/UIDrawcallMesh.h"
#include "Materials/MaterialInstanceDynamic.h"


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
	if (!IsValid(DynamicMaterial))
	{
		if (Material.IsValid())
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(Material.Get(), this);
			UIDrawcallMesh->SetMaterial(0, DynamicMaterial);
		}
	}
}
void UUIParticleSystemRendererItem::SetRectClipParameter(const FVector4& OffsetAndSize, const FVector4& Feather)
{
	if (IsValid(DynamicMaterial))
	{
		DynamicMaterial->SetVectorParameterValue(FName("RectClipOffsetAndSize"), FLinearColor(OffsetAndSize));
		DynamicMaterial->SetVectorParameterValue(FName("RectClipFeather"), FLinearColor(Feather));
	}
}
void UUIParticleSystemRendererItem::SetTextureClipParameter(UTexture* ClipTex, const FVector4& OffsetAndSize)
{
	if (IsValid(DynamicMaterial))
	{
		DynamicMaterial->SetTextureParameterValue(FName("ClipTexture"), ClipTex);
		DynamicMaterial->SetVectorParameterValue(FName("TextureClipOffsetAndSize"), FLinearColor(OffsetAndSize));
	}
}

AUIParticleSystemRendererItemActor::AUIParticleSystemRendererItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UIParticleSystemRendererItem = CreateDefaultSubobject<UUIParticleSystemRendererItem>(TEXT("UIParticleSystemRendererItem"));
	RootComponent = UIParticleSystemRendererItem;
}

#undef LOCTEXT_NAMESPACE
