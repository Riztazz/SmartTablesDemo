using UnrealBuildTool;
using System.Collections.Generic;

public class SmartTablesDemoTarget : TargetRules
{
	public SmartTablesDemoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[] { "SmartTablesDemoGame", "SmartTablesDemo" });
	}
}
