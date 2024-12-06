/*******************************************************
This program was created by the
CodeWizardAVR V3.12 Advanced

Project : Vehicle Control System
Date    : 7/20/2020
Author  : Mohamad Akhavan
ID : 96242005


Chip type               : ATmega32
Program type            : Application
AVR Core Clock frequency: 8.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512
*******************************************************/

#include <mega32.h>
#include <stdlib.h>
#include <delay.h>
#include <stdio.h>
#include <DHT.h>       //specific directory for DHT sensor

// Alphanumeric LCD functions
#include <alcd.h>

float fuel=100, consume=0;
char gauge[17];
char flag_move=0;       //determine the direction of the movement
float hum;
float temp;
char buffer[17];     //for showing temp and hum on lcd

int overflow=0;
unsigned long int  Counter=0;
float distance=0.0;
unsigned char dis_lcd[20];

//prototypes of functions:

void velocity();
void disp(int speed);
void Sound_alarm();
void Sound_backward();
void SRdis();
void fuelgauge();
void horn();

// External Interrupt 0 service routine
interrupt [EXT_INT0] void ext_int0_isr(void) //FORWARD
{
#asm("cli")
    flag_move=1;
    PORTD.0=1;
    PORTD.1=0;
    PORTD.4=1;
    PORTD.5=0;
    PORTB.4=1;
    PORTB.5=0;
    PORTB.6=1;
    PORTB.7=0;

    PORTA.4=0;
#asm("sei")
}

// External Interrupt 1 service routine
interrupt [EXT_INT1] void ext_int1_isr(void) //BACKWARD
{
#asm("cli")
    flag_move=2;
    PORTD.0=0;
    PORTD.1=1;
    PORTD.4=0;
    PORTD.5=1;
    PORTB.4=0;
    PORTB.5=1;
    PORTB.6=0;
    PORTB.7=1;

    PORTA.4=1;
#asm("sei")    

}

// External Interrupt 2 service routine
interrupt [EXT_INT2] void ext_int2_isr(void)  //STOP
{
#asm("cli")
    flag_move=0;
    PORTD.0=0;
    PORTD.1=0;
    PORTD.4=0;
    PORTD.5=0;
    PORTB.4=0;
    PORTB.5=0;
    PORTB.6=0;
    PORTB.7=0;

    PORTA.4=0;
#asm("sei")

}

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
overflow++;       
TCNT0=0x00;

}

// Voltage Reference: AREF pin
#define ADC_VREF_TYPE ((0<<REFS1) | (0<<REFS0) | (1<<ADLAR))

// Read the 8 most significant bits
// of the AD conversion result
unsigned char read_adc(unsigned char adc_input)
{
ADMUX=adc_input | ADC_VREF_TYPE;
// Delay needed for the stabilization of the ADC input voltage
delay_us(10);
// Start the AD conversion
ADCSRA|=(1<<ADSC);
// Wait for the AD conversion to complete
while ((ADCSRA & (1<<ADIF))==0);
ADCSRA|=(1<<ADIF);
return ADCH;
}

