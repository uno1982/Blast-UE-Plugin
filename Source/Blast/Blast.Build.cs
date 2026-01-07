using System;
using System.IO;
using EpicGames.Core;

namespace UnrealBuildTool.Rules
{
    public class Blast : ModuleRules
    {
        public static string GetBlastLibraryFolderName(ModuleRules Rules)
        {
            if (Rules.Target.Configuration == UnrealTargetConfiguration.Debug)
            {
                return "debug";
            }
            else
            {
                return "release";
            }
        }

        public static void SetupModuleBlastSupport(ModuleRules Rules, string[] BlastLibs)
        {
            string LibFolderName = GetBlastLibraryFolderName(Rules);

            Rules.PublicDefinitions.Add(string.Format("BLAST_LIB_CONFIG_STRING=\"{0}\"", LibFolderName));

            const bool bUsePhysX = false;
            Rules.PublicDefinitions.Add(string.Format("BLAST_USE_PHYSX={0}", bUsePhysX ? 1 : 0));

            //Go up two from Source/Blast
            DirectoryReference ModuleRootFolder = (new DirectoryReference(Rules.ModuleDirectory)).ParentDirectory.ParentDirectory;
            
            string BLASTLibDir;
            if (Rules.Target.Platform == UnrealTargetPlatform.Android)
            {
                // Android requires absolute paths for static library linking
                BLASTLibDir = Path.Combine(ModuleRootFolder.FullName, "Libraries", "Android", LibFolderName);
            }
            else
            {
                DirectoryReference EngineDirectory = new DirectoryReference(Path.GetFullPath(Rules.Target.RelativeEnginePath));
                BLASTLibDir = Path.Combine("$(EngineDir)", ModuleRootFolder.MakeRelativeTo(EngineDirectory), "Libraries", Rules.Target.Platform.ToString(), LibFolderName);
            }

            string DLLSuffix = "";
            string DLLPrefix = "";
            string LibSuffix = "";

            // Libraries and DLLs for windows platform
            if (Rules.Target.Platform == UnrealTargetPlatform.Win64)
            {
                DLLSuffix = ".dll";
                LibSuffix = ".lib";
            }
            else if (Rules.Target.Platform == UnrealTargetPlatform.Linux)
            {
                DLLPrefix = "lib";
                DLLSuffix = ".so";
                LibSuffix = ".so";
            }
            else if (Rules.Target.Platform == UnrealTargetPlatform.Android)
            {
                // Android uses static libraries (.a)
                DLLPrefix = "lib";
                DLLSuffix = "";  // No DLL loading on Android
                LibSuffix = ".a";
            }

            Rules.PublicDefinitions.Add(string.Format("BLAST_LIB_DLL_SUFFIX=\"{0}\"", DLLSuffix));
            Rules.PublicDefinitions.Add(string.Format("BLAST_LIB_DLL_PREFIX=\"{0}\"", DLLPrefix));

            if (Rules.Target.Platform == UnrealTargetPlatform.Android)
            {
                // For Android, use SystemLibraryPaths + SystemLibraries so libraries are
                // placed inside the --start-group/--end-group block for proper linking
                Rules.PublicSystemLibraryPaths.Add(BLASTLibDir);
                foreach (string Lib in BlastLibs)
                {
                    // SystemLibraries expects library name without lib prefix and extension
                    // The toolchain will add "lib" prefix and search in SystemLibraryPaths
                    Rules.PublicSystemLibraries.Add(Lib);
                }
            }
            else
            {
                foreach (string Lib in BlastLibs)
                {
                    string LibPath = Path.Combine(BLASTLibDir, String.Format("{0}{1}{2}", DLLPrefix, Lib, LibSuffix));
                    Rules.PublicAdditionalLibraries.Add(LibPath);
                    var DllName = String.Format("{0}{1}{2}", DLLPrefix, Lib, DLLSuffix);
                    Rules.PublicDelayLoadDLLs.Add(DllName);
                    Rules.RuntimeDependencies.Add(Path.Combine(BLASTLibDir, DllName));
                }
            }

            if (bUsePhysX)
            {
            }
            else
            {
                Rules.PrivateDependencyModuleNames.Add("Chaos");
            }
            
            //It's useful to periodically turn this on since the order of appending files in unity build is random.
            //The use of types without the right header can creep in and cause random build failures

            //Rules.bFasterWithoutUnity = true;
        }

        public Blast(ReadOnlyTargetRules Target) : base(Target)
        {
            OptimizeCode = CodeOptimization.InNonDebugBuilds;

            // Android uses Cap'n Proto in LITE mode
            if (Target.Platform == UnrealTargetPlatform.Android)
            {
                PublicDefinitions.Add("CAPNP_LITE=1");
                // Fix for NvPreprocessor.h: NV_C_EXPORT is empty for Android (it only checks NV_LINUX, not NV_ANDROID)
                // This causes C++ name mangling on API functions. Define NV_C_EXPORT before including Blast headers.
                PublicDefinitions.Add("NV_C_EXPORT=extern \"C\"");
            }

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Projects",
                    "RenderCore",
                    "Renderer",
                    "RHI",
                    "BlastLoader",
                    "PhysicsCore"
                }
            );

            if (Target.bBuildEditor)
            {
                PrivateDependencyModuleNames.AddRange(
                  new string[]
                  {
                        "RawMesh",
                        "UnrealEd",
                        "BlastLoaderEditor"
                  }
                );
            }

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Engine"
                }
            );

            string[] BlastLibs;
            
            // Android: Include core Blast libraries + serialization with Cap'n Proto
            // Note: Stress extension skipped (needs SSE->NEON port)
            // Note: PhysX serialization DTOs excluded (core serialization works)
            if (Target.Platform == UnrealTargetPlatform.Android)
            {
                BlastLibs = new string[]
                {
                     "NvBlast",
                     "NvBlastGlobals",
                     "NvBlastExtSerialization",
                     "NvBlastExtShaders",
                     // Cap'n Proto libraries required for serialization
                     "capnp",
                     "kj",
                };
            }
            else
            {
                BlastLibs = new string[]
                {
                     "NvBlast",
                     "NvBlastGlobals",
                     "NvBlastExtSerialization",
                     "NvBlastExtShaders",
                     "NvBlastExtStress",
                };
            }

            PrivateIncludePaths.AddRange(
                new string[]
                {
	                Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include")),
	                Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include", "blast-sdk", "lowlevel")),
	                Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include", "blast-sdk", "globals")),
	                Path.GetFullPath(Path.Combine(PluginDirectory, "Libraries", "include", "blast-sdk", "shared", "NvFoundation"))
                }
            );

            SetupModuleBlastSupport(this, BlastLibs);

            SetupModulePhysicsSupport(Target);
        }
    }
}
