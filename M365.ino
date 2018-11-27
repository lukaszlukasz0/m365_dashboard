// First Developer made hard work with dataFSM - https://electro.club/forum/displey_dlya_syaokata 
// and his version on GitHub https://github.com/fogbox/m365_display
// Second Developer _xxx_ made such nice Menu and fix bugs. Thanks to him it works very good, and looks like good.
// Third Augisbud, make languages support and fix https://github.com/augisbud/m365_dashboard
// and I only make some addons ;) (PL Lang, WheelSize, Lock Code, Battery 12s ...)

#include "defines.h"
char sv[4] = "1.41";
// 1.41 Lock image fix
// 1.40 12s battery info on fsbattinfo, if only 10s then display exit info.
// 1.39 lock code info bug fixed - lock/unlock code works !
// 1.35 lock info more
// 1.32  lock info
// 1.31: bugs
// 1.30: unlock Code 3digit
// 1.25 temp battery
// 1.2: Lock, save settings m365
// 1.1: Hibernate
// 1.0: Wheel size

bool displayClear(byte ID = 1, bool force = false)
{
  volatile static byte oldID = 0;

  if ((oldID != ID) || force)
  {
    display.clear();
    oldID = ID;
    return true;
  }
  else
    return false;
}

void WDTint_()
{
  if (WDTcounts > 2)
  {
    WDTcounts = 0;
    resetFunc();
  }
  else
    WDTcounts++;
}

void setup()
{
  XIAOMI_PORT.begin(115200);

  byte cfgID = EEPROM.read(0);
  if (cfgID == 128)
  {
    autoBig = EEPROM.read(1);
    warnBatteryPercent = EEPROM.read(2);
    bigMode = EEPROM.read(3);
    bigWarn = EEPROM.read(4);
    WheelSize = EEPROM.read(5);
    cfgCruise = EEPROM.read(6);
    cfgTailight = EEPROM.read(7);
    cfgKERS = EEPROM.read(8);
    wtemp = EEPROM.read(9);
    LockCode1 = EEPROM.read(10);
    LockCode2 = EEPROM.read(11);
    LockCode3 = EEPROM.read(12);
  }
  else
  {
    EEPROM.put(0, 128);
    EEPROM.put(1, autoBig);
    EEPROM.put(2, warnBatteryPercent);
    EEPROM.put(3, bigMode);
    EEPROM.put(4, bigWarn);
    EEPROM.put(5, WheelSize);
    EEPROM.put(6, cfgCruise);
    EEPROM.put(7, cfgTailight);
    EEPROM.put(8, cfgKERS);
    EEPROM.put(9, wtemp);
    EEPROM.put(10, LockCode1);
    EEPROM.put(11, LockCode2);
    EEPROM.put(12, LockCode3);
  }

#ifdef DISPLAY_I2C
  Wire.begin();
  Wire.setClock(400000L);
  display.begin(&Adafruit128x64, 0x3C);
#endif
#ifdef DISPLAY_SPI
  display.begin(&Adafruit128x64, PIN_CS, PIN_DC, PIN_RST);
#endif

  display.setFont(m365);
  displayClear(0, true);
  display.setCursor(0, 0);
  display.print((char)0x20);
  display.setFont(defaultFont);

  unsigned long wait = millis() + 2000;
  while ((wait > millis()) || ((wait - 1000 > millis()) && (S25C31.current != 0) && (S25C31.voltage != 0) && (S25C31.remainPercent != 0)))
  {
    dataFSM();
    if (_Query.prepared == 0)
      prepareNextQuery();
    Message.Process();
  }

  if ((S25C31.current == 0) && (S25C31.voltage == 0) && (S25C31.remainPercent == 0))
  {
    displayClear(1);
    display.set2X();
    display.setCursor(0, 0);
    display.println((const __FlashStringHelper *)noBUS1);
    display.println((const __FlashStringHelper *)noBUS2);
    display.println((const __FlashStringHelper *)noBUS3);
    display.println((const __FlashStringHelper *)noBUS4);
    display.set1X();
  }
  else
    displayClear(1);

  WDTcounts = 0;
  WatchDog::init(WDTint_, 500);
}

void loop()
{ //cycle time w\o data exchange ~8 us :)

  if (hibernate)
  { // if hibernate show only info
    displayClear(1);
    display.set2X();
    display.setFont(defaultFont);
    display.setCursor(0, 0);
    display.println((const __FlashStringHelper *)hibernate_info1);
    display.set1X();
    display.setFont(defaultFont);
    display.println((const __FlashStringHelper *)hibernate_info2);
    display.println((const __FlashStringHelper *)hibernate_info3);
    display.println((const __FlashStringHelper *)hibernate_info4);
    display.println((const __FlashStringHelper *)hibernate_info5);
    display.println((const __FlashStringHelper *)hibernate_info6);
    display.println((const __FlashStringHelper *)hibernate_info7);
  }
  else
  {

    dataFSM();

    if (_Query.prepared == 0)
      prepareNextQuery();
    if (_NewDataFlag == 1)
    {
      _NewDataFlag = 0;
      displayFSM();
    }
    Message.Process();
    Message.ProcessBroadcast();
  }
  WDTcounts = 0;
}

void showBatt(int percent, bool blinkIt)
{
  display.set1X();
  display.setFont(defaultFont);
  display.setCursor(0, 7);
  if (bigWarn || (warnBatteryPercent == 0) || (percent > warnBatteryPercent) || ((warnBatteryPercent != 0) && (millis() % 1000 < 500)))
  {
    display.print((char)0x81);
    for (int i = 0; i < 19; i++)
    {
      display.setCursor(5 + i * 5, 7);
      if (blinkIt && (millis() % 1000 < 500))
        display.print((char)0x83);
      else if (float(19) / 100 * percent > i)
        display.print((char)0x82);
      else
        display.print((char)0x83);
    }
    display.setCursor(99, 7);
    display.print((char)0x84);
    if (percent < 100)
      display.print(' ');
    if (percent < 10)
      display.print(' ');
    display.print(percent);
    display.print('\%');
  }
  else
    for (int i = 0; i < 34; i++)
    {
      display.setCursor(i * 5, 7);
      display.print(' ');
    }
}

