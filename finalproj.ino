/*
Team Name: hello world 💓
Team Members: Brandon Ramirez, Austin Parkerson, Robert Fleming
Date: 4/23/2024
*/

#include <LiquidCrystal.h>
#include <Stepper.h>
#include <dht.h>
#include <RTClib.h>

// UART Pointers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
// ADC Pointers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;
// Timer Pointers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int *myTCNT1 = (unsigned int *) 0x84;
volatile unsigned char *myTIFR1 = (unsigned char *) 0x36;
// Port Pointers
volatile unsigned char *portDDRA = (unsigned char *) 0x21;
volatile unsigned char *portA = (unsigned char *) 0x22;
volatile unsigned char *pinA = (unsigned char *) 0x20;
volatile unsigned char *portDDRB = (unsigned char *) 0x24;
volatile unsigned char *portB = (unsigned char *) 0x25;
volatile unsigned char *pinB = (unsigned char *) 0x23;
volatile unsigned char *portDDRC = (unsigned char *) 0x27;
volatile unsigned char *portC = (unsigned char *) 0x28;
volatile unsigned char *pinC = (unsigned char *) 0x26;
volatile unsigned char *portDDRE = (unsigned char *) 0x2D;
volatile unsigned char *portE = (unsigned char *) 0x2E;
volatile unsigned char *pinE = (unsigned char *) 0x2C;
volatile unsigned char *portDDRG = (unsigned char *) 0x33;
volatile unsigned char *portG = (unsigned char *) 0x34;
volatile unsigned char *pinG = (unsigned char *) 0x32;
volatile unsigned char *portDDRH = (unsigned char *) 0x101;
volatile unsigned char *portH = (unsigned char *) 0x102;
volatile unsigned char *pinH = (unsigned char *) 0x100;


// Global Parameters
dht DHT;
RTC_DS1307 rtc;

#define DHT11_PIN 10
const int stepsPerRevolution = 2038;
Stepper myStepper = Stepper(stepsPerRevolution, 50, 51, 52, 53);
const int rs = 42, en = 43, d4 = 44, d5 = 45, d6 = 46, d7 = 47;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
enum State {DISABLED, IDLE, ERROR, RUNNING, UNKNOWN} state;
unsigned int startButton = 0;
unsigned int monitorWater = 0;
unsigned int triggerWaterRead = 0;
unsigned int monitorTempandHumi = 0;
unsigned int triggerTempandHumiRead = 0;
unsigned int monitorVent = 1;
unsigned int triggerVentChange = 0;
unsigned int water_level_val;
unsigned int one_minute_counter = 0;
unsigned int triggerGetTime = 0;
unsigned int set_state_to_idle_flag = 0;
int water_level_threshold = 100;
int water_level = 0;
int temp_threshold = 35;
int temp = 0;
State prev_state = UNKNOWN; // use placeholder state for initial state
/*
PIN ASSIGNMENTS:
DHT11_PIN: 10
WATER_LEVEL_POWER_PIN: 12, PB6
WATER_LEVEL_SIGNAL_PIN: A0
START_BUTTON: 2, PE4
STOP_BUTTON: 3, PE5
STEPPER MOTOR POSITION CONTROL: A1
BLOWER MOTOR ON/OFF: 7, PH4
RESET BUTTON: 23, PA1
TEMP UP BUTTON: 25, PA3
TEMP DOWN BUTTON: 27, PA5
LCD {
  RS: 42
  EN: 43
  D4: 44
  D5: 45
  D6: 46
  D7: 47
}
BLUE LED (RUNNING): 35, PC2
GREEN LED (IDLE): 37, PC0
YELLOW LED (DISABLED): 39, PG2
RED LED (ERROR): 41, PG0
STEPPER MOTOR PINS {
  IN1: 50
  IN2: 51
  IN3: 52
  IN4: 53
}
*/

void setup(){
  // setup the UART
  U0init(9600);
  // setup the ADC
  adc_init();
  // setup the timer
  setup_timer_regs();
  // setup the start button ISR
  attachInterrupt(digitalPinToInterrupt(2), start_button, RISING);
  // setup the stop button ISR
  attachInterrupt(digitalPinToInterrupt(3), stop_button, RISING);

  // check for RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  // check for RTC time
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  U0puts("RTC is running\n");

  // set PB6 to output; digital pin 12: water level power pin
  *portDDRB |= 0b01000000;
  // set PE4 to input; digital pin 2: start button
  *portDDRE &= 0b11101111;
  // set PE5 to input; digital pin 3: stop button
  *portDDRE &= 0b11011111;
  // set PA1 to input; digital pin 23: reset button
  *portDDRA &= 0b11111101;
  // set PA3 to input; digital pin 25: temp up button
  *portDDRA &= 0b11110111;
  // set PA5 to input; digital pin 27: temp down button
  *portDDRA &= 0b11011111;
  // set PC2 to output; digital pin 35: blue led
  *portDDRC |= 0b00000100;
  // set PC0 to output; digital pin 37: green led
  *portDDRC |= 0b00000001;
  // set PG2 to output; digital pin 39: yellow led
  *portDDRG |= 0b00000100;
  // set PG0 to output; digital pin 41: red led
  *portDDRG |= 0b00000001;
  // set PB0 to output; digital pin 53: blower motor on/off
  *portDDRB |= 0b00000001;
  // set PH4 to output; digital pin 7: stepper motor on/off
  *portDDRH |= 0b00010000;
  // set the stepper motor pins to output
  *portDDRB |= 0b00001111;

  // set the state to be disabled initially
  state = DISABLED;
  print_state();
  get_time();
  myStepper.setSpeed(10);
  // set the blower motor to off
  blower_motor_off();

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Humidifier");

}

