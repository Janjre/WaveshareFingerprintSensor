#ifndef FINGERPRINT_SENSOR_H
#define FINGERPRINT_SENSOR_H

#include <Arduino.h>

/* =========================
   ACK / RESPONSE CODES
   ========================= */
#define WVFP_ACK_SUCCESS           0x00
#define WVFP_ACK_FAIL              0x01
#define WVFP_ACK_FULL              0x04
#define WVFP_ACK_NO_USER           0x05
#define WVFP_ACK_USER_OCCUPIED     0x06
#define WVFP_ACK_FINGER_OCCUPIED   0x07
#define WVFP_ACK_TIMEOUT           0x08
#define WVFP_ACK_GO_OUT            0x0F

/* =========================
   USER TYPES
   ========================= */
#define WVFP_ACK_ALL_USER          0x00
#define WVFP_ACK_GUEST_USER        0x01
#define WVFP_ACK_NORMAL_USER       0x02
#define WVFP_ACK_MASTER_USER       0x03

#define WVFP_USER_MAX_CNT          1000

/* =========================
   COMMANDS
   ========================= */
#define WVFP_CMD_HEAD              0xF5
#define WVFP_CMD_TAIL              0xF5
#define WVFP_CMD_SERIALNO          0x2A
#define WVFP_CMD_ADD_1             0x01
#define WVFP_CMD_ADD_2             0x02
#define WVFP_CMD_ADD_3             0x03
#define WVFP_CMD_MATCH             0x0C
#define WVFP_CMD_DEL               0x04
#define WVFP_CMD_DEL_ALL           0x05
#define WVFP_CMD_USER_CNT          0x09
#define WVFP_CMD_COM_LEV           0x28
#define WVFP_CMD_GET_EV            0x31
#define WVFP_CMD_SET_EV            0x41
#define WVFP_CMD_LP_MODE           0x2C
#define WVFP_CMD_TIMEOUT           0x2E
#define WVFP_CMD_FINGER_DETECTED   0x14

/* =========================
   CONSTANTS
   ========================= */
#define WVFP_IMAGE_LENGTH          9800
#define WVFP_EIGENVALUE_LENGTH     193
#define WVFP_TXRXBUFFER_SIZE       8
#define WVFP_TXRXDATA_SIZE         4

class FingerprintSensor {
public:
    bool DebugPrints = false;
    FingerprintSensor(
        HardwareSerial &sr,
        long baud,
        int8_t rx,
        int8_t tx,
        int8_t wake,
        int8_t rst
    );

    void begin();
    void setSleepMode(bool sleep);

    bool txAndRxCmd(byte commands[], byte response[], uint32_t timeout);
    unsigned long getModelSN();
    bool enrollUser(uint16_t id, byte permission);

private:
    void clearTxBuffer();
    void clearRxBuffer();
    void shiftBuffer(byte buffer[], uint16_t bufferSize);
    void txCmd(byte commands[]);
    bool rxCmd(byte response[], byte command);

    int8_t RX, TX, WAKE, RST;
    long Baud;
    HardwareSerial *SensorSerial;

    byte _cmdSendBuffer[WVFP_TXRXBUFFER_SIZE];
    byte _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE];
};

#endif // FINGERPRINT_SENSOR_H
