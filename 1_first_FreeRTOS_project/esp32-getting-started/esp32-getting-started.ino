#define RGB_BUILTIN 38 
#define RGB_BRIGHTNESS 50 
 
void setup() { 
  Serial.begin(115200);
} 
 
void loop() { 
  // Red ON 
  neopixelWrite(RGB_BUILTIN, 59, 50, 0); 
  Serial.println("Blue");
  delay(200); 
 
  // LED OFF 
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); 
  delay(200); 

    // blue ON 
  neopixelWrite(RGB_BUILTIN, 0, 0, 50); 
  Serial.println("Green");
  delay(100); 
 
  // LED OFF 
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); 
  delay(100);
    // White ON 
  neopixelWrite(RGB_BUILTIN, 50, 50, 50); 
  Serial.println("White");
  delay(1000); 
 
  // LED OFF 
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); 
  delay(1000); 
} 