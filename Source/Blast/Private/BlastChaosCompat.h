// UE 5.1 Compatibility header for Chaos physics scene locking
// This provides compatibility between UE 5.5's FScopedSceneLock_Chaos and UE 5.1's APIs

#pragma once

#include "CoreMinimal.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PBDRigidsSolver.h"

// UE 5.1 doesn't have EPhysicsInterfaceScopedLockType - define a compatibility enum
enum class EPhysicsInterfaceScopedLockType : uint8
{
	Read,
	Write
};

// UE 5.1 compatibility wrapper for FScopedSceneLock_Chaos
// In UE 5.5, this class takes a scene and lock type
// In UE 5.1, we implement locking directly using the solver's external data lock
struct FScopedSceneLock_Chaos
{
	FScopedSceneLock_Chaos() : Solver(nullptr) {}
	
	FScopedSceneLock_Chaos(FPhysScene* InScene, EPhysicsInterfaceScopedLockType LockType)
		: Solver(nullptr)
	{
		if (InScene)
		{
			FPhysScene_Chaos* ChaosScene = static_cast<FPhysScene_Chaos*>(InScene);
			if (ChaosScene)
			{
				Solver = ChaosScene->GetSolver();
				if (Solver)
				{
					if (LockType == EPhysicsInterfaceScopedLockType::Write)
					{
						Solver->GetExternalDataLock_External().WriteLock();
						bIsWriteLock = true;
					}
					else
					{
						Solver->GetExternalDataLock_External().ReadLock();
						bIsWriteLock = false;
					}
				}
			}
		}
	}
	
	FScopedSceneLock_Chaos(FScopedSceneLock_Chaos&& Other)
		: Solver(Other.Solver), bIsWriteLock(Other.bIsWriteLock)
	{
		Other.Solver = nullptr;
	}
	
	FScopedSceneLock_Chaos& operator=(FScopedSceneLock_Chaos&& Other)
	{
		if (this != &Other)
		{
			Release();
			Solver = Other.Solver;
			bIsWriteLock = Other.bIsWriteLock;
			Other.Solver = nullptr;
		}
		return *this;
	}
	
	~FScopedSceneLock_Chaos()
	{
		Release();
	}
	
	// Release the lock early
	void Release()
	{
		if (Solver)
		{
			if (bIsWriteLock)
			{
				Solver->GetExternalDataLock_External().WriteUnlock();
			}
			else
			{
				Solver->GetExternalDataLock_External().ReadUnlock();
			}
			Solver = nullptr;
		}
	}
	
	// Non-copyable
	FScopedSceneLock_Chaos(const FScopedSceneLock_Chaos&) = delete;
	FScopedSceneLock_Chaos& operator=(const FScopedSceneLock_Chaos&) = delete;

private:
	Chaos::FPBDRigidsSolver* Solver;
	bool bIsWriteLock = false;
};