void main(void)
{
char flag_light=0;
DHT_setup();

// Input/Output Ports initialization
// Port A initialization
// Function: Bit7=In Bit6=Out Bit5=Out Bit4=Out Bit3=Out Bit2=In Bit1=In Bit0=In 
DDRA=(0<<DDA7) | (1<<DDA6) | (1<<DDA5) | (1<<DDA4) | (1<<DDA3) | (0<<DDA2) | (0<<DDA1) | (0<<DDA0);
// State: Bit7=T Bit6=0 Bit5=0 Bit4=0 Bit3=0 Bit2=P Bit1=P Bit0=T 
PORTA=(0<<PORTA7) | (0<<PORTA6) | (0<<PORTA5) | (0<<PORTA4) | (0<<PORTA3) | (1<<PORTA2) | (1<<PORTA1) | (0<<PORTA0);

// Port B initialization
// Function: Bit7=Out Bit6=Out Bit5=Out Bit4=Out Bit3=In Bit2=In Bit1=In Bit0=Out 
DDRB=(1<<DDB7) | (1<<DDB6) | (1<<DDB5) | (1<<DDB4) | (0<<DDB3) | (0<<DDB2) | (0<<DDB1) | (1<<DDB0);
// State: Bit7=0 Bit6=0 Bit5=0 Bit4=0 Bit3=T Bit2=P Bit1=T Bit0=0 
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (1<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

// Port C initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRC=(0<<DDC7) | (0<<DDC6) | (0<<DDC5) | (0<<DDC4) | (0<<DDC3) | (0<<DDC2) | (0<<DDC1) | (0<<DDC0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTC=(0<<PORTC7) | (0<<PORTC6) | (0<<PORTC5) | (0<<PORTC4) | (0<<PORTC3) | (0<<PORTC2) | (0<<PORTC1) | (0<<PORTC0);

// Port D initialization
// Function: Bit7=Out Bit6=In Bit5=Out Bit4=Out Bit3=In Bit2=In Bit1=Out Bit0=Out 
DDRD=(1<<DDD7) | (0<<DDD6) | (1<<DDD5) | (1<<DDD4) | (0<<DDD3) | (0<<DDD2) | (1<<DDD1) | (1<<DDD0);
// State: Bit7=0 Bit6=P Bit5=0 Bit4=0 Bit3=P Bit2=P Bit1=0 Bit0=0 
PORTD=(0<<PORTD7) | (1<<PORTD6) | (0<<PORTD5) | (0<<PORTD4) | (1<<PORTD3) | (1<<PORTD2) | (0<<PORTD1) | (0<<PORTD0);

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 1000.000 kHz
// Mode: Normal top=0xFF
// OC0 output: Disconnected
// Timer Period: 0.256 ms
TCCR0=(0<<WGM00) | (0<<COM01) | (0<<COM00) | (0<<WGM01) | (0<<CS02) | (1<<CS01) | (0<<CS00);
TCNT0=0x00;
OCR0=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: 125.000 kHz
// Mode: Phase correct PWM top=0xFF
// OC2 output: Non-Inverted PWM
// Timer Period: 4.08 ms
// Output Pulse(s):
// OC2 Period: 4.08 ms Width: 0 us
ASSR=0<<AS2;
TCCR2=(1<<PWM2) | (1<<COM21) | (0<<COM20) | (0<<CTC2) | (1<<CS22) | (0<<CS21) | (0<<CS20);
TCNT2=0x00;
OCR2=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=(0<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (0<<TOIE1) | (0<<OCIE0) | (1<<TOIE0);

// External Interrupt(s) initialization
// INT0: On
// INT0 Mode: Low level
// INT1: On
// INT1 Mode: Low level
// INT2: On
// INT2 Mode: Rising Edge
GICR|=(1<<INT1) | (1<<INT0) | (1<<INT2);
MCUCR=(0<<ISC11) | (0<<ISC10) | (0<<ISC01) | (0<<ISC00);
MCUCSR=(1<<ISC2);
GIFR=(1<<INTF1) | (1<<INTF0) | (1<<INTF2);

// ADC initialization
// ADC Clock frequency: 500.000 kHz
// ADC Voltage Reference: AREF pin
// ADC Auto Trigger Source: ADC Stopped
// Only the 8 most significant bits of
// the AD conversion result are used
ADMUX=ADC_VREF_TYPE;
ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (1<<ADPS2) | (0<<ADPS1) | (0<<ADPS0);
SFIOR=(0<<ADTS2) | (0<<ADTS1) | (0<<ADTS0);

// Alphanumeric LCD initialization
// Connections are specified in the
// Project|Configure|C Compiler|Libraries|Alphanumeric LCD menu:
// RS - PORTC Bit 0
// RD - PORTC Bit 1
// EN - PORTC Bit 2
// D4 - PORTC Bit 4
// D5 - PORTC Bit 5
// D6 - PORTC Bit 6
// D7 - PORTC Bit 7
// Characters/line: 20
lcd_init(20);
lcd_clear();
// Global enable interrupts
#asm("sei")

while (1)
      {
      DHT_read(&temp,&hum);           //updating temp and humid from sensor
      velocity();
      disp(OCR2);

      if(OCR2>=200 && flag_move!=0){
            Sound_alarm();            //alarm for speed more than 200
            
            //delay_ms(20);

            }
      if(flag_move==2){Sound_backward();}      //backward movement alarm
                      
      sprintf(buffer,"T=%3.1fC  H=%3.1f%%",temp,hum);
      lcd_gotoxy(0,1);
      lcd_puts(buffer);               //print temp and humid on LCD
      delay_ms(10);

      SRdis();
      fuelgauge();
      if(fuel==0){                    //stop motors when fuel=0
        flag_move=0;
        PORTD.0=0;
        PORTD.1=0;
        PORTD.4=0;
        PORTD.5=0;
        PORTB.4=0;
        PORTB.5=0;
        PORTB.6=0;
        PORTB.7=0;

        PORTA.4=0;
        PORTA.6=0;
      }

      if(PINA.1==0){horn();}

      delay_ms(500);

      if(PIND.6==0 && flag_light==0){         //turn on/off Lights
        flag_light=1;
        PORTA.6=1;
      }
      else if(PIND.6==0 && flag_light==1){
        flag_light=0;
        PORTA.6=0;
      }
      }
}

void velocity(){
 if(flag_move==2){
        OCR2=30*read_adc(0)/255;   //BACKWARD SPEED
        }
        else{OCR2=read_adc(0);}    //FORWARD SPEED
}
void disp(int speed)       //display speed on LCD
{
char s[20];
lcd_gotoxy(0,0);
if(flag_move!=0){
    sprintf(s,"Motor Speed: %d ",speed); 
    lcd_puts(s);
    }
else{
    lcd_putsf("Motor speed: 0   ");
    }
}
void Sound_alarm(){         //make sounds with buzzer with different functions
    unsigned int i_alarm;
    for (i_alarm=1;i_alarm<=10;++i_alarm) {
        PORTA.3=1; delay_us(700);
        PORTA.3=0; delay_us(700);
    };
    PORTA.3=0;    
}
void Sound_backward(){
    unsigned int i_backward;
    for (i_backward=1;i_backward<=10;++i_backward) {
        PORTA.3=1; delay_us(2500);
        PORTA.3=0; delay_us(2500);
    };
    PORTA.3=0;    
}
void SRdis(){
    PORTB.0=1;               //make Trig signal 10us
    delay_us(10);
    PORTB.0=0;

    while(PINB.1==0){};      //wait for Echo to become 1
    overflow=0;              //start Timer
    TCNT0=0;
    TCCR0=0x02;
    while(PINB.1==1){};            //wait until Echo Become 0
    TCCR0=0x00;                    //Stop Timer
    Counter=(overflow*256)+TCNT0;        //counts the total number of counter (time)
    distance=(Counter/2)*0.03432 ;       //measure distance with this formula
    lcd_gotoxy(0,2);
    if(flag_move==2)                          //Print distance only in the BACKWARD mode
    {
    sprintf(dis_lcd,"rear distant=%0.1fcm",distance);
        lcd_puts(dis_lcd);
        }
    else{lcd_putsf("rear distant= --   ");}    //print nothing on the FORWARD mode
}
void fuelgauge(){
    if(flag_move==0){fuel=fuel;}
    else if(flag_move!=0 && fuel>1){
        consume = OCR2*0.0016;            //with the specific coefficient fuel will be consume
        fuel=fuel-consume;
    }
    else{fuel=0;}                        //print fuel real time on LCD
    lcd_gotoxy(0,3);
    sprintf(gauge,"Fuel Gauge=%5.1f%%",fuel);
    lcd_puts(gauge);
    if(fuel<=15){                     //if Fuel is less than 15, the Alarm LED turn on
        PORTA.5=1;
    }else if(fuel>15){PORTA.5=0;}
    
    if(PINA.2==0){                    //with Refill PushButton, Fuel will be full
        fuel=100;
    }
 
}
void horn(){                         //make horn by two Sounds Functions
    unsigned int i_horn;
    for (i_horn=1;i_horn<=10;++i_horn){
    Sound_alarm();
    Sound_backward();
    }    
}