
//***********************************************
//
// Funciones para el manejo de la Interfaz TWI
//
//***********************************************

void		TWI_Config();
uint8_t	    TWI_Inicio();
void 		TWI_Paro();
uint8_t 	TWI_EscByte(uint8_t dato);
uint8_t 	TWI_LeeByte(uint8_t *dato,uint8_t ack);