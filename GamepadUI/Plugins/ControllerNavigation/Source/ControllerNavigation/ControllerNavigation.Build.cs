// (C) Gaslight Games Ltd, 2017-2018.  All rights reserved.

using UnrealBuildTool;

public class ControllerNavigation : ModuleRules
{
	public ControllerNavigation( ReadOnlyTargetRules Target ) : base( Target )
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {
				//"ControllerNavigation/Public"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"ControllerNavigation/Private",
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "UMG",
                "InputCore",
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
