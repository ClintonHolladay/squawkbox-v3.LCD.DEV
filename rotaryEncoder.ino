 /*
  Rotary Encoder Demo
  rot-encode-demo.ino
  Demonstrates operation of Rotary Encoder
  Displays results on Serial Monitor
  DroneBot Workshop 2019
  https://dronebotworkshop.com
*/
/* Read Quadrature Encoder
   Connect Encoder to Pins encoder0PinA, encoder0PinB, and +5V.

   Sketch by max wolf / www.meso.net
   v. 0.1 - very basic functions - mw 20061220

*/

int val;
int encoder0PinA = 4;
int encoder0PinB = 3;
int encoder0Pos = 0;
int encoder0PinALast = LOW;
int n = LOW;
int pushButton = 5;

void setup() 
{
  pinMode (encoder0PinA, INPUT);
  pinMode (encoder0PinB, INPUT);
  pinMode (pushButton, INPUT_PULLUP);
  digitalWrite (pushButton, HIGH);
  Serial.begin (9600);
}

void loop() {
  
  n = digitalRead(encoder0PinA);
  if ((encoder0PinALast == LOW) && (n == HIGH)) 
  {
    if (digitalRead(encoder0PinB) == LOW) 
    {
      encoder0Pos--;
    } 
    else 
    {
      encoder0Pos++;
    }
    Serial.println (encoder0Pos);
  }
  encoder0PinALast = n;
  
  if(digitalRead(pushButton) == LOW)
  {
    Serial.println ("Hello World");
  }
}
