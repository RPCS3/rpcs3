#ifdef __APPLE__
#import <Foundation/Foundation.h>

NSOperatingSystemVersion osver;

void fetchNSVer()
{
    osver = NSProcessInfo.processInfo.operatingSystemVersion;
}

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
#endif
