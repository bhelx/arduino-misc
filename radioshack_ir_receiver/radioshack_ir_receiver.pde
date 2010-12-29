/**
 * @author Benjamin Eckel 2010
 *
 * This code reads IR signals following Sony's 12 bit protocol.
 * The explanation/schematics can be found here:
 * 
 * http://www.gumbolabs.org/2010/05/29/radioshack-infrared-receiver-arduino/ 
 */
 
#define BITS_PER_MESSAGE 12    
#define B_1 1200              //1.2 milliseconds        
#define B_0 600                //0.6 milliseconds       
#define START_BIT 2000   //making it a little shorter than the actual 2400   
#define IR_PIN 2
 
//find the threshold time b/w the two pulse sizes
#define BIT_THRESH B_0+((B_1-B_0)/2)
 
void setup() {
  pinMode(IR_PIN, INPUT);
  delay(1000);
  Serial.begin(9600);
  Serial.println("Ready...");
}
 
void loop() {
  //int where we are storing code, must start at 0
  unsigned int code = 0x00;
 
  Serial.println("Waiting for first pulse");
 
  //block until start pulse
  while (pulseIn(IR_PIN, LOW) < START_BIT) {}
 
  //read in next 12 bits shifting and masking them into the unsigned int 'code'
  //remeber boolean is basically 0 [false] or 1 [true]
  for (byte i = 0; i < BITS_PER_MESSAGE; i++) {
      code |= (pulseIn(IR_PIN, LOW) > BIT_THRESH) << i;  
  }
 
  Serial.println(code);
  delay(500);
}