void fsBattInfo()
{
  displayClear(6);

  int tmp_0, tmp_1;

  display.setCursor(0, 0);
  display.set1X();

  tmp_0 = abs(S25C31.voltage) / 100; //voltage
  tmp_1 = abs(S25C31.voltage) % 100;

  if (tmp_0 < 10)
    display.print(' ');
  display.print(tmp_0);
  display.print('.');
  if (tmp_1 < 10)
    display.print('0');
  display.print(tmp_1);
  display.print((const __FlashStringHelper *)l_v);
  display.print(' ');

  tmp_0 = abs(S25C31.current) / 100; //current
  tmp_1 = abs(S25C31.current) % 100;

  if (tmp_0 < 10)
    display.print(' ');
  display.print(tmp_0);
  display.print('.');
  if (tmp_1 < 10)
    display.print('0');
  display.print(tmp_1);
  display.print((const __FlashStringHelper *)l_a);
  display.print(' ');
  if (S25C31.remainCapacity < 1000)
    display.print(' ');
  if (S25C31.remainCapacity < 100)
    display.print(' ');
  if (S25C31.remainCapacity < 10)
    display.print(' ');
  display.print(S25C31.remainCapacity);
  display.print((const __FlashStringHelper *)l_mah);
  int temp;
  temp = S25C31.temp1 - 20;
  display.setCursor(9, 1);
  display.print((const __FlashStringHelper *)l_t);
  display.print("1: ");
  if (temp < 10)
    display.print(' ');
  display.print(temp);
  display.print((char)0x80);
  display.print("C");
  display.setCursor(74, 1);
  display.print((const __FlashStringHelper *)l_t);
  display.print("2: ");
  temp = S25C31.temp2 - 20;
  if (temp < 10)
    display.print(' ');
  display.print(temp);
  display.print((char)0x80);
  display.print("C");

  int v;
  int *ptr;
  int *ptr2;
  ptr = (int *)&S25C40;
  ptr2 = ptr + 5;
  for (int i = 0; i < 5; i++)
  {
    display.setCursor(5, 2 + i);
    display.print(i);
    display.print(": ");
    v = *ptr / 1000;
    display.print(v);
    display.print('.');
    v = *ptr % 1000;
    if (v < 100)
      display.print('0');
    if (v < 10)
      display.print('0');
    display.print(v);
    display.print((const __FlashStringHelper *)l_v);

    display.setCursor(70, 2 + i);
    display.print(i + 5);
    display.print(": ");
    v = *ptr2 / 1000;
    display.print(v);
    display.print('.');
    v = *ptr2 % 1000;
    if (v < 100)
      display.print('0');
    if (v < 10)
      display.print('0');
    display.print(v);
    display.print((const __FlashStringHelper *)l_v);

    ptr++;
    ptr2++;
  }
  int cell11 = *ptr2;
  ptr2++;
  int cell12 = *ptr2;
  if (cell11 > 1) {

    display.setCursor(0, 7);
    display.print(10);
    display.print(": ");
    v = cell11 / 1000;
    display.print(v);
    display.print('.');
    v = cell11 % 1000;
    if (v < 100)
      display.print('0');
    if (v < 10)
      display.print('0');
    display.print(v);
    display.print((const __FlashStringHelper *)l_v);

    display.setCursor(65, 7);
    display.print(11);
    display.print(": ");
    v = cell12 / 1000;
    display.print(v);
    display.print('.');
    v = cell12 % 1000;
    if (v < 100)
      display.print('0');
    if (v < 10)
      display.print('0');
    display.print(v);
    display.print((const __FlashStringHelper *)l_v);
  }  else {
    display.setCursor(0, 7);
    display.print((const __FlashStringHelper *)battScr);

  }

  
}

