// Control of RockBLOCK to access the Iridium SatComm Network with Arduino
// Author: Raymond Yang
// Date: 20190627
// Version 1.4

/*
   Commands:
   1. wake
   2. sleep
   3. systime
   4. sendgps
   5. signal
   6. exit
   
   Error Codes:
   ISBD_SUCCESS 0
   ISBD_ALREADY_AWAKE 1
   ISBD_SERIAL_FAILURE 2
   ISBD_PROTOCOL_ERROR 3
   ISBD_CANCELLED 4
   ISBD_NO_MODEM_DETECTED 5
   ISBD_SBDIX_FATAL_ERROR 6
   ISBD_SENDRECEIVE_TIMEOUT 7
   ISBD_RX_OVERFLOW 8
   ISBD_REENTRANT 9
   ISBD_IS_ASLEEP 10
   ISBD_NO_SLEEP_PIN 11
   ISBD_NO_NETWORK 12
   ISBD_MSG_TOO_LONG 13
*/

#include <SoftwareSerial.h>
#include <IridiumSBD.h>
#include <time.h>
#include <TinyGPS++.h>

#define DIAGNOSTICS true
#define SLEEP_PIN 4

// command states
#define ENTER_CMD 0
#define EXIT -1
#define SLEEP_MODEM -2
#define GET_SYSTEM_TIME -3
#define WAKE_MODEM -4
#define GET_SIGNAL_QUALITY -5
#define SEND_GPS -6

SoftwareSerial rbserial(10, 11); // 10 is RX and 11 is TX
SoftwareSerial gpsserial(3, 4); // 3 is RX and 4 is TX
IridiumSBD modem(rbserial, SLEEP_PIN); // declare IridiumSBD object
TinyGPSPlus gps; // gps object

static int fsmstate = 0; // user command

void setup() {
  Serial.begin(115200); // data rate for desktop - arduino
  while (!Serial) {} // wait for serial to connect
  Serial.println("Desktop - Arduino Connection Established.");

  rbserial.begin(19200); // data rate for arduino - iridium serial port
  Serial.println("Arduino - Iridium Connection Established.");

  gpsserial.begin(9600); // data rate for arduino - gps serial port
  Serial.println("Arduino - GPS Connection Established.");

  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE); // low power profile

  Serial.println("Starting modem...");
  rbserial.listen(); // listen to RockBLOCK
  int status = modem.begin(); // wake modem
  if (status != ISBD_SUCCESS) {
    Serial.print("Begin failed: error ");
    Serial.println(status);
    if (status == ISBD_NO_MODEM_DETECTED) {
      Serial.println("No modem detected: check wiring.");
    }
    Serial.println("Program terminated.\n");
    delay(500UL);
    exit(0);
  } else {
    Serial.println("Modem Ready.\n");
  }
}

