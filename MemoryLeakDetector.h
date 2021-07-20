// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MEMORYLEAKDETECTOR_H
#define PRIME_MEMORYLEAKDETECTOR_H

#if defined(_MSC_VER) && !defined(NDEBUG)

#include <crtdbg.h>

#define PRIME_CHECK_HEAP() _CrtCheckMemory()

namespace Prime {

/// Detects memory leaks that occur during the time it is created and the time it is destructed. Requires the
/// Visual C++ runtime library.
class MemoryLeakDetector {
public:
    /// Begin checking for memory leaks. If toStdout is true, any leaks that are found will be printed to
    /// stdout.
    MemoryLeakDetector(bool toStdout = true)
    {
        _CrtMemCheckpoint(&_state);

        if (toStdout) {
            // Output goes to stdout.
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
            _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
            _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
        } else {
            // Output goes to the debug console.
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
            _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
            _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
        }
    }

    ~MemoryLeakDetector()
    {
        _CrtCheckMemory();
        _CrtMemDumpAllObjectsSince(&_state);
    }

private:
    _CrtMemState _state;
};

}

#else

#define PRIME_CHECK_HEAP() ((void)0)

namespace Prime {

/// A no-op MemoryLeakDetector for other platforms.
class MemoryLeakDetector {
public:
    MemoryLeakDetector(bool = true) { } // the bool = true stops the compiler warning about an unused variable
};

}

#endif

#endif
