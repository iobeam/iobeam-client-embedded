#include <SPI.h>
#include <EEPROM.h>
#include <math.h>
#include "include/arduino/Iobeam.hpp"

Iobeam::Iobeam(Client& client) : mClient(client) { }

void Iobeam::init(uint32_t projId, const char *projToken, int deviceIdAddr) 
{
    mProjectId = projId;
    mToken = projToken;
    if (deviceIdAddr >= 0) {
        readDeviceIdFromMem((unsigned int) deviceIdAddr);
    }
}

// Attempts to read a device ID from the EEPROM if it exists, at the
// provided memory address. First, check for IOBEAM_MEM_PREFIX, then
// parse the ID size, and finally read the ID.
//
// Returns the amount of bytes taken up in memory by us.
int Iobeam::readDeviceIdFromMem(unsigned int memoryOffset)
{
    int off = 0;
    for (int i = 0; i < sizeof(IOBEAM_MEM_PREFIX) - 1; i++) {
        char c = (char) pgm_read_byte(IOBEAM_MEM_PREFIX + i);
        if (EEPROM.read(memoryOffset + i) != c) {
            return -1;
        }
    }
    off += sizeof(IOBEAM_MEM_PREFIX) - 1;
    int len = -1;
    EEPROM.get(memoryOffset + off, len);
    if (len < 16 || len > API_MAX_DEVICE_ID_LEN)
        return -1;

    memset(mDeviceId, '\0', API_MAX_DEVICE_ID_LEN);
    off += sizeof(int);
    for (int i = 0; i < len; i++) {
        mDeviceId[i] = EEPROM.read(memoryOffset + off + i);
    }

    return off + len;
}

int Iobeam::callWrite(void *obj, char * c, size_t l) {
    ((Iobeam *) obj)->write(c, l);
}

// Tells iobeam to begin keeping track of the (approximate) global time.
//
// This call uses an API in the iobeam cloud and some simple math to roughly
// estimate the start time of this sketch with relation to the global clock.
// The client can then use this approximation to assign timestamps rather than
// the user having to manage it.
bool Iobeam::startTimeKeeping()
{
    if (!connect())
        return false;

    uint32_t start = (uint32_t) millis();
    startGet(API_GET_TIME);
    _iobeam_WriteCommonHeaders(this, callWrite, mBuf, SCRATCH_BUF_LEN);
    writeTokenHeader();
    _iobeam_EndHeaders(this, callWrite);
    
    uint32_t rspSize = 0;
    bool success = processResponse(200, mBuf, &rspSize);
    if (success && rspSize > 0) {
        uint32_t elapsed = (uint32_t)(millis() - start);
        uint32_t relative = (elapsed / 2) + start;
        success = setStartTime(mBuf, relative);
    }

    return success;
}

// Approximates the 'start' time of this sketch, in terms of real world time.
//
// Given a timestamp response message from the iobeam server, and a calculated
// `relative` offset for when that timestamp is from, this function sets
// `mStart` equal to a time that subjects that relative offset from the
// given timestamp. `relative` should be calculated by taking the number
// of millis() before the timestamp request, and adding half the elapsed time
// to it, to estimate the cost of one-way of the round trip.
bool Iobeam::setStartTime(char *getTimeRsp, uint32_t relative)
{
    // First parse out seconds, and subtract any whole seconds from `relative`
    char *start = strstr(getTimeRsp, "sec\":");
    start += (sizeof("sec\":") - 1);
    mStart.sec = (uint32_t) strtol(start, NULL, 10);
    mStart.sec -= relative / 1000;
    
    // Parse out milliseconds and determine the number of `relative` msecs
    start = strstr(getTimeRsp, "usec\":");
    start += (sizeof("usec\":") - 1);
    int msec = strtol(start, NULL, 10) / 1000;
    int relMsec = relative % 1000;

    // If there are more msecs than the timestamp, need to carry over;
    // otherwise, simple subtraction.
    if (relMsec > msec) {
        mStart.sec -= 1;
        mStart.msec = (uint32_t) (1000 + msec - relMsec);
    } else {
        mStart.msec = (uint32_t) (msec - relMsec);
    }
    
    return true;
}