void displayFSM()
{
  struct
  {
    unsigned int curh;
    unsigned int curl;
    unsigned int vh;
    unsigned int vl;
    unsigned int sph;
    unsigned int spl;
    unsigned int milh;
    unsigned int mill;
    unsigned int Min;
    unsigned int Sec;
    unsigned int temp;
  } m365_info;

  int brakeVal = -1;
  //  int little_brakeVal = -1;
  int throttleVal = -1;
  int tmp_0, tmp_1;
  long c_speed; //current speed

  // CURRENT SPEED CALCULATE ALGORYTHM
  if (S23CB0.speed < -10000)
  {                                        
    c_speed = S23CB0.speed + 32768 + 32767; // calculate speed over 32.767 km/h (32767) (hex 0x8000 and above) add 32768 and 32767 to conver to unsigned int
  }
  else
  {
    c_speed = abs(S23CB0.speed); //always + speed, even drive backwards ;)
  }; 

  // 10 INCH WHEEL SIZE CALCULATE
  if (WheelSize)
  {
    c_speed = (long)c_speed * 10 / 8.5; // 10" Whell
  };

  m365_info.sph = (unsigned long)abs(c_speed) / 1000L; // speed (GOOD)
  m365_info.spl = (unsigned int)c_speed % 1000 / 100;
  m365_info.curh = abs(S25C31.current) / 100; //current
  //m365_info.curh = S25C31.current / 100;       //current //testing only
  m365_info.curl = abs(S25C31.current) % 100;
  m365_info.vh = abs(S25C31.voltage) / 100; //voltage
  m365_info.vl = abs(S25C31.voltage) % 100;

  if ((m365_info.sph > 1) && Settings)
  {
    ShowBattInfo = false;
    M365Settings = false;
    Settings = false;
  }
  
  if (S21C00HZ64.headLamp > 50)
      Headlamp = 1; 
    else
      Headlamp = 0;

  if ((c_speed <= 200) || Settings)
  {
    if (S20C00HZ65.brake > 130)
      brakeVal = 1;
    else if (S20C00HZ65.brake < 50)
      brakeVal = -1;
    else
      brakeVal = 0;
    if (S20C00HZ65.throttle > 150)
      throttleVal = 1;
    else if (S20C00HZ65.throttle < 50)
      throttleVal = -1;
    else
      throttleVal = 0;

    if (((brakeVal == 1) && (throttleVal == 1) && !Settings) && ((oldBrakeVal != 1) || (oldThrottleVal != 1)))
    { // brake max + throttle max = menu on
      menuPos = 0;
      timer = millis() + LONG_PRESS;
      Settings = true;
    }

    if(SetNewCode)
    {
     
      if ((throttleVal == 1) && (oldThrottleVal != 1) && (brakeVal == -1) && (oldBrakeVal == -1)) // brake min + throttle max = change menu value
        switch (active)
        {
        case 1: //------------------------------------------------------------------------------
           if (num1 < 9)
              num1++;
            else
              num1 = 0;
          break;
        case 2: //------------------------------------------------------------------------------
           if (num2 < 9)
              num2++;
            else
              num2 = 0;
          break;
        case 3: //------------------------------------------------------------------------------
         if (num3 < 9)
              num3++;
            else
              num3 = 0;
        }
      else if ((brakeVal == 1) && (oldBrakeVal != 1) && (throttleVal == -1) && (oldThrottleVal == -1))
      { // brake max + throttle min = change menu position
        if (active < 4)
          active++;
        else
          active = 1;
        //timer = millis() + LONG_PRESS;
      }

     if (displayClear(10))
        active = 1;
      //CODE 1 //------------------------------------------------------------------------------
        //displayClear(94);
        display.setFont(bigNumb);
        display.set1X();
        display.setCursor(20, 0);

        display.print(num1);
        display.print(num2);
        display.print(num3);

        display.setFont(defaultFont);
        display.set1X();
        display.setCursor(0, 7);
        display.print((const __FlashStringHelper *)setnewcode_info);

        if (active > 3)
        {
          LockCode1 = num1;
          LockCode2 = num2;
          LockCode3 = num3;
          displayClear(96);
          display.setFont(m365);
          display.setCursor(0, 0);
          display.print((char)0x22);
          delay(500);
          active = 1;
          SetNewCode = false; 
        };
        display.setFont(defaultFont);
        oldBrakeVal = brakeVal;
        oldThrottleVal = throttleVal;
        
        return;
    } else 
    if (M365Settings)
    {
      if ((throttleVal == 1) && (oldThrottleVal != 1) && (brakeVal == -1) && (oldBrakeVal == -1)) // brake min + throttle max = change menu value
        switch (sMenuPos)
        {
        case 0: //------------------------------------------------------------------------------
          cfgCruise = !cfgCruise;
          break;
        case 1: //------------------------------------------------------------------------------
          cfgTailight = !cfgTailight;
          break;
        case 2: //------------------------------------------------------------------------------
          switch (cfgKERS)
          {
          case 1:
            cfgKERS = 2;
            break;
          case 2:
            cfgKERS = 0;
            break;
          default:
            cfgKERS = 1;
          }
          break;
        case 3: //------------------------------------------------------------------------------
          WheelSize = !WheelSize;
          break;
        case 4: //------------------------------------------------------------------------------
          if (!locked)
            SetNewCode = true;
          break;
        case 5: //------------------------------------------------------------------------------
          wtemp = !wtemp;
          break;
        case 6: //------------------------------------------------------------------------------
          break;
        case 7: //SAVE AND EXIT-----------------------------------------------------------------------
          if (cfgCruise)
            prepareCommand(CMD_CRUISE_ON);
          else
            prepareCommand(CMD_CRUISE_OFF);

          if (cfgTailight)
            prepareCommand(CMD_LED_ON);
          else
            prepareCommand(CMD_LED_OFF);

          switch (cfgKERS)
          {
          case 1:
            prepareCommand(CMD_MEDIUM);
          case 2:
            prepareCommand(CMD_STRONG);
          default:
            prepareCommand(CMD_WEAK);
          };
              EEPROM.put(5, WheelSize);
              EEPROM.put(6, cfgCruise);
              EEPROM.put(7, cfgTailight);
              EEPROM.put(8, cfgKERS);
              EEPROM.put(9, wtemp);
              EEPROM.put(10, LockCode1);
              EEPROM.put(11, LockCode2);
              EEPROM.put(12, LockCode3);
          oldBrakeVal = brakeVal;
          oldThrottleVal = throttleVal;
          timer = millis() + LONG_PRESS;
          M365Settings = false;
          break;
        }
      else if ((brakeVal == 1) && (oldBrakeVal != 1) && (throttleVal == -1) && (oldThrottleVal == -1))
      { // brake max + throttle min = change menu position
        if (sMenuPos < 7)
          sMenuPos++;
        else
          sMenuPos = 0;
        timer = millis() + LONG_PRESS;
      }

      if (displayClear(7))
        sMenuPos = 0;
      display.set1X();
      //MENU 1 //------------------------------------------------------------------------------
      display.setCursor(0, 0);

      if (sMenuPos == 0)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)M365CfgScr1);
      if (cfgCruise)
        display.print((const __FlashStringHelper *)l_On);
      else
        display.print((const __FlashStringHelper *)l_Off);
      //MENU 2 //------------------------------------------------------------------------------
      display.setCursor(0, 1);

      if (sMenuPos == 1)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)M365CfgScr2);
      if (cfgTailight)
        display.print((const __FlashStringHelper *)l_Yes);
      else
        display.print((const __FlashStringHelper *)l_No);
      //MENU 3 //------------------------------------------------------------------------------
      display.setCursor(0, 2);

      if (sMenuPos == 2)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)M365CfgScr3);
      switch (cfgKERS)
      {
      case 1:
        display.print((const __FlashStringHelper *)l_Medium);
        break;
      case 2:
        display.print((const __FlashStringHelper *)l_Strong);
        break;
      default:
        display.print((const __FlashStringHelper *)l_Weak);
        break;
      }
      //MENU 4 //------------------------------------------------------------------------------
      display.setCursor(0, 3);

      if (sMenuPos == 3)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)M365CfgScr4);
      if (WheelSize)
        display.print((const __FlashStringHelper *)l_10inch);
      else
        display.print((const __FlashStringHelper *)l_85inch);
      //MENU 5 //------------------------------------------------------------------------------
      display.setCursor(0, 4);

      if (sMenuPos == 4)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)M365CfgScr5);
      if (!locked){ //display only if unlocked
       display.print(LockCode1);
       display.print(LockCode2);
       display.print(LockCode3);
      }
      //MENU 6 //------------------------------------------------------------------------------
      display.setCursor(0, 5);

      if (sMenuPos == 5)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)M365CfgScr6);
      if (wtemp)
        display.print((const __FlashStringHelper *)l_temp1);
      else
        display.print((const __FlashStringHelper *)l_temp2);
      //MENU 7 //------------------------------------------------------------------------------
      display.setCursor(0, 6);

      /*if (sMenuPos == 6)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)M365CfgScr7);
      */
      //display.setCursor(0, 7);
      if (sMenuPos == 6)
        sMenuPos++;
      for (int i = 0; i < 25; i++) {
        display.setCursor(i * 5, 6);
        display.print('-');
      }
      //MENU 8 EXIT //------------------------------------------------------------------------------
      display.setCursor(0, 7);

      if (sMenuPos == 7)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)M365CfgScr8);
      display.print(" ");display.print("v");
      display.print(sv); //Software info

      oldBrakeVal = brakeVal;
      oldThrottleVal = throttleVal;

      return;
    }
    else if (ShowBattInfo)
    {
      if ((brakeVal == 1) && (oldBrakeVal != 1) && (throttleVal == -1) && (oldThrottleVal == -1))
      {
        oldBrakeVal = brakeVal;
        oldThrottleVal = throttleVal;
        timer = millis() + LONG_PRESS;
        ShowBattInfo = false;
        return;
      }

      fsBattInfo();

      

      oldBrakeVal = brakeVal;
      oldThrottleVal = throttleVal;

      return;
    }
    else if (Settings)
    {
      if ((brakeVal == 1) && (oldBrakeVal == 1) && (throttleVal == -1) && (oldThrottleVal == -1) && (timer != 0))
        if (millis() > timer)
        {
          Settings = false;
          return;
        }

      if ((throttleVal == 1) && (oldThrottleVal != 1) && (brakeVal == -1) && (oldBrakeVal == -1)) // brake min + throttle max = change menu value
        switch (menuPos)
        {
        case 0: //-------------------------------------------------------------------------------
          hibernate = true;
          break;
        case 1: //-------------------------------------------------------------------------------
          switch (bigMode)
          {
          case 0: //speed
            bigMode = 1;
            autoBig = true;
            break;
          case 1: //current
            bigMode = 2;
            autoBig = true;
            break;
          case 2: //no big display
            bigMode = 0;
            autoBig = false;
          }
          break;
        case 2: //-------------------------------------------------------------------------------
          switch (warnBatteryPercent)
          {
          case 0:
            warnBatteryPercent = 5;
            break;
          case 5:
            warnBatteryPercent = 10;
            break;
          case 10:
            warnBatteryPercent = 15;
            break;
          default:
            warnBatteryPercent = 0;
          }
          break;
        case 3: //-------------------------------------------------------------------------------
          bigWarn = !bigWarn;
          break;
        case 4: //-------------------------------------------------------------------------------
          ShowBattInfo = true;
          break;
        case 5: //-------------------------------------------------------------------------------
          M365Settings = true;
          break;
        case 6: //-------------------------------------------------------------------------------
          EEPROM.put(1, autoBig);
          EEPROM.put(2, warnBatteryPercent);
          EEPROM.put(3, bigMode);
          EEPROM.put(4, bigWarn);
          Settings = false;
          break;
        }
      else if ((brakeVal == 1) && (oldBrakeVal != 1) && (throttleVal == -1) && (oldThrottleVal == -1))
      { // brake max + throttle min = change menu position
        if (menuPos < 6)
          menuPos++;
        else
          menuPos = 0;
        timer = millis() + LONG_PRESS;
      }

      displayClear(2);

      display.set1X();
      //-------------------------------------------------------------------------------
      display.setCursor(0, 0);

      if (menuPos == 0)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)confScr1);
      //-------------------------------------------------------------------------------
      display.setCursor(0, 1);

      if (menuPos == 1)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)confScr2);
      switch (bigMode)
      {
      case 1:
        display.print((const __FlashStringHelper *)confScr2b);
        break;
      case 2:
        display.print((const __FlashStringHelper *)confScr2a);
        break;
      case 0:
        display.print((const __FlashStringHelper *)confScr2c);
      };
      //-------------------------------------------------------------------------------
      display.setCursor(0, 2);

      if (menuPos == 2)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)confScr3);
      switch (warnBatteryPercent)
      {
      case 5:
        display.print(" 5%");
        break;
      case 10:
        display.print("10%");
        break;
      case 15:
        display.print("15%");
        break;
      default:
        display.print((const __FlashStringHelper *)l_Off);
        break;
      }
      //-------------------------------------------------------------------------------
      display.setCursor(0, 3);

      if (menuPos == 3)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)confScr4);
      if (bigWarn)
        display.print((const __FlashStringHelper *)l_Yes);
      else
        display.print((const __FlashStringHelper *)l_No);
      //-------------------------------------------------------------------------------
      display.setCursor(0, 4);

      if (menuPos == 4)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)confScr5);
      //-------------------------------------------------------------------------------
      display.setCursor(0, 5);

      if (menuPos == 5)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)confScr6);
      //-------------------------------------------------------------------------------
      display.setCursor(0, 6);

      for (int i = 0; i < 25; i++)
      {
        display.setCursor(i * 5, 6);
        display.print('-');
      }
      //-------------------------------------------------------------------------------
      display.setCursor(0, 7);

      if (menuPos == 6)
        display.print((char)0x7E);
      else
        display.print(" ");

      display.print((const __FlashStringHelper *)confScr7);
      //-------------------------------------------------------------------------------
      /*display.setCursor(0, 8);

      if (menuPos == 7)
        display.print((char)0x7E);
      else
        display.print(" ");*/
      //-------------------------------------------------------------------------------
      oldBrakeVal = brakeVal;
      oldThrottleVal = throttleVal;

      return;
    }
    else if(SetCode)
    { 
      if ((throttleVal == 1) && (oldThrottleVal != 1) && (brakeVal == -1) && (oldBrakeVal == -1)) // brake min + throttle max = change menu value
        switch (active)
        {
        case 1:
          if (num1 < 9)
            num1++;
          else
            num1 = 0;
          break;
        case 2:
          if (num2 < 9)
            num2++;
          else
            num2 = 0;
          break;
        case 3: 
         if (num3 < 9)
              num3++;
            else
              num3 = 0;
        }
      else if ((brakeVal == 1) && (oldBrakeVal != 1) && (throttleVal == -1) && (oldThrottleVal == -1))
      { // brake max + throttle min = change menu position
        if (active < 4)
          active++;
        else
          active = 1;
        timer = millis() + LONG_PRESS;
      }

     if (displayClear(8))
        active = 1;
      //CODE 1 //------------------------------------------------------------------------------
        //displayClear(94);
        display.setFont(bigNumb);
        display.set1X();
        display.setCursor(20, 0);

        display.print(num1); 
        display.print(num2);
        display.print(num3);
        
        display.setFont(defaultFont);
        display.set1X();
        display.setCursor(0, 7);
        display.print((const __FlashStringHelper *)setcode_info);

        if (active > 3)
        {
          if ((LockCode1 == num1) && (LockCode2 == num2) && (LockCode3 == num3))
          {
           prepareCommand(CMD_UNLOCK); 
           locked = false;
           SetCode = false;
           displayClear(98);
           return;
          };
          displayClear(99);
          display.setFont(m365);
          display.setCursor(0, 0);
          display.print((char)0x22);
          display.setFont(defaultFont);
          delay(1000);
          active = 1;
          SetCode = false; 
        };

        oldBrakeVal = brakeVal;
        oldThrottleVal = throttleVal;
        
        return;
    }
    else if ((throttleVal == 1) && (oldThrottleVal != 1) && (brakeVal == -1) && (oldBrakeVal == -1))
    { //Info screen, speed 0, throttle full
     
        //display odo and time from start
        displayClear(3);
        display.set1X();
        display.setFont(defaultFont);
        display.setCursor(0, 0);
        display.print((const __FlashStringHelper *)infoScr1);
        display.print(':');
        display.setFont(stdNumb);
        display.setCursor(15, 1);
        tmp_0 = S23CB0.mileageTotal / 1000;
        tmp_1 = (S23CB0.mileageTotal % 1000) / 10;
        if (tmp_0 < 1000)
          display.print(' ');
        if (tmp_0 < 100)
          display.print(' ');
        if (tmp_0 < 10)
          display.print(' ');
        display.print(tmp_0);
        display.print('.');
        if (tmp_1 < 10)
          display.print('0');
        display.print(tmp_1);
        display.setFont(defaultFont);
        display.print((const __FlashStringHelper *)l_km);

        display.setCursor(0, 4);
        display.print((const __FlashStringHelper *)infoScr2);
        display.print(':');
        display.setFont(stdNumb);
        display.setCursor(15, 5);
        tmp_0 = S23C3A.powerOnTime / 60;
        tmp_1 = S23C3A.powerOnTime % 60;
        if (tmp_0 < 100)
          display.print(' ');
        if (tmp_0 < 10)
          display.print(' ');
        display.print(tmp_0);
        display.print(':');
        if (tmp_1 < 10)
          display.print('0');
        display.print(tmp_1);


        // DISPLAY LOCK INFO ----------------------------------------------------------
        display.setFont(defaultFont);
        display.set1X();
        display.setCursor(0, 7);
        if (!locked)
          { //if unlocked
            display.print((const __FlashStringHelper *)lock); // display lock info
          }
          else
          {
            display.print((const __FlashStringHelper *)unlock);
          }
        // LOCK //------------------------------------------------------------------------
        if (((Headlamp == 1) && (oldHeadlamp == 0)) || ((Headlamp == 0) && (oldHeadlamp == 1)))
        {
          if (!locked)
          { //if unlocked
            locked = true;
            oldHeadlamp = Headlamp;
            prepareCommand(CMD_LOCK);
            return;
          }
          else
          {
              num1 = 0;
              num2 = 0;
              num3 = 0;
              //oldThrottleVal = throttleVal;
              SetCode = true;
              oldHeadlamp = Headlamp;
              return;
          }
        }
        
        oldHeadlamp = Headlamp;
        return;
      
    };

    oldBrakeVal = brakeVal;
    oldThrottleVal = throttleVal;
}

