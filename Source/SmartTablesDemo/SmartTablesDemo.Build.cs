using UnrealBuildTool;

public class SmartTablesDemo : ModuleRules
{
	public SmartTablesDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"Slate",
			"SlateCore",
			"SmartTables",

			"Json",

			"FieldNotification",
			"ModelViewViewModel",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{

			"InputCore",

			"EnhancedInput",

			"RenderCore",
		});
	}
}