// Registers this device with iobeam and stores the device ID at the
// provided memory location in EEPROM. It will return early if an ID
// already exists.
//
// Returns the amount of bytes of EEPROM used by iobeam for the id, or
// -1 if there is an error.
int Iobeam::registerDevice(unsigned int memoryOffset)
{
    // Skip registration if a device ID exists.
    if (isRegistered()) {
        return strlen(mDeviceId) + sizeof(IOBEAM_MEM_PREFIX) - 1 + sizeof(int);
    }

    if (!connect()) {
        return -1;
    }

    // Formats are stored in program space to save memory.
    const size_t FMT_SIZE = sizeof(addDeviceJson);
    char format[FMT_SIZE];
    readBytesFromPgm(format, FMT_SIZE, addDeviceJson, FMT_SIZE);

    size_t contentLen = snprintf(NULL, 0, format, mProjectId);
    writePostHeaders(API_REGISTER, contentLen);

    snprintf(mBuf, contentLen + 1, format, mProjectId);
    _iobeam_WriteBody(this, callWrite, mBuf, contentLen);

    // This response will contain a body including our new device ID, 
    // so we'll need a pointer to where the response is stored in 
    // additional to checking the response code.
    uint32_t rspSize = 0;
    bool success = processResponse(201, mBuf, &rspSize);
    if (success && rspSize > 0) {
        int idLen = _iobeam_ParseDeviceId(mDeviceId, mBuf);
        if (idLen < 1)
            return -1;

        // We store the ID in EEPROM so it persists. The format is the
        // IOBEAM_MEM_PREFIX (8 bytes), followed by the size of the id,
        // followed by the id.
        int offset = memoryOffset;
        for (int i = 0; i < sizeof(IOBEAM_MEM_PREFIX) - 1; i++) {
            char c = (char) pgm_read_byte(IOBEAM_MEM_PREFIX + i);
            EEPROM.update(memoryOffset + i, c);
        }
        offset += sizeof(IOBEAM_MEM_PREFIX) - 1;
        EEPROM.put(offset, idLen);
        offset += sizeof(int);
        for (int i = 0; i < idLen; i++) {
            EEPROM.update(offset+i, mDeviceId[i]);
        }

        return idLen + sizeof(IOBEAM_MEM_PREFIX) - 1 + sizeof(int);
    }
    return -1;
}

// Common function for the two types of sending/import requests.
bool Iobeam::sendFormat(const char *fmt, ...) {
    if (!connect())
        return false;

    va_list args;
    va_start(args, fmt);
    const size_t contentLen = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    writePostHeaders(API_IMPORTS, contentLen);
        
    va_start(args, fmt);
    vsnprintf(mBuf, contentLen + 1, fmt, args);
    _iobeam_WriteBody(this, callWrite, mBuf, contentLen);
    va_end(args);
    
    return processResponse(200, NULL, NULL);
}

bool Iobeam::send(char *key, double value)
{
    Timeval t = {0};
    uint32_t tOff = (uint32_t) millis();
    t.sec = mStart.sec + (tOff / 1000);
    t.msec = mStart.msec + (tOff % 1000);
    if (t.msec > 1000) {
        t.sec += 1;
        t.msec -= 1000;
    }
    return send(key, t, value);
}

bool Iobeam::send(char *key, Timeval& t, double value) {
    // Formats are stored in program space to save memory.
    const size_t FMT_SIZE = sizeof(importStart) + sizeof(importFloat);
    char format[FMT_SIZE] = {0};
    readStringFromPgm(format, sizeof(importStart) - 1, importStart);
    readStringFromPgm(format + sizeof(importStart) - 1,
        sizeof(importFloat), importFloat);

    int intval = trunc(value);
    int decval = trunc((value - intval) * 10000);
    
    return sendFormat(format, mDeviceId, mProjectId, key, t.sec, t.msec,
        intval, decval);
}

bool Iobeam::send(char *key, int value)
{
    Timeval t = {0};
    uint32_t tOff = (uint32_t) millis();
    t.sec = mStart.sec + (tOff / 1000);
    t.msec = mStart.msec + (tOff % 1000);
    if (t.msec > 1000) {
        t.sec += 1;
        t.msec -= 1000;
    }
    return send(key, t, value);
}

