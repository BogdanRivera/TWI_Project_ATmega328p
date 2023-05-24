
#define  F_CPU 1000000UL
#define  P_LCD  PORTB		// Puerto donde se conectará el LCD

#include <util/delay.h>
#include <avr/io.h>
#include "LCD.h"

// Conexión del LCD: Interfaz de 4 bits
// D4 - P_LCD_0
// D5 - P_LCD_1
// D6 - P_LCD_2
// D7 - P_LCD_3
// E  - P_LCD_4
// RS - P_LCD_5
// RW - debe conectarse a Tierra

// Manda el pulso de habilitación sin alterar otras terminales
void LCD_pulso_E()
{
	P_LCD = P_LCD | 0x10;
	P_LCD = P_LCD & 0xEF;
}

// Escribe una instrucción de 4 bits, recibida en el nibble inferior
void LCD_write_inst4(uint8_t  inst)
{
	P_LCD = inst & 0x0F;        // Rs =  0
	LCD_pulso_E();
}

// Escribe una instrucción de 8 bits
void LCD_write_inst8(uint8_t   inst)
{
	uint8_t   temp;

	temp = (inst & 0xF0) >> 4;
	LCD_write_inst4(temp);      // Primero el nibble alto

	temp = inst & 0x0F;
	LCD_write_inst4(temp);      // Después el nibble bajo

	_delay_us(40);
}

// Escrible un dato de 8 bits
void LCD_write_data(uint8_t dat_8b)
{
	uint8_t temp;

	temp = (dat_8b & 0xF0) >> 4;	// PARTE ALTA
	P_LCD = temp | 0x20; 			// RS = 1
	LCD_pulso_E();

	temp = dat_8b & 0x0F; 			// PARTE BAJA
	P_LCD = temp | 0x20;  			// RS = 1
	LCD_pulso_E();

	_delay_us(40);
}

// Inicializa al LCD
void LCD_reset()
{
	_delay_ms(15);
	LCD_write_inst4(0x03);      // Configura la función del Display
	// La interfaz aún es de 8 bits
	_delay_ms(4.1);
	LCD_write_inst4(0x03);      // Configura la función del Display
	// La interfaz aún es de 8 bits
	_delay_us(100);
	LCD_write_inst4(0x03);      // Configura la función del Display
	_delay_us(40);              // La interfaz aún es de 8 bits

	LCD_write_inst4(0x02);      // Configura la función del Display
	_delay_us(40);              // Define la interfaz de 4 bits

	LCD_write_inst8(0X28);      // Configura la función del Display
	// Dos líneas y 5x7 puntos
	LCD_write_inst8(0X0C);      // Encendido del display

	LCD_clear();                // Limpieza del display

	LCD_write_inst8(0X06);      //  Ajuste de entrada de datos
	// Autoincremento del contador
	// y sin desplazamiento
}

// Funcion para la limpieza del display
void LCD_clear()
{
	LCD_write_inst8(0X01);
	_delay_ms(2);
}

// Ubica al cursor, el nibble alto indica el renglon, el bajo la columna
void LCD_cursor(uint8_t   pos)
{
	uint8_t  col;
	col = pos & 0X0F;
	if((pos & 0XF0) == 0)
	{
		col = col | 0X80;
		LCD_write_inst8(col);
	}
	else
	{
		col = col | 0XC0;
		LCD_write_inst8(col);
	}
}

// Escribe una cadena, supone un display de 2 renglones y 16 columnas
void LCD_write_cad(char  cad[], uint8_t  tam)
{
	uint8_t  i;

	LCD_clear();

	for(i=0; i<tam; i++)
	{
		LCD_write_data((uint8_t) cad[i]);
		if(i==15)
		LCD_cursor(0X10);
	}
}