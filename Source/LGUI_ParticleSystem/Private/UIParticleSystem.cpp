// Copyright 2021 LexLiu. All Rights Reserved.

#include "UIParticleSystem.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Engine.h"
#include "LGUIWorldParticleSystemComponent.h"
#include "UIParticleSystemRendererItem.h"
#include "Core/LGUIMesh/UIDrawcallMesh.h"
#include "SLGUIParticleSystemUpdateAgentWidget.h"

#define LOCTEXT_NAMESPACE "UIParticleSystem"

UUIParticleSystem::UUIParticleSystem(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UUIParticleSystem::BeginPlay()
{
	Super::BeginPlay();

	if (!UpdateAgentWidget.IsValid())
	{
		UpdateAgentWidget = SNew(SLGUIParticleSystemUpdateAgentWidget);
		GEngine->GameViewport->AddViewportWidgetContent(UpdateAgentWidget.ToSharedRef());
		UpdateAgentWidget->OnPaintCallbackDelegate.BindUObject(this, &UUIParticleSystem::OnPaintUpdate);
	}
	if (IsValid(ParticleSystemTemplate))
	{
		CheckParticleSystem();
	}
}
/**
 * Ϊʲô�ֿ���������(Tick�и�������Slate��OnPaint�ж�ȡ��������)��
 * ������ŵ�Tick���ôRibbon��ȡ�������Ǵ��ҵģ�������ŵ�OnPaint���ôSprite��UIMesh�����ڴ�������������ԭ��
 * ���������ڶ��߳���ɵģ�OnPaint��ParticleSystem�е�Tick��Ĭ�ϵ�Tick������һ���߳��
 */ 
void UUIParticleSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Update mesh in tick
	if (IsUIActiveInHierarchy())
	{
		if (WorldParticleComponent.IsValid())
		{
			for (int i = 0; i < RenderEntries.Num(); i++)
			{
				auto UIDrawcallMesh = UIParticleSystemRenderers[i]->GetUIDrawcallMesh();
				if (UIDrawcallMesh.IsValid())
				{
					UIDrawcallMesh->GenerateOrUpdateMesh(false, 1);
				}
			}
		}
	}
}
void UUIParticleSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	WorldParticleComponent.Reset();
}
DECLARE_CYCLE_STAT(TEXT("UIParticleSystem RenderToUI"), STAT_UIParticleSystem, STATGROUP_LGUI);
void UUIParticleSystem::OnPaintUpdate()
{
	//Read particle data in slate OnPaint
	if (IsUIActiveInHierarchy())
	{
		if (WorldParticleComponent.IsValid())
		{
			SCOPE_CYCLE_COUNTER(STAT_UIParticleSystem);
			auto rootUIItem = this->GetRootCanvas()->GetUIItem();
			auto rootSpaceLocation = rootUIItem->GetComponentTransform().InverseTransformPosition(this->GetComponentLocation());
			auto rootSpaceLocation2D = FVector2D(rootSpaceLocation.X, rootSpaceLocation.Y);
			WorldParticleComponent->SetTransformationForUIRendering(rootSpaceLocation2D, FVector2D(this->GetRelativeScale3D()), this->GetRelativeRotation().Yaw);
			//auto layoutScale = this->GetRootCanvas()->GetCanvasScale();
			auto layoutScale = 1.0f;
			//auto locationOffset = FVector2D(-rootUIItem->GetWidth() * 0.5f, -rootUIItem->GetHeight() * 0.5f);
			auto locationOffset = FVector2D::ZeroVector;
			for (int i = 0; i < RenderEntries.Num(); i++)
			{
				auto UIDrawcallMesh = UIParticleSystemRenderers[i]->GetUIDrawcallMesh();
				if (UIDrawcallMesh.IsValid())
				{
					WorldParticleComponent->RenderUI(UIDrawcallMesh.Get(), RenderEntries[i], layoutScale, locationOffset);
				}
			}
		}
	}
}
void UUIParticleSystem::CheckParticleSystem()
{
	if (!WorldParticleComponent.IsValid())
	{
		if (GetOuter())
		{
			UWorld* World = this->GetWorld();
			if (World)
			{
				auto WorldParticleSystemActor = World->SpawnActor<ALGUIParticleSystemActor>();
#if WITH_EDITOR
				WorldParticleSystemActor->SetActorLabel(FString(TEXT("LGUIPaticleSystem")));
#endif
				WorldParticleComponent = WorldParticleSystemActor->Emit(ParticleSystemTemplate);
				WorldParticleComponent->Activate();
				RenderEntries = WorldParticleComponent->GetRenderEntries();
				for (int i = 0; i < RenderEntries.Num(); i++)
				{
					auto ParticleSytemRendererItemActor = World->SpawnActor<AUIParticleSystemRendererItemActor>();
					auto RendererItem = ParticleSytemRendererItemActor->GetUIParticleSystemRendererItem();
					RendererItem->AttachToComponent(this->GetRootCanvas()->GetUIItem(), FAttachmentTransformRules::KeepRelativeTransform);
					RendererItem->SetWidth(0);
					RendererItem->SetHeight(0);
					RendererItem->SetMaterial(RenderEntries[i].Material);
					RendererItem->SetDepth(this->GetDepth() + i);
					UIParticleSystemRenderers.Add(RendererItem);
				}
			}
		}
	}
}

#if WITH_EDITOR
void UUIParticleSystem::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif



AUIParticleSystemActor::AUIParticleSystemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UIParticleSystem = CreateDefaultSubobject<UUIParticleSystem>(TEXT("UIParticleSystem"));
	RootComponent = UIParticleSystem;
}

#undef LOCTEXT_NAMESPACE
