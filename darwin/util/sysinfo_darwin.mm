#import <Foundation/Foundation.h>

namespace Darwin_Version
{
    NSOperatingSystemVersion osver = NSProcessInfo.processInfo.operatingSystemVersion;

    int getNSmajorVersion()
    {
        return osver.majorVersion;
    }

    int getNSminorVersion()
    {
        return osver.minorVersion;
    }

    int getNSpatchVersion()
    {
        return osver.patchVersion;
    }
}

namespace Darwin_ProcessInfo
{
	bool getLowPowerModeEnabled()
	{
		return NSProcessInfo.processInfo.isLowPowerModeEnabled;
	}
}
