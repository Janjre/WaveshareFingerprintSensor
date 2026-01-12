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
    for (uint8_t i = 0; i<4; i++) {
        _cmdSendBuffer[1+i] = commands[i];
    }
    _cmdSendBuffer[5] = 0;

    byte checksum = 0;
    for (uint8_t i = 1; i<6; i++) {
        checksum = checksum ^ _cmdSendBuffer[i];
    }
    _cmdSendBuffer[6] = checksum;
    _cmdSendBuffer[7] = WVFP_CMD_TAIL;

    if (DebugPrints) {
        Serial.println("About to sent: ");
        Serial.print(String(_cmdSendBuffer[0],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[1],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[2],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[3],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[4],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[5],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[6],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[7],HEX) + ",");
    }

    SensorSerial->write(_cmdSendBuffer, WVFP_TXRXBUFFER_SIZE);
}

bool FingerprintSensor::rxCmd(byte response[], byte command) {
    if (DebugPrints) {
        Serial.println("START OF RX AND TX, fairly well printed");
    }

    int cnt = 0;
    bool success = false;
    while (SensorSerial->available()) {
        shiftBuffer(_cmdReceiverBuffer, WVFP_TXRXBUFFER_SIZE);
        _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 1] = SensorSerial->read();
        cnt++;



        // stop reading if there is a correct frame
        if (_cmdReceiverBuffer[0] == WVFP_CMD_HEAD && _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 1] == WVFP_CMD_TAIL){// && _cmdReceiverBuffer[1] == command) {

            if (DebugPrints) {
                Serial.print("Command received");
            }

            success=true;
            break;
        }
    }

    if (DebugPrints) {
        Serial.println("Command thing recieved:");
        Serial.print(String(_cmdReceiverBuffer[0],HEX) + ",");
        Serial.print(String(_cmdReceiverBuffer[1],HEX) + ",");
        Serial.print(String(_cmdReceiverBuffer[2],HEX) + ",");
        Serial.print(String(_cmdReceiverBuffer[3],HEX) + ",");
        Serial.print(String(_cmdReceiverBuffer[4],HEX) + ",");
        Serial.print(String(_cmdReceiverBuffer[5],HEX) + ",");
        Serial.print(String(_cmdReceiverBuffer[6],HEX) + ",");
        Serial.println(String(_cmdReceiverBuffer[7],HEX) );
    }

    if (success) {
        Serial.println("It does!");
        byte checksum = 0;
        for (uint8_t i = 1; i<6; i++) {
            checksum ^= _cmdReceiverBuffer[i];
        }

        // check if checksum is correct
        if (checksum == _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 2]) {
            if (DebugPrints) {
                Serial.println("Checksum is right:");
            }

            if (DebugPrints) {
                Serial.println("Printing what we are returning: ");
            }

            for (uint8_t i = 0; i < WVFP_TXRXDATA_SIZE; i++) {
                if (DebugPrints) {
                    Serial.print(String(_cmdReceiverBuffer[i+1],HEX) + ", ");
                }
                response[i] = _cmdReceiverBuffer[i+1]; // CHANGED THIS HERE
            }
            if (DebugPrints) Serial.println("EXITING RX successfully!");
            return true;
        } else {
            Serial.println("Invalid checksum");
        }
    }else {
        if (DebugPrints) {
            Serial.println("Command is wrong!");
        }
    }


    if (DebugPrints) Serial.println("EXITING RX unsuccessfully!");
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
    Serial.println("You are trying to enroll a user! ");
    for (int step = 1; step <= 3; step++) {
        if (DebugPrints) {
            Serial.println("Step "+String(step));
        }

        byte response[WVFP_TXRXBUFFER_SIZE-1];

        byte command[] = {(byte)step, 0, (byte)(id & 0xff00 >> 8),(byte)(id & 0x00ff),permission};

        if (txAndRxCmd(command,response,1000)) {
            if (DebugPrints) {
                Serial.print("Response: ");
                Serial.print(String(response[0],HEX) + ", ");
                Serial.print(String(response[1],HEX) + ", ");
                Serial.print(String(response[2],HEX) + ", ");
                Serial.println(String(response[3],HEX) + ", ");
            }
            switch (response[3]) {
                case WVFP_ACK_SUCCESS:
                    if (DebugPrints) {
                        Serial.println("Success on this step");
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
                default:
                    Serial.println("Here for some reason?");

            }

        }else {
            Serial.println("Failed at step 1");
            return false;
        }
    }

    Serial.println("Got to the end!");


    return true;
}