if (bigWarn && (((warnBatteryPercent > 0) && (S25C31.remainPercent <= warnBatteryPercent)) && (millis() % 2000 < 700)))
{
  if (displayClear(4))
  {
    display.setFont(m365);
    display.setCursor(0, 0);
    display.print((char)0x21);
    display.setFont(defaultFont);
  }
}
else if ((m365_info.sph > 1) && (autoBig))
{
  displayClear(5);
  display.set1X();

  switch (bigMode)
  {
  case 1:
    display.setFont(bigNumb);
    tmp_0 = m365_info.curh / 10;
    tmp_1 = m365_info.curh % 10;
    display.setCursor(2, 0);
    if (tmp_0 > 0)
      display.print(tmp_0);
    else
      display.print((char)0x3B);
    display.setCursor(32, 0);
    display.print(tmp_1);
    tmp_0 = m365_info.curl / 10;
    tmp_1 = m365_info.curl % 10;
    display.setCursor(75, 0);
    display.print(tmp_0);
    display.setCursor(108, 0);
    display.setFont(stdNumb);
    display.print(tmp_1);
    display.setFont(defaultFont);
    if ((S25C31.current >= 0) || ((S25C31.current < 0) && (millis() % 1000 < 500)))
    {
      display.set2X();
      display.setCursor(108, 4);
      display.print((const __FlashStringHelper *)l_a);
    }
    display.set1X();
    display.setCursor(64, 5);
    display.print((char)0x85);
    break;
  case 2:
    display.setFont(bigNumb);
    tmp_0 = m365_info.sph / 10;
    tmp_1 = m365_info.sph % 10;
    display.setCursor(2, 0);
    if (tmp_0 > 0)
      display.print(tmp_0);
    else
      display.print((char)0x3B);
    display.setCursor(32, 0);
    display.print(tmp_1);
    display.setCursor(75, 0);
    display.print(m365_info.spl);
    display.setCursor(106, 0);
    display.print((char)0x3A);
    display.setFont(defaultFont);
    display.set1X();
    display.setCursor(64, 5);
    display.print((char)0x85);
  }
  showBatt(S25C31.remainPercent, S25C31.current < 0);
}
else
{
  if ((S25C31.current < -100) && (c_speed <= 200))
  {
    fsBattInfo();
  }
  else
  {
    if (S23CB0.locks == 6)
    {
      displayClear(95);
      display.setFont(m365);
      display.setCursor(0, 0);
      display.print((char)0x22);
      return;
      //delay(500);
    }
    // OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -  OKNO POSTOJU -
    oldHeadlamp = Headlamp;
    displayClear(0);

    m365_info.milh = S23CB0.mileageCurrent / 100; //mileage
    m365_info.mill = S23CB0.mileageCurrent % 100;
    m365_info.Min = S23C3A.ridingTime / 60; //riding time
    m365_info.Sec = S23C3A.ridingTime % 60;
    if (wtemp)
      m365_info.temp = S23CB0.mainframeTemp / 10; //temperature mainframe
    else
      m365_info.temp = S25C31.temp1 - 20; //temperature battery

    display.set1X();
    display.setFont(stdNumb);
    display.setCursor(0, 0);

    if (m365_info.sph < 10)
      display.print(' ');
    display.print(m365_info.sph);
    display.print('.');
    display.print(m365_info.spl);
    display.setFont(defaultFont);
    display.print((const __FlashStringHelper *)l_kmh);
    display.setFont(stdNumb);

    display.setCursor(95, 0);

    if (m365_info.temp < 10)
      display.print(' ');
    display.print(m365_info.temp);
    display.setFont(defaultFont);
    display.print((char)0x80);
    display.print((const __FlashStringHelper *)l_c);
    display.setFont(stdNumb);

    display.setCursor(0, 2);

    if (m365_info.milh < 10)
      display.print(' ');
    display.print(m365_info.milh);
    display.print('.');
    if (m365_info.mill < 10)
      display.print('0');
    display.print(m365_info.mill);
    display.setFont(defaultFont);
    display.print((const __FlashStringHelper *)l_km);
    display.setFont(stdNumb);

    display.setCursor(0, 4);

    if (m365_info.Min < 10)
      display.print('0');
    display.print(m365_info.Min);
    display.print(':');
    if (m365_info.Sec < 10)
      display.print('0');
    display.print(m365_info.Sec);

    display.setCursor(68, 4);

    if (m365_info.curh < 10)
      display.print(' ');
    display.print(m365_info.curh);
    display.print('.');
    if (m365_info.curl < 10)
      display.print('0');
    display.print(m365_info.curl);
    display.setFont(defaultFont);
    display.print((const __FlashStringHelper *)l_a);
  }
}
if ((S23CB0.locks == 2) || (S23CB0.locks == 6))
{
  display.setCursor(0, 6);
  display.print((const __FlashStringHelper *)info_locked);
} else {
  display.setCursor(0, 6);
  display.print((const __FlashStringHelper *)space);
};
 
  showBatt(S25C31.remainPercent, S25C31.current < 0);
}

