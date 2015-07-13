#ifndef ARDUINO
#include "common.h"

#ifndef NOTERM
#include "uart_if.h"
#endif

#include "systick.h"

#define DEBUG_LEVEL 0
#include "../../include/cc3200/iobeam.h"
#include "../../include/iobeam_log.h"

static unsigned long _apiIp = 0;
static int _currSock = 0;
static uint64_t _time = 0;  // Our best estimate of global time

static uint32_t _projectId = 0;
static char _deviceId[API_MAX_DEVICE_ID_LEN + 1] = {0};
static const char *_projectToken;

// _millis tracks how many millis has been passed since tracking starts
static uint64_t _millis = {0};

// Callback for SysTick to update the number of millis that passed
static void _update_timer()
{
    _millis += 1;
}

// Get the current # of millis that have elapsed since iobeam started
static uint64_t getMillis()
{
    return _millis;
}

int iobeam_Init(Iobeam *i, uint32_t projId, const char *projToken,
        const char *deviceId)
{
    if (projId <= 0)
        return -1;
    if (projToken == NULL)
        return -1;

    _projectId = projId;
    if (deviceId) {
        size_t len = strlen(deviceId);
        if (len > API_MAX_DEVICE_ID_LEN)
            len = API_MAX_DEVICE_ID_LEN;
        memcpy(_deviceId, deviceId, len);
    } else {  // Check on disk
        _iobeam_ReadFromDisk(_deviceId, API_MAX_DEVICE_ID_LEN);
        IOBEAM_DEBUG("startup device id: %s\r\n", _deviceId);
    }
    _projectToken = projToken;
    SysTickPeriodSet(80000);
    SysTickIntRegister(_update_timer);
    SysTickIntEnable();
    SysTickEnable();

    i->IsRegistered = _iobeam_IsRegistered;
    i->StartTimeKeeping = _iobeam_StartTimeKeeping;
    i->RegisterDevice = _iobeam_RegisterDevice;
    i->SendInt = _iobeam_SendInt;
    i->SendIntWithTime = _iobeam_SendIntWithTime;
    i->SendFloat = _iobeam_SendFloat;
    i->SendFloatWithTime = _iobeam_SendFloatWithTime;

    return 0;
}

static int _iobeam_IsRegistered()
{
    return _deviceId[0] != '\0';
}

static int _iobeam_StartTimeKeeping()
{
    _currSock = _iobeam_GetSocket();

    if (_currSock < 0) {
        IOBEAM_ERR("Unable to get TCP socket.\r\n");
        _currSock = 0;
        return -1;
    }

    char buf[256] = {0};
    uint64_t start = getMillis();
    _iobeam_StartGet(_iobeam_WriteSocket, buf, sizeof(buf), RESOURCE_GET_TIME,
            sizeof(RESOURCE_GET_TIME) - 1);
    _iobeam_WriteCommonHeaders(_iobeam_WriteSocket, buf, sizeof(buf));
    _iobeam_WriteTokenHeader(_iobeam_WriteSocket, buf, sizeof(buf),
            _projectToken);
    _iobeam_EndHeaders(_iobeam_WriteSocket);

    uint32_t rspSize = 0;
    int success = _iobeam_ProcessResponse(200, buf, &rspSize);
    if (success && rspSize > 0) {
        uint64_t elapsed = getMillis() - start;
        uint64_t offset = (elapsed / 2) + start;
        _time = _iobeam_ParseServerTime(buf) - offset;
    }
    return success;
}

static int _iobeam_RegisterDevice()
{
    if (_iobeam_IsRegistered()) {
        return 1;
    }
    _currSock = _iobeam_GetSocket();

    if (_currSock < 0) {
        IOBEAM_ERR("Unable to get TCP socket.\r\n");
        _currSock = 0;
        return -1;
    }

    const char *fmt = ADD_DEVICE_JSON;
    const uint32_t contentLen = snprintf(NULL, 0, fmt, _projectId);

    _iobeam_WritePostHeaders(RESOURCE_ADD_DEVICE,
            sizeof(RESOURCE_ADD_DEVICE) - 1, contentLen);

    char buf[256] = {0};
    snprintf(buf, contentLen + 1, fmt, _projectId);
    _iobeam_WriteBody(_iobeam_WriteSocket, buf, contentLen);
    IOBEAM_VERBOSE("\r\n\r\n");

    uint32_t rspSize = 0;
    int success = _iobeam_ProcessResponse(201, buf, &rspSize);
    if (success && rspSize > 0) {
        success = _iobeam_ParseDeviceId(_deviceId, buf);
        success = _iobeam_WriteToDisk(_deviceId, rspSize) > 0;
    }
    return success;
}

