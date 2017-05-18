// Cameron Napoli, 73223093
// CS145L Lab 4

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define TIMER0_CNT 256 // 8 bit counter max

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <stdio.h>


void LcdCmdWrite(unsigned char cm) {
	// Write upper nibble
	PORTC = (PORTC & 0xf0) | (cm >> 4);
	PORTC &= ~(1<<4); // RS = 0
	PORTC |= 1<<5; // E = 1
	_delay_ms(1);
	PORTC &= ~(1<<5); // E = 0
	_delay_ms(1);
	// Write lower nibble
	PORTC = (PORTC & 0xf0) | (cm & 0x0f);
	PORTC &= ~(1<<4); // RS = 0
	PORTC |= 1<<5; // E = 1
	_delay_ms(1);
	PORTC &= ~(1<<5); // E = 0
	_delay_ms(1);
}
void LcdDataWrite(unsigned char cm) {
	// Write upper nibble
	PORTC = (PORTC & 0xf0) | (cm >> 4);
	PORTC |= 1<<4; // RS = 1
	PORTC |= 1<<5; // E = 1
	_delay_ms(1);
	PORTC &= ~(1<<5); // E = 0
	_delay_ms(1);
	// Write lower nibble
	PORTC = (PORTC & 0xf0) | (cm & 0x0f);
	PORTC |= 1<<4; // RS = 1
	PORTC |= 1<<5; // E = 1
	_delay_ms(1);
	PORTC &= ~(1<<5); // E = 0
	_delay_ms(1);
}
void LcdCommandWrite_UpperNibble(unsigned char cm) {
	PORTC = (PORTC & 0xf0) | (cm >> 4);
	PORTC &= ~(1<<4); // RS = 0
	PORTC |= 1<<5; // E = 1
	_delay_ms(1);
	PORTC &= ~(1<<5); // E = 0
	_delay_ms(1);
}
void LcdInit() {
	LcdCommandWrite_UpperNibble(0x30);
	_delay_ms(4.1);
	LcdCommandWrite_UpperNibble(0x30);
	_delay_us(100);
	LcdCommandWrite_UpperNibble(0x30);
	LcdCommandWrite_UpperNibble(0x20);
	LcdCmdWrite(0x28);
	LcdCmdWrite(0x08);
	LcdCmdWrite(0x01);
	LcdCmdWrite(0x06);
	LcdCmdWrite(0x0c);
	_delay_ms(120);
}


void updateLCD(int value) { // updates to LCD with maximum 3 digit value
	LcdCmdWrite(0x01); // clear display
	LcdCmdWrite(0x02); // reset cursor
	
	char buffer[3];
	sprintf( buffer, "%d", value );
	
	short i;
	for(i = 0; i < 3; i++)
		if(buffer[i] >= 48 && buffer[i] <= 57)
			LcdDataWrite(buffer[i]);
}

void InitTimer0() { // Used to poll the push button every 100ms
	TCCR0A = 0x0;
	TCCR0B = 0x3; // prescaler -> clk/64
	OCR0A = TIMER0_CNT;
	TIMSK0 = _BV(OCIE0A);
	TCNT0 = 177; // Initialize timer to 177
}

void InitTimer1() { // Timer for actual application (1000 ms)
	TCCR1B |= (1 << WGM12);
	OCR1A = 15624;
	TIMSK1 |= (1 << OCIE1A);
	TCCR1B |= (1 << CS12) | (1 << CS10);
}

void resetTimer1() {
	TCNT1 = 0;
}

#define NotPushed 0
#define NewMaybe 1
#define JustPushed 2
#define OldPushed 3
#define OldMaybe 4

#define RESET_MODE 0
#define COUNT_MODE 1
#define STOP_MODE 2

unsigned char PushState = NotPushed; // state machine (implements de-bouncing)
unsigned char StopwatchMode = RESET_MODE;
unsigned short LCDCounter = 0;

ISR(TIMER0_COMPA_vect) {
	switch (PushState) {
		case NotPushed:
			if(PIND & (1<<PIND3)) { // If external button is active
				PushState = NewMaybe;
			} else {
				PushState = NotPushed;
			}
			break;
		case NewMaybe:
			if(PIND & (1<<PIND3)) { // Trigger to change state in state machine
				PushState = JustPushed;
				
				switch (StopwatchMode) {
					case RESET_MODE:
						StopwatchMode = COUNT_MODE;
						resetTimer1();
						break;
					case COUNT_MODE:
						StopwatchMode = STOP_MODE;
						break;
					case STOP_MODE:
						StopwatchMode = RESET_MODE;
						LCDCounter = 0;
						updateLCD(0);
						break;
				}
				
			} else {
				PushState = NotPushed;
			}
			break;
		case JustPushed:
			if(PIND & (1<<PIND3)) {
				PushState = OldPushed;
			} else {
				PushState = OldMaybe;
			}
			break;
		case OldPushed:
			if(PIND & (1<<PIND3)) {
				PushState = OldPushed;
			} else {
				PushState = OldMaybe;
			}
			break;
		case OldMaybe:
			if(PIND & (1<<PIND3)) {
				PushState = OldPushed;
			} else {
				PushState = NotPushed;
			}
			break;
	}
	
}


ISR(TIMER1_COMPA_vect) {
	if(StopwatchMode == COUNT_MODE) {
		LCDCounter++;
		if(LCDCounter > 255) {
			LCDCounter = 0;
		}
		updateLCD(LCDCounter);
	}
}


int main() {
	
	DDRC |= 0x3f; // LCD Control pins

	// Init LCD
	LcdInit();
	updateLCD(0);

	// Init interrupt
	InitTimer0();
	InitTimer1();
	sei();

	while(1) {}

	return 0;
}