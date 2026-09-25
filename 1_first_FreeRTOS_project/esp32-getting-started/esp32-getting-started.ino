#define RGB_BUILTIN 38 
#define RGB_BRIGHTNESS 50 
 
void setup() { 
  // No separate initialisation is required for this example. 
} 
 
void loop() { 
  // Red ON 
  neopixelWrite(RGB_BUILTIN, 0, 50, 0); 
  delay(1000); 
 
  // LED OFF 
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); 
  delay(1000); 

    // blue ON 
  neopixelWrite(RGB_BUILTIN, 0, 0, 50); 
  delay(1000); 
 
  // LED OFF 
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); 
  delay(1000);
    // White ON 
  neopixelWrite(RGB_BUILTIN, 50, 50, 50); 
  delay(1000); 
 
  // LED OFF 
  neopixelWrite(RGB_BUILTIN, 0, 0, 0); 
  delay(1000); 
} 