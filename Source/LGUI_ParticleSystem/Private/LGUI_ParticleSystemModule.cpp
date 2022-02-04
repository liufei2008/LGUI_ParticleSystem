// Copyright 2021-present LexLiu. All Rights Reserved.

#include "LGUI_ParticleSystemModule.h"

#define LOCTEXT_NAMESPACE "UIParticleSystemModule"

void FLGUI_ParticleSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FLGUI_ParticleSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLGUI_ParticleSystemModule, LGUI_ParticleSystem)