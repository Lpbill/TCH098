/*
 * GccApplication1.c
 *
 * Created: 2021-02-12 14:15:52
 * Author : ar09150
 */ 
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "fifo.h"
#include "uart.h"
#include "utils.h"
#include "driver.h"
#include "lcd.h"
#include <util/delay.h>

double scale(double entre,double entre_min, double entre_max, double sortie_min, double sortie_max);

int main(void)
{
	//Variables à envoyer
	int vertical;
	int horizontal;
	int valeurPotentiomentre;
	bool joystick_button_state;
	bool sw1_button_state;
	bool sw2_button_state;
	bool sw3_button_state;
	
	
	
	
	// COnstruction du string UDP
	char str[17];


	 
	
	
    //initialisation des couvertisseurs adc
    adc_init();   
	 
    //initialisation des ports de communications
    uart_init(0);
		
	//BP JOYSTICK---------------------------------
	// Mettre la broche du bouton du joystick en entrée
	DDRA = clear_bit(DDRA, PA2);
	// Activer la "pull-up" interne de la broche pour forcer un état haut
	// quand le bouton n'est pas enfoncé
	PORTA = set_bit(PORTA, PA2);
	
	// SW1--------------------------------------------
	// Mettre la broche sw1 en entree
	DDRD = clear_bit(DDRD, PD5);
	// Activer la "pull-up" interne de la broche pour forcer un état haut
	// quand le bouton n'est pas enfoncé
	PORTD = set_bit(PORTD, PD5);
	
	// SW2--------------------------------------------
	// Mettre la broche du bouton de sw2 en entrée
	DDRD = clear_bit(DDRD, PD6);
	// Activer la "pull-up" interne de la broche pour forcer un état haut
	// quand le bouton n'est pas enfoncé
	PORTD = set_bit(PORTD, PD6);
	
	// SW3--------------------------------------------	
	// Mettre la broche du bouton de sw3 en entrée
	DDRD = clear_bit(DDRD, PD7);
	// Activer la "pull-up" interne de la broche pour forcer un état haut
	// quand le bouton n'est pas enfoncé
	PORTD = set_bit(PORTD, PD7);
	
	
	
    //Activer les interruptions globales
    sei();

    lcd_init();
    lcd_clear_display();
    
  
    
    while (1)
    {
		//lecteur joystick horizontal
        horizontal = adc_read(PINA1);
	
	
		//Lecteur joystick vertical
        vertical = adc_read(PINA0);
	
		
		//lecteur potentiomentre
		valeurPotentiomentre = adc_read(PINA3);

		//lecteur sw1
		sw1_button_state = read_bit(PIND, PD7);
  
		//lecteur sw2
		sw2_button_state = read_bit(PIND, PD6);
		
		
		//lecteur sw3
		sw3_button_state = read_bit(PIND, PD5);
		
		//Lecture BP joystick
		joystick_button_state = read_bit(PINA, PA2);
		

		//Ajouter les valeurs lues dans le string message UDP
		str[0] = '[';
		
		uint8_to_string(str+1, horizontal);
		uint8_to_string(str+4, vertical);
		uint8_to_string(str+7, valeurPotentiomentre);
		str[10] = sw1_button_state + 48;
		str[11] = sw2_button_state + 48;
		str[12] = sw3_button_state + 48;
		str[13] = joystick_button_state + 48;
		
		str[14]= ']';
		str[15] = '\n';
		str[16] = '\0';
		
		//Ecrire sur le lcd pour tester
		lcd_set_cursor_position(0,0);
		lcd_write_string(str);
		
		//Envoyer le message dans le UART
		uart_put_string(UART_0,str);
		
	
        _delay_ms(100);
        
 
    }
}

