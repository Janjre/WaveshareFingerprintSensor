#include <Arduino.h>
#include <FingerprintSensor.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Serial is printing things!");
    pinMode(7,OUTPUT);
}

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

FingerprintSensor Sensor(Serial1,19200,2,1,4,3); // 2 = RX, 1 = TX, 4 = WAKE, 3 = RST

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
            Serial.println("You have the permission of " + String(response[3],HEX));

        }
        else{
            Serial.println("Unknown command");
        }
    }
}