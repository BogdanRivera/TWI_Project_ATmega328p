/*
 * twi_project_slave.c
 *
 * Created: 30/01/2023 05:44:07 p. m.
 * Author : bugy1
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

uint8_t led_On = 0;

ISR (TWI_vect){
	uint8_t dato, edo;
	edo = TWSR & 0xFC; 
	switch(edo){
		case 0x60:		//Direccionado con su SLA
		case 0x70: TWCR |= 1 << TWINT;
				break; 
		case 0x80: 
		case 0x90:	dato = TWDR;
					if(dato == 0x05) led_On=1;
					else led_On = 0;
					TWCR |= 1 <<TWINT;
					break;
			
		default: TWCR |= (1<<TWINT) | (1<<TWSTO);
	}
	
	
}


int main(void)
{
	uint8_t dir = 0x12; //Dirección del eslavo
	DDRD = 0xFF; //Salida para el dato
	
	dir <<= 1; //Ubica la dirección y habilita para reconocer el dato
	dir |= 0x01;
	TWAR = dir; 
	//Habilita la interfaz, con reconocimiento e interrupción
	TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE) ;
	sei();
    while (1) 
    {
		if (led_On == 1) PORTD = 0x01; //Enciende led
		else PORTD = 0x00; //Apaga led
    }
}

