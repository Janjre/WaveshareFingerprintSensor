#include <Arduino.h>

// Basic response message definition
#define WVFP_ACK_SUCCESS           0x00
#define WVFP_ACK_FAIL              0x01
#define WVFP_ACK_FULL              0x04
#define WVFP_ACK_NO_USER           0x05
#define WVFP_ACK_TIMEOUT           0x08
#define WVFP_ACK_GO_OUT            0x0F     // The center of the fingerprint is out of alignment with sensor

#define WVFP_ACK_USER_OCCUPIED 0x06 //The user was exist
#define WVFP_ACK_FINGER_OCCUPIED 0x07 //The fingerprint was exist
#define WVFP_ACK_TIMEOUT 0x08 //Time out

// User information definition
#define WVFP_ACK_ALL_USER          0x00
#define WVFP_ACK_GUEST_USER        0x01
#define WVFP_ACK_NORMAL_USER       0x02
#define WVFP_ACK_MASTER_USER       0x03

#define WVFP_USER_MAX_CNT          1000        // Maximum fingerprint number

// Command definition
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

#define WVFP_IMAGE_LENGTH          9800
#define WVFP_EIGENVALUE_LENGTH     193
#define WVFP_TXRXBUFFER_SIZE       8
#define WVFP_TXRXDATA_SIZE         4

void setup() {
    Serial.begin(115200);
    Serial.println("Serial is printing things!");
    pinMode(7,OUTPUT);
}

class FingerprintSensor {
public:
    int8_t RX;
    int8_t TX;
    int8_t WAKE;
    int8_t RST;
    long Baud;
    HardwareSerial *SensorSerial;


    FingerprintSensor(HardwareSerial &sr, const long baud, const int8_t rx,const int8_t tx,const int8_t wake,const int8_t rst)  {
        RX = rx;
        TX = tx;
        WAKE = wake;
        RST = rst;
        Baud = baud;
        SensorSerial = &sr;

        pinMode(WAKE,INPUT);
        pinMode(RST,OUTPUT);


    }

    void begin () {
        SensorSerial->begin(19200,SERIAL_8N1,RX,TX);
    }

    void clearTxBuffer() {
        for (uint8_t i=0; i<WVFP_TXRXBUFFER_SIZE; i++) {
            _cmdSendBuffer[i] = 0x00;
        }
    }

    void clearRxBuffer() {
        for (uint8_t i=0; i<WVFP_TXRXBUFFER_SIZE; i++) {
            _cmdReceiverBuffer[i] = 0x00;
        }
    }

    void shiftBuffer(byte buffer[], uint16_t bufferSize) {
        for (uint8_t i=1; i< bufferSize; i++) {
            buffer[i-1] = buffer[i];
        }
        buffer[bufferSize - 1] = 0x00;
    }

    void txCmd(byte commands[]) {
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
        Serial.println("About to sent: ");
        Serial.print(String(_cmdSendBuffer[0],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[1],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[2],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[3],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[4],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[5],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[6],HEX) + ",");
        Serial.print(String(_cmdSendBuffer[7],HEX) + ",");

        SensorSerial->write(_cmdSendBuffer, WVFP_TXRXBUFFER_SIZE);
    }

    void setSleepMode(bool sleep) {
        digitalWrite(RST, sleep ? LOW : HIGH);
        if (!sleep) {
            delay(10); // give time to wake up
        }
    }

