// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_IOSDEVICE_H
#define PRIME_OSX_IOSDEVICE_H

#include "../Config.h"
#include "../StringUtils.h"
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>
#include <sys/types.h>

namespace Prime {

/// Detect the iOS device/simulator the process is running on.
/// e.g., "iPhone3,1" for iPhone 4 or "iPad2,1" for iPad 2.
class iOSDevice {
public:
    iOSDevice()
    {
        _device[0] = 0;
        _major = 0;
        _minor = 0;
    }

    bool detect()
    {
        _device[0] = 0;
        _major = _minor = 0;

        size_t size;
        sysctlbyname("hw.machine", 0, &size, 0, 0);

        char buffer[128];
        if (size > sizeof(buffer) - 1) {
            return false;
        }

        sysctlbyname("hw.machine", buffer, &size, 0, 0);

        char* ptr = buffer;

        while (*ptr && !ASCIIIsDigit(*ptr)) {
            ++ptr;
        }

        StringCopy(_device, buffer, (size_t)(ptr - buffer));

        char* endPtr;
        _major = (int)strtol(ptr, &endPtr, 10);
        if (endPtr == ptr) {
            _major = 0;
            return true;
        }

        ptr = endPtr;
        if (*ptr != ',') {
            return true;
        }

        ++ptr;

        _minor = (int)strtol(ptr, &endPtr, 10);
        if (endPtr == ptr) {
            _minor = 0;
            return true;
        }

        return true;
    }

    /// "iPhone", "iPad" or "iPod"
    const char* getDeviceType() const { return _device; }

    bool isiPhone() const { return PRIME_GUARD(_device[0]) && StringsEqual(_device, "iPhone"); }
    bool isiPad() const { return PRIME_GUARD(_device[0]) && StringsEqual(_device, "iPad"); }
    bool isiPod() const { return PRIME_GUARD(_device[0]) && StringsEqual(_device, "iPod"); }

    int getMajor() const { return _major; }
    int getMinor() const { return _minor; }

private:
    char _device[128];
    int _major;
    int _minor;
};

}

#endif
