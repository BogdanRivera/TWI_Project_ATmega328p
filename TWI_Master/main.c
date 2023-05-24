
#define DIR_DS 0x68
#define DIR_TIEMPO 0x00 
#define DIR_SLAVE 0x12 
#include <avr/io.h>
#include <avr/interrupt.h>
#include "TWI.h"
#include "LCD.h"


uint8_t hora[3] = {0,0,0};  //Segundos(0),Minutos(1),Horas(2)

uint8_t rec[],transf[]; //Variables para recepción y transferencia
uint8_t opc = 0; //Variable para la escritura de las horas, minutos y segundos
uint8_t cont_hora = 0; //Contador para introducir los números de la hora

uint8_t read_ds(uint8_t data[],uint8_t n,uint8_t dir);
void imprime_hora(uint8_t datos[]);
uint8_t send_ds(uint8_t data[],uint8_t n,uint8_t dir);
void print_all(uint8_t h[]);
uint8_t led_On(uint8_t dato);


ISR(TIMER1_COMPA_vect){
	read_ds(hora,3,DIR_TIEMPO);
	print_all(hora);
}


ISR(USART_RX_vect){
	uint8_t ins,stat; //Variable para la instrucción y el estado 
	ins = UDR0;
	switch(opc){ //Determina si se verá únicamente la hora o se podrá modificar (lee = 0,escribe = 1)
		case 0:
			opc = 0; //Inicializa el contador debido a que se está registrando únicamente la lectura
			if(ins==0x48 || ins==0x68){ //Se utiliza la letra h o H para ver la hora del RTC
			stat = read_ds(hora,3,DIR_TIEMPO); //Realiza la lectura en el DS
			
			if(stat!=0x00){ //Si no hubo una falla imprime la hora en la LCD
			imprime_hora(hora);
			} else  LCD_write_cad("NO",2);
			opc = 0; //Retorna para realizar nuevamente una lectura
		}else{ //En dado caso de que no se requiera visualizar la hora sino cambiarla entra en esta condición
			if(ins==0x54 || ins==0x74){ //Utiliza la letra T para el cambio de la hora
				TCCR1B = 0x08; //Desactiva el timer 1
				LCD_write_cad("Escribe una hora(ssmmhh)",24); //Debes escribir segundos, minutos y horas
				LCD_cursor(0x11);
				opc = 1; //Se pasa a la siguiente caso del switch para realizar la escritura de la hora
			}else{
				if (ins==0x49||ins==0x69) opc=2; //Variable para encender el LED en el esclavo, letra i, I
				if (ins==0x4F||ins==0x6F) opc=3; //Variable para apagar LED en el esclavo, letra o, O
			}
		}
		break;
		
		case 1:
		/*
		Esta instrucción permite que solamente se introduzcan números al DS1307 del 0 al 9
		Además de esto el contador debe ser menor a 6 porque registra ss,mm,hh (segundos, minutos y horas)
		*/
		if(ins>=0x30 && ins<=0x39 && cont_hora<6){ 
			rec[cont_hora] = ins; //La posición del contador de horas se establece en el arreglo de recepción
			cont_hora++; //Aumenta la posición del arreglo
			opc=1; //La opción vuelve para realizar la introducción de los datos
			LCD_write_data(ins);
			if(cont_hora==6) LCD_write_cad("Presiona cualquier tecla",24); //Si llega a 6 entonces manda mensaje a la LCD
			}else{
			if(cont_hora==6){ //Si alcanzó la lectura entonces manda los parámetros
				if(rec[1]>=0x36) transf[0] = 0; //En los segundos decimales no puede ser mayor a 6
				else transf[0] = ((rec[0]-0x30)<<4) + (rec[1]-0x30); //Si no transfiere los segundos
				transf[1] = ((rec[2]-0x30)<<4) + (rec[3]-0x30); //Transfiere los minutos
				transf[2] = ((rec[4]-0x30)<<4) + (rec[5]-0x30); //Transfiere las horas
				stat = send_ds(transf,3,DIR_TIEMPO); //Manda el vector de transferencia
				if(stat!=0x00){
					TCCR1B = 0x0A;
					opc = 0; //Regresa para la lectura
					cont_hora = 0; //La cuenta nuevamente regresa a la posición 0 del vector
				}				
			}
		}
		break;
		
		case 2: 
		stat = led_On(0x05);
		LCD_cursor(0x14);
		LCD_write_cad("LED encendido",13);
		opc = 0;
		break;
		
		case 3: 
		stat = led_On(0x06);
		LCD_cursor(0x14);
		LCD_write_cad("LED apagado",11);
		opc = 0;
		break;
		 
	}
	
}