    bool rxCmd(byte response[], byte command) {
        int cnt = 0;
        bool success = false;
        while (SensorSerial->available()) {
            Serial.println("Recieving things from the sensor");
            shiftBuffer(_cmdReceiverBuffer, WVFP_TXRXBUFFER_SIZE);
            _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 1] = SensorSerial->read();
            cnt++;



            // stop reading if there is a correct frame
            if (_cmdReceiverBuffer[0] == WVFP_CMD_HEAD && _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 1] == WVFP_CMD_TAIL){// && _cmdReceiverBuffer[1] == command) {

               Serial.print("Command recieved");
                success=true;
                break;
            }
        }

        Serial.println("Printing command recieve buffer 0:" + String(_cmdReceiverBuffer[0],HEX));
        Serial.println("Printing command recieve buffer 1:" + String(_cmdReceiverBuffer[1],HEX));
        Serial.println("Printing command recieve buffer 2:" + String(_cmdReceiverBuffer[2],HEX));
        Serial.println("Printing command recieve buffer 3:" + String(_cmdReceiverBuffer[3],HEX));
        Serial.println("Printing command recieve buffer 4:" + String(_cmdReceiverBuffer[4],HEX));
        Serial.println("Printing command recieve buffer 5:" + String(_cmdReceiverBuffer[5],HEX));
        Serial.println("Printing command recieve buffer 6:" + String(_cmdReceiverBuffer[6],HEX));
        Serial.println("Printing command recieve buffer 7:" + String(_cmdReceiverBuffer[7],HEX));


        if (_cmdReceiverBuffer[0] == WVFP_CMD_HEAD && _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 1] == WVFP_CMD_TAIL && success) {
            byte checksum = 0;
            for (uint8_t i = 1; i<6; i++) {
                checksum ^= _cmdReceiverBuffer[i];
            }

            // check if checksum is correct
            if (checksum == _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE - 2]) {
                for (uint8_t i = 0; i < WVFP_TXRXDATA_SIZE; i++) {
                    response[i] = _cmdReceiverBuffer[i+1];
                }
                return true;
            } else {
                Serial.println("invalid checksum");
            }
        }

        return false;
    }

    bool txAndRxCmd(byte commands[], byte response[], uint32_t timeout) { // addded driving wake pin

        setSleepMode(false);
        txCmd(commands);
        unsigned long waitUntil = millis() + timeout;
        bool success = false;
        Serial.println("Waiting");
        while(!success && millis() < waitUntil) {
            Serial.print(".");
            if (rxCmd(response,commands[1]) && response[0] == commands[0]) {
                success = true;
            }

            delay(10);
        }

        clearRxBuffer();
        setSleepMode(true);
        return success;
    }

    unsigned long getModelSN () {
        byte send[] = {0x2A,0,0,0};
        byte recieve[WVFP_TXRXBUFFER_SIZE-1];
        txAndRxCmd(send,recieve,1000);
        unsigned long combined = 0;
        combined = (unsigned long)recieve[1] << 16 | (unsigned long)recieve[2] << 8 |recieve[3];
        Serial.print("resp:");
        Serial.println(recieve[1]);
        Serial.println(recieve[2]);
        Serial.println(recieve[3]);


        return combined;
    }

    bool enrollUser (uint16_t id,byte permission) {

        for (int step = 1; step <= 3; step++) {
            Serial.println("Step "+String(step));
            byte response[WVFP_TXRXBUFFER_SIZE-1];
            byte command[] = {(byte)step, 0, (byte)(id & 0xff00 >> 8),(byte)(id & 0x00ff),permission};

            if (txAndRxCmd(command,response,1000)) {
                switch (response[4]) {
                    case WVFP_ACK_SUCCESS:
                        Serial.println("Success");
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

        Serial.println("Step2");
        Serial.println("Step2");
        //wait for things
        Serial.println("Step3");
        //wait for things

        return false;
    }


private:
    byte _cmdSendBuffer[WVFP_TXRXBUFFER_SIZE]; // Senden-Buffer
    byte _cmdReceiverBuffer[WVFP_TXRXBUFFER_SIZE]; // Empfangen-Buffer
};

void splitString(String str, char delimiter, String output[20]) {
    int StringCount = 0;

    while (str.length() > 0)
    {
        int index = str.indexOf(delimiter);
        if (index == -1) // No space found
        {
            output[StringCount++] = str;
            break;
        }
        else
        {
            output[StringCount++] = str.substring(0, index);
            str = str.substring(index+1);
        }
    }
}

 HardwareSerial serial = HardwareSerial(1);

FingerprintSensor Sensor(Serial1,19200,2,1,4,3);



void loop() {

    if (Serial.available() > 0) {
        Serial.println("SOMETHING HAS HA{P{ENDED");
        String input_str = Serial.readStringUntil('\n');
        input_str.trim();

        String args[20];
        splitString(input_str,' ',args);

        if (args[0] == "hello") {
            digitalWrite(7,HIGH);
            delay(1000);
            digitalWrite(7,LOW);

            Serial.println("Hello world!");

        } else if (args[0] == "begin") {
            Sensor.begin();
            Serial.println("Begun!");
        } else if (args[0] == "send_command") {
            Serial.println("Received input");
            byte commands[4];
            for (int i = 0; i < 4; i++) {
                commands[i] = (byte) strtol(args[i+1].c_str(), NULL, 16);
            }
            byte response[4];

            // print command
            Serial.print("Command:");
            for (int i = 0; i < 4; i++) {
                Serial.print(commands[i]);
            }

            if (Sensor.txAndRxCmd(commands, response, 1000)) {
                Serial.print("Response:");
                for (int i = 0; i < 4; i++) {
                    Serial.print(" ");
                    Serial.print(response[i], HEX);
                }
                Serial.println();
            } else {
                Serial.println("Command failed");
            }
        } else if (args[0] == "sleep_high"){
            digitalWrite(3,HIGH);
        }else if (args[0] == "sleep_low") {
            digitalWrite(3,LOW);
        }else if (args[0] == "get_SN") {
            Serial.print("Printing model SN ...");
            Serial.println("SN" + String(Sensor.getModelSN()));
        }else if (args[0] == "add_user"){
            auto user_id = static_cast<uint16_t>(strtol(args[1].c_str(), nullptr, 16));
            auto permission = static_cast<uint8_t>(strtol(args[2].c_str(), nullptr, 16));
            Sensor.enrollUser(user_id, permission);
        }else if (args[0] == "check") {
            byte command [] = {0x0C,0,0,0};
            byte response [WVFP_TXRXDATA_SIZE-1];
            Sensor.txAndRxCmd(command,response,1000);

            Serial.println("You are user " + String(uint16_t(response[1]) << 8 | uint16_t(response[2]),HEX));
            Serial.println("You have the permission of " + String(command[3],HEX));

        }
        else{
            Serial.println("Unknown command");
        }
    }



}