void loop() {

  char msg[32]; // command message buffer
  boolean data = false; // if there is serial data between Arduino and desktop

  while (fsmstate == ENTER_CMD) {
    static int idx = 0; // index for string
    char rc; // character
    while (Serial.available() > 0 && data == false) {
      rc = Serial.read(); // read character
      if (rc != '\n') {
        msg[idx] = rc;
        idx++;
        if (idx >= 32) {
          idx = 31;
        }
      } else {
        msg[idx] = '\0';
        idx = 0;
        data = true;
      }
    }
    if (data == true) {
      Serial.print("Command: ");
      Serial.println(msg);
      data = false;
      if (strcmp(msg, "sleep") == 0) {
        fsmstate = SLEEP_MODEM;
      } else if (strcmp(msg, "systime") == 0) {
        fsmstate = GET_SYSTEM_TIME;
      } else if (strcmp(msg, "wake") == 0) {
        fsmstate = WAKE_MODEM;
      } else if (strcmp(msg, "exit") == 0) {
        fsmstate = EXIT;
      } else if (strcmp(msg, "signal") == 0) {
        fsmstate = GET_SIGNAL_QUALITY;
      } else if (strcmp(msg, "sendgps") == 0) {
        fsmstate = SEND_GPS;
      } else {
        fsmstate = ENTER_CMD;
        Serial.println("Command not recognized.");
      }
    }
  }

  if (fsmstate == WAKE_MODEM) {
    Serial.println("Starting modem...");
    int status = modem.begin(); // wake modem
    if (status != ISBD_SUCCESS) {
      Serial.print("Begin failed: error ");
      Serial.println(status);
      if (status == ISBD_NO_MODEM_DETECTED) {
        Serial.println("No modem detected: check wiring.\n");
      }
    } else {
      Serial.println("Modem Ready.\n");
    }
  } else if (fsmstate == SLEEP_MODEM) {
    Serial.println("Putting modem to sleep...");
    int status = modem.sleep();
    if (status != ISBD_SUCCESS) {
      Serial.print("Sleep failed: error ");
      Serial.println(status);
    } else {
      Serial.println("Modem is asleep.\n");
    }
  } else if (fsmstate == GET_SYSTEM_TIME) {
    boolean network = false;
    int tries = 0;
    while (tries < 5 && network == false) {
      struct tm t; // time structure to store time
      int status = modem.getSystemTime(t); // retrieve time
      if (status == ISBD_SUCCESS) {
        char buffer[32]; // data buffer
        sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        Serial.print("Iridium time/date is: ");
        Serial.println(buffer);
        Serial.println();
        network = true;
      } else if (status == ISBD_NO_NETWORK) {
        Serial.println("No network detected. Waiting 10 seconds.");
        tries++;
        delay(10 * 1000UL); // 10 seconds
      } else {
        Serial.print("Unexpected error ");
        Serial.println(status);
        Serial.println();
      }
    }
  } else if (fsmstate == GET_SIGNAL_QUALITY) {
    // returns a number between 0 and 5
    // 2 or better is preferred
    int signalQuality = -1;
    int status = modem.getSignalQuality(signalQuality);
    if (status != ISBD_SUCCESS) {
      Serial.print("SignalQuality failed: error ");
      Serial.println(status);
    } else {
      Serial.print("Signal Quality (0~5): ");
      Serial.println(signalQuality);
      Serial.println();
    }
  } else if (fsmstate == EXIT) {
    Serial.println("Connection Terminated.\n");
    delay(500UL);
    exit(0);
  } else if (fsmstate == SEND_GPS) {
//    gpsserial.listen(); // listen to GPS
//    // listen for gps traffic
//    unsigned long loopStartTime = millis(); // start time
//    Serial.println("Beginning to listen for GPS traffic...");
//    while ((!gps.location.isValid() || !gps.date.isValid()) && millis() - loopStartTime < 7UL * 60UL * 1000UL) {
//      if (gpsserial.available()) {
//        gps.encode(gpsserial.read());
//      }
//    }
//
//    // check for GPS fix
//    while (!gps.location.isValid()) {
//      Serial.println("Could not get GPS fix.\n");
//      delay(1000UL);
//    }
//    Serial.println("A GPS fix was found!\n");
//
//    char outBuffer[60];
//    sprintf(outBuffer, "%d%02d%02d%02d%02d%02d,%s%u.%09lu,%s%u.%09lu,%lu,%ld",
//            gps.date.year(),
//            gps.date.month(),
//            gps.date.day(),
//            gps.time.hour(),
//            gps.time.minute(),
//            gps.time.second(),
//            gps.location.rawLat().negative ? "-" : "",
//            gps.location.rawLat().deg,
//            gps.location.rawLat().billionths,
//            gps.location.rawLng().negative ? "-" : "",
//            gps.location.rawLng().deg,
//            gps.location.rawLng().billionths,
//            gps.speed.value() / 100,
//            gps.course.value() / 100
//           );
//
//    Serial.print("Transmitting message '");
//    Serial.print(outBuffer);
//    Serial.println("'");

    rbserial.listen(); // listen to RockBLOCK
    int status = modem.sendSBDText("hello");
    if (status != ISBD_SUCCESS) {
      Serial.print("Transmission failed with error code ");
      Serial.println(status);
      Serial.println();
    } else {
      Serial.println("Message sent successfully.\n");
    }
  } else {
    // not going to happen
  }

  fsmstate = ENTER_CMD; // reset state
}

// retrieve system time
void getIridiumTime(IridiumSBD modem) {
  boolean network = false;
  int tries = 0;
  while (tries < 5 && network == false) {
    struct tm t; // time structure to store time
    int status = modem.getSystemTime(t); // retrieve time
    if (status == ISBD_SUCCESS) {
      char buffer[32]; // data buffer
      sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
      Serial.print("Iridium time/date is: ");
      Serial.println(buffer);
      network = true;
    } else if (status == ISBD_NO_NETWORK) {
      Serial.println("No network detected. Waiting 5 seconds.");
      tries++;
      delay(5 * 1000UL); // 5 seconds
    } else {
      Serial.print("Unexpected error ");
      Serial.println(status);
      Serial.println();
    }
  }

  if(tries >= 4){
    Serial.println("Maximum tries reached. Failed to retrieve system time.");
    Serial.println("Modem Ready.\n");
  }
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD * device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD * device, char c)
{
  Serial.write(c);
}
#endif
