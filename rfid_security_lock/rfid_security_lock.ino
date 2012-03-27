/**
 * @author bhelx 2008
 * 
 * Using the Parallax RFID reader to create a simple 
 * servo-based security lock. Hardware includes:
 *   + Parallax RFID reader
 *   + Piezo Speaker
 *   + Servo 5.2 kg torque
 * 
 * Working on refactoring from crappy 2008 code.
 */
#include <EEPROM.h>

#define DEBUG 1

/** Pins **/
#define SPEAKER      3
#define RFID_ENABLE  2
#define BUTTON       5
#define SERVO        4

/** Hardware Constants **/
#define DEBOUNCE         200   // ms
#define LOCK_ANGLE       45    // give it some extra
#define KEY_WAIT         1500
#define CODE_LEN         10    // max length of RFID tag
#define VALIDATE_TAG     1     // should we validate tag?
#define VALIDATE_LENGTH  500   // maximum reads b/w tag read and validate
#define ITERATION_LENGTH 2000  // time, in ms, given to the user to move hand away
#define START_BYTE       0x0A 
#define STOP_BYTE        0x0D

/** Events **/
#define REAL_KEY   0
#define NOISE_KEY  1
#define MASTER_KEY 2

char current_key[10];  // holds the current key read
boolean master_known;  // is the master key known?
boolean door_locked;   // is the door locked or unlocked right now?
 
void setup() { 
  Serial.begin(2400);              // RFID reader SOUT pin connected to Serial RX pin at 2400bps 
  pinMode(SERVO, OUTPUT);
  digitalWrite(SERVO, LOW);        // disable right away
  pinMode(RFID_ENABLE, OUTPUT);    // Set digital pin 2 as OUTPUT [connected to RFID /ENABLE] 
  pinMode(SPEAKER, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(SERVO, OUTPUT);
  master_known = true;
  door_locked = true;
  clearCode();
  //clearEEPROM();                  
  setEEPROMIndex(11);               // start one after the master key
  resetRFID(false); 
  playMasterTone();
} 

void loop() {
  if (DEBUG) Serial.println("wait for evt");
  int evt = waitForEvent();
  boolean hard_reset = true;
  if (master_known) {
    switch (evt) {
      case REAL_KEY:
        if (DEBUG) { Serial.print(current_key); Serial.println("good key"); }
        playWelcomeTone();
        toggleLock();
        resetRFID(true);
        break;
      case MASTER_KEY:
        if (DEBUG) { Serial.print(current_key); Serial.println("good key"); }
        toggleLock();
        resetRFID(true);
        break;
      case NOISE_KEY:
        if (DEBUG) { Serial.print(current_key); Serial.println("bad key"); }
        resetRFID(false);
        break;
    }
  } else {
    Serial.println("masterizing");
    if (evt == REAL_KEY) {
      setCurrentAsMaster();
      if (DEBUG) { Serial.print(current_key); Serial.println("set as master"); }
      storeCurrentKey();
      playMasterTone();
      resetRFID(true);
    } else {
      if (DEBUG) { Serial.print(current_key); Serial.println("bad key"); }
      playRejectionTone();
      resetRFID(false);
    }
  }  
} 

/**************************************************************/
/********************* System Functions ***********************/
/**************************************************************/

int waitForEvent() {
  while (Serial.available() <= 0 && !checkButton()) {}
  getRFIDTag();
  if (validateCurrentKey()) {
    disableRFID();
    if (currentIsMaster()) return MASTER_KEY;
    return REAL_KEY; 
  }
  disableRFID();
  return NOISE_KEY;
}

/**
 * Resets system parameters. To be called before the system waits 
 * for input again.
 */
void resetRFID(boolean delayed) {
  disableRFID();
  clearCode();  // clear the code out
  if (delayed) delay(KEY_WAIT);
  realFlush();  // flush any unwanted bytes in serial buffer
  enableRFID();
}

/** 
 * Serial#flush not working for some reason.
 * This seems to do it.
 */
void realFlush() {
  while (Serial.available() > 0) Serial.read();
}

/**
 * This clears the space in memory where the current key is stored
 */ 
void clearCode() {
  for (int i = 0; i < 10; i++)
    current_key[i] = 0;
} 

/**
 * Checks to see if the lock/unlock button is pressed.
 */
boolean checkButton() {
  if (digitalRead(BUTTON) == HIGH) {  
    delay(DEBOUNCE);           // make sure this isn't a bounce
    return (digitalRead(BUTTON) == HIGH);
  }
  return false; 
}

/**************************************************************/
/***************** END System Functions ***********************/
/**************************************************************/



/**************************************************************/
/******************** Speaker Functions ***********************/
/**************************************************************/

/**
 * Plays the rejection tone.
 */
void playRejectionTone() {
  playTone(7000, 1500000);
}

/**
 * Plays the tone that welcomes a validated user into the door.
 */
void playWelcomeTone() {
  for(int i = 0; i < 3; i++) {
    playTone(2500, 100000);
    delay(10);
  }    
}

/**
 * Plays a special tone when configuration is changed.
 */
void playMasterTone() {
  for(int i = 0; i < 5; i++) {
    playTone(5000-(i*1000), 100000);
    delay(10);
  }  
}

/**
 * Play one tone for a certain duration.
 */
void playTone(int tone, long duration) {
  long elapsed_time = 0;
  int wave = 1;
  int half_tone = tone / 2;
  while (elapsed_time < duration) {
    digitalWrite(SPEAKER, wave);
    delayMicroseconds(half_tone);
    elapsed_time += (tone/2); wave = !wave;
  }                           
}

/**************************************************************/
/****************** END Speaker Functions *********************/
/**************************************************************/

/**************************************************************/
/********************* EEPROM Functions ***********************/
/**************************************************************/

/**
 * Returns the current index. The eeprom_index is stored at byte 0.
 */
int getEEPROMIndex() {
  return EEPROM.read(0);  
}

/**
 * Sets the current EEPROM index.
 */
void setEEPROMIndex(int new_index) {
  EEPROM.write(0, new_index);    
}

/**
 * Clears everything out of the internal EEPROM.
 */ 
void clearEEPROM() {
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0x00);
  }
}

