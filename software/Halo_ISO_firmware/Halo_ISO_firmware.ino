// 1. Make sure you have installed "ch55xduino" from: https://github.com/DeqingSun/ch55xduino
// 2. Seelct "Tools -> Board -> CH55xDuino MC551 plain C core (non-C++) -> CH552 Board"
// 3. Select following option:
//   - Clock Source:  16 MHz (internal), 3.3V or 5V
//   - Upload Method: USB
//   - USB Settings:  USER CODE /w 266B USB RAM
// 4. Press button and connect USB cable.
// 5. Click "Upload" to upload program.

#ifndef USER_USB_RAM
#error "This firmware needs to be compiled with a USER USB setting"
#endif

#include "src\userUsbHidKeyboard\USBHIDKeyboard.h"
#include "src\WS2812\WS2812.h"

//Pin mapping
#define PIN_NEO 14
#define PIN_KEY 15
#define PIN_LED 16

//button states
bool PrevState = false;
bool CurrState = false;

//RGB LED
#define NUM_BYTES 3
uint8_t hue = 191; //0...191
uint8_t val = 2; //0...2
__xdata uint8_t ledData[NUM_BYTES];
bool LEDstate = true;

//EEPROM
uint8_t eepromData = 0;
uint8_t eepromAddr = 63;

void bootloaderRoutine() {
  USB_CTRL = 0;
  EA = 0;  //Disabling all interrupts is required.
  TMOD = 0;
  __asm__("lcall #0x3800");  //Jump to bootloader code
}

void NEO_writeHue(uint8_t hue, uint8_t bright) {
  uint8_t phase = hue >> 6;
  uint8_t step = (hue & 63) << bright;
  uint8_t nstep = (63 << bright) - step;
  switch (phase) {
    case 0:
      set_pixel_for_GRB_LED(ledData, 0, nstep, step, 0);
      break;
    case 1: 
      set_pixel_for_GRB_LED(ledData, 0, 0, nstep, step); 
      break;
    case 2: 
      set_pixel_for_GRB_LED(ledData, 0, step, 0, nstep);
      break;
    default: break;
  }
  neopixel_show_P1_4(ledData, NUM_BYTES);
}

void setup() {
  //Start HID keyboard devoce
  USBInit();

  eepromData = eeprom_read_byte(eepromAddr);

  //Pin input/output
  pinMode(PIN_KEY, INPUT_PULLUP);
  pinMode(PIN_NEO, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_LED, LOW);

  //Enter bootloader if encoder switch is pressed
  if (!digitalRead(PIN_KEY)) {
    set_pixel_for_GRB_LED(ledData, 0, 0, 255, 0);
    neopixel_show_P1_4(ledData, NUM_BYTES);

    //Change +Gaming+ setting
    if (eepromData == 0){
      eeprom_write_byte(eepromAddr, 1);
    }else{
      eeprom_write_byte(eepromAddr, 0);
    }
    delay(5000);

    //Enter bootloader
    bootloaderRoutine();
  }
}

void loop() {
  CurrState = digitalRead(PIN_KEY);
  digitalWrite(PIN_LED, !CurrState);

  if (PrevState != CurrState) {
    PrevState = CurrState;
    if (!CurrState) {
      Keyboard_press(KEY_RETURN);
      delay(10);
      Keyboard_releaseAll();
    }
  }
  delay(5);  //naive debouncing

  //Update RGB LED
  if(eepromData == 1){
    NEO_writeHue(hue, val);
    if (hue == 0) {
      hue = 191;
    } else {
      hue--;
    }
  }
}