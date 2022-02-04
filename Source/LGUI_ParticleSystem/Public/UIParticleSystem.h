// Copyright 2021-present LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/ActorComponent/UIItem.h"
#include "Core/Actor/UIBaseActor.h"
#include "UIParticleSystem.generated.h"

class UNiagaraSystem;

UCLASS(ClassGroup = (LGUI), NotBlueprintable, meta = (BlueprintSpawnableComponent))
class LGUI_PARTICLESYSTEM_API UUIParticleSystem : public UUIItem
{
	GENERATED_BODY()

public:
	UUIParticleSystem(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay()override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)override;
private:
	TWeakObjectPtr<class ULGUIWorldParticleSystemComponent> ParticleSystemInstance = nullptr;
	void SetRenderEntries();
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	void OnPaintUpdate();

	TArray<struct FLGUINiagaraRendererEntry> RenderEntries;
	bool RenderEntriesValid = false;
	UPROPERTY(Transient)
		TArray<class UUIParticleSystemRendererItem*> UIParticleSystemRenderers;
	TSharedPtr<class SLGUIParticleSystemUpdateAgentWidget> UpdateAgentWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "LGUI")
		UNiagaraSystem* ParticleSystem;
	/** Auto activate particle system when create it in begin play. */
	UPROPERTY(EditAnywhere, Category = "LGUI", DisplayName = "Auto Activate")
		bool bAutoActivateParticleSystem = true;
	/** Particle color relate to this UI element's alpha. */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		bool bUseAlpha = true;
	/** Remap material for LGUI's normally render (without clip), if not assigned then use default material in particle system. */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		TMap<UMaterialInterface*, UMaterialInterface*> NormalMaterialMap;
	/** Remap material for LGUI's rect clip. If you don't use rect clip, just ignore this. */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		TMap<UMaterialInterface*, UMaterialInterface*> RectClipMaterialMap;
	/** Remap material for LGUI's texture clip. If you don't use texture clip, just ignore this. */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		TMap<UMaterialInterface*, UMaterialInterface*> TextureClipMaterialMap;
public:
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		class ULGUIWorldParticleSystemComponent* GetParticleSystemInstance()const { return ParticleSystemInstance.Get(); }
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		UNiagaraSystem* GetParticleSystemTemplate()const { return ParticleSystem; }
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		bool GetUseAlpha()const { return bUseAlpha; }
	UMaterialInterface* GetNormalMaterial(UMaterialInterface* keyMaterial);
	UMaterialInterface* GetRectClipMaterial(UMaterialInterface* keyMaterial);
	UMaterialInterface* GetTextureClipMaterialMap(UMaterialInterface* keyMaterial);

	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetUseAlpha(bool value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetParticleSystemTemplate(UNiagaraSystem* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void ActivateParticleSystem(bool Reset);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void DeactivateParticleSystem();
};


UCLASS()
class LGUI_PARTICLESYSTEM_API AUIParticleSystemActor : public AUIBaseActor
{
	GENERATED_BODY()

public:
	AUIParticleSystemActor();

	FORCEINLINE virtual UUIItem* GetUIItem()const override { return UIParticleSystem; }
	FORCEINLINE UUIParticleSystem* GetUIParticleSystem()const { return UIParticleSystem; }
private:
	UPROPERTY(Category = "LGUI", VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
		class UUIParticleSystem* UIParticleSystem;

};
