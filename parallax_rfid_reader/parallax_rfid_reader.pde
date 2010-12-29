/**
 * @author Benjamin Eckel 2009
 *
 * This code can be used to interface to the Parallax RFID
 * reader. More info and schematics here:
 * 
 * http://www.gumbolabs.org/2009/10/17/parallax-rfid-reader-arduino/
 */
#define RFID_ENABLE 2   //to RFID ENABLE
#define CODE_LEN 10      //Max length of RFID tag
#define VALIDATE_TAG 1  //should we validate tag?
#define VALIDATE_LENGTH  200 //maximum reads b/w tag read and validate
#define ITERATION_LENGTH 2000 //time, in ms, given to the user to move hand away
#define START_BYTE 0x0A 
#define STOP_BYTE 0x0D
 
char tag[CODE_LEN];  
 
void setup() { 
  Serial.begin(2400);  
  pinMode(RFID_ENABLE,OUTPUT);   
}
 
void loop() { 
  enableRFID(); 
  getRFIDTag();
  if(isCodeValid()) {
    disableRFID();
    sendCode();
    delay(ITERATION_LENGTH);
  } else {
    disableRFID();
    Serial.println("Got some noise");  
  }
  Serial.flush();
  clearCode();
} 
 
/**
 * Clears out the memory space for the tag to 0s.
 */
void clearCode() {
  for(int i=0; i<CODE_LEN; i++) {
    tag[i] = 0; 
  }
}
 
/**
 * Sends the tag to the computer.
 */
void sendCode() {
    Serial.print("TAG:");  
    //Serial.println(tag);
    for(int i=0; i<CODE_LEN; i++) {
      Serial.print(tag[i]); 
    } 
}
 
/**************************************************************/
/********************   RFID Functions  ***********************/
/**************************************************************/
 
void enableRFID() {
   digitalWrite(RFID_ENABLE, LOW);    
}
 
void disableRFID() {
   digitalWrite(RFID_ENABLE, HIGH);  
}
 
/**
 * Blocking function, waits for and gets the RFID tag.
 */
void getRFIDTag() {
  byte next_byte; 
  while(Serial.available() <= 0) {}
  if((next_byte = Serial.read()) == START_BYTE) {      
    byte bytesread = 0; 
    while(bytesread < CODE_LEN) {
      if(Serial.available() > 0) { //wait for the next byte
          if((next_byte = Serial.read()) == STOP_BYTE) break;       
          tag[bytesread++] = next_byte;                   
      }
    }                
  }    
}
 
/**
 * Waits for the next incoming tag to see if it matches
 * the current tag.
 */
boolean isCodeValid() {
  byte next_byte; 
  int count = 0;
  while (Serial.available() < 2) {  //there is already a STOP_BYTE in buffer
    delay(1); //probably not a very pure millisecond
    if(count++ > VALIDATE_LENGTH) return false;
  }
  Serial.read(); //throw away extra STOP_BYTE
  if ((next_byte = Serial.read()) == START_BYTE) {  
    byte bytes_read = 0; 
    while (bytes_read < CODE_LEN) {
      if (Serial.available() > 0) { //wait for the next byte      
          if ((next_byte = Serial.read()) == STOP_BYTE) break;
          if (tag[bytes_read++] != next_byte) return false;                     
      }
    }                
  }
  return true;   
}