static int _iobeam_Send(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const size_t contentLen = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    _currSock = _iobeam_GetSocket();
    if (_currSock < 0) {
        IOBEAM_ERR("Unable to get TCP socket.\r\n");
        _currSock = 0;
        return -1;
    }

    _iobeam_WritePostHeaders(RESOURCE_IMPORTS, sizeof(RESOURCE_IMPORTS) - 1,
            contentLen);

    char buf[256] = {0};
    va_start(args, fmt);
    vsnprintf(buf, contentLen + 1, fmt, args);
    _iobeam_WriteBody(_iobeam_WriteSocket, buf, contentLen);
    va_end(args);

    IOBEAM_VERBOSE("\r\n\r\n");
    return _iobeam_ProcessResponse(200, NULL, NULL);
}

static int _iobeam_SendInt(const char *key, int64_t value)
{
    return _iobeam_SendIntWithTime(key, _time + getMillis(), value);
}

static int _iobeam_SendIntWithTime(const char *key, uint64_t timestamp,
        int64_t value)
{
#define FMT_SIZE (sizeof(IMPORT_JSON_START) + sizeof(IMPORT_JSON_INT))
    char format[FMT_SIZE] = {0};
    memcpy(format, IMPORT_JSON_START, sizeof(IMPORT_JSON_START) - 1);
    memcpy(format + sizeof(IMPORT_JSON_START) - 1, IMPORT_JSON_INT,
            sizeof(IMPORT_JSON_INT) - 1);

    return _iobeam_Send(format, _deviceId, _projectId, key, timestamp, value);
#undef FMT_SIZE
}

static int _iobeam_SendFloat(const char *key, double value)
{
    return _iobeam_SendFloatWithTime(key, _time + getMillis(), value);
}

static int _iobeam_SendFloatWithTime(const char *key, uint64_t timestamp,
        double value)
{
#define FMT_SIZE sizeof(IMPORT_JSON_START) + sizeof(IMPORT_JSON_FLOAT)
    char format[FMT_SIZE];
    memcpy(format, IMPORT_JSON_START, sizeof(IMPORT_JSON_START) - 1);
    memcpy(format + sizeof(IMPORT_JSON_START) - 1, IMPORT_JSON_FLOAT,
            sizeof(IMPORT_JSON_FLOAT) - 1);

    return _iobeam_Send(format, _deviceId, _projectId, key, timestamp, value);
#undef FMT_SIZE
}

static void _iobeam_WritePostHeaders(char *resource, size_t resourceLen,
        uint32_t contentLen)
{
    char buf[256] = {0};
    const size_t BUF_LEN = sizeof(buf);
    _iobeam_StartPost(_iobeam_WriteSocket, buf, BUF_LEN, resource, resourceLen);
    _iobeam_WriteCommonHeaders(_iobeam_WriteSocket, buf, BUF_LEN);
    _iobeam_WriteContentLengthHeader(_iobeam_WriteSocket, buf, BUF_LEN,
            contentLen);
    _iobeam_WriteTokenHeader(_iobeam_WriteSocket, buf, BUF_LEN, _projectToken);
    _iobeam_EndHeaders(_iobeam_WriteSocket);
}

static int _iobeam_WriteSocket(char *buf, size_t bufLen)
{
    if (_currSock == 0)
        return -1;

    IOBEAM_VERBOSE("%s", buf);
    return sl_Send(_currSock, buf, bufLen, 0);
}

static inline char *_iobeam_MoveToNextLine(char *buf)
{
    char *pos = strchr(buf, '\r');
    if (!pos)
        return NULL;

    if (pos[1] != '\n')
        return NULL;

    pos[0] = '\0'; // make a c-string
    pos += 2;  // move past \r\n too
    return pos;
}

