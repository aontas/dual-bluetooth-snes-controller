
enum button_name {
  buttonA      = 1<<0,
  buttonB      = 1<<1,
  buttonX      = 1<<2,
  buttonY      = 1<<3,
  buttonStart  = 1<<4,
  buttonSelect = 1<<5,
  buttonL      = 1<<6,
  buttonR      = 1<<7,
};

enum dpad_name {
  up         = 1<<0,
  down       = 1<<1,
  left       = 1<<2,
  right      = 1<<3,
  up_left    = 1<<4,
  up_right   = 1<<5,
  down_left  = 1<<6,
  down_right = 1<<7
};

// Available button bytes:
// 0x04 -> 0x27 (a-z,0-9)
// Key characters
#define UP1         0x04
#define DOWN1       0x05
#define LEFT1       0x06
#define RIGHT1      0x07
#define UP_LEFT1    0x08
#define UP_RIGHT1   0x09
#define DOWN_LEFT1  0x0A
#define DOWN_RIGHT1 0x0B
#define A1          0x0C
#define B1          0x0D
#define X1          0x0E
#define Y1          0x0F
#define UP2         0x10
#define DOWN2       0x11
#define LEFT2       0x12
#define RIGHT2      0x13
#define UP_LEFT2    0x14
#define UP_RIGHT2   0x15
#define DOWN_LEFT2  0x16
#define DOWN_RIGHT2 0x17
#define A2          0x18
#define B2          0x19
#define X2          0x1A
#define Y2          0x1B
#define START1      0x1C 
#define SELECT1     0x1D
#define START2      0x1E
#define SELECT2     0x1F

// 1st controller can use "left" modifiers, 2nd controller can use
// "right" modifiers. (shift, alt, control, gui) are the modifier keys.
// The shoulder buttons use modifier keys so that we can press both of them
// as well as 2 face buttons. Start/select are counted as face buttons as
// they are not pressed at the same time as other buttons.
// Modifier bits
#define L1      0x02
#define R1      0x04
#define L2      0x20
#define R2      0x40

// Pin definitions - where the wires are connected
const int pinLED = 13;
const int pinData1 = 16; // Controller 1 data
const int pinLatch1 = 15; // Controller 1 latch
const int pinClock1 = 14; // Controller 1 clock
const int pinHID = 17; // HID pin for RN-42
const int pinAutoConnect = 18; // Auto-connect pin for RN-42
const int pinData2 = 21; // Controller 2 data
const int pinLatch2 = 20; // Controller 2 latch
const int pinClock2 = 19; // Controller 2 clock

//Variables for the states of the SNES buttons
byte buttons = 0x00;
byte dpad = 0x00;
byte storedButtons1 = 0x00;
byte storedDpad1 = 0x00;
byte storedButtons2 = 0x00;
byte storedDpad2 = 0x00;

// Variable to record the presence of a second controller (tested on startup).
bool usingController2 = false;

// Bluetooth serial
HardwareSerial bt = HardwareSerial();

void setup()
{
  // Setup the pin modes.
  pinMode(pinLED, OUTPUT);

  // Controller pins
  pinMode(pinData1, INPUT);
  pinMode(pinLatch1, OUTPUT);
  pinMode(pinClock1, OUTPUT);
  pinMode(pinData2, INPUT);
  pinMode(pinLatch2, OUTPUT);
  pinMode(pinClock2, OUTPUT);

  // RN-42 pins
  pinMode(pinHID, OUTPUT);
//  pinMode(pinAutoConnect, OUTPUT);

  // Teensy pins
  pinMode(pinLED, OUTPUT);

  // Set default controller values
  digitalWrite(pinLatch1, HIGH);
  digitalWrite(pinClock1, HIGH);
  digitalWrite(pinLatch2, HIGH);
  digitalWrite(pinClock2, HIGH);

  // Set default RN-42 connection values
  digitalWrite(pinHID, HIGH);
//  digitalWrite(pinAutoConnect, LOW);
  
  // Setup bluetooth
  bt.begin(115200);

  // The following will send commands to setup the RN-42. After setup it is no
  // longer required.
//  setupBluetoothChip();

  delay(500); // wait for RN-42.

  // Check state of controller 2
  readState(pinLatch2, pinClock2, pinData2);
  usingController2 = checkController();
}

