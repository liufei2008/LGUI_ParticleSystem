// Copyright 2021-present LexLiu. All Rights Reserved.

using UnrealBuildTool;

public class LGUI_ParticleSystem : ModuleRules
{
	public LGUI_ParticleSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		
		{
			PublicIncludePaths.AddRange(
				new string[]
                {
					// ... add public include paths required here ...
				}
			);


			PrivateIncludePaths.AddRange(
				new string[]
				{
					// ... add other private include paths required here ...
				}
			);


			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"UMG",
					// ... add other public dependencies that you statically link with here ...
				}
			);


			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
					"Engine",
					"Slate",
					"SlateCore",
					"LGUI",
					"Niagara",
					// "RenderCore",
					// ... add private dependencies that you statically link with here ...	
				}
            );


			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
			);
		}
	}
}
