using UnrealBuildTool;

public class AuthorityArenaCore : ModuleRules
{
    public AuthorityArenaCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.Add("Core");
    }
}
