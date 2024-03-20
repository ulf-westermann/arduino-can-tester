#include "AsyncDelay.h"
#include "SPI.h"
#include "mbed.h"
#include "mcp_can.h"


#define CAN_INT 9


MCP_CAN can(10); // CS is pin 10
static bool pause = false;
static AsyncDelay can_tx_timer;
static AsyncDelay can_error_timer;
static uint8_t tx_data_bytes[8] = {'h', 'e', 'l', 'l', 'o'};
static uint32_t tx_data_length = 5;
static uint32_t txId = 0x2a;


void setup() {
    Serial.begin(1152000);

    Serial.setTimeout(60 * 1000);

    while (!Serial)
        ;

    if (can.begin(MCP_ANY, CAN_125KBPS, MCP_16MHZ) == CAN_OK) {
        Serial.println("can.begin() ok");
    } else {
        Serial.println("can.begin() failed");
        return;
    }

    // since we do not set NORMAL mode, we are in loopback mode by default.
    can.setMode(MCP_NORMAL);

    pinMode(CAN_INT, INPUT); // configuring pin for /INT input

    can_tx_timer.start(1000, AsyncDelay::MILLIS);
    can_error_timer.start(1000, AsyncDelay::MILLIS);
}


void loop() {
    handle_inputs();
    handle_message_tx();
    handle_message_rx();
    handle_errors();
}


void handle_errors() {
    if (can_tx_timer.isExpired()) {
        can_tx_timer.repeat();

        uint8_t eflg = can.getError();

        if (eflg) {
            Serial.print("error eflg register: ");
            Serial.println(eflg);
        }

        uint8_t rx_error_count = can.errorCountRX();

        if (rx_error_count) {
            Serial.print("rx error count: ");
            Serial.println(rx_error_count);
        }

        uint8_t tx_error_count = can.errorCountTX();

        if (tx_error_count) {
            Serial.print("tx error count: ");
            Serial.println(tx_error_count);
        }
    }
}


static void handle_inputs(void) {
    int c = Serial.read();

    switch (c) {
        case -1: {
            break;
        }

        case '?': {
            Serial.println("?=help, p=pause, s=send");
            break;
        }

        case 'p': {
            pause = pause ? false : true;

            if (pause) {
                Serial.println("paused...");
            } else {
                Serial.println("...resume");
            }
            break;
        }

        case 's': {
            Serial.print("can id (hex): ");
            String tx_id_string = Serial.readStringUntil('\r');
            sscanf(tx_id_string.c_str(), "%u", &txId);

            Serial.print("\r\ndata length:");
            String data_length_string = Serial.readStringUntil('\r');
            sscanf(data_length_string.c_str(), "%u", &tx_data_length);

            String data_bytes_string;

            for (int i = 0; i < tx_data_length; i++) {
                Serial.print("\r\ndata byte");
                Serial.print(i);
                Serial.print(" (hex): ");

                data_bytes_string = Serial.readStringUntil('\r');
                sscanf(data_bytes_string.c_str(), "%hhx", &tx_data_bytes[i]);
            }

            Serial.print("\r\nrepeat millis: ");
            String repeat_millis_string = Serial.readStringUntil('\r');
            uint32_t repeat_millis;
            sscanf(repeat_millis_string.c_str(), "%u", &repeat_millis);

            can_tx_timer.start(repeat_millis, AsyncDelay::MILLIS);

            break;
        }

        default: {
            Serial.println("unknown command");
        }
    }
}


static void handle_message_tx(void) {
    if (pause) {
        return;
    }

    if (can_tx_timer.isExpired()) {
        can_tx_timer.repeat();

        byte result = can.sendMsgBuf(txId, 0, tx_data_length, tx_data_bytes);

        if (result == CAN_OK) {
            char msg_string[128];
            sprintf(msg_string, "TX std: 0x%.3lX, dlc: %1d, data:", txId, tx_data_length);

            Serial.print(msg_string);

            for (byte i = 0; i < tx_data_length; i++) {
                sprintf(msg_string, " 0x%.2X", tx_data_bytes[i]);
                Serial.print(msg_string);
            }

            Serial.println();
        } else {
            Serial.print("can.sendMsgBuf() failed (");
            Serial.print(result);
            Serial.print("). error: 0x");
            Serial.println(can.getError(), 16);
        }
    }
}


static void handle_message_rx(void) {
    if (pause) {
        return;
    }

    if (CAN_MSGAVAIL == can.checkReceive()) {
        char msg_string[128];
        long unsigned int rx_id;
        unsigned char rx_data_length;
        unsigned char rx_data_bytes[8];

        can.readMsgBuf(&rx_id, &rx_data_length, rx_data_bytes); // read data: rx_data_length = data length, buf = data byte(s)

        if ((rx_id & 0x80000000) == 0x80000000) // determine if ID is standard (11 bits) or extended (29 bits)
            sprintf(msg_string, "RX ext: 0x%.8lX, dlc: %1d, data:", (rx_id & 0x1FFFFFFF), rx_data_length);
        else
            sprintf(msg_string, "RX std: 0x%.3lX, dlc: %1d, data:", rx_id, rx_data_length);

        Serial.print(msg_string);

        if ((rx_id & 0x40000000) == 0x40000000) { // determine if message is a remote request frame.
            sprintf(msg_string, "RX REMOTE REQUEST FRAME");
            Serial.print(msg_string);
        } else {
            for (byte i = 0; i < rx_data_length; i++) {
                sprintf(msg_string, " 0x%.2X", rx_data_bytes[i]);
                Serial.print(msg_string);
            }
        }

        Serial.println();
    }
}
