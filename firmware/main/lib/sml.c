#include <string.h>

#include "sml.h"
#include "smlCrcTable.h"

#define MAX_LIST_SIZE 80
#define MAX_TREE_SIZE 10

static sml_states_t currentState = SML_START;
static char nodes[MAX_TREE_SIZE];
static unsigned char currentLevel = 0;
static unsigned short crc = 0xFFFF;
static signed char sc;
static unsigned short crcMine = 0xFFFF;
static unsigned short crcReceived = 0x0000;
static unsigned char len = 4;
static unsigned char listBuffer[MAX_LIST_SIZE];
static unsigned char listPos = 0;

void crc16(unsigned char *byte) {
    crc = smlCrcTable[((*byte) ^ crc) & 0xff] ^ (crc >> 8 & 0xff);
}

void setState(sml_states_t state, int byteLen) {
    currentState = state;
    len = byteLen;
}

void pushListBuffer(unsigned char byte) {
    if (listPos < MAX_LIST_SIZE) {
        listBuffer[listPos++] = byte;
    }
}

void reduceList() {
    if (currentLevel >= 0 && nodes[currentLevel] > 0)
        nodes[currentLevel]--;
}

void smlNewList(unsigned char size) {
    reduceList();
    if (currentLevel < MAX_TREE_SIZE - 1)
        currentLevel++;
    nodes[currentLevel] = size;
    setState(SML_LISTSTART, size);
    if (size > 5) {
        listPos = 0;
        memset(listBuffer, '\0', MAX_LIST_SIZE);
    }
    else {
        pushListBuffer(size);
        pushListBuffer(currentState);
    }
}

void checkMagicByte(unsigned char *byte) {
    unsigned int size = 0;
    while (currentLevel > 0 && nodes[currentLevel] == 0) {
        currentLevel--;
    }
    if (*byte > 0x70 && *byte <= 0x7F) {
        size = (*byte) & 0x0F;
        smlNewList(size);
    }
    else if (*byte >= 0x01 && *byte <= 0x6F && nodes[currentLevel] > 0) {
        if (*byte == 0x01) {
            pushListBuffer(0);
            pushListBuffer(currentState);
            if (nodes[currentLevel] == 1) {
                setState(SML_LISTEND, 1);
            }
            else {
                setState(SML_NEXT, 1);
            }
        }
        else {
            size = ((*byte) & 0x0F) - 1;
            setState(SML_DATA, size);
            if (((*byte) & 0xF0) == 0x50) {
                setState(SML_DATA_SIGNED_INT, size);
            }
            else if (((*byte) & 0xF0) == 0x60) {
                setState(SML_DATA_UNSIGNED_INT, size);
            }
            else if (((*byte) & 0xF0) == 0x00) {
                setState(SML_DATA_OCTET_STRING, size);
            }
            pushListBuffer(size);
            pushListBuffer(currentState);
        }
        reduceList();
    }
    else if (*byte == 0x00) {
        reduceList();
        if (currentLevel == 0) {
            setState(SML_NEXT, 1);
        }
        else {
            setState(SML_BLOCKEND, 1);
        }
    }
    else if (*byte & 0x80) {
        if (*byte >= 0x80 && *byte <= 0x8F) {
            setState(SML_HDATA, ((*byte) & 0x0F) << 4);
        }
        else if (*byte >= 0xF0 && *byte <= 0xFF) {
            setState(SML_LISTEXTENDED, ((*byte) & 0x0F) << 4);
        }
    }
    else if (*byte == 0x1B && currentLevel == 0) {
        setState(SML_END, 3);
    }
    else {
        setState(SML_UNEXPECTED, 4);
    }
}

