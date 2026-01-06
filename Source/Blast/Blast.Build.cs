using System;
using System.IO;
using Tools.DotNETCommon;

namespace UnrealBuildTool.Rules
{
    public class Blast : ModuleRules
    {
        public static string GetBlastLibraryFolderName(ModuleRules Rules)
        {
            switch (Rules.Target.Configuration)
            {
                case UnrealTargetConfiguration.Debug:
                    if (Rules.Target.Configuration == UnrealTargetConfiguration.Debug)
                    {
                        return "debug";
                    }
                    else
                    {
                        return "checked";
                    }
                case UnrealTargetConfiguration.Shipping:
                    return "release";
                case UnrealTargetConfiguration.Test:
                    return "profile";
                case UnrealTargetConfiguration.Development:
                case UnrealTargetConfiguration.DebugGame:
                case UnrealTargetConfiguration.Unknown:
                default:
                    if (Rules.Target.bUseShippingPhysXLibraries)
                    {
                        return "release";
                    }
                    else if (Rules.Target.bUseCheckedPhysXLibraries)
                    {
                        return "checked";
                    }
                    else
                    {
                        return "profile";
                    }
            }
        }

        public static void SetupModuleBlastSupport(ModuleRules Rules, string[] BlastLibs,
 string[] AndroidBlastLibs = null)
        {
            string LibFolderName = GetBlastLibraryFolderName(Rules);
            bool bIsAndroid = Rules.Target.Platform == UnrealTargetPlatform.Android;
            
            // Only set BLAST_DISABLE_STRESS_SOLVER once here
            if (bIsAndroid)
            {
                Rules.PublicDefinitions.Add("BLAST_DISABLE_STRESS_SOLVER=1");
            }
            else
            {
                Rules.PublicDefinitions.Add("BLAST_DISABLE_STRESS_SOLVER=0");
            }
            
            Rules.PublicDefinitions.Add(string.Format("BLAST_LIB_CONFIG_STRING=\"{0}\"",
LibFolderName));

            // Go up two from Source/Blast to get plugin root
            DirectoryReference ModuleRootFolder = (new DirectoryReference(Rules.ModuleDirectory)).ParentDirectory.ParentDirectory;
            string DLLSuffix = "";
            string DLLPrefix = "";
            string LibSuffix = "";
            string BLASTLibDir;

            // Use absolute path for all platforms - works in both Engine/Plugins and Project/Plugins
            if (Rules.Target.Platform == UnrealTargetPlatform.Win64)
            {
                DLLSuffix = "_x64.dll";
                LibSuffix = "_x64.lib";
                BLASTLibDir = Path.Combine(ModuleRootFolder.FullName, "Libraries", "Win64", LibFolderName);
            }
            else if (Rules.Target.Platform == UnrealTargetPlatform.Win32)
            {
                DLLSuffix = "_x86.dll";
                LibSuffix = "_x86.lib";
                BLASTLibDir = Path.Combine(ModuleRootFolder.FullName, "Libraries", "Win32", LibFolderName);
            }
            else if (Rules.Target.Platform == UnrealTargetPlatform.Linux)
            {
                DLLPrefix = "lib";
                DLLSuffix = ".so";
                LibSuffix = ".so";
                BLASTLibDir = Path.Combine(ModuleRootFolder.FullName, "Libraries", "Linux", LibFolderName);
            }
            else if (bIsAndroid)
            {
                DLLPrefix = "lib";
                DLLSuffix = ".a";
                LibSuffix = ".a";
                BLASTLibDir = Path.Combine(ModuleRootFolder.FullName, "Libraries", "Android", LibFolderName);
            }
            else
            {
                BLASTLibDir = Path.Combine(ModuleRootFolder.FullName, "Libraries", Rules.Target.Platform.ToString(), LibFolderName);
            }

            Rules.PublicDefinitions.Add(string.Format("BLAST_LIB_DLL_SUFFIX=\"{0}\"", DLLSuffix));
            Rules.PublicDefinitions.Add(string.Format("BLAST_LIB_DLL_PREFIX=\"{0}\"", DLLPrefix));

            // Use Android-specific libs if provided and on Android platform
            string[] LibsToUse = (bIsAndroid && AndroidBlastLibs != null) ? AndroidBlastLibs : BlastLibs;

            foreach (string Lib in LibsToUse)
            {
                string LibPath = Path.Combine(BLASTLibDir, String.Format("{0}{1}{2}", DLLPrefix, Lib, LibSuffix));
                Rules.PublicAdditionalLibraries.Add(LibPath);

                if (!bIsAndroid) // Android uses static libs, no DLLs to load
                {
                    var DllName = String.Format("{0}{1}{2}", DLLPrefix, Lib, DLLSuffix);
                    Rules.PublicDelayLoadDLLs.Add(DllName);
                    Rules.RuntimeDependencies.Add(Path.Combine(BLASTLibDir, DllName));
                }
            }
        }

        public Blast(ReadOnlyTargetRules Target) : base(Target)
        {
            OptimizeCode = CodeOptimization.InNonDebugBuilds;

            PublicIncludePaths.AddRange(
                new string[] {
                    Path.GetFullPath(Path.Combine(ModuleDirectory, "Public/extensions/serialization/include/")),
                    Path.GetFullPath(Path.Combine(ModuleDirectory, "Public/extensions/shaders/include/")),
                    Path.GetFullPath(Path.Combine(ModuleDirectory, "Public/extensions/stress/include/")),
                    Path.GetFullPath(Path.Combine(ModuleDirectory, "Public/globals/include/")),
                    Path.GetFullPath(Path.Combine(ModuleDirectory, "Public/lowlevel/include/")),
                }
            );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Projects",
                    "RenderCore",
                    "Renderer",
                    "RHI",
                    "BlastLoader"
                }
            );

            if (Target.bBuildEditor)
            {
                PrivateDependencyModuleNames.AddRange(
                  new string[]
                  {
                        "RawMesh",
                        "UnrealEd",
                        "BlastLoaderEditor",
                  }
                );
            }

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Engine",
                    "PhysX",
                }
            );

            // Windows/Linux libs (includes stress solver)
            string[] BlastLibs =
            {
                 "NvBlast",
                 "NvBlastGlobals",
                 "NvBlastExtSerialization",
                 "NvBlastExtShaders",
                 "NvBlastExtStress",
            };

            // Android libs (stress solver uses SSE intrinsics, not available on ARM)
            string[] AndroidBlastLibs =
            {
                 "NvBlast",
                 "NvBlastGlobals",
                 "NvBlastExtSerialization",
                 "NvBlastExtShaders",
            };

            PrivateIncludePaths.AddRange(
                new string[]
                {
                    "Blast/Public/extensions/assetutils/include",
                    "Blast/Public/extensions/authoring/include",
                    "Blast/Public/extensions/authoringCommon/include",
                    "Blast/Public/extensions/serialization/include",
                    "Blast/Public/extensions/shaders/include",
                    "Blast/Public/extensions/stress/include",
                    "Blast/Public/globals/include",
                    "Blast/Public/lowlevel/include"
                }
            );

            SetupModuleBlastSupport(this, BlastLibs, AndroidBlastLibs);

            SetupModulePhysicsSupport(Target);
        }
    }
}