void loop()
{
  bool stateChanged = false;

  // Update the state of the first controller
  readState(pinLatch1, pinClock1, pinData1);
  stateChanged =
    hasButtonStateChanged(storedButtons1) || hasDpadStateChanged(storedDpad1);
  storedDpad1 = dpad;
  storedButtons1 = buttons;

  // Update the state of the second controller
  if (usingController2)
  {
    digitalWrite(pinLED, HIGH);
    readState(pinLatch2, pinClock2, pinData2);
    if (buttons == 0xFF && dpad == 0x0F)
    {
      // All the buttons are pressed. This means that the connection to the
      // second controller has been lost. Unset usingController2.
      usingController2 = false;
      storedDpad2 = 0x00;
      storedButtons2 = 0x00;
    }
    else
    {
      stateChanged |=
        hasButtonStateChanged(storedButtons2) || hasDpadStateChanged(storedDpad2);
      storedDpad2 = dpad;
      storedButtons2 = buttons;
    }
  }
  else
  {
    digitalWrite(pinLED, LOW);
  }

  // Write the controller state if it has changed
  // We need to send all the buttons that are currently pressed if the state
  // has changed, or none if it has not changed.
  if (stateChanged)
  {
    writeState();
  }

  if (isButtonPressed(storedButtons1, buttonSelect) &&
      isButtonPressed(storedButtons1, buttonStart) &&
      isButtonPressed(storedButtons1, buttonR) &&
      isButtonPressed(storedButtons1, buttonL))
  {
    // Check to see if the second controller is connected and set the
    // controller state.
    readState(pinLatch2, pinClock2, pinData2);
    usingController2 = checkController();
  }

  delay(20);
}

// Command mode setup
void setupBluetoothChip()
{
  // NOTE: we have to wait a second between each command. I believe this is
  // actually due to the fact that we should be waiting for either the "AOK" or
  // "ERR" response instead of waiting a second. But the end result is the same,
  // assuming that the command is successful.
  delay(1500); // wait for bluetooth to initialise
  const byte commandMode[3] = {'$', '$', '$'};
  bt.write(commandMode, 3); // Enter command mode
  delay(1500); // need to wait for 1 sec before sending anything else
  const byte nameCommand[19] = {'S','N',',','S','N','E','S',' ','C','o','n','t','r','o','l','l','e','r','\r'};
  bt.write(nameCommand, 19); // Set name
  delay(1000);
  const byte HIDcommand[5] = {'S','~',',','6','\r'};
  bt.write(HIDcommand, 5); // Set HID mode
  delay(1000);

  // Commands that either I haven't got working or I don't necesarily understand.
//  const byte connectCommand[15] = {'C',',','9','C','0','2','9','8','3','1','D','7','8','5','\r'};
//  bt.write(connectCommand, 15); // Connect to my phone
 // const byte fastReconnectString[5] = {'S', 'Q', ',', 128, '\r'};
//  bt.write(fastReconnectString, 5);

//  Other potentially useful commands:
//  Config timer 5 seconds: "ST,5\r"
//  reboot: "R,1\r"

  const byte exitCommandMode[4] = {'-','-','-','\r'};
  bt.write(exitCommandMode, 4); // exit command mode
}

// This should only be called directly after a call to readState() using the
// input for the controller that should be checked. Controller 1 should always
// return true;
bool checkController()
{
  if (buttons == 0x00 && dpad == 0x00)
  {
    // If both buttons and dpad are blank, this means that the current is
    // being sent through the controllers as LOW means the button is currently
    // pressed.
    // NOTE: This only works if the person holding the second controller is not
    // pressing any buttons at the time the controllers are turned on.
    return true;
  }
  return false;
}

