# ScreenUi
Automatically exported from code.google.com/p/screenui

```
// A Sketch demonstrating ScreenUi using a 20x4 character LCD for output and
// a rotary encoder with pushbutton for input. 

#include <pin.h>
#include <encoder.h>
#include <LiquidCrystalFP.h>
// This is the only required include for ScreenUi. 
// The others above are for a specific set of hardware input
// and output.
#include <ScreenUi.h>

// Set up the rotary encoder and LCD.
#define ENCODER_TYPE ALPS

#define ENCA_PIN 3
#define ENCB_PIN 2
#define ENTER_PIN 1
  
#define LCD_RS_PIN 4
#define LCD_ENABLE_PIN 23
#define LCD_DATA4_PIN 28
#define LCD_DATA5_PIN 29
#define LCD_DATA6_PIN 30
#define LCD_DATA7_PIN 31

#define LCD_BRIGHT_PIN 13
#define LCD_CONTRAST_PIN 14

LiquidCrystal lcd(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_DATA4_PIN, LCD_DATA5_PIN, LCD_DATA6_PIN, LCD_DATA7_PIN);

void setup() {
  // Everything here is for setting up the hardware. Nothing ScreenUi specific here.
  lcd.begin(20, 4);
  TCCR2B = 0x01;
  pinMode(LCD_BRIGHT_PIN, OUTPUT);
  pinMode(LCD_CONTRAST_PIN, OUTPUT);
  setBright(128);
  setContrast(98);
  Encoder.begin(ENCODER_TYPE, ENTER_PIN, ENCA_PIN, ENCB_PIN);
  Encoder.setWrap(true);
  Encoder.setMin(-10000);
  Encoder.setMax(10000);
  Encoder.setCount(0);
  Serial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.println();
}

// The loop() function is where all the ScreenUi magic happens. We'll create a
// Screen, display it to the user, allow the user to interact and we'll take
// input from the Components.
void loop() {
  // Create a new Screen with width 20, height 4.
  Screen screen(20, 4);
  
  Label lbls[32];
  
  // Some static text that will be at the top of the screen.
  Label titleLabel("RGB Settings");
  
  // An Input field and a Label to describe it. We pass in the text "0xffee" and
  // any changes the user makes to that text will be reflected in the address 
  // variable.
  Label addressLabel("Address:");
  char *address = "0xffee";
  Input addressInput(address);

  // A List field and a Label to describe it. We add three items to the List for
  // the user to select from.
  Label colorLabel("Color:");
  List colorList(7);
  colorList.addItem("Red");
  colorList.addItem("Orange");
  colorList.addItem("Yellow");

  // A Checkbox field and a Label to describe it.
  Label rgbEnabledLabel("RGB Enabled:");
  Checkbox rgbEnabledCheckbox;
  
  Label brightnessLabel("Brightness:");
  Spinner brightnessSpinner(128, 0, 255, 1, true);
  
  Label contrastLabel("Contrast:");
  Spinner contrastSpinner(128, 0, 255, 1, true);
  
  // A ScrollContainer to allow scrolling through multiple Components. Since our
  // screen is only 4 lines high but we want to show 5 lines of Components, we add
  // three of the widgets to the ScrollContainer. The ScrollContainer will appear
  // in the middle two lines of the display and allow the user to scroll through
  // as many Components as we like.
  // This line creates the ScrollContainer, passing the screen it will be attached
  // to and the width and height for the new ScrollContainer.
  ScrollContainer scrollContainer(&screen, screen.width(), 2);
  // Add the Components to the ScrollContainer, setting their position within it.
  scrollContainer.add(&addressLabel, 0, 0);
  scrollContainer.add(&addressInput, 8, 0);
  scrollContainer.add(&colorLabel, 0, 1);
  scrollContainer.add(&colorList, 6, 1);
  scrollContainer.add(&rgbEnabledLabel, 0, 2);
  scrollContainer.add(&rgbEnabledCheckbox, 12, 2);
  scrollContainer.add(&brightnessLabel, 0, 3);
  scrollContainer.add(&brightnessSpinner, 11, 3);
  scrollContainer.add(&contrastLabel, 0, 4);
  scrollContainer.add(&contrastSpinner, 9, 4);

  // A simple Cancel button.
  Button cancelButton("Cancel");

  // A Simple Ok button.
  Button okButton("Ok");
 
  // Add the title Label, the ScrollContainer and the Cancel and Ok buttons to the
  // screen.
  screen.add(&titleLabel, 0, 0);
  screen.add(&scrollContainer, 0, 1);
  screen.add(&cancelButton, 0, 3);
  screen.add(&okButton, 16, 3);

  // Start processing the Screen in a loop. 
  while (1) {
    // screen.update() tells the Screen to display itself, accept input and manage
    // it's resources. 
    screen.update();
    // After calling screen.update(), all of the Components have been updated and drawn
    // to the Screen and their inputs are now available for querying.
    setBright(brightnessSpinner.intValue());
    setContrast(contrastSpinner.intValue());
    if (okButton.pressed()) {
      // Do some work
    }
  }
}

// The next 8 methods are required to be implemented by the user of ScreenUi. These
// methods are what tie ScreenUi to your specific hardware for input and output.
// In general, these will be very similar across platforms and can probably be copied
// from one program to another and slightly modified.

// User defined method that receives input from the input method. ScreenUi calls
// this method during each update to see how the input state has changed since
// the last update. ScreenUi expects the function to fill in the values
// for x, y, selected and cancelled.
// x and y are the number of inputs in either the x or y axis since the last call
// to this method. The values can be positive or negative. A common control scheme
// for a rotary encoder would be negative y for left, positive y for right. For
// an input method consisting of the buttons on a NES control pad, for instance, might
// have the D pad control x and y, the A button control selected and the B button
// control cancelled.
void Screen::getInputDeltas(int *x, int *y, bool *selected, bool *cancelled) {
  *x = 0;
  *y = Encoder.getDelta();
  *selected = Encoder.ok();
  *cancelled = Encoder.cancel();
  Encoder.setCount(0);
}

// User defined method that clears the output device completely.
void Screen::clear() {
  lcd.clear();
}  

// User defined method that creates a custom character in font memory. This is
// currently used by the Checkbox Component to create a nice check mark.
void Screen::createCustomChar(uint8_t slot, uint8_t *data) {
  lcd.createChar(slot, data);
}

// User defined method that draws the given text at the given x and y position.
// The text should be drawn exactly as specified with no interpretation, scrolling
// or wrapping. 
void Screen::draw(uint8_t x, uint8_t y, const char *text) {
  lcd.setCursor(x, y);
  lcd.print(text);
}

// User defined method that draws the given custom character at the given x
// and y position. The custom character will be one specified to the
// Screen::createCustomChar() method.
void Screen::draw(uint8_t x, uint8_t y, uint8_t customChar) {
  lcd.setCursor(x, y);
  lcd.write(customChar);
}

// User defined method that turns the character cursor on or off.
void Screen::setCursorVisible(bool visible) {
  visible ? lcd.cursor() : lcd.noCursor();
}

// User defined method positions the character cursor.
void Screen::moveCursor(uint8_t x, uint8_t y) {
  lcd.setCursor(x, y);
}

// User defined method that turns the blinking character on or off.
void Screen::setBlink(bool blink) {
  blink ? lcd.blink() : lcd.noBlink();
}

// Utility function for setting the brightness of the LCD. Not required for ScreenUi.
void setBright(byte val) {
  analogWrite(LCD_BRIGHT_PIN, 255 - val);
}

// Utility function for setting the contrast of the LCD. Not required for ScreenUi.
void setContrast(byte val) {
  analogWrite(LCD_CONTRAST_PIN, val);
}
```