sml_states_t smlState(unsigned char *currentByte) {
    unsigned char size;
    if (len > 0)
        len--;
    crc16(currentByte);
    switch (currentState) {
    case SML_UNEXPECTED:
    case SML_CHECKSUM_ERROR:
    case SML_FINAL:
    case SML_START:
        currentState = SML_START;
        currentLevel = 0;
        if (*currentByte != 0x1b)
            setState(SML_UNEXPECTED, 4);
        if (len == 0) {
            crc = 0xFFFF;
            *currentByte = 0x1b;
            crc16(currentByte);
            crc16(currentByte);
            crc16(currentByte);
            crc16(currentByte);
            setState(SML_VERSION, 4);
        }
        break;
    case SML_VERSION:
        if (*currentByte != 0x01) {
            setState(SML_UNEXPECTED, 4);
        }
        if (len == 0) {
            setState(SML_BLOCKSTART, 1);
        }
        break;
    case SML_END:
        if (*currentByte != 0x1b) {
            setState(SML_UNEXPECTED, 4);
        }
        if (len == 0) {
            setState(SML_CHECKSUM, 4);
        }
        break;
    case SML_CHECKSUM:
        if (len == 2) {
            crcMine = crc ^ 0xFFFF;
        }
        if (len == 1) {
            crcReceived += *currentByte;
        }
        if (len == 0) {
            crcReceived = crcReceived | ((*currentByte) << 8);
            if (crcMine == crcReceived) {
                setState(SML_FINAL, 4);
            }
            else {
                setState(SML_CHECKSUM_ERROR, 4);
            }
            crc = 0xFFFF;
            crcReceived = 0x000;
        }
        break;
    case SML_HDATA:
        size = len + *currentByte - 1;
        setState(SML_DATA, size);
        pushListBuffer(size);
        pushListBuffer(currentState);
        break;
    case SML_LISTEXTENDED:
        size = len + (*currentByte & 0x0F);
        smlNewList(size);
        break;
    case SML_DATA:
    case SML_DATA_SIGNED_INT:
    case SML_DATA_UNSIGNED_INT:
    case SML_DATA_OCTET_STRING:
        pushListBuffer(*currentByte);
        if (nodes[currentLevel] == 0 && len == 0) {
            currentState = SML_LISTEND;
        }
        else if (len == 0) {
            currentState = SML_DATAEND;
        }
        break;
    case SML_DATAEND:
    case SML_NEXT:
    case SML_LISTSTART:
    case SML_LISTEND:
    case SML_BLOCKSTART:
    case SML_BLOCKEND:
        checkMagicByte(currentByte);
        break;
    }
    return currentState;
}

bool smlOBISCheck(const unsigned char *obis) {
    return (memcmp(obis, &listBuffer[2], 6) == 0);
}

void smlOBISManufacturer(unsigned char *str, int maxSize) {
    int i = 0, pos = 0, size = 0;
    while (i < listPos) {
        size = (int)listBuffer[i];
        i++;
        pos++;
        if (pos == 6) {
            size = (size > maxSize - 1) ? maxSize - 1 : size;
            memcpy(str, &listBuffer[i + 1], size);
            str[size] = 0;
        }
        i += size + 1;
    }
}

void smlPow(double *val, signed char *scaler) {
    if (*scaler < 0) {
        while ((*scaler)++) {
            *val /= 10;
        }
    }
    else {
        while ((*scaler)--) {
            *val *= 10;
        }
    }
}

void smlOBISByUnit(long long int *val, signed char *scaler, sml_units_t unit) {
    unsigned char i = 0, pos = 0, size = 0, y = 0, skip = 0;
    sml_states_t type;
    *val = -1;
    while (i < listPos) {
        pos++;
        size = (int)listBuffer[i++];
        type = (sml_states_t)listBuffer[i++];
        if (type == SML_LISTSTART && size > 0) {
            skip = size;
            while (skip > 0) {
                size = (int)listBuffer[i++];
                type = (sml_states_t)listBuffer[i++];
                i += size;
                skip--;
            }
            size = 0;
        }
        if (pos == 4 && listBuffer[i] != unit) {
            return;
        }
        if (pos == 5) {
            *scaler = listBuffer[i];
        }
        if (pos == 6) {
            *val = (type == SML_DATA_SIGNED_INT && (listBuffer[i] & (1 << 7))) ? ~0 : 0;
            for (y = 0; y < size; y++) {
                *val = (*val << 8) | listBuffer[i + y];
            }
        }
        i += size;
    }
}

void smlOBISWh(double *wh) {
    long long int val;
    smlOBISByUnit(&val, &sc, SML_WATT_HOUR);
    *wh = val;
    smlPow(wh, &sc);
}

void smlOBISW(double *w) {
    long long int val;
    smlOBISByUnit(&val, &sc, SML_WATT);
    *w = val;
    smlPow(w, &sc);
}

void smlOBISVolt(double *v) {
    long long int val;
    smlOBISByUnit(&val, &sc, SML_VOLT);
    *v = val;
    smlPow(v, &sc);
}

void smlOBISAmpere(double *a) {
    long long int val;
    smlOBISByUnit(&val, &sc, SML_AMPERE);
    *a = val;
    smlPow(a, &sc);
}

void smlOBISHertz(double *h) {
    long long int val;
    smlOBISByUnit(&val, &sc, SML_HERTZ);
    *h = val;
    smlPow(h, &sc);
}

void smlOBISDegree(double *d) {
    long long int val;
    smlOBISByUnit(&val, &sc, SML_DEGREE);
    *d = val;
    smlPow(d, &sc);
}
