// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NLPPCG : ModuleRules
{
	public NLPPCG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
			}
		);


		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"PCG",
				"HTTP",
				"Json",
				"JsonUtilities",
				"Slate",
				"SlateCore",
				"UMG",
				"Blutility",
				"UMGEditor"
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects"
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
