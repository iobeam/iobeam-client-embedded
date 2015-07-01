#ifndef Iobeam_h
#define Iobeam_h

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <Client.h>

#define __STDC_LIMIT_MACROS
#include <inttypes.h>

#ifndef API_DEFAULT_SERVER
#define API_DEFAULT_SERVER "api.iobeam.com"
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#include "../iobeam_log.h"
#include "../iobeam_common.h"


#undef RESOURCE_GET_TIME

// PROGMEM these long strings to save RAM space.
PROGMEM const char importStart[] = {
    "{\"device_id\":\"%s\",\"project_id\":%"PRIu32",\"sources\":[{\"name\":\"%s\","
    "\"data\":[{\"time\":%"PRIu32"%03"PRIu32",\"value\":"
};
PROGMEM const char importInt[] = {"%d}]}]}"};
PROGMEM const char importFloat[] = {"%d.%d}]}]}"};
PROGMEM const char addDeviceJson[] = ADD_DEVICE_JSON;

PROGMEM const char IOBEAM_MEM_PREFIX[] = "iobeamid";

PROGMEM const char METHOD_GET[] = HTTP_METHOD_GET " ";
PROGMEM const char METHOD_POST[] = HTTP_METHOD_POST " ";
PROGMEM const char HTTP11[] = " " PROTOCOL;

PROGMEM const char HTTP_TOKEN_PREFIX[] = HTTP_HEADER_TOKEN ": Bearer ";

PROGMEM const char API_IMPORTS[] = RESOURCE_IMPORTS;
PROGMEM const char API_REGISTER[] = RESOURCE_ADD_DEVICE;
PROGMEM const char API_GET_TIME[] = 
    "/v1/devices/timestamp?timefmt=TIMEVAL";

class Iobeam {
public:
    // A struct similar to `struct timeval`, this breaks down time into
    // two 32-bit components: seconds and milliseconds (NOT microseconds like
    // `struct timeval`). 
    typedef struct _time {
        uint32_t sec;
        uint32_t msec;
    } Timeval;

    Iobeam(Client& client);

    void init(uint32_t projId, const char *projToken, int deviceIdAddr);
    bool isRegistered() 
    {
        return strlen(mDeviceId) > 0;
    }

    // Fetches global timestamp from iobeam, starts tracking time.
    bool startTimeKeeping();
    int registerDevice(unsigned int memoryOffset);
    bool send(char *key, Timeval& timestamp, double value);
    bool send(char *key, Timeval& timestamp, int value);
    bool send(char *key, double value);
    bool send(char *key, int value);

private:
#define SCRATCH_BUF_LEN 256

    // iobeam metadata.
    uint32_t mProjectId;
    const char *mToken;
    char mDeviceId[API_MAX_DEVICE_ID_LEN] = {0};
    
    // The network client to use for communicating with iobeam cloud.
    Client& mClient;

    // The approximate start time to use in conjunction with the value from
    // `millis()` to construct timestamps.
    Timeval mStart = {0};

    // This is a 'scratch' space for strings to be written/buffered rather
    // than creating a lot of temporary buffers that tend screw up memory
    // safety if too many are made.
    char mBuf[SCRATCH_BUF_LEN];

    // A static call needed by the common library to callback to
    // a function pointer.
    static int callWrite(void*, char*, size_t);

    bool sendFormat(const char *fmt, ...);
    bool setStartTime(char *rsp, uint32_t relative);
    int readDeviceIdFromMem(unsigned int offset);
    bool processResponse(int code, char *bodyPtr, uint32_t *bodyLen);

    void startGet(const char *resource);
    void startPost(const char *resource);
    void startHeaders(const char *method, const char *resource);

    void writeTokenHeader();
    void writePostHeaders(const char *resource, size_t contentLen);


    int write(char *msg, size_t msgLen);
    int write(const uint8_t *msg, size_t msgLen);


    // Creates a network connection to iobeam cloud.
    bool connect()
    {
        int code = mClient.connect(API_DEFAULT_SERVER, API_DEFAULT_PORT);
        return code > 0;
    }

    // Reads an HTTP header line into `buf`.
    int readLine(char *buf, size_t bufLen)
    {
        int i = 0;
        while (true) {
            if (i == bufLen) {
                break;
            }

            if (mClient.available()) {
                char c = mClient.read();
                if (c == '\r') {  // marks the end of the line
                    mClient.read();  // read past \n too
                    memset(buf + i, '\0', bufLen - i);
                    return i;
                }
                buf[i] = c;
                i++;
            }
        }

        return -1;  // buffer not large enough to find end
    }

    // Copies bytes from PROGMEM into buf, up to min(bufLen, srcLen)
    // bytes.
    // Returns the number of bytes copied.
    int readBytesFromPgm(char *buf, size_t bufLen, const char *src,
        size_t srcLen)
    {
        size_t limit = srcLen < bufLen ? srcLen : bufLen;
        int i = 0;
        for (; i < limit; i++)
            buf[i] = (char) pgm_read_byte(src + i);

        return i;
    }

    // Reads a c-string from PROGMEM and copies up to bufLen
    // characters to buf. The NUL character is NOT copied.
    // Returns the number of bytes copied.
    int readStringFromPgm(char *buf, size_t bufLen, const char *src)
    {
        int i = 0;
        for (; i < bufLen; i++) {
            char c = (char) pgm_read_byte(src + i);
            if (c == '\0')
                break;
            buf[i] = c;
        }
        return i;
    }
};

#endif
