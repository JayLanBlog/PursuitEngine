#include "initializer.h"
#include "Engine/pfengine.h"
#include <thread>
#include <atomic>

#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX)
#include "Utility/cpuinfo.hpp"
#endif // defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX)

namespace pf::initializer {


	// Initializes systems and blocks CPU until it is complete
	void InitializeComponentsImmediate() {
	
	}

	// Begins initializing systems, but doesn't block CPU. Check completion status with IsInitializeFinished()
	void InitializeComponentsAsync() {
	
	}

	// Check if systems have been initialized or not
	//	system : specify to check a specific system, or leave default to check all systems
	bool IsInitializeFinished(INITIALIZED_SYSTEM system) {

	
		return true;
	}
	// Wait for all system initializations to finish
	void WaitForInitializationsToFinish() {
	
	}
}