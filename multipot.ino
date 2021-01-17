#include <EEPROM.h>
#include <LedControl.h>

byte max_cs_pin = 10;
byte max_clk_pin = 11;
byte max_data_pin = 12;

LedControl disp = LedControl(max_data_pin, max_clk_pin, max_cs_pin, 1);
byte max_digits = 6;

byte pin_btn[] = {2, 3, 4, 5, 6};
bool b_btn_status_p[] = {false, false, false, false, false, false};
bool b_btn_status_c[] = {false, false, false, false, false, false};
byte b_btn_state[] = {0, 0, 0, 0, 0, 0};
bool btn_pressed = false;
bool btn_pressed_timeout = false;
bool btn_pressed_action = false;
bool btn_disable = false;

bool led_status = false;
byte pin_led_status = 7;

bool led_heat = false;
byte pin_led_heat = 8;

bool relay = false;
byte pin_relay = 9;

bool buzzer = false;
bool buzzer_btn = false;
bool buzzer_cook_end = false;
byte pin_buzzer = 13;
byte pin_buzzer_plus = A1;

uint32_t now = 0;
uint32_t timer[] = {0, 0, 0, 0, 0, 0, 0};

char charArray[] = {
  // TBCADEFG
  B01111110,
  B01100000,
  B01011101,
  B01111001,
  B01100011,
  B00111011,
  B00111111,
  B01110000,
  B01111111,
  B01111011
};

byte power = 2;
byte power_start = 2;
uint16_t msec_per_power_percent = 2000;
uint16_t msec_power_update = power * msec_per_power_percent;

uint16_t counter_start = 30 * 60;
uint16_t counter_cur = 30 * 60;

bool blinker = false;
byte work_mode_prev = 0;
byte work_mode = 0;   // 0 - stop, 1 - run, 2 - pause, 3 - cook end

byte edit_mode = 0;   // 0 - no edit, 1 - power, 2 - counter
byte edit_timeout = 0;
byte edit_timeout_max = 5;

bool afterheat_mode = false;

byte digits[5] = {0, 0, 0, 0, 0};

//

void maxFill(byte val)
{
  for (byte i = 0; i < max_digits; i++)
  {
    disp.setRow(0, i, val);
  }
}

byte getDigitsFromNumber(uint32_t number, byte digits, char format[], byte numbarr[])
{
  // itoa(number, numbarr, 10);
  sprintf(numbarr, format, number);
  for (byte i = 0; i < digits; i++)
  {
    numbarr[i] -= '0';
  }
}

void drawUI()
{
  if (work_mode != 3 && (edit_mode == 1 && blinker || edit_mode != 1) || work_mode == 3 && blinker)
  {
    getDigitsFromNumber(power, 2, "%2u", digits);
    disp.setRow(0, 0, charArray[digits[0]] | (afterheat_mode ? (work_mode == 1 ? (blinker ? B10000000 : 0) : B10000000) : 0));
    disp.setRow(0, 1, charArray[digits[1]] | (afterheat_mode ? (work_mode == 1 ? (blinker ? B10000000 : 0) : B10000000) : 0));
  }
  else
  {
    disp.setRow(0, 0, afterheat_mode && work_mode != 3 ? B10000000 : 0);
    disp.setRow(0, 1, afterheat_mode && work_mode != 3 ? B10000000 : 0);
  }

  if (work_mode != 3 && (edit_mode == 2 && blinker || edit_mode != 2) || work_mode == 3 && blinker)
  {
    uint16_t mins = counter_cur / 60;
    // uint16_t hrs = mins / 60;
    // mins -= hrs * 60;
    getDigitsFromNumber(mins, 3, "%3u", digits);
    // disp.setRow(0, 3, charArray[hrs > 9 ? 9 : hrs] | (work_mode == 1 ? (blinker ? B10000000 : 0) : B10000000));
    disp.setRow(0, 2, work_mode == 1 ? (blinker ? B10000000 : 0) : B10000000);
    disp.setRow(0, 3, charArray[digits[0]] | (afterheat_mode ? (work_mode == 1 ? (blinker ? B10000000 : 0) : B10000000) : 0));
    disp.setRow(0, 4, charArray[digits[1]] | (afterheat_mode ? (work_mode == 1 ? (blinker ? B10000000 : 0) : B10000000) : 0));
    disp.setRow(0, 5, charArray[digits[2]] | (afterheat_mode ? (work_mode == 1 ? (blinker ? B10000000 : 0) : B10000000) : 0));
  }
  else
  {
    disp.setRow(0, 2, afterheat_mode && work_mode != 3 ? B10000000 : 0);
    disp.setRow(0, 3, afterheat_mode && work_mode != 3 ? B10000000 : 0);
    disp.setRow(0, 4, afterheat_mode && work_mode != 3 ? B10000000 : 0);
    disp.setRow(0, 5, afterheat_mode && work_mode != 3 ? B10000000 : 0);
  }

  if (work_mode == 2)
  {
    led_status = blinker;
  }
  else if (work_mode == 3)
  {
    led_status = blinker;
    if (buzzer_cook_end)
    {
      buzzer = blinker;
    }
    else
    {
      buzzer = false;
    }
  }

  digitalWrite(pin_relay, relay);
  digitalWrite(pin_led_status, !led_status);
  digitalWrite(pin_led_heat, !led_heat);
  digitalWrite(pin_buzzer, !buzzer);
  digitalWrite(pin_buzzer_plus, buzzer);

  //  for (byte i = 0; i < sizeof(pin_btn); i++)
  //  {
  //    disp.setRow(0, i, charArray[b_btn_status_c[i] ? 1 : 0]);
  //  }
}

