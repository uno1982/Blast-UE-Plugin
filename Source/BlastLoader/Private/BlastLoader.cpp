#include "BlastLoader.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

FString GetBlastDLLPath(FString ConfigFolderName)
{
	FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("Blast"))->GetBaseDir();
	FString DllPath;
#if PLATFORM_WINDOWS
	DllPath = FPaths::Combine(BaseDir, TEXT("Libraries/Win64/"));
#elif PLATFORM_LINUX
	DllPath = FPaths::Combine(BaseDir, TEXT("Libraries/Linux/"));
#elif PLATFORM_ANDROID
	// Android uses static libraries, no DLL path needed
	DllPath = TEXT("");
#else
#error No Blast libraries for this platform
#endif
	DllPath.Append(ConfigFolderName).Append("/");
	return FPaths::ConvertRelativePathToFull(DllPath);
}

void* LoadBlastDLL(const FString& DLLPath, const TCHAR* BaseName)
{
#if PLATFORM_ANDROID
	// Android uses static libraries - no runtime DLL loading needed
	return nullptr;
#else
	FString FullPath = DLLPath / BaseName;
	return FPlatformProcess::GetDllHandle(*FullPath);
#endif
}