static int _iobeam_ProcessResponse(int wantedCode, char *bodyPtr,
        uint32_t *bodyLen)
{
    char buf[TEMP_BUF_LEN] = {0};
    size_t maxLen = sizeof(buf) - 1;

    int totalBytesRead = 0;
    int totalBytesParsed = 0;
    int bytesParsed = 0;

    int correctCode = 0;
    int unparsedBytes = _iobeam_ReadSocket(buf, maxLen, NULL, 0);
    if (unparsedBytes < 0) {
        _iobeam_CloseSocket();
        return -1;
    }
    totalBytesRead += unparsedBytes;

    int rspCode = parseResponseCode(buf);
    IOBEAM_DEBUG("Rsp code %d %d\r\n", wantedCode, rspCode);
    if (rspCode != wantedCode) {
        _iobeam_CloseSocket();
        return -1;
    }
    correctCode = 1;

    // Find the body length
    char *next;
    char *prev = buf;
    int cLen = 0;
    while (1) {
        next = _iobeam_MoveToNextLine(prev);

        // No full line left in buffer, need to read more, making sure to keep
        // data we haven't processed as well.
        if (!next) {
            int leftover = maxLen - bytesParsed;
            if (leftover <= 0)
                leftover = 0;
            unparsedBytes = _iobeam_ReadSocket(buf, maxLen, prev, leftover);
            totalBytesRead += (unparsedBytes - leftover);
            bytesParsed = 0;
            prev = buf;
            continue;
        }

        int len = (next - prev) / sizeof(char);
        bytesParsed += len;
        totalBytesParsed += len;
        len -= 2;  // Header length: total minus \r\n (2)

        if (len <= 0)  // Reached the end of the headers
            break;
        IOBEAM_VERBOSE("%s\r\n", prev);

        // Body length not yet known, check if this header is it
        if (cLen == 0) {
            if (prev[0] == 'c' || prev[0] == 'C') {
                cLen = parseContentLength(prev);
                if (cLen < 0)
                    cLen = 0;
            }
        }
        prev = next;
    }  // At this point, 'next' points to the start of the body

    // Rsp size = header size (totalByesParsed) + body size (cLen)
    // If they aren't equal, we still have bytes to read from buffer
    if ((totalBytesParsed + cLen) != totalBytesRead) {
        int leftover = maxLen - bytesParsed;
        unparsedBytes = _iobeam_ReadSocket(buf, maxLen, next, leftover);
        totalBytesRead += unparsedBytes - leftover;
        next = buf;
    }

    if (bodyPtr && cLen > 0) {
        memcpy(bodyPtr, next, cLen);
        bodyPtr[cLen] = '\0';
        *bodyLen = cLen;
    }

    _iobeam_CloseSocket();
    return correctCode;
}

static int _iobeam_ReadSocket(char *buf, size_t bufLen, char *prefix,
        size_t prefixLen)
{
    // Safely copy prefix to buffer
    int offset = 0;
    if (prefixLen > 0 && prefix != NULL) {
        char temp[TEMP_BUF_LEN];
        memcpy(temp, prefix, prefixLen);
        memset(buf, '\0', bufLen);
        memcpy(buf, temp, prefixLen);
        offset += prefixLen;
    }

    int ret = sl_Recv(_currSock, buf + offset, bufLen - offset, 0);
    if (ret < 0) {
        IOBEAM_ERR("err: %d\r\n", ret);
        return ret;
    } else {
        offset += ret;
    }

    return offset;
}

static int _iobeam_GetSocket()
{
    SlSockAddrIn_t sAddr;
    int addrSize;
    int sock;
    int err;

    if (_apiIp == 0) {
        err = sl_NetAppDnsGetHostByName(API_DEFAULT_SERVER,
                sizeof(API_DEFAULT_SERVER), &_apiIp, SL_AF_INET);
        if (err < 0)
            return -1;
    }

    //filling the TCP server socket address
    sAddr.sin_family = SL_AF_INET;
    sAddr.sin_port = sl_Htons((unsigned short) API_DEFAULT_PORT);
    sAddr.sin_addr.s_addr = sl_Htonl(_apiIp);
    addrSize = sizeof(SlSockAddrIn_t);

    // creating a TCP socket
    sock = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    // connecting to TCP server
    err = sl_Connect(sock, (SlSockAddr_t *) &sAddr, addrSize);
    if (err < 0) {
        sl_Close(sock);
        return err;
    }

    return sock;
}

static inline void _iobeam_CloseSocket()
{
    if (_currSock > 0)
        sl_Close(_currSock);
    _currSock = 0;
}

void iobeam_Finish()
{
    if (_currSock != 0) {
        sl_Close(_currSock);
        _currSock = 0;
    }
    _apiIp = 0;
    _projectId = 0;
    _projectToken = NULL;
    _time = 0;
    SysTickDisable();
    SysTickIntDisable();
    SysTickIntUnregister();
    memset(_deviceId, '\0', sizeof(_deviceId));
}
#endif /* #ifndef ARDUINO */