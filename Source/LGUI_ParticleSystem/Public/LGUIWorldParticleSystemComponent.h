// Copyright 2021-present LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "LGUIWorldParticleSystemComponent.generated.h"

#if ENGINE_MAJOR_VERSION >= 5
typedef FVector3f MyVector3;
typedef FVector2f MyVector2;
typedef FVector4f MyVector4;
#else
typedef FVector MyVector3;
typedef FVector2D MyVector2;
typedef FVector4 MyVector4;
#endif

struct FLGUIMeshSection;
class FNiagaraEmitterInstance;
class UNiagaraSpriteRendererProperties;
class UNiagaraRibbonRendererProperties;

struct FLGUINiagaraRendererEntry
{
	FLGUINiagaraRendererEntry(UNiagaraRendererProperties* PropertiesIn, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInstIn, UNiagaraEmitter* EmitterIn, UMaterialInterface* MaterialIn)
		: RendererProperties(PropertiesIn), EmitterInstance(EmitterInstIn), Emitter(EmitterIn), Material(MaterialIn) {}

	UNiagaraRendererProperties* RendererProperties;
	TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInstance;
	UNiagaraEmitter* Emitter;
	UMaterialInterface* Material;
};

UCLASS()
class LGUI_PARTICLESYSTEM_API ULGUIWorldParticleSystemComponent : public UNiagaraComponent
{
    GENERATED_BODY()
public:
	TArray<FLGUINiagaraRendererEntry> GetRenderEntries();

    void SetTransformationForUIRendering(MyVector2 Location, MyVector2 Scale, float Angle);

	void RenderUI(FLGUIMeshSection* UIMeshSection, FLGUINiagaraRendererEntry RendererEntry, float ScaleFactor, MyVector2 LocationOffset, float Alpha01, const int ParticleCountIncreaseAndDecrease);
private:
    void AddSpriteRendererData(FLGUIMeshSection* UIMeshSection
		, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
		, UNiagaraSpriteRendererProperties* SpriteRenderer
		, float ScaleFactor, MyVector2 LocationOffset, float Alpha01
		, const int ParticleCountIncreaseAndDecrease
	);
    void AddRibbonRendererData(FLGUIMeshSection* UIMeshSection
		, TSharedRef<const FNiagaraEmitterInstance, ESPMode::ThreadSafe> EmitterInst
		, UNiagaraRibbonRendererProperties* RibbonRenderer
		, float ScaleFactor, MyVector2 LocationOffset, float Alpha01
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
	UPROPERTY(VisibleAnywhere, Transient)
		ULGUIWorldParticleSystemComponent* Niagara;
};