// Read the next button
static byte readSnesBit(int clock, int data, byte button)
{
  const bool b = (digitalRead(data)==LOW);
  digitalWrite(clock, LOW);
  digitalWrite(clock, HIGH);
  return (b)? button: 0;
}

// Read complete controller state
void readState(int latch, int clock, int data)
{
  // Reset the varables
  dpad = 0x00;
  buttons = 0x00;
  // Need to be in this order
  digitalWrite(latch, LOW);
  buttons |= readSnesBit(clock, data, buttonB);
  buttons |= readSnesBit(clock, data, buttonY);
  buttons |= readSnesBit(clock, data, buttonSelect);
  buttons |= readSnesBit(clock, data, buttonStart);
  dpad |= readSnesBit(clock, data, up);
  dpad |= readSnesBit(clock, data, down);
  dpad |= readSnesBit(clock, data, left);
  dpad |= readSnesBit(clock, data, right);
  buttons |= readSnesBit(clock, data, buttonA);
  buttons |= readSnesBit(clock, data, buttonX);
  buttons |= readSnesBit(clock, data, buttonL);
  buttons |= readSnesBit(clock, data, buttonR);
  digitalWrite(latch, HIGH);
}

// Check the current state against the stored state
bool hasButtonStateChanged(byte storedButtons)
{
  return buttons != storedButtons;
}

bool hasDpadStateChanged(byte storedDpad)
{
  return dpad != storedDpad;
}

// Test if the button is pressed
bool isButtonPressed(byte storedButtons, byte n)
{
  return (storedButtons&n)!=0;
}
bool isDpadPressed(byte storedDpad, byte n)
{
  return (storedDpad&n)!=0;
}

// Returns the dpad direction
byte dpadDirection(byte storedDpad)
{
  // Note: opposite buttons cancel each other out.
  switch ((int)storedDpad)
  {
    case 0:  return 0x00;       // no direction
    case 1:  return up;         // up
    case 2:  return down;       // down
    case 3:  return 0x00;       // up, down
    case 4:  return left;       // left
    case 5:  return up_left;    // up, left
    case 6:  return down_left;  // down, left
    case 7:  return left;       // up, down, left
    case 8:  return right;      // right
    case 9:  return up_right;   // up, right
    case 10: return down_right; // down, right
    case 11: return right;      // up, down, right
    case 12: return 0x00;       // left, right
    case 13: return up;         // up, left, right
    case 14: return down;       // down, left, right
    case 15: return 0x00;       // up, down, left, right
  }
  // If none of the above match, then nothing is pressed.
  return 0x00;
}

