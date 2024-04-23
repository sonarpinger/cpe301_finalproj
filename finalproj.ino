/*
Team Name:
Team Members: Brandon Ramirez, Austin Parkerson, Robert Fleming
Date: 4/23/2024
*/

// #include <LiquidCrystal.h>
// #include <Stepper.h>
// #include <dht.h>

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

// Global Parameters
// #define DHT11_PIN 
// #define WATER_LEVEL_POWER_PIN 
// #define WATER_LEVEL_SIGNAL_PIN 
const int stepsPerRevolution = 2038;
// Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);
enum State {DISABLED, IDLE, ERROR, RUNNING};
unsigned int startButton = 0;
unsigned int monitorWater = 0;
unsigned int monitorTemp = 0;
unsigned int monitorHumidity = 0;
unsigned int monitorVent = 0;
unsigned int water_level_val;
unsigned int one_minute_counter = 0;
// dht DHT;

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

  /*
  set water_level_power_pin to output
  set water_level_signal_pin to input
  set power_pin low
  */
}

void loop(){
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
  if (one_minute_counter == 12000){
    U0puts("1 minute has passed\n");
    one_minute_counter = 0;
  }
}
// Start Button ISR
// ISR(PCINT2_vect){
//   // check if the start button is pressed
//   if(*myPIND & 0x04){
//     // check if the system is in the disabled state
//     if(state == DISABLED){
//       // change the state to idle
//       state = IDLE;
//       // turn off the yellow LED
//       *myPORTB &= 0b11111011;
//       // turn on the green LED
//       *myPORTB |= 0b00000100;
//       // report the state transition
//       U0putchar('D');
//       U0putchar('I');
//       U0putchar(' ');
//       U0putchar('T');
//     }
//   }
// }
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