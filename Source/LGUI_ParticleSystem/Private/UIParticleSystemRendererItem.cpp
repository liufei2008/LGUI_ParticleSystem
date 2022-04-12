// Copyright 2021-present LexLiu. All Rights Reserved.

#include "UIParticleSystemRendererItem.h"
#include "Engine.h"
#include "Core/LGUIMesh/LGUIMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UIParticleSystem.h"
#include "Core/UIDrawcall.h"


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

void UUIParticleSystemRendererItem::SetMaterial(UMaterialInterface* InMaterial)
{
	if (Material != InMaterial)
	{
		Material = InMaterial;
		if (RenderCanvas.IsValid())
		{
			if (drawcall.IsValid())
			{
				drawcall->bMaterialChanged = true;
			}
			MarkCanvasUpdate(true, false, false);
		}
	}
}

void UUIParticleSystemRendererItem::OnMeshDataReady()
{
	Super::OnMeshDataReady();
	if (drawcall->DrawcallMeshSection.IsValid() && Material.IsValid())
	{
		drawcall->DrawcallMesh->SetMeshSectionMaterial(drawcall->DrawcallMeshSection.Pin(), Material.Get());
	}
}

bool UUIParticleSystemRendererItem::HaveValidData()const
{
	return true;//because this UIParticleSystemRendererItem is created by UIParticleSystem, and UIParticleSystem will always check if have valid data
}

UMaterialInterface* UUIParticleSystemRendererItem::GetMaterial()const
{
	return Material.Get();
}

AUIParticleSystemRendererItemActor::AUIParticleSystemRendererItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UIParticleSystemRendererItem = CreateDefaultSubobject<UUIParticleSystemRendererItem>(TEXT("UIParticleSystemRendererItem"));
	RootComponent = UIParticleSystemRendererItem;
}

#undef LOCTEXT_NAMESPACE
