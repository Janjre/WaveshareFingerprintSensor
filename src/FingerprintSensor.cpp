#include "FingerprintSensor.h"

FingerprintSensor::FingerprintSensor(
    HardwareSerial &sr,
    long baud,
    int8_t rx,
    int8_t tx,
    int8_t wake,
    int8_t rst
) {
    RX = rx;
    TX = tx;
    WAKE = wake;
    RST = rst;
    Baud = baud;
    SensorSerial = &sr;

    pinMode(WAKE, INPUT);
    pinMode(RST, OUTPUT);
}

void FingerprintSensor::begin() {
    SensorSerial->begin(Baud, SERIAL_8N1, RX, TX);
}

void FingerprintSensor::clearTxBuffer() {
    memset(_cmdSendBuffer, 0, WVFP_TXRXBUFFER_SIZE);
}

void FingerprintSensor::clearRxBuffer() {
    memset(_cmdReceiverBuffer, 0, WVFP_TXRXBUFFER_SIZE);
}

void FingerprintSensor::shiftBuffer(byte buffer[], uint16_t bufferSize) {
    for (uint8_t i = 1; i < bufferSize; i++) {
        buffer[i - 1] = buffer[i];
    }
    buffer[bufferSize - 1] = 0x00;
}

void FingerprintSensor::txCmd(byte commands[]) {
    clearTxBuffer();
    _cmdSendBuffer[0] = WVFP_CMD_HEAD;
    _cmdSendBuffer[5] = 0;

    for (uint8_t i = 0; i < 4; i++) {
        _cmdSendBuffer[1 + i] = commands[i];
    }

    byte checksum = 0;
    for (uint8_t i = 1; i < 6; i++) {
        checksum ^= _cmdSendBuffer[i];
    }

    _cmdSendBuffer[6] = checksum;
    _cmdSendBuffer[7] = WVFP_CMD_TAIL;

    SensorSerial->write(_cmdSendBuffer, WVFP_TXRXBUFFER_SIZE);
}

bool FingerprintSensor::rxCmd(byte response[], byte command) {
    while (SensorSerial->available()) {
        shiftBuffer(_cmdReceiverBuffer, WVFP_TXRXBUFFER_SIZE);
        _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 1] = SensorSerial->read();

        if (_cmdReceiverBuffer[0] == WVFP_CMD_HEAD &&
            _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 1] == WVFP_CMD_TAIL) {

            byte checksum = 0;
            for (uint8_t i = 1; i < 6; i++) {
                checksum ^= _cmdReceiverBuffer[i];
            }

            if (checksum == _cmdReceiverBuffer[6]) {
                memcpy(response, &_cmdReceiverBuffer[1], WVFP_TXRXDATA_SIZE);
                return true;
            }
        }
    }
    return false;
}

bool FingerprintSensor::txAndRxCmd(byte commands[], byte response[], uint32_t timeout) {
    setSleepMode(false);
    txCmd(commands);

    unsigned long endTime = millis() + timeout;
    while (millis() < endTime) {
        if (rxCmd(response, commands[1])) {
            setSleepMode(true);
            return true;
        }
        delay(10);
    }

    setSleepMode(true);
    return false;
}

void FingerprintSensor::setSleepMode(bool sleep) {
    digitalWrite(RST, sleep ? LOW : HIGH);
    if (!sleep) delay(10);
}

unsigned long FingerprintSensor::getModelSN() {
    byte cmd[] = { WVFP_CMD_SERIALNO, 0, 0, 0 };
    byte resp[WVFP_TXRXDATA_SIZE];

    if (!txAndRxCmd(cmd, resp, 1000)) return 0;

    return ((unsigned long)resp[1] << 16) |
           ((unsigned long)resp[2] << 8)  |
            (unsigned long)resp[3];
}

bool FingerprintSensor::enrollUser (uint16_t id,byte permission) {

    for (int step = 1; step <= 3; step++) {
        if (DebugPrints) {
            Serial.println("Step "+String(step));
        }

        byte response[WVFP_TXRXBUFFER_SIZE-1];
        byte command[] = {(byte)step, 0, (byte)(id & 0xff00 >> 8),(byte)(id & 0x00ff),permission};

        if (txAndRxCmd(command,response,1000)) {
            switch (response[4]) {
                case WVFP_ACK_SUCCESS:
                    if (DebugPrints) {
                        Serial.println("Success");
                    }
                    break;
                case WVFP_ACK_FULL:
                    Serial.println("Error: database full");
                    return false;
                case WVFP_ACK_FAIL:
                    Serial.println("Error");
                    return false;
                case WVFP_ACK_USER_OCCUPIED:
                    Serial.println("Already registered user");
                    return false;
                case WVFP_ACK_FINGER_OCCUPIED:
                    Serial.println("Already registered finger");
                    return false;

            }

        }else {
            Serial.println("Failed at step 1");
            return false;
        }
    }


    return false;
}
