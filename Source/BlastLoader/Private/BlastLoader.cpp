#include "BlastLoader.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

FString GetBlastDLLPath(FString ConfigFolderName)
{
#if PLATFORM_ANDROID
// Android uses static linking, no DLL path needed
return FString();
#else
FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("Blast"))->GetBaseDir();
FString DllPath;
#if PLATFORM_WINDOWS && PLATFORM_64BITS
DllPath = FPaths::Combine(BaseDir, TEXT("Libraries/Win64/"));
#elif PLATFORM_WINDOWS
DllPath = FPaths::Combine(BaseDir, TEXT("Libraries/Win32/"));
#elif PLATFORM_LINUX
DllPath = FPaths::Combine(BaseDir, TEXT("Libraries/Linux/"));
#else
// Unsupported platform for DLL loading
return FString();
#endif
DllPath.Append(ConfigFolderName).Append("/");
return FPaths::ConvertRelativePathToFull(DllPath);
#endif
}

void* LoadBlastDLL(const FString& DLLPath, const TCHAR* BaseName)
{
#if PLATFORM_ANDROID
// Android uses static linking, libraries are linked at build time
return nullptr;
#else
FString FullPath = DLLPath / BaseName;
return FPlatformProcess::GetDllHandle(*FullPath);
#endif
}
