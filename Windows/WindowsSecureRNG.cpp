// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsSecureRNG.h"

#ifdef PRIME_HAVE_WINDOWSSECURERNG

#include "WindowsConfig.h"

#if defined(_MSC_VER)
#pragma comment(lib, "crypt32.lib")
#endif

#include <wincrypt.h>

namespace Prime {

WindowsSecureRNG::WindowsSecureRNG()
{
}

WindowsSecureRNG::~WindowsSecureRNG()
{
    close();
}

bool WindowsSecureRNG::init(Log* log)
{
    (void)log;

    return true;
}

void WindowsSecureRNG::close()
{
}

bool WindowsSecureRNG::generateBytes(void* buffer, size_t bufferSize, Log* log)
{
    if (!isInitialised()) {
        if (!init(log)) {
            return false;
        }
    }

    HCRYPTPROV context = NULL;
    if (CryptAcquireContext(&context, NULL, NULL, PROV_RSA_FULL, 0) && context) {

        PRIME_ASSERT((DWORD)bufferSize == bufferSize);
        BOOL ok = CryptGenRandom(context, (DWORD)bufferSize, (BYTE*)buffer);

        CryptReleaseContext(context, 0);

        if (ok) {
            return true;

        } else {
            log->error(PRIME_LOCALISE("Error during CryptGenRandom."));
        }

    } else {
        log->error(PRIME_LOCALISE("Error during CryptAcquireContext."));
    }

    return false;
}

WindowsSecureRNG::Result WindowsSecureRNG::generate()
{
    Result r;
    if (!generateBytes(&r, sizeof(r), Log::getGlobal())) {
        r = 0;
    }

    return r;
}
}

#endif // PRIME_HAVE_WINDOWSSECURERNG