int main(void)
{
	//Configuración de entradas y salidas
	DDRB = 0xFF; //Puerto B como salidas
	DDRD = 0x02; //TXD como salida  
	
	//Configuración de la USART
	UBRR0 = 12; 
	UCSR0A = 0x02; 
	UCSR0B = 0x98; 
	UCSR0C = 0x06; 
	
	/*
	Se utiliza el timer 1 para la impresión por segundo en la LCD
	*/
	//Configura el timer 1
	OCR1A = 24999; //Para 200ms
	TCCR1A = 0x00; //Modo 4 CTC
	TCCR1B = 0x0A; //Preescala de 8 con reloj inicialmente
	TCCR1C = 0x00; 
	TIMSK1 = 0x02; //Habilita la interrupción por comparación 
	
	//Configura la TWI 
	TWI_Config();
	LCD_reset();
	
	sei();
	
    while (1) 
    {
		 asm("NOP"); 
    }	
}
//Función para la lectura de fecha, hora, etc
uint8_t read_ds(uint8_t data[],uint8_t n,uint8_t dir){
	uint8_t aux,i; //variable auxiliar para denotar estado y contador (i)
	
	aux = TWI_Inicio(); //Inicializa el sistema TWI
	if(aux!=0x01){
	TWCR |= 1<<TWINT; //Limpia bandera 
	return 0;
	}
	
	aux = TWI_EscByte(DIR_DS<<1); //SLA+W, dirección del DS1307
	if(aux!=0x01){
		TWI_Paro(); //Si algo ha salido mal termina el proceso de TWI
		return 0;
	}
	
	aux = TWI_EscByte(dir); //Direccion de acceso (0x00 para tiempo, 0x04 para fecha)
	if(aux!=0x01){
	TWI_Paro();	//Si algo ha salido mal termina el proceso de TWI
	return 0;
	}
	
	aux = TWI_Inicio();		//Inicio repetido para la lectura de los elementos
	if(aux!=0x01){
		TWCR |= 1<<TWINT; //Limpia bandera
		return 0;
	}
	
	aux = TWI_EscByte((DIR_DS<<1) + 1); //SLA+R
	if(aux!=0x01){
		TWI_Paro();			//Libera al bus, si algo ha salid mal
		return 0;
	}
	
	for(i = 0;i<n-1;i++){
		aux = TWI_LeeByte(&data[i],1); //Recibe n-1 datos con Ack
		if(aux!= 0x01){
			TWI_Paro();
			return 0;
		}
	}	
	TWI_LeeByte(&data[i],0); //Lee último dato sin reconocimiento para determinar el final
	TWI_Paro();
	return 0x01;  //Indica que todo ha salido bien
}

//Función para enviar fecha, hora, etc 
uint8_t send_ds(uint8_t data[],uint8_t n,uint8_t dir){
	uint8_t aux,i; //variable auxiliar para denotar estado y contador (i)
	
	aux = TWI_Inicio(); //Realiza el inicio de la TWI
	if(aux!=0x01){
		TWCR |= 1<<TWINT; //Limpia bandera si no ha resultado correctamente
		return 0;
	}
	
	aux = TWI_EscByte(DIR_DS<<1); //SLA+W, dirección del DS1307
	if(aux!=0x01){
		TWI_Paro();
		return 0;
	}
	
	aux = TWI_EscByte(dir);   //Direccion de acceso (0x00 para tiempo, 0x04 para fecha)
	if(aux!=0x01){
		TWI_Paro();
		return 0;
	}
	
	for(i=0;i<n;i++){          //Envia datos
		aux = TWI_EscByte(data[i]);
		if(aux!=0x01){
			TWI_Paro();
			return 0;
		}
	}
	TWI_Paro();
	return 1;
	
}

void imprime_hora(uint8_t datos[]){
	uint8_t i;
	for (i=2;i>0;i--){
		while(!(UCSR0A & 1 <<UDRE0));
		UDR0 = ((datos[i]&0xF0)>>4) + 0x30;
		while(!(UCSR0A & 1 <<UDRE0));
		UDR0 = (datos[i]&0x0F) + 0x30;
		while(!(UCSR0A & 1 <<UDRE0));
		UDR0 = 0x3A;
	}
	while(!(UCSR0A & 1 <<UDRE0));
	UDR0 = ((datos[i]&0xF0)>>4) + 0x30;
	while(!(UCSR0A & 1 <<UDRE0));
	UDR0 = (datos[i]&0x0F) + 0x30;
	while(!(UCSR0A & 1 <<UDRE0));
	UDR0 = 0x0D;
}

void print_all(uint8_t h[]){
	//Imprime toda la hora
	LCD_write_cad("    ",4);
	LCD_write_data(((h[2]&0xF0)>>4) + 0x30);
	LCD_write_data((h[2]&0x0F) + 0x30);
	LCD_write_data(0x3A);
	LCD_write_data(((h[1]&0xF0)>>4) + 0x30);
	LCD_write_data((h[1]&0x0F) + 0x30);
	LCD_write_data(0x3A);
	LCD_write_data(((h[0]&0xF0)>>4) + 0x30);
	LCD_write_data((h[0]&0x0F) + 0x30);
}

uint8_t led_On(uint8_t dato){
	uint8_t aux; //Variable auxiliar
	uint8_t dir_aux = DIR_SLAVE;
	dir_aux = dir_aux << 1; //Para realizar la SLA + W
	aux = TWI_Inicio(); //Realiza el inicio de la TWI
	if (aux != 0x01){ //Si hubo error
		TWCR |= (1<<TWINT); //Limpia bandera
		return 0;
	}
	aux = TWI_EscByte(dir_aux); //Direcciona la SLA+W
	if (aux != 0x01){ //Si hubo error
		TWI_Paro();
		return 0;
	}
	TWI_EscByte(dato); //Envía el dato
	TWI_Paro();
	return 1;
}