void dataFSM()
{
  static unsigned char step = 0, _step = 0, entry = 1;
  static unsigned long beginMillis;
  static unsigned char Buf[RECV_BUFLEN];
  static unsigned char *_bufPtr;
  _bufPtr = (unsigned char *)&Buf;

  switch (step)
  {
  case 0: //search header sequence
    while (XIAOMI_PORT.available() >= 2)
      if (XIAOMI_PORT.read() == 0x55 && XIAOMI_PORT.peek() == 0xAA)
      {
        XIAOMI_PORT.read(); //discard second part of header
        step = 1;
        break;
      }
    break;
  case 1: //preamble complete, receive body
    static unsigned char readCounter;
    static unsigned int _cs;
    static unsigned char *bufPtr;
    static unsigned char *asPtr; //
    unsigned char bt;
    if (entry)
    { //init variables
      memset((void *)&AnswerHeader, 0, sizeof(AnswerHeader));
      bufPtr = _bufPtr;
      readCounter = 0;
      beginMillis = millis();
      asPtr = (unsigned char *)&AnswerHeader; //pointer onto header structure
      _cs = 0xFFFF;
    }
    if (readCounter >= RECV_BUFLEN)
    { //overrun
      step = 2;
      break;
    }
    if (millis() - beginMillis >= RECV_TIMEOUT)
    { //timeout
      step = 2;
      break;
    }

    while (XIAOMI_PORT.available())
    { //read available bytes from port-buffer
      bt = XIAOMI_PORT.read();
      readCounter++;
      if (readCounter <= sizeof(AnswerHeader))
      { //separate header into header-structure
        *asPtr++ = bt;
        _cs -= bt;
      }
      if (readCounter > sizeof(AnswerHeader))
      { //now begin read data
        *bufPtr++ = bt;
        if (readCounter < (AnswerHeader.len + 3))
          _cs -= bt;
      }
      beginMillis = millis(); //reset timeout
    }

    if (AnswerHeader.len == (readCounter - 4))
    { //if len in header == current received len
      unsigned int cs;
      unsigned int *ipcs;
      ipcs = (unsigned int *)(bufPtr - 2);
      cs = *ipcs;
      if (cs != _cs)
      { //check cs
        step = 2;
        break;
      }
      //here cs is ok, header in AnswerHeader, data in _bufPtr
      processPacket(_bufPtr, readCounter);

      step = 2;
      break;
    }
    break; //case 1:
  case 2:  //end of receiving data
    step = 0;
    break;
  }

  if (_step != step)
  {
    _step = step;
    entry = 1;
  }
  else
    entry = 0;
}