void setup()
{
  // put your setup code here, to run once:

  // initialize serial interface
  // Serial.begin(19200);

  // recycle_pipe	= EEPROM.read(0);

  for (byte i = 0; i < sizeof(pin_btn); i++)
  {
    pinMode(pin_btn[i], INPUT_PULLUP);
  }

  pinMode(pin_relay, OUTPUT);
  pinMode(pin_led_status, OUTPUT);
  pinMode(pin_led_heat, OUTPUT);
  pinMode(pin_buzzer, OUTPUT);
  pinMode(pin_buzzer_plus, OUTPUT);
  
  digitalWrite(pin_relay, relay);
  digitalWrite(pin_led_status, !led_status);
  digitalWrite(pin_led_heat, !led_heat);
  digitalWrite(pin_buzzer, !buzzer);
  digitalWrite(pin_buzzer_plus, buzzer);

  disp.shutdown(0, false);
  disp.setIntensity(0, 8);
  // disp.clearDisplay(0);
  maxFill(B11111111);

  delay(2000);

  // read last power and counter fromm eeprom
  EEPROM.get(0, power_start);
  EEPROM.get(1, counter_start);
  EEPROM.get(3, afterheat_mode);

  power_start = power_start > 10 ? 10 : (power_start < 0 ? 0 : power_start);
  counter_start = (counter_start / (5 * 60)) * 5 * 60;

  power = power_start;
  counter_cur = counter_start;

  drawUI();
}