// state logic: DISABLED -> IDLE -> ERROR -> IDLE -> RUNNING -> IDLE -> DISABLED
/*
State Descriptions:
ALL STATES: RTC is used to report time of state transitions, time and changes to stepper motor position are reported
ALL STATES EXCEPT DISABLED: Humidity & Temp reported every minute to LCD Screen, system responds to changes in vent control, stop button turns fan motor off: turns system to disabled
DISABLED: yellow led on, to monitoring of temp or water, start button monitored with ISR
IDLE: green led on, exact timestamp records transition times, water level is mononitored: if too low, error state: red led
ERROR: motor does not start automatically, reset button triggers change to IDLE state iff water level is above threshold, error message displayed on LCD, red led
RUNNING: fan motor on, transistion to idle on temp drop below thresh, transition to error if water level becomes to low, blue led is on
*/

// start in idle state

void loop(){
  if (state != prev_state){ // check for state change and print time and new state
    if (state == IDLE || state == RUNNING){
      lcd.clear();
      triggerTempandHumiRead = 1;
    }
    print_state();
    get_time();
    prev_state = state;
  }
  // check the state of the system
  switch (state){
  case DISABLED:
    // blank_lcd();
    disabled_led_on();
    idle_led_off();
    error_led_off();
    running_led_off();
    monitorTempandHumi = 0;
    monitorWater = 1;
    monitorVent = 1;
    blower_motor_off();
    break;
  case IDLE:
    // blank_lcd();
    disabled_led_off();
    idle_led_on();
    error_led_off();
    running_led_off();
    blower_motor_off();
    monitorTempandHumi = 1;
    monitorWater = 1;
    monitorVent = 1;
    if (water_level_val < water_level_threshold){
      state = ERROR; // water level too low
      U0puts("ERROR: Water Level Too Low\n");
      unsigned char snum[6] = {0};
      sprintf(snum, "%d", water_level_val);
      U0puts(snum);
      blank_lcd();
      lcd.setCursor(0, 0);
      lcd.print("ERROR: Water Level");
      lcd.setCursor(0, 1);
      lcd.print("Too Low");
    }
    if (temp >= temp_threshold){
      U0puts("Temp: ");
      U0puts("OK\n");
      state = RUNNING; // temp too high
    }
    break;
  case ERROR:
    // blank_lcd();
    disabled_led_off();
    idle_led_off();
    error_led_on();
    running_led_off();
    blower_motor_off();
    monitorTempandHumi = 0;
    monitorWater = 1;
    monitorVent = 0;
    if (water_level_val >= water_level_threshold){
      U0puts("Water Level: ");
      U0puts("OK\n");
      state = IDLE;
      blank_lcd();
    }
    break;
  case RUNNING:
    // blank_lcd();
    disabled_led_off();
    idle_led_off();
    error_led_off();
    running_led_on();
    blower_motor_on();
    monitorTempandHumi = 1;
    monitorVent = 1;
    monitorWater = 1;
    if (water_level_val < water_level_threshold){
      state = ERROR; // water level too low
      U0puts("Water Level: ");
      U0puts("Too Low\n");
      blank_lcd();
      lcd.setCursor(0, 0);
      lcd.print("ERROR: Water Level");
      lcd.setCursor(0, 1);
      lcd.print("Too Low");
    }
    if (temp < temp_threshold){
      U0puts("Temp: ");
      U0puts("Too Low\n");
      state = IDLE; // temp too low
    }
    break;
  default:
    break;
  }
  if (triggerGetTime){
    get_time();
    triggerGetTime = 0;
  }
  if (monitorWater){
    get_water_level();
    // triggerWaterRead = 0;
  }
  if (triggerTempandHumiRead){
    get_temperature_and_humidity();
    triggerTempandHumiRead = 0;
  }
  if (monitorVent){
    // check the state of the vent control
    int vent_posit = adc_read(1);
    if (vent_posit > 400 && !triggerVentChange){
      triggerVentChange = 1;
      myStepper.step(200);
      U0puts("Opening Vent\n");
      get_time();
    } else if (vent_posit < 100 && triggerVentChange){
      triggerVentChange = 0;
      myStepper.step(-200);
      U0puts("Closing Vent\n");
      get_time();
    }
  }
  if (set_state_to_idle_flag){
    U0puts("Start Button Pressed\n");
    state = IDLE;
    set_state_to_idle_flag = 0;
  }
  // reset button
  if (*pinA & 0b00000010){
    print_state();
    if (state == ERROR){
      state = IDLE;
    }
  }
  // temp up button
  if (*pinA & 0b00001000){
    temp_threshold++;
  }
  // temp down button
  if (*pinA & 0b00100000){
    temp_threshold--;
  }
}
// TIMER OVERFLOW ISR (1 minute)
ISR(TIMER1_OVF_vect){
  // stop the timer
  *myTCCR1B &= 0xF8;
  // Load the Count
  *myTCNT1 = 0;
  // Start the Timer
  *myTCCR1B |= 0b00000001;
  // increment the one minute counter
  one_minute_counter++;
  // one_minute_counter = 14400 for 1 minute
  // one_minute_counter = 1200 for 5 seconds
  if (one_minute_counter == 14400){ // in 5 seconds mode for testing
    one_minute_counter = 0;
    // triggerGetTime = 1; //only during state change and vent control
    if (monitorTempandHumi){
      triggerTempandHumiRead = 1;
    }
    // if (monitorWater){
    //   triggerWaterRead = 1;
    // }
  }
}
void get_time(){
  // print_state();
  DateTime now = rtc.now();
  unsigned char snum[8] = {0};
  U0puts("Time: ");
  sprintf(snum, "%02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  U0puts(snum);
}
void display_time(){
  DateTime now = rtc.now();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(now.hour());
  lcd.print(":");
  lcd.print(now.minute());
  lcd.print(":");
  lcd.print(now.second());
  lcd.setCursor(0, 1);
}
void get_temperature_and_humidity(){
  blank_lcd();
  int chk = DHT.read11(DHT11_PIN);
  temp = (int) DHT.temperature;
  int humidity = (int) DHT.humidity;
  lcd.setCursor(1, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print("C");
  // set the cursor to the second line
  lcd.setCursor(0, 1);
  lcd.print(" Humidity: ");
  lcd.print(humidity);
  lcd.print("%");
}
void get_water_level(){
  // result of the water level signal
  unsigned char snum[6] = {0};
  // set the power pin high (PB6)
  *portB |= 0b01000000;
  // read the water level signal pin
  water_level_val = adc_read(0);
  // set the power pin low
  *portB &= 0b10111111;
  // print the water level signal to the UART
  // sprintf(snum, "%d", water_level_val);
  // U0puts("Water Level: ");
  // U0puts(snum);
  // U0puts("\n");
}
// Start Button ISR
void start_button(){
  // U0puts("Start Button Pressed\n");
  set_state_to_idle_flag = 1;
  // state = IDLE;
  // print_state();
}
// Stop Button ISR
void stop_button(){
  state = DISABLED;
}
void print_state(){
  switch(state){
    case DISABLED:
      U0puts("DISABLED\n");
      break;
    case IDLE:
      U0puts("IDLE\n");
      break;
    case ERROR:
      U0puts("ERROR\n");
      break;
    case RUNNING:
      U0puts("RUNNING\n");
      break;
  }
}
void blank_lcd(){
  lcd.clear();
}
void open_vent(){
  myStepper.setSpeed(5);
  myStepper.step(100);
}
void close_vent(){
  myStepper.setSpeed(5);
  myStepper.step(-100);
}

// ******************************************************************************************************************************************
void adc_init(){
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}
unsigned int adc_read(unsigned char adc_channel_num){
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}
void U0init(int U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit(){
  return (*myUCSR0A & (1 << 7));
}
unsigned char getChar(){
  while(!(*myUCSR0A & (1 << 7)));
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata){
  while(!(*myUCSR0A & (1 << 5)));
  *myUDR0 = U0pdata;
}
// Function to send an entire string to the UART
void U0puts(char *U0pstr){
  while(*U0pstr){
    U0putchar(*U0pstr++);
  }
}
void running_led_on(){
  *portC |= 0b00000100;
}
void running_led_off(){
  *portC &= 0b11111011;
}
void idle_led_on(){
  *portC |= 0b00000001;
}
void idle_led_off(){
  *portC &= 0b11111110;
}
void disabled_led_on(){
  *portG |= 0b00000100;
}
void disabled_led_off(){
  *portG &= 0b11111011;
}
void error_led_on(){
  *portG |= 0b00000001;
}
void error_led_off(){
  *portG &= 0b11111110;
}
void blower_motor_on(){
  *portH |= 0b00010000;
}
void blower_motor_off(){
  *portH &= 0b11101111;
}
// Timer setup function (1 minute)
void setup_timer_regs(){
  // setup the timer control registers
  *myTCCR1A= 0x00;
  *myTCCR1B = 0x05; // set the prescaler to 256
  *myTCCR1C= 0x00;
  // reset the TOV flag
  *myTIFR1 |= 0x01;
  // enable the TOV interrupt
  *myTIMSK1 |= 0x01;
}