void processPacket(unsigned char *data, unsigned char len)
{
  unsigned char RawDataLen;
  RawDataLen = len - sizeof(AnswerHeader) - 2; //(crc)

  switch (AnswerHeader.addr)
  {          //store data into each other structure
  case 0x20: //0x20
    switch (AnswerHeader.cmd)
    {
    case 0x00:
      switch (AnswerHeader.hz)
      {
      case 0x64: //BLE ask controller
        break;
      case 0x65:
        if (_Query.prepared == 1 && !hibernate)
          writeQuery();

        memcpy((void *)&S20C00HZ65, (void *)data, RawDataLen);

        break;
      default: //no suitable hz
        break;
      }
      break;
    case 0x1A:
      break;
    case 0x69:
      break;
    case 0x3E:
      break;
    case 0xB0:
      break;
    case 0x23:
      break;
    case 0x3A:
      break;
    case 0x7C:
      break;
    default: //no suitable cmd
      break;
    }
    break;
  case 0x21:
    switch (AnswerHeader.cmd)
    {
    case 0x00:
      switch (AnswerHeader.hz)
      {
      case 0x64: //answer to BLE
        memcpy((void *)&S21C00HZ64, (void *)data, RawDataLen);
        break;
      }
      break;
    default:
      break;
    }
    break;
  case 0x22:
    switch (AnswerHeader.cmd)
    {
    case 0x3B:
      break;
    case 0x31:
      break;
    case 0x20:
      break;
    case 0x1B:
      break;
    case 0x10:
      break;
    default:
      break;
    }
    break;
  case 0x23:
    switch (AnswerHeader.cmd)
    {
    case 0x17:
      break;
    case 0x1A:
      break;
    case 0x69:
      break;
    case 0x3E: //mainframe temperature
      if (RawDataLen == sizeof(A23C3E))
        memcpy((void *)&S23C3E, (void *)data, RawDataLen);
      break;
    case 0xB0: //speed, average speed, mileage total, mileage current, power on time, mainframe temp
      if (RawDataLen == sizeof(A23CB0))
        memcpy((void *)&S23CB0, (void *)data, RawDataLen);
      break;
    case 0x23: //remain mileage
      if (RawDataLen == sizeof(A23C23))
        memcpy((void *)&S23C23, (void *)data, RawDataLen);
      break;
    case 0x3A: //power on time, riding time
      if (RawDataLen == sizeof(A23C3A))
        memcpy((void *)&S23C3A, (void *)data, RawDataLen);
      break;
    case 0x7C:
      break;
    case 0x7B:
      break;
    case 0x7D:
      break;
    default:
      break;
    }
    break;
  case 0x25:
    switch (AnswerHeader.cmd)
    {
    case 0x40: //cells info
      if (RawDataLen == sizeof(A25C40))
        memcpy((void *)&S25C40, (void *)data, RawDataLen);
      break;
    case 0x3B:
      break;
    case 0x31: //capacity, remain persent, current, voltage
      if (RawDataLen == sizeof(A25C31))
        memcpy((void *)&S25C31, (void *)data, RawDataLen);
      break;
    case 0x20:
      break;
    case 0x1B:
      break;
    case 0x10:
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }

  for (unsigned char i = 0; i < sizeof(_commandsWeWillSend); i++)
    if (AnswerHeader.cmd == _q[_commandsWeWillSend[i]])
    {
      _NewDataFlag = 1;
      break;
    }
}

void prepareNextQuery()
{
  static unsigned char index = 0;

  _Query._dynQueries[0] = 1;
  _Query._dynQueries[1] = 8;
  _Query._dynQueries[2] = 10;
  _Query._dynQueries[3] = 14;
  _Query._dynSize = 4;

  if (preloadQueryFromTable(_Query._dynQueries[index]) == 0)
    _Query.prepared = 1;

  index++;

  if (index >= _Query._dynSize)
    index = 0;
}

unsigned char preloadQueryFromTable(unsigned char index)
{
  unsigned char *ptrBuf;
  unsigned char *pp; //pointer preamble
  unsigned char *ph; //pointer header
  unsigned char *pe; //pointer end

  unsigned char cmdFormat;
  unsigned char hLen; //header length
  unsigned char eLen; //ender length

  if (index >= sizeof(_q))
    return 1; //unknown index

  if (_Query.prepared != 0)
    return 2; //if query not send yet

  cmdFormat = pgm_read_byte_near(_f + index);

  pp = (unsigned char *)&_h0;
  ph = NULL;
  pe = NULL;

  switch (cmdFormat)
  {
  case 1: //h1 only
    ph = (unsigned char *)&_h1;
    hLen = sizeof(_h1);
    pe = NULL;
    break;
  case 2: //h2 + end20
    ph = (unsigned char *)&_h2;
    hLen = sizeof(_h2);

    //copies last known throttle & brake values
    _end20t.hz = 0x02;
    _end20t.th = S20C00HZ65.throttle;
    _end20t.br = S20C00HZ65.brake;
    pe = (unsigned char *)&_end20t;
    eLen = sizeof(_end20t);
    break;
  }

  ptrBuf = (unsigned char *)&_Query.buf;

  memcpy_P((void *)ptrBuf, (void *)pp, sizeof(_h0)); //copy preamble
  ptrBuf += sizeof(_h0);

  memcpy_P((void *)ptrBuf, (void *)ph, hLen); //copy header
  ptrBuf += hLen;

  memcpy_P((void *)ptrBuf, (void *)(_q + index), 1); //copy query
  ptrBuf++;

  memcpy_P((void *)ptrBuf, (void *)(_l + index), 1); //copy expected answer length
  ptrBuf++;

  if (pe != NULL)
  {
    memcpy((void *)ptrBuf, (void *)pe, eLen); //if needed - copy ender
    ptrBuf += hLen;
  }

  //unsigned char
  _Query.DataLen = ptrBuf - (unsigned char *)&_Query.buf[2];           //calculate length of data in buf, w\o preamble and cs
  _Query.cs = calcCs((unsigned char *)&_Query.buf[2], _Query.DataLen); //calculate cs of buffer

  return 0;
}

void prepareCommand(unsigned char cmd)
{
  unsigned char *ptrBuf;

  _cmd.len = 4;
  _cmd.addr = 0x20;
  _cmd.rlen = 0x03;

  switch (cmd)
  {
  case CMD_CRUISE_ON: //0x7C, 0x01, 0x00
    _cmd.param = 0x7C;
    _cmd.value = 1;
    break;
  case CMD_CRUISE_OFF: //0x7C, 0x00, 0x00
    _cmd.param = 0x7C;
    _cmd.value = 0;
    break;
  case CMD_LED_ON: //0x7D, 0x02, 0x00
    _cmd.param = 0x7D;
    _cmd.value = 2;
    break;
  case CMD_LED_OFF: //0x7D, 0x00, 0x00
    _cmd.param = 0x7D;
    _cmd.value = 0;
    break;
  case CMD_WEAK: //0x7B, 0x00, 0x00
    _cmd.param = 0x7B;
    _cmd.value = 0;
    break;
  case CMD_MEDIUM: //0x7B, 0x01, 0x00
    _cmd.param = 0x7B;
    _cmd.value = 1;
    break;
  case CMD_STRONG: //0x7B, 0x02, 0x00
    _cmd.param = 0x7B;
    _cmd.value = 2;
    break;
  case CMD_LOCK: //4 20 3 70 1 0
    _cmd.param = 0x70;
    _cmd.value = 1;
    break;
  case CMD_UNLOCK: //4 20 3 71 1 0
    _cmd.param = 0x71;
    _cmd.value = 1;
    break;
  default:
    return; //undefined command - do nothing
    break;
  }
  ptrBuf = (unsigned char *)&_Query.buf;

  memcpy_P((void *)ptrBuf, (void *)_h0, sizeof(_h0)); //copy preamble
  ptrBuf += sizeof(_h0);

  memcpy((void *)ptrBuf, (void *)&_cmd, sizeof(_cmd)); //copy command body
  ptrBuf += sizeof(_cmd);

  //unsigned char
  _Query.DataLen = ptrBuf - (unsigned char *)&_Query.buf[2];           //calculate length of data in buf, w\o preamble and cs
  _Query.cs = calcCs((unsigned char *)&_Query.buf[2], _Query.DataLen); //calculate cs of buffer

  _Query.prepared = 1;
}

void writeQuery()
{
  RX_DISABLE;
  XIAOMI_PORT.write((unsigned char *)&_Query.buf, _Query.DataLen + 2); //DataLen + length of preamble
  XIAOMI_PORT.write((unsigned char *)&_Query.cs, 2);
  RX_ENABLE;
  _Query.prepared = 0;
}

unsigned int calcCs(unsigned char *data, unsigned char len)
{
  unsigned int cs = 0xFFFF;
  for (int i = len; i > 0; i--)
    cs -= *data++;

  return cs;
}

void showhibernate()
{

  displayClear(1);
  display.set2X();
  display.setFont(defaultFont);
  display.setCursor(0, 0);
  display.println((const __FlashStringHelper *)hibernate_info1);
  display.set1X();
  display.setFont(defaultFont);
  display.println((const __FlashStringHelper *)hibernate_info2);
  display.println((const __FlashStringHelper *)hibernate_info3);
  display.println((const __FlashStringHelper *)hibernate_info4);
  display.println((const __FlashStringHelper *)hibernate_info5);
  display.println((const __FlashStringHelper *)hibernate_info6);
  display.println((const __FlashStringHelper *)hibernate_info7);
}
