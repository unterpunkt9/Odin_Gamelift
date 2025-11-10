// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OdinFleet : ModuleRules
{
	public OdinFleet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		if (Target.Type == TargetType.Server)
		{
			PublicDependencyModuleNames.Add("GameLiftServerSDK");
		}
		else
		{
			PublicDefinitions.Add("WITH_GAMELIFT=0");
			PublicDependencyModuleNames.Add("GameLiftBackendService");
		}
		bEnableExceptions = true;

		PrivateDependencyModuleNames.AddRange(new string[] { "GameLiftBackendService" });

		PublicIncludePaths.AddRange(new string[] {
			"OdinFleet",
			"OdinFleet/Variant_Horror",
			"OdinFleet/Variant_Horror/UI",
			"OdinFleet/Variant_Shooter",
			"OdinFleet/Variant_Shooter/AI",
			"OdinFleet/Variant_Shooter/UI",
			"OdinFleet/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
