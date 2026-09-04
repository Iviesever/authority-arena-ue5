using UnrealBuildTool;
using System.Collections.Generic;

public class AuthorityArenaServerTarget : TargetRules
{
    public AuthorityArenaServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new[] { "AuthorityArenaCore", "AuthorityArena" });
    }
}