void loop()
{
  // put your main code here, to run repeatedly:

  // Serial.print(pipe_3);
  // Serial.println(".\n");

  // EEPROM.put(0, recycle_pipe);

  // delay(1000);

  now = millis();

  // reading buttons
  if (now - timer[0] >= 50)
  {
    timer[0] = now;

    if (buzzer_btn)
    {
      buzzer_btn = false;
      digitalWrite(pin_buzzer, HIGH);
      digitalWrite(pin_buzzer_plus, LOW);
    }

    if (btn_disable)
    {
      btn_disable = false;
    }

    for (byte i = 0; i < sizeof(pin_btn); i++)
    {
      b_btn_status_c[i] = digitalRead(pin_btn[i]);

      if (!b_btn_status_c[i] && !b_btn_status_p[i])
      {
        b_btn_state[i] = 0; // released
      }
      else if (!b_btn_status_c[i] && b_btn_status_p[i])
      {
        b_btn_state[i] = 1; // releasing

        // btn_pressed = false;
        btn_pressed_timeout = false;
        // btn_pressed_action = false;
      }
      else if (b_btn_status_c[i] && !b_btn_status_p[i])
      {
        b_btn_state[i] = 2; // pressing

        btn_pressed = false;
        btn_pressed_timeout = true;
        btn_pressed_action = false;
        timer[5] = now;

        buzzer_btn = true;
        digitalWrite(pin_buzzer, LOW);
        digitalWrite(pin_buzzer_plus, HIGH);

        if (edit_mode != 0)
        {
          edit_timeout = edit_timeout_max;
        }

        if (work_mode == 3)
        {
          work_mode = 0;
          edit_mode = 0;
          counter_cur = counter_start;
          power = power_start;
          msec_power_update = power * msec_per_power_percent;
          buzzer = false;
          buzzer_cook_end = false;
          led_status = false;
          relay = false;
          led_heat = false;
          btn_disable = true;
        }
      }
      else if (b_btn_status_c[i] && b_btn_status_p[i])
      {
        b_btn_state[i] = 3; // pressed
      }

      b_btn_status_p[i] = b_btn_status_c[i];
    }

    if (btn_pressed)
    {
      edit_timeout = edit_timeout_max;
    }

    work_mode_prev = work_mode;

    if (!btn_disable && b_btn_state[0] == 2) // 'Start' btn
    {
      work_mode = work_mode != 1 ? 1 : 0;

      if (work_mode == 0)
      {
        counter_cur = counter_start;
        power = power_start;
        relay = false;
        led_heat = false;
        led_status = false;
      }
      else if (work_mode == 1)
      {
        relay = true;
        led_heat = true;
        led_status = true;
        timer[4] = now;
        msec_power_update = power * msec_per_power_percent;
        if (work_mode_prev == 0)
        {
          counter_start = counter_cur;
          power_start = power;

          // save power & counter to eeprom
          EEPROM.put(0, power_start);
          EEPROM.put(1, counter_start);
        }
      }

      edit_mode = 0;
    }

    if (!btn_disable && b_btn_state[4] == 1 && !btn_pressed) // 'Pause' btn released not hold
    {
      work_mode = work_mode == 1 ? 2 : (work_mode == 2 ? 1 : work_mode);

      if (work_mode == 2)
      {
        relay = false;
        led_heat = false;
      }
      else if (work_mode == 1)
      {
        relay = true;
        led_heat = true;
        led_status = true;
        timer[4] = now;
        msec_power_update = power * msec_per_power_percent;
      }

      edit_mode = 0;
    }
    else if (!btn_disable && b_btn_state[4] == 3 && btn_pressed && !btn_pressed_action) // 'Pause' btn hold
    {
      buzzer_btn = true;
      digitalWrite(pin_buzzer, LOW);
      digitalWrite(pin_buzzer_plus, HIGH);

      afterheat_mode = afterheat_mode ? false : true;
      btn_pressed_action = true;

      // save afterheat mode to eeprom
      EEPROM.put(3, afterheat_mode);
    }

    if (!btn_disable && b_btn_state[2] == 2) // 'Menu' btn
    {
      edit_mode += 1;
      if (edit_mode > 2)
      {
        edit_mode = 1;
      }

      edit_timeout = edit_timeout_max;
    }

    if (!btn_disable && (b_btn_state[1] == 2 || btn_pressed && b_btn_state[1] == 3)) // 'Left' btn
    {
      if (edit_mode == 1)
      {
        if (power > 0)
        {
          power -= 1;

          if (relay)
          {
            msec_power_update = power * msec_per_power_percent;
          }
          else
          {
            msec_power_update = (10 - power) * msec_per_power_percent;
          }
        }
      }
      else if (edit_mode == 2)
      {
        if (counter_cur >= 5 * 60)
        {
          counter_cur -= 5 * 60;
        }
        else
        {
          counter_cur = 0;
        }
      }
    }
    else if (!btn_disable && (b_btn_state[3] == 2 || btn_pressed && b_btn_state[3] == 3)) // 'Right' btn
    {
      if (edit_mode == 1)
      {
        if (power < 10)
        {
          power += 1;

          if (relay)
          {
            msec_power_update = power * msec_per_power_percent;
          }
          else
          {
            msec_power_update = (10 - power) * msec_per_power_percent;
          }
        }
      }
      else if (edit_mode == 2)
      {
        if (counter_cur < (995 * 60))
        {
          counter_cur += 5 * 60;
        }
        else
        {
          counter_cur = 995 * 60;
        }
      }
    }

    
  }

  // 1/sec
  if (now - timer[1] >= 1000)
  {
    timer[1] = now;

    if (work_mode == 1)
    {
      if (counter_cur == 0)
      {
        work_mode = 3;
        relay = afterheat_mode ? true : false;
        led_heat = afterheat_mode ? true : false;
        led_status = false;
        buzzer_cook_end = true;
        timer[6] = now;

        if (afterheat_mode)
        {
          timer[4] = now;
          power = 1;
          msec_power_update = power * msec_per_power_percent;
        }
      }
      else if (counter_cur > 0)
      {
        counter_cur -= 1;
      }
    }

    if (edit_mode > 0)
    {
      if (edit_timeout == 0)
      {
        edit_mode = 0;
      }
      else if (edit_timeout > 0)
      {
        edit_timeout -= 1;
      }
    }
  }

  // blink timeout
  if (now - timer[2] >= 500)
  {
    timer[2] = now;
    blinker = blinker ? false : true;
  }

  // update display
  if (now - timer[3] >= 100)
  {
    timer[3] = now;
    drawUI();
  }

  // power timer
  if (work_mode == 1 || afterheat_mode && work_mode == 3)
  {
    if (now - timer[4] >= msec_power_update)
    {
      timer[4] = now;

      if (power == 10 || !relay)
      {
        relay = true;
        led_heat = true;

        msec_power_update = power * msec_per_power_percent;
      }
      else if (relay)
      {
        relay = false;
        led_heat = false;

        msec_power_update = (10 - power) * msec_per_power_percent;
      }
    }
  }

  if (btn_pressed_timeout)
  {
    if (now - timer[5] >= 1000)
    {
      timer[5] = now;
      btn_pressed_timeout = false;
      btn_pressed = true;
    }
  }

  if (buzzer_cook_end)
  {
    if (now - timer[6] >= 5000)
    {
      timer[6] = now;
      buzzer_cook_end = false;
    }
  }
}
