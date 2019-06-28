// Control of RockBLOCK to access the Iridium SatComm Network with Arduino
// Author: Raymond Yang
// Date: 20190627

/*
 * Error Codes:
 * ISBD_SUCCESS 0
 * ISBD_ALREADY_AWAKE 1
 * ISBD_SERIAL_FAILURE 2
 * ISBD_PROTOCOL_ERROR 3
 * ISBD_CANCELLED 4
 * ISBD_NO_MODEM_DETECTED 5
 * ISBD_SBDIX_FATAL_ERROR 6
 * ISBD_SENDRECEIVE_TIMEOUT 7
 * ISBD_RX_OVERFLOW 8
 * ISBD_REENTRANT 9
 * ISBD_IS_ASLEEP 10
 * ISBD_NO_SLEEP_PIN 11
 * ISBD_NO_NETWORK 12
 * ISBD_MSG_TOO_LONG 13
 */


#include <SoftwareSerial.h>
#include <IridiumSBD.h>
#include <time.h>

#define DIAGNOSTICS true
#define SLEEP_PIN 4

// command states
#define ENTER_CMD 0
#define EXIT -1
#define SLEEP_MODEM -2
#define GET_SYSTEM_TIME -3
#define WAKE_MODEM -4

SoftwareSerial rbserial(10,11); // 10 is RX and 11 is TX
IridiumSBD modem(rbserial,SLEEP_PIN); // declare IridiumSBD object

char msg[32]; // command message buffer
boolean data = false; // if there is serial data between Arduino and desktop
int fsmstate = 0; // user command

void setup() {
  Serial.begin(115200); // data rate for desktop - arduino
  while(!Serial){} // wait for serial to connect
  Serial.println("Desktop - Arduino Connection Established.");

  rbserial.begin(19200); // data rate for arduino - iridium serial port
  Serial.println("Arduino - Iridium Connection Established.");
  
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE); // low power profile

  Serial.println("Starting modem...");
  int status = modem.begin(); // wake modem
  if(status != ISBD_SUCCESS){
    Serial.print("Begin failed: error ");
    Serial.println(status);
    if(status == ISBD_NO_MODEM_DETECTED){
      Serial.println("No modem detected: check wiring.");
    }
    Serial.println("Program terminated.");
    return;
  } else {
    Serial.println("Modem Ready.\n");
  }
}

void loop() {

  while(fsmstate == ENTER_CMD){
    static int idx = 0; // index for string
    char rc; // character
    while(Serial.available() > 0 && data == false){
      rc = Serial.read(); // read character
      if(rc !='\n'){
        msg[idx] = rc;
        idx++;
        if(idx >= 32){
          idx = 31;
        }
      } else {
        msg[idx] = '\0';
        idx = 0;
        data = true;
      }
    }
    if(data == true){
      Serial.print("Command: ");
      Serial.println(msg);
      data = false;
      if(strcmp(msg,"sleep") == 0){
        fsmstate = SLEEP_MODEM;
      } else if(strcmp(msg,"getsystemtime") == 0){
        fsmstate = GET_SYSTEM_TIME;
      } else if(strcmp(msg,"start") == 0){
        fsmstate = WAKE_MODEM; 
      } else {
        fsmstate = ENTER_CMD;
        Serial.println("Command not recognized.");
      } 
    }
  }

  if(fsmstate == WAKE_MODEM){
    Serial.println("Starting modem...");
    int status = modem.begin(); // wake modem
    if(status != ISBD_SUCCESS){
      Serial.print("Begin failed: error ");
      Serial.println(status);
      if(status == ISBD_NO_MODEM_DETECTED){
        Serial.println("No modem detected: check wiring.");
      }
      Serial.println("Program terminated.");
      return;
    } else {
      Serial.println("Modem Ready.Enter command: \n");
    }
  } else if(fsmstate == SLEEP_MODEM){
    Serial.println("Putting modem to sleep...");
    int status = modem.sleep();
    if(status != ISBD_SUCCESS){
      Serial.print("Sleep failed: error ");
      Serial.println(status);
    } else {
      Serial.println("Modem is asleep.\n");
    }
  } else if(fsmstate == GET_SYSTEM_TIME){
    boolean network = false;
    int tries = 0;
    while(tries < 5 && network == false){
      struct tm t; // time structure to store time
      int status = modem.getSystemTime(t); // retrieve time
      if(status == ISBD_SUCCESS){
        char buffer[32]; // data buffer
        sprintf(buffer,"%d-%02d-%02d %02d:%02d:%02d",t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        Serial.print("Iridium time/date is: ");
        Serial.println(buffer);
        Serial.println("");
        network = true;
      } else if(status == ISBD_NO_NETWORK){
        Serial.println("No network detected. Waiting 10 seconds.");
        tries++;
        delay(10 * 1000UL); // 10 seconds
      } else {
        Serial.print("Unexpected error ");
        Serial.println(status);
        return;
      }
    }
  } else if(fsmstate == EXIT){
     Serial.println("Connection Terminated.\n");
     return;
  } else {
    // not going to happen
  }
  
  fsmstate = ENTER_CMD; // reset state
}

// retrieve system time
void getIridiumTime(IridiumSBD modem){
  boolean network = false;
  int tries = 0;
  while(tries < 5 && network == false){
    struct tm t; // time structure to store time
    int status = modem.getSystemTime(t); // retrieve time
    if(status == ISBD_SUCCESS){
      char buffer[32]; // data buffer
      sprintf(buffer,"%d-%02d-%02d %02d:%02d:%02d",t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
      Serial.print("Iridium time/date is: ");
      Serial.println(buffer);
      network = true;
    } else if(status == ISBD_NO_NETWORK){
      Serial.println("No network detected. Waiting 10 seconds.");
      tries++;
      delay(10 * 1000UL); // 10 seconds
    } else {
      Serial.print("Unexpected error ");
      Serial.println(status);
      return;
    }
  }
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}
#endif
