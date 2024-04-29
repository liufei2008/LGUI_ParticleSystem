// Copyright 2021-present LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "LGUIWorldParticleSystemComponent.generated.h"

struct FLGUIMeshSection;
class FNiagaraEmitterInstance;
class UNiagaraRendererProperties;
class UNiagaraSpriteRendererProperties;
class UNiagaraRibbonRendererProperties;

struct FLGUINiagaraRendererEntry
{
	FLGUINiagaraRendererEntry(UNiagaraRendererProperties* PropertiesIn, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInstIn, UMaterialInterface* MaterialIn)
		: RendererProperties(PropertiesIn), EmitterInstance(EmitterInstIn), Material(MaterialIn) {}

	UNiagaraRendererProperties* RendererProperties;
	TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInstance;
	UMaterialInterface* Material;
};

UCLASS()
class LGUI_PARTICLESYSTEM_API ULGUIWorldParticleSystemComponent : public UNiagaraComponent
{
    GENERATED_BODY()
public:
	void GetRenderEntries(TArray<FLGUINiagaraRendererEntry>& Renderers);

    void SetTransformationForUIRendering(FVector2f Location, FVector2f Scale, float Angle);

	void RenderUI(FLGUIMeshSection* UIMeshSection, FLGUINiagaraRendererEntry RendererEntry, float ScaleFactor, FVector2f LocationOffset, float Alpha01, const int ParticleCountIncreaseAndDecrease);
private:
    void AddSpriteRendererData(FLGUIMeshSection* UIMeshSection
		, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
		, UNiagaraSpriteRendererProperties* SpriteRenderer
		, float ScaleFactor, FVector2f LocationOffset, float Alpha01
		, const int ParticleCountIncreaseAndDecrease
	);
    void AddRibbonRendererData(FLGUIMeshSection* UIMeshSection
		, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
		, UNiagaraRibbonRendererProperties* RibbonRenderer
		, float ScaleFactor, FVector2f LocationOffset, float Alpha01
		, const int ParticleCountIncreaseAndDecrease
	);
};

UCLASS(ClassGroup = LGUI, NotPlaceable)
class LGUI_PARTICLESYSTEM_API ALGUIWorldParticleSystemActor : public AActor
{
	GENERATED_BODY()

public:
	ALGUIWorldParticleSystemActor();

	ULGUIWorldParticleSystemComponent* Emit(UNiagaraSystem* NiagaraSystemTemplate, bool AutoActivate);
	UPROPERTY(VisibleAnywhere, Transient, Category = "LGUI")
		ULGUIWorldParticleSystemComponent* Niagara;
};
