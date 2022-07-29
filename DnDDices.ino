#include "funshield.h"

///
///           Constants 
///

//Numbers:
constexpr int number_of_orders = 4;
constexpr int order_numbers[4] = {1, 10 ,100 ,1000};
constexpr int number_of_dices = 7;
constexpr int dice_types[number_of_dices] = {4,6,8,10,12,20,100};

//Hardware:
constexpr int number_of_buttons = 3;
constexpr int number_of_glyphs = 4;
constexpr int number_of_segments = 4;
constexpr int null_character = 10;
constexpr byte LETTER_GLYPH[] {
  0b10001000,   // A
  0b10000011,   // b
  0b11000110,   // C
  0b10100001,   // d
  0b10000110,   // E
  0b10001110,   // F
  0b10000010,   // G
  0b10001001,   // H
  0b11111001,   // I
  0b11100001,   // J
  0b10000101,   // K
  0b11000111,   // L
  0b11001000,   // M
  0b10101011,   // n
  0b10100011,   // o
  0b10001100,   // P
  0b10011000,   // q
  0b10101111,   // r
  0b10010010,   // S
  0b10000111,   // t
  0b11000001,   // U
  0b11100011,   // v
  0b10000001,   // W
  0b10110110,   // ksi
  0b10010001,   // Y
  0b10100100,   // Z
};
constexpr byte EMPTY_GLYPH = 0b11111111;

//Delays:
constexpr unsigned long big_delay = 1000;
constexpr unsigned long short_delay = 50;
constexpr unsigned long default_time = 0;

///
///           Variables
///

//Time:
unsigned long current_time = 0;

//Delays:
unsigned long button0_short_delay_factor = 0;
unsigned long button1_short_delay_factor = 0;
unsigned long button2_short_delay_factor = 0;

///
/// 
///

struct Button
{
  int pin;
  bool is_pressed;
  bool last_state;
  bool changed;                        
  unsigned long time_of_last_change;

  Button(int button_pin)
  {
    pin = button_pin;
    is_pressed = false;
    last_state = false;
    changed = false;
    time_of_last_change = default_time;
  }

  void setup()
  {
    pinMode(pin, INPUT);
  }

  void check_state()
  {
    is_pressed = !digitalRead(pin);
    changed = false;

    if(is_pressed != last_state)
    {
      time_of_last_change = current_time; 
      last_state = is_pressed;
      changed = true;
    }
  }

  unsigned long time_since_last_change()
  {
    return current_time - time_of_last_change;
  }
};

struct Display
{
  int to_show = 0;

  void setup()
  {
    pinMode(latch_pin, OUTPUT);
    pinMode(data_pin, OUTPUT);
    pinMode(clock_pin, OUTPUT);
  }

  void write_glyph(byte glyph, byte position)
  {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin,clock_pin,MSBFIRST,glyph);
    shiftOut(data_pin,clock_pin,MSBFIRST,position);
    digitalWrite(latch_pin, HIGH);
  }

  void display_char(char ch, byte pos)
  {
    byte glyph = EMPTY_GLYPH;
    if (isAlpha(ch)) {
      glyph = LETTER_GLYPH[ ch - (isUpperCase(ch) ? 'A' : 'a') ];
    }

    write_glyph(glyph, digit_muxpos[number_of_glyphs-pos-1]);
  }

  void display_number(int number, int order)
  {
    number = number / order_numbers[order];
    if(number == 0)
    {
      if((order == 3) || (order == 2) || (order == 1))
      {
        write_glyph(digits[null_character], digit_muxpos[number_of_glyphs-order-1]);
      }
    }
    else
    {
      number = number % 10;
      write_glyph(digits[number], digit_muxpos[number_of_glyphs-order-1]); // rotate since positions of segments in funshield are in reverse (-1 because indexing from 0)
    }
  }

  void show_whole_number(int number)
  {
    display_number(number, to_show);
    to_show++;
    to_show = to_show % number_of_segments;
  }

};

///
///          Hardware Structures
///
Display display;
///
///
///

struct DnDDices
{
  bool configuration_mode = true;
  int number_of_throws = 0; // Must add 1 
  int dice_type = 0;
  int result = 0;

  void change_number_of_throws()
  {
    if (!configuration_mode)
    {
      change_mode();
    }
    else
    {
      number_of_throws++;
      number_of_throws = number_of_throws % 9;
    }
  }

  void change_dice_type()
  {
    if (!configuration_mode)
    {
      change_mode();
    }
    else
    {
      dice_type++;
      dice_type = dice_type % number_of_dices;
    }
  }

  void generate_result()
  {
    result = random( 1 * (number_of_throws + 1), dice_types[dice_type] * (number_of_throws + 1));
  }

  void change_to_normal_mode()
  {
    if (configuration_mode)
    {
      change_mode();
    }
  }

  void change_mode()
  {
    configuration_mode = !configuration_mode;
  }

  void show()
  {
    if (configuration_mode)
    {
      display_configuration_mode();
    }
    else
    {
      display.show_whole_number(result);
    }
  }

  void display_configuration_mode()
  {
    int number_to_show = (number_of_throws + 1) * 1000 + dice_types[dice_type];
    if (display.to_show == 2)
    {
      display.display_char('d', display.to_show);
      display.to_show++;
    }
    else
    {
      display.show_whole_number(number_to_show);
    }
  }
};

///
///           Structures
///
DnDDices dnd;
///
///
///

struct Buttons
{
  Button buttons[number_of_buttons] = {Button(button1_pin), Button(button2_pin), Button(button3_pin)};

  void setup()
  {
    for(int i = 0; i < number_of_buttons; i++)
    {
      buttons[i].setup();
    }
  }

  void update()
  {
    for(int i = 0; i < number_of_buttons; i++)
    {
      buttons[i].check_state();
    }
  }

  void button0_action()
  {
    if(buttons[0].changed)
    {
      button0_short_delay_factor = 0;
      //here put what should happen after short click 
        dnd.change_to_normal_mode();
      //
    }
    else
    {
      if(buttons[0].time_since_last_change() >= big_delay + short_delay * button0_short_delay_factor)
      {
        button0_short_delay_factor++;
        //here put what should happen when you hold the button
        dnd.generate_result();
        //
      }
    }
  }

  void button1_action()
  {
    if(buttons[1].changed)
    {
      button1_short_delay_factor = 0;
      //here put what should happen after short click 
        dnd.change_number_of_throws();
      //
    }
    else
    {
      if(buttons[1].time_since_last_change() >= big_delay + short_delay * button1_short_delay_factor)
      {
        button1_short_delay_factor++;
        //here put what should happen when you hold the button

        //
      }
    }
  }

  void button2_action()
  {
    if(buttons[2].changed)
    {
      button2_short_delay_factor = 0;
      //here put what should happen after short click 
        dnd.change_dice_type();
      //
    }
    else
    {
      if(buttons[2].time_since_last_change() >= big_delay + short_delay * button2_short_delay_factor)
      {
        button2_short_delay_factor++;
        //here put what should happen when you hold the button

        //
      }
    }
  }
};

///
///          Hardware Structures
///
Buttons buttons;
///
///
///

void setup() {
  // put your setup code here, to run once:

  buttons.setup();
  display.setup();
}

void loop() {
  // put your main code here, to run repeatedly:
  current_time = millis();

  buttons.update();

  if(buttons.buttons[0].is_pressed)
  {
    buttons.button0_action();
  }

  if(buttons.buttons[1].is_pressed)
  {
    buttons.button1_action();
  }

  if(buttons.buttons[2].is_pressed)
  {
    buttons.button2_action();
  }

  dnd.show();
}