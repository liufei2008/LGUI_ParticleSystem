// Copyright 2021 LexLiu. All Rights Reserved.

#include "UIParticleSystemRendererItem.h"
#include "Engine.h"


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

AUIParticleSystemRendererItemActor::AUIParticleSystemRendererItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UIParticleSystemRendererItem = CreateDefaultSubobject<UUIParticleSystemRendererItem>(TEXT("UIParticleSystemRendererItem"));
	RootComponent = UIParticleSystemRendererItem;
}

#undef LOCTEXT_NAMESPACE
