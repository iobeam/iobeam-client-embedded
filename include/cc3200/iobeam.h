#ifndef IOBEAM_H_
#define IOBEAM_H_

#include <stdio.h>
#include <stdint.h>

#ifndef API_DEFAULT_SERVER
#define API_DEFAULT_SERVER  "api.iobeam.com"
#endif
#include "../iobeam_common.h"

#include "simplelink.h"

#define TEMP_BUF_LEN 192
#define IOBEAM_DEVICE_FILE "iobeam-device-id"

#define IMPORT_JSON_START "{\"device_id\":\"%s\",\"project_id\":%"PRIu32"," \
						  "\"sources\":[{\"name\":\"%s\",\"data\":" \
						  "[{\"time\":%llu,\"value\":"
#define IMPORT_JSON_INT "%d}]}]}"
#define IMPORT_JSON_FLOAT "%f}]}]}"

typedef struct _iobeam {
    int (*IsRegistered)();
    int (*StartTimeKeeping)();
    int (*RegisterDevice)();
    int (*SendInt)(const char *key, int64_t val);
    int (*SendIntWithTime)(const char *key, uint64_t ts, int64_t val);
    int (*SendFloat)(const char *key, double val);
    int (*SendFloatWithTime)(const char *key, uint64_t ts, double val);
} Iobeam;

int iobeam_Init(Iobeam *i, uint32_t projId, const char *projToken,
        const char *deviceId);
static int _iobeam_StartTimeKeeping();
static int _iobeam_IsRegistered();
static int _iobeam_RegisterDevice();
static int _iobeam_SendFloat(const char *key, double value);
static int _iobeam_SendFloatWithTime(const char *key, uint64_t timestamp,
        double value);
static int _iobeam_SendInt(const char *key, int64_t value);
static int _iobeam_SendIntWithTime(const char *key, uint64_t timestamp,
        int64_t value);
void iobeam_Finish();
static void iobeam_Reset() {
    sl_FsDel(IOBEAM_DEVICE_FILE, 0);
}

static uint64_t _iobeam_ParseServerTime(const char *getTimeRsp)
{
    char *start = strstr(getTimeRsp, API_SERVER_TIME_KEY);
    start += (sizeof(API_SERVER_TIME_KEY) - 1);
    return (uint64_t) strtoll(start, NULL, 10);
}

static int _iobeam_ReadFromDisk(char *dst, size_t dstLen)
{
    long fd;
    unsigned char *fn = (unsigned char *) IOBEAM_DEVICE_FILE;

    int ret = sl_FsOpen(fn, FS_MODE_OPEN_READ, NULL, &fd);
    if (ret < 0)
        return -1;

    ret = sl_FsRead(fd, 0, (unsigned char *) dst, dstLen);
    if (ret < 0)
        return -1;

    sl_FsClose(fd, 0, 0, 0);

    return ret;
}

static int _iobeam_WriteToDisk(char *buf, size_t bufLen)
{
    long fd;
    unsigned char *fn = (unsigned char *) IOBEAM_DEVICE_FILE;

    int ret = sl_FsOpen(fn, FS_MODE_OPEN_WRITE, NULL, &fd);
    if (ret == SL_FS_ERR_FILE_NOT_EXISTS) {
        ret = sl_FsOpen(fn,
                FS_MODE_OPEN_CREATE(128,
                        _FS_FILE_OPEN_FLAG_COMMIT | _FS_FILE_PUBLIC_WRITE),
                NULL, &fd);
        if (ret < 0)
            return -1;

        ret = sl_FsClose(fd, 0, 0, 0);
        if (ret < 0)
            return -1;

        ret = sl_FsOpen(fn, FS_MODE_OPEN_WRITE, NULL, &fd);
        if (ret < 0)
            return -1;
    }
    ret = sl_FsWrite(fd, 0, (unsigned char *) buf, bufLen);
    sl_FsClose(fd, 0, 0, 0);

    return ret;
}

static void _iobeam_WritePostHeaders(char *resource, size_t resourceLen,
        uint32_t contentLen);

static int _iobeam_ProcessResponse(int wantedCode, char *bodyPtr,
        uint32_t *bodyLen);

static int _iobeam_GetSocket();
static void _iobeam_CloseSocket();
static int _iobeam_WriteSocket(char *buf, size_t bufLen);
static int _iobeam_ReadSocket(char *buf, size_t bufLen, char *prefix,
        size_t prefixLen);

#endif /* IOBEAM_H_ */
