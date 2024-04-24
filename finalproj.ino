/*
Team Name:
Team Members: Brandon Ramirez, Austin Parkerson, Robert Fleming
Date: 4/23/2024
*/

// #include <LiquidCrystal.h>
// #include <Stepper.h>
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
volatile unsigned char *portDDRB = (unsigned char *) 0x24;
volatile unsigned char *portB = (unsigned char *) 0x25;
volatile unsigned char *pinB = (unsigned char *) 0x23;
volatile unsigned char *portDDRE = (unsigned char *) 0x2D;
volatile unsigned char *portE = (unsigned char *) 0x2E;
volatile unsigned char *pinE = (unsigned char *) 0x2C;

// Global Parameters
dht DHT;
RTC_DS1307 rtc;

#define DHT11_PIN 10
// #define WATER_LEVEL_POWER_PIN 
// #define WATER_LEVEL_SIGNAL_PIN 
const int stepsPerRevolution = 2038;
// Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);
enum State {DISABLED, IDLE, ERROR, RUNNING} state;
unsigned int startButton = 0;
unsigned int monitorWater = 0;
unsigned int triggerWaterRead = 0;
unsigned int monitorTemp = 0;
unsigned int triggerTempRead = 0;
unsigned int monitorHumidity = 0;
unsigned int triggerHumiRead = 0;
unsigned int monitorVent = 0;
unsigned int water_level_val;
unsigned int one_minute_counter = 0;
unsigned int triggerGetTime = 0;

// Start Button = Digital Pin 2, PE4
// Stop Button = Digital Pin 3, PE5
// Reset Button = Digital Pin 18, PB6
// Temp Up Button = Digital Pin 19, PB7
// Temp Down Button = Digital Pin 20, PD0
// Toggle Vent Button = Digital Pin 21, PD1

/*
State Descriptions:
ALL STATES: RTC is used to report time of state transitions, time and changes to stepper motor position are reported
ALL STATES EXCEPT DISABLED: Humidity & Temp reported every minute to LCD Screen, system responds to changes in vent control, stop button turns fan motor off: turns system to disabled
DISABLED: yellow led on, to monitoring of temp or water, start button monitored with ISR
IDLE: green led on, exact timestamp records transition times, water level is mononitored: if too low, error state: red led
ERROR: motor does not start automatically, reset button triggers change to IDLE state iff water level is above threshold, error message displayed on LCD, red led
RUNNING: fan motor on, transistion to idle on temp drop below thresh, transition to error if water level becomes to low, blue led is on
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
  // setup the reset button ISR
  // attachInterrupt(digitalPinToInterrupt(18), reset_button, RISING);
  // // setup the tempup button ISR
  // attachInterrupt(digitalPinToInterrupt(19), tempup_button, RISING);
  // // setup the tempdown button ISR
  // attachInterrupt(digitalPinToInterrupt(20), tempdown_button, RISING);
  // // setup the togglevent button ISR
  // attachInterrupt(digitalPinToInterrupt(21), togglevent_button, RISING);

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

  // set PB6 to input; digital pin 12
  *portDDRB &= 0b10111111;
  // set PE4 to input; digital pin 2
  *portDDRE &= 0b11101111;
  // set PE5 to input; digital pin 3
  *portDDRE &= 0b11011111;
  // set monitorWater to true
  monitorWater = 1;
  // set monitorTemp to true
  monitorTemp = 1;
  // set monitorHumidity to true
  monitorHumidity = 1;

}

void loop(){
  if (triggerGetTime){
    get_time();
    triggerGetTime = 0;
  }
  if (triggerWaterRead){
    get_water_level();
    triggerWaterRead = 0;
  }
  if (triggerTempRead){
    get_temperature();
    triggerTempRead = 0;
  }
  if (triggerHumiRead){
    get_humidity();
    triggerHumiRead = 0;
  }
  /*
  set power_pin high
  delay 10ms
  read water_level_signal_pin
  set power_pin low
  print water_level_signal_pin to UART
  */

  // int chk = DHT.read11(DHT11_PIN);
  // U0puts("\nTemperature = ");
  // U0puts(DHT.temperature);
  // U0puts("\nHumidity = ");
  // U0puts(DHT.humidity);

  // // check if the start button is pressed
  // if(*myPIND & 0x04){
  //   // check if the system is in the disabled state
  //   if(state == DISABLED){
  //     // change the state to idle
  //     state = IDLE;
  //     // turn off the yellow LED
  //     *myPORTB &= 0b11111011;
  //     // turn on the green LED
  //     *myPORTB |= 0b00000100;
  //     // report the state transition
  //     U0putchar('D');
  //     U0putchar('I');
  //     U0putchar(' ');
  //     U0putchar('T');
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
  if (one_minute_counter == 1200){
    one_minute_counter = 0;
    triggerGetTime = 1;
    if (monitorTemp){
      triggerTempRead = 1;
    }
    if (monitorHumidity){
      triggerHumiRead = 1;
    }
    if (monitorWater){
      triggerWaterRead = 1;
    }
  }
}
void get_time(){
  DateTime now = rtc.now();
  unsigned char snum[8] = {0};
  U0puts("Time: ");
  sprintf(snum, "%02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  U0puts(snum);
}
void get_temperature(){
  int chk = DHT.read11(DHT11_PIN);
  unsigned char snum[6] = {0};
  U0puts("Temperature = ");
  int temp = (int) DHT.temperature;
  sprintf(snum, "%d\n", temp);
  U0puts(snum);
}
void get_humidity(){
  int chk = DHT.read11(DHT11_PIN);
  unsigned char snum[6] = {0};
  U0puts("Humidity = ");
  int humidity = (int) DHT.humidity;
  sprintf(snum, "%d\n", humidity);
  U0puts(snum);
}
void get_water_level(){
  // result of the water level signal
  unsigned char snum[6] = {0};
  // set the power pin high (PB6)
  *portB |= 0b01000000;
  // delay 10ms
  //delay(100);
  // read the water level signal pin
  water_level_val = adc_read(0);
  // set the power pin low
  *portB &= 0b10111111;
  // print the water level signal to the UART
  sprintf(snum, "%d", water_level_val);
  U0puts("Water Level: ");
  U0puts(snum);
  U0puts("\n");
}
// Start Button ISR
void start_button(){
  state = RUNNING;
  print_state();
  state = IDLE;
  print_state();
}
// Stop Button ISR
void stop_button(){
  U0puts("Stop Button Pressed\n");
}
// Reset Button ISR
void reset_button(){
  U0puts("Reset Button Pressed\n");
}
// Temp Up Button ISR
void tempup_button(){
  U0puts("Temp Up Button Pressed\n");
}
// Temp Down Button ISR
void tempdown_button(){
  U0puts("Temp Down Button Pressed\n");
}
// Toggle Vent Button ISR
void togglevent_button(){
  U0puts("Toggle Vent Button Pressed\n");
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