/**
 * This function sets the current code as the master key.
 * It stores the master key in EEPROM index 1-10.
 */
void setCurrentAsMaster() {
  for (int i = 1; i < 11; i++) {          
    EEPROM.write(i, current_key[i]);   
  }
  master_known = true;
}

/**
 * Checks to see if the current key is the master key.
 */ 
boolean currentIsMaster() {
 for (int i = 1; i < 11; i++) {
   if (current_key[i] != EEPROM.read(i)) return false; 
 }
 return true;
}

/**
 * Stores the current key into the list of allowed keys in EEPROM.
 */ 
void storeCurrentKey() {
  int eeprom_index = getEEPROMIndex();
  for(int i = 0; i < 10; i++) {
    EEPROM.write(i+eeprom_index, current_key[i]);    
  }
  setEEPROMIndex(eeprom_index+10); 
}

/**
 * Checks the keys in EEPROM to see if current_key matches a valid key.
 */
boolean authenticateCurrentKey() {
  for (int key_index = 11; key_index <= getEEPROMIndex(); key_index += 10) {
    if (compareKeys(key_index)) return true;      // found a match
  }
  return false;  // no matches
}

/**
 * Compares one key with current key.
 */
boolean compareKeys(int key_index) {
  for (int j = 0; j < 10; j++) {
    if (EEPROM.read(j+key_index) != current_key[j]) return false;  // not a match  
  }  
  return true;   // all must have gone well
}

boolean isMasterSet() {
  for (int i = 1; i < 11; i++) {
    if (EEPROM.read(i) != 0x00) return true; 
  }
  return false;
}

/**************************************************************/
/*************** END EEPROM Functions *************************/
/**************************************************************/


/**************************************************************/
/********************* RFID Functions *************************/
/**************************************************************/

/**
 * Blocking function, waits for and gets the RFID tag.
 */
void getRFIDTag() {
  byte next_byte; 
  while (Serial.available() <= 0) {}
  if ((next_byte = Serial.read()) == START_BYTE) {      
    byte bytesread = 0; 
    while (bytesread < CODE_LEN) {
      if (Serial.available() > 0) {
         if((next_byte = Serial.read()) == STOP_BYTE) break;       
         current_key[bytesread++] = next_byte;                   
      }
    }
  }    
}

/**
 * Waits for the next incoming tag to see if it matches
 * the current tag.
 */
boolean validateCurrentKey() {
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
      if (Serial.available() > 0) {       
        if ((next_byte = Serial.read()) == STOP_BYTE) break;
        if (current_key[bytes_read++] != next_byte) return false;                     
      }
    }                
  }
  return true;   
}

void enableRFID() {
   digitalWrite(RFID_ENABLE, LOW);    
}
 
void disableRFID() {
   digitalWrite(RFID_ENABLE, HIGH);  
}

/**************************************************************/
/******************** END RFID Functions **********************/
/**************************************************************/

/**************************************************************/
/********************* Servo Functions ************************/
/**************************************************************/

/**
 * Toggles the lock.
 */
void toggleLock() {
  int pulse = (door_locked ? 2000 : 1000);
  for (int i = 0; i < LOCK_ANGLE; i++) {
    digitalWrite(SERVO, HIGH);   
    delayMicroseconds(pulse);  
    digitalWrite(SERVO, LOW);    
    delay(20);
  } 
  door_locked = !door_locked;  
}

/**************************************************************/
/******************* END Servo Functions **********************/
/**************************************************************/

