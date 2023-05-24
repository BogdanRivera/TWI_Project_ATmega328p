
//********************************************************************
//
// Funciones para el manejo de la Interfaz TWI como maestro.
// Las funciones de esta biblioteca NO usan interrupciones.
//
//********************************************************************

#include  <avr/io.h>

//********************************************************************
// Configuración de la Interfaz
//********************************************************************

void TWI_Config() {
	
	TWBR = 0x02; 	// Frecuencia de reloj TWI a 50 KHz
	TWSR = 0x00;	// Pre-escalador en 0
					// Ajustar según el dispositivo
	TWCR = 1<<TWEN;	// Habilita a la interfaz
}

//********************************************************************
// TWI_Inicio:	Genera una condición de inicio
// Regresa:  	1	- Sin problemas
//           	Edo	- No se consiguió ganar el bus
//********************************************************************

uint8_t TWI_Inicio() {
uint8_t  edo;
                                                
TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA);		// Condición de inicio 
while(!(TWCR & (1<<TWINT)));            	// Espera TWINT
edo = TWSR & 0xF8; 							// Obtiene el estado
if( edo == 0x08 || edo == 0x10)  			// 2 posibilidades de éxito	
	return 1;	 							// 0x08: inicio
											// 0x10: Inicio repetido
return edo;									// No ganó el bus
}

//********************************************************************
// TWI_Paro: Genera una condición de paro
//********************************************************************

void TWI_Paro() {
	
TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);		// Condicion de paro
	                                                  
while(TWCR & 1<<TWSTO);						// El bit se limpia por HW
}

//********************************************************************
// TWI_EscByte: Escribe un dato (o una SLA + W/R) por la interfaz TWI
// Recibe:	El dato de 8 bits a enviar
// Regresa:	1   - Sin problemas (Envío con reconocimiento)
//         	Edo - Si el dato no se envío como se esperaba
//********************************************************************

uint8_t TWI_EscByte(uint8_t dato) {
uint8_t  edo;
	
TWDR = dato;							// Carga el dato
TWCR = (1<<TWEN)|(1<<TWINT);			// Inicia envío
while(!(TWCR & (1<<TWINT)));			// Espera estado
edo = TWSR & 0xF8; 						// Obtiene el estado

 if(edo == 0x18 || edo == 0x28 || edo == 0x40)
										// 3 posibilidades de éxito		
	   return 1;						// 0x18: SLA + W Transmitido con ACK 
		           						// 0x28: SLA + R Transmitido con ACK
				   						// 0x40: Dato Transmitido con ACK
 return edo;							// Situación diferente
}

//********************************************************************
// TWI_LeeByte: Lee un dato por la interfaz TWI
// Recibe:	Referencia para ubicar el dato leído
//			Indicación de reconocimiento
// Regresa:	1  - Si no hubo problema
//      	Edo - Si el dato no se envío como se esperaba
//********************************************************************

uint8_t TWI_LeeByte(uint8_t *dato, uint8_t ack){
uint8_t  edo;

	if(ack)							// Cuando se lee una secuencia
		TWCR |= (1<<TWEA);			// para indicar que es el último byte 
	else							// no se señaliza el reconicimiento
	  	TWCR &= (~(1<<TWEA));
	                                                  	
    TWCR |= (1<<TWINT);				// Limpia bandera TWINT
	
	while(!(TWCR & (1<<TWINT)));	// Espera a la bandera TWINT

	edo = TWSR & 0xF8; 				// Obtiene el estado de la interfaz TWI
	if(edo == 0x58 || edo == 0x50) {
									// Dos posibilidades de éxito					
	 	*dato=TWDR; 				// 0x58: Dato leído con ACK
		return 1;					// 0x50: Dato leído sin ACK
	}								// Lee el dato

	return edo;						// Situación diferente
}