//----Function to process the buttons from the SNES controller
// L, R, Start, Select buttons all use the "modifier" section of the HID
// keyboard. This is done so that more buttons can be pressed at the same
// time.
void writeState()
{
  // Array used for sending key presses. Initialised here so that it uses the
  // same buffer each time.
  byte keys[9] = {0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // We start at 3 as the first 3 bytes are {0xFE, <length>, 0x00}.
  int length = 3;
  bool modifier = false;

  //////////////////////////////
  // Check directional buttons
  // Only send up/down and left/right if the opposite direction is not pressed.

  // Controller 1 Dpad
  byte dpadKey1 = 0x00;
  switch((int)dpadDirection(storedDpad1))
  {
    case 0:                                   break;
    case up:          dpadKey1 = UP1;         break;
    case down:        dpadKey1 = DOWN1;       break;
    case left:        dpadKey1 = LEFT1;       break;
    case right:       dpadKey1 = RIGHT1;      break;
    case up_left:     dpadKey1 = UP_LEFT1;    break;
    case up_right:    dpadKey1 = UP_RIGHT1;   break;
    case down_left:   dpadKey1 = DOWN_LEFT1;  break;
    case down_right:  dpadKey1 = DOWN_RIGHT1; break;
  }
  if (dpadKey1 != 0x00)
  {
    keys[length] = dpadKey1;
    length++;
  }

  if (usingController2)
  {
    // Controller 2 Dpad
    byte dpadKey2 = 0x00;
    switch((int)dpadDirection(storedDpad2))
    {
      case 0:                                   break;
      case up:          dpadKey2 = UP2;         break;
      case down:        dpadKey2 = DOWN2;       break;
      case left:        dpadKey2 = LEFT2;       break;
      case right:       dpadKey2 = RIGHT2;      break;
      case up_left:     dpadKey2 = UP_LEFT2;    break;
      case up_right:    dpadKey2 = UP_RIGHT2;   break;
      case down_left:   dpadKey2 = DOWN_LEFT2;  break;
      case down_right:  dpadKey2 = DOWN_RIGHT2; break;
    }
    if (dpadKey2 != 0x00)
    {
      keys[length] = dpadKey2;
      length++;
    }
  }

  //////////////////////////
  // Check face buttons
  const int max1Length = usingController2 ? length + 2 : length + 4;
  const int max2Length = length + 4;

  // Controller 1
  if (isButtonPressed(storedButtons1, buttonA))
  {
    keys[length] = A1;
    length++;
  }
  if (isButtonPressed(storedButtons1, buttonB))
  {
    keys[length] = B1;
    length++;
  }
  if (isButtonPressed(storedButtons1, buttonX) && length < max1Length)
  {
    keys[length] = X1;
    length++;
  }
  if (isButtonPressed(storedButtons1, buttonY) && length < max1Length)
  {
    keys[length] = Y1;
    length++;
  }
  if (isButtonPressed(storedButtons1, buttonStart) && length < max1Length)
  {
    keys[length] = START1;
    length++;
  }
  if (isButtonPressed(storedButtons1, buttonSelect) && length < max1Length)
  {
    keys[length] = SELECT1;
    length++;
  }

  // Controller 2
  if (usingController2)
  {
    if (isButtonPressed(storedButtons2, buttonA))
    {
      keys[length] = A2;
      length++;
    }
    if (isButtonPressed(storedButtons2, buttonB))
    {
      keys[length] = B2;
      length++;
    }
    if (isButtonPressed(storedButtons2, buttonX) && length < max2Length)
    {
      keys[length] = X2;
      length++;
    }
    if (isButtonPressed(storedButtons2, buttonY) && length < max2Length)
    {
      keys[length] = Y2;
      length++;
    }
    if (isButtonPressed(storedButtons2, buttonStart) && length < max2Length)
    {
      keys[length] = START2;
      length++;
    }
    if (isButtonPressed(storedButtons2, buttonSelect) && length < max2Length)
    {
      keys[length] = SELECT2;
      length++;
    }
  }

  ////////////////////////////////////
  // Check shoulder buttons
  // Controller 1
  if (isButtonPressed(storedButtons1, buttonL))
  {
    keys[2] |= L1;
    modifier = true;
  }
  if (isButtonPressed(storedButtons1, buttonR))
  {
    keys[2] |= R1;
    modifier = true;
  }

  // Controller 2
  if (usingController2)
  {
    if (isButtonPressed(storedButtons2, buttonL))
    {
      keys[2] |= L2;
      modifier = true;
    }
    if (isButtonPressed(storedButtons2, buttonR))
    {
      keys[2] |= R2;
      modifier = true;
    }
  }

  /////////////////////////////
  // Send results via Bluetooth
  // If no buttons have been pressed, simply send that.
  if (length == 3)
  {
    if (modifier)
    {
      // Only L/R buttons pressed. As these use the modifier keys, they need 
      // to be sent with a blank key pressed.
      keys[1] = (byte)2;
      bt.write(keys, 4);
    }
    else
    {
      // No keys are pressed.
      keys[1] = 0x00;
      bt.write(keys, 2);
    }
  }
  else
  {
    digitalWrite(pinLED, LOW);
    keys[1] = (byte)(length - 2);
    bt.write(keys, length);
  }
}