bool Iobeam::send(char *key, Timeval& t, int value)
{
    // Formats are stored in program space to save memory.
    const size_t FMT_SIZE = sizeof(importStart) + sizeof(importInt);
    char format[FMT_SIZE] = {0};
    int offset = 0;
    offset += readStringFromPgm(format, sizeof(importStart) - 1,
        importStart);
    readStringFromPgm(format + offset, sizeof(importInt), importInt);

    return sendFormat(format, mDeviceId, mProjectId, key, t.sec, t.msec,
        value);
}

// Writes the POST header for API calls for a resource.
void Iobeam::writePostHeaders(const char *resource, size_t contentLen)
{
    startPost(resource);
    _iobeam_WriteCommonHeaders(this, callWrite, mBuf, SCRATCH_BUF_LEN);
    _iobeam_WriteContentLengthHeader(this, callWrite, mBuf,
        SCRATCH_BUF_LEN, contentLen);
    writeTokenHeader();
    _iobeam_EndHeaders(this, callWrite);
}

void Iobeam::startHeaders(const char *method, const char *resource)
{
    int offset = 0;

    offset += readStringFromPgm(mBuf, SCRATCH_BUF_LEN, method);
    offset += readStringFromPgm(mBuf + offset, SCRATCH_BUF_LEN - offset,
        resource);
    offset += readStringFromPgm(mBuf + offset, SCRATCH_BUF_LEN - offset,
        HTTP11);
    memcpy(mBuf + offset, HEADER_END, sizeof(HEADER_END) - 1);
    write(mBuf, offset + 2);   
}

void Iobeam::startGet(const char *resource)
{
    startHeaders(METHOD_GET, resource);
}

void Iobeam::startPost(const char *resource)
{
    startHeaders(METHOD_POST, resource);
}

// We write our own token header write so as to save a copy to RAM.
void Iobeam::writeTokenHeader()
{
    int offset = 0;
    memset(mBuf, '\0', SCRATCH_BUF_LEN);

    offset += readStringFromPgm(mBuf, SCRATCH_BUF_LEN, HTTP_TOKEN_PREFIX);
    offset += readStringFromPgm(mBuf + offset, SCRATCH_BUF_LEN - offset,
        mToken);
    memcpy(mBuf + offset, HEADER_END, sizeof(HEADER_END) - 1);
    write(mBuf, offset + 2);
}

int Iobeam::write(char *msg, size_t msgLen)
{
    return write((const uint8_t *) msg, msgLen);
}

int Iobeam::write(const uint8_t *msg, size_t msgLen) 
{
    mClient.write(msg, msgLen);
    IOBEAM_VERBOSE_W(msg, msgLen);
    return msgLen;
}

bool Iobeam::processResponse(int code, char *bodyPtr, uint32_t *bodyLen)
{
    // First check the response code. If wrong, close connection and
    // return as unsuccessful.
    bool correctCode = false;
    int ret = readLine(mBuf, SCRATCH_BUF_LEN);
    int returnCode = parseResponseCode(mBuf);
    correctCode = returnCode == code;
    if (!correctCode) {
        mClient.stop();
        return false;
    }

    // Extract the body if caller has provided a pointer for it to go,
    // otherwise we're done.
    if (bodyPtr) {
        uint32_t contentLen = 0;
        while (1) {
            ret = readLine(mBuf, SCRATCH_BUF_LEN);
            if (ret <= 0) // err or finished headers
                break;

            int temp = parseContentLength(mBuf);
            if (temp >= 0) {
                contentLen = (uint32_t) temp;
                break;
            }
        }

        *bodyLen = contentLen;
        if (bodyPtr && contentLen > 0) {
            while(readLine(mBuf, SCRATCH_BUF_LEN) > 0);  // skips to body

            int i = 0;
            while (i < contentLen) {
                if (mClient.available() > 0) {
                    bodyPtr[i] = mClient.read();
                    i++;
                }
            }
        } else {
            bodyPtr[0] = '\0';
        }
    } else {
        // The YunClient is broken in that it doesn't clear its buffers on
        // stop() calls. So we must read the whole message to not have
        // issues on the next request.
        // TODO: Write a client that extends YunClient with fixes.
        #ifdef ARDUINO_AVR_YUN
            while(readLine(mBuf, SCRATCH_BUF_LEN) > 0);    
        #endif
    }

    mClient.stop();
    return true;
}
