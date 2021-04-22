/*
 * Projet: RECEPTION WIFI (Véhicule)
 * Auteurs : Pier-Olivier Beaulieu et Louis-Philippe Bilodeau
 * Dernière version: 21/04/2021 2:30:27 PM
*/ 

//Librairies
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"
#include "utils.h"
#include "driver.h"
#include "uart.h"
#include "fifo.h"
#include <math.h>

//Constantes
#define MODE_SW1 1
#define	MODE_SW2 2 

//Constantes du mode 1
const int VITESSE_MAX_ROUES_MODE_1 = 255;

//Vitesse max de rotation sur lui meme
const int VITESSE_MAX_ROTATION_MODE_1 = 100;
const int VITESSE_MAX_ROTATION_TIR = 75;

//le differentiel devrait toujours etre de 10% de la vitesse max selon Youri
const int DIFFERENTIEL_MODE_1 = 25;

//Vitesses du disque d'inertie. Celles-ci seront augmenté avec le bouton sw2
const int VITESSE_INERTIE_2 = 100;
const int VITESSE_INERTIE_2_5 = 110;
const int VITESSE_INERTIE_3 = 120;

//Changer les valeurs pour changer les angles du servo moteur. 0 degres: 1000, 90 degres: 2000
const int VALEUR_SERVO_ATTENTE = 1900; //Position d'attente où il empêche les disques de sortir
const int VALEUR_SERVO_ARME = 2000;     //Pour pousser le disque
const int VALEUR_SERVO_REVIENS = 1200;  //Pour laisser tomber le disque de bois

//Elevateur
const int VITESSE_MAX_ELEVATEUR = 255;
	

//Prototypes
double scaleAvantArriere(int entree, int vitesseMax);
double scaleElevateur(int entree, int vitesseMax);
void fonctionConnection(int *manette_connecte);
double scaleDroiteGauche(int entree, int differentielMax);


int main(void){

    /*------------------------------
                VARIABLES
    --------------------------------*/

    //Valeur int recues de la manette
    int valeur_axe_x = 130; //0 en x pour le potentiomètre
    int valeur_axe_y = 123; //0 en y pour le potentiomètre
    int valeur_pot = 135;
    int valeur_sw2 = 1;
    int valeur_sw3 = 1;
    int valeur_bp_joystick = 1; //1 si joystick appuyé

    double vitesse;
    double differentiel;
    double vitesseRD;
    double vitesseRG;

    //Déclaration des vitesse de l'élévateur et de la roue d'inertie
    double vitesseElevateur = 0;
    int vitesse_inertie = VITESSE_INERTIE_2;

    // Variables d'États pour le bouton 2
    int bouton_2_enfonce = FALSE;
    int mode_SW2 = 1;

    // Variables d'États pour le bouton 3 et la roue d'inertie.
    int bouton_3_enfonce = FALSE;
    int roue_inertie_marche = FALSE;

    //Variables d'États pour le servo
    int joystick_enfonce = FALSE;
    int servo_en_cours = FALSE;
    int servo_etape = 0;
    int servo_cycle = 0;

    //Variable d'États pour la connection de la manette.
    int mannette_connecte = FALSE;

    // Création d'un tableau pour la réception de données de la manette.
    char string_recu[33];

    //-----------------------------------------------------------------

    //Initialisation des sorties et du MLI

	pwm0_init();
	pwm1_init(20000);
	pwm2_init();
	
	//Initialisation du module UART0
	uart_init(UART_0);
	sei();

    //Initialisation Lcd
	lcd_init();

    /****************************************************
        AFFECTATION DES VALEURS AUX MOTEURS (INITIALES)
    *****************************************************/
	vitesseRG = abs(0); //roues gauches
	vitesseRD = abs(0); //roues droites

	//Affcetation de la vitesse au moteur roues gauches
	pwm0_set_PB3(0);
	//Affcetation de la vitesse au moteur roues droites
	pwm0_set_PB4(0);
			
	//Elevateur 
	vitesseElevateur = 0;
	pwm2_set_PD6(0);
			
	//roue inertie
	pwm2_set_PD7(0);
			
	//servomoteur
	pwm1_set_PD5(VALEUR_SERVO_ATTENTE);

    //-----------------------------------------------------------------


	while (1){

		//Verification si la mannette est connectee
		fonctionConnection(&mannette_connecte);
		
		//Verifier l'etat de la file rx, si elle est pleine on rentre
		if (mannette_connecte == TRUE){
			
			uart_get_line(UART_0, string_recu, 17);

			if (string_recu[0] == '['){

				//Remettre a zero a chaque bouble while les valeurs construites de UART
				valeur_axe_x = 0;
				valeur_axe_y = 0;
				valeur_pot = 0;
				valeur_sw2 = 0;
				valeur_sw3 = 0;
				valeur_bp_joystick = 0;

				//Remettre a zero les valeurs de moteurs
				vitesse = 0;
				vitesseRD = 0;
				vitesseRG= 0;
				differentiel= 0;
				vitesseElevateur = 0;

				//********************************************
				//construction des valeurs numeriques recues du transmetteur
				//*******************************************
				valeur_axe_x += 100* char_to_uint(string_recu[1]) +  10* char_to_uint(string_recu[2])+  char_to_uint(string_recu[3]);
				valeur_axe_y += 100* char_to_uint(string_recu[4]) +  10* char_to_uint(string_recu[5])+  char_to_uint(string_recu[6]);
				valeur_pot += 100* char_to_uint(string_recu[7]) +  10* char_to_uint(string_recu[8])+  char_to_uint(string_recu[9]);
				valeur_sw2 = string_recu[11]- 48;
				valeur_sw3 = string_recu[12]- 48;
				valeur_bp_joystick = string_recu[13]- 48;
			}
		} 

			//Scaling de la vitesse
			vitesse = scaleAvantArriere(valeur_axe_x,VITESSE_MAX_ROUES_MODE_1);
			differentiel = scaleDroiteGauche(valeur_axe_y, DIFFERENTIEL_MODE_1);	

			//Affectation du differentiel et du sens de rotation 
			if (vitesse > 0){
				vitesseRD = vitesse - differentiel;
				vitesseRG = vitesse + differentiel;
				//Roues gauches U7 et U9------------------------------
				//Affectation de la bonne direction sur les pont en H respectifs
				PORTB = set_bit(PORTB, PB1);	
				//Roues droites U6 et U8------------------------------
				//Affectation de la bonne durection sur les pont en H respectifs
				PORTB = clear_bit(PORTB, PB2);
			}
			if (vitesse < 0){
					vitesseRD = vitesse + differentiel;
					vitesseRG = vitesse - differentiel;
				//Roues gauches U7 et U9------------------------------
				//Affectation de la bonne direction sur les ponts en H respectifs
				PORTB = clear_bit(PORTB, PB1);
				//Roues droites U6 et U8------------------------------
				//Affectation de la bonne direction sur les ponts en H respectifs
				PORTB = set_bit(PORTB, PB2);
				
			}
			
			/////////////////////
			//EDGE CASES !
			///////////////////
	
			if (vitesseRD > VITESSE_MAX_ROUES_MODE_1){
				vitesseRD = VITESSE_MAX_ROUES_MODE_1;
			}
			if (vitesseRG > VITESSE_MAX_ROUES_MODE_1){
				vitesseRG = VITESSE_MAX_ROUES_MODE_1;
			}
			if (vitesseRD < -VITESSE_MAX_ROUES_MODE_1){
				vitesseRD = -VITESSE_MAX_ROUES_MODE_1;
			}
			if (vitesseRG < -VITESSE_MAX_ROUES_MODE_1){
				vitesseRG = -VITESSE_MAX_ROUES_MODE_1;
			}
			
			//Cas special permettant de faire tourner le vehicule sur lui même
			else if (valeur_axe_x < 145 && valeur_axe_x > 110){

				if (roue_inertie_marche == TRUE){
					vitesse = scaleDroiteGauche(valeur_axe_y, VITESSE_MAX_ROTATION_TIR);
				}
				else{
					vitesse = scaleDroiteGauche(valeur_axe_y, VITESSE_MAX_ROTATION_MODE_1);
				}
				vitesseRG = vitesse;
				vitesseRD = vitesse;

				if (vitesse > 0){
					//Roues gauches U7 et U9-----------------------------
					//Affectation de la bonne direction sur les pont en H respectif
					PORTB = set_bit(PORTB, PB1);
					//Roues droites U6 et U8------------------------------
					//Affectation de la bonne durection sur les pont en H respectif
					PORTB = set_bit(PORTB, PB2);
				}
				else if (vitesse < 0){
					//Roues gauches U7 et U9-----------------------------
					//Affectation de la bonne direction sur les pont en H respectif
					PORTB = clear_bit(PORTB, PB1);
					//Roues droites U6 et U8------------------------------
					//Affectation de la bonne durection sur les pont en H respectif
					PORTB = clear_bit(PORTB, PB2);
					
				}
			}

		///////////////////////////////////////////////////
		//				    ELEVATEUR
		///////////////////////////////////////////////////
		
		//1 Calcul vitesse moteur
		vitesseElevateur = scaleElevateur(valeur_pot, VITESSE_MAX_ELEVATEUR);
		
		//2. sens de rotation
		if (vitesseElevateur > 0){
			PORTB = set_bit(PORTB, PB0);
		}
		if (vitesseElevateur < 0){
			PORTB = clear_bit(PORTB, PB0);
		}
		
		//3. Edge Cases
		//Ajutement des valeurs hors du range
		if (vitesseElevateur > VITESSE_MAX_ELEVATEUR){
			vitesseElevateur = VITESSE_MAX_ELEVATEUR;
		}
		if (vitesseElevateur < -VITESSE_MAX_ELEVATEUR){
			vitesseElevateur = -VITESSE_MAX_ELEVATEUR;
		}	
			
		/*************************************************
		        AFFECTATION DES VALEURS AUX MOTEURS
		*************************************************/
		
		vitesseRG = abs(vitesseRG);
		vitesseRD = abs(vitesseRD);
			
		if (mannette_connecte == TRUE){

		    //Affcetation de la vitessem au moteur roues gauches
			pwm0_set_PB3(vitesseRG);
			//Affcetation de la vitessem au moteur roues droites
			pwm0_set_PB4(vitesseRD);
				
			//Elevateur
			vitesseElevateur = abs(vitesseElevateur);
			pwm2_set_PD6(vitesseElevateur);
				
			//Bouton 2 qui fait varier la vitesse de la roue d'inertie
			if (valeur_sw2 == 1){
				bouton_2_enfonce = FALSE;
			}
			if (valeur_sw2 == 0 && bouton_2_enfonce == FALSE){
				bouton_2_enfonce = TRUE;
				switch(mode_SW2){
					case 1:
						mode_SW2++;
						vitesse_inertie = VITESSE_INERTIE_2;
						if (roue_inertie_marche == TRUE){
							pwm2_set_PD7(vitesse_inertie);
						}
						break;
					case 2:
						mode_SW2++;
						vitesse_inertie = VITESSE_INERTIE_2_5;
						if (roue_inertie_marche == TRUE){
							pwm2_set_PD7(vitesse_inertie);
						}
						break;
					case 3:
						mode_SW2 = 1;
						vitesse_inertie = VITESSE_INERTIE_3;
						if (roue_inertie_marche == TRUE){
							pwm2_set_PD7(vitesse_inertie);
						}
						break;
				}
			}
			//Bouton 3 : permet la mise en marche et la mise en arrêt de la roue d'inertie
			if(valeur_sw3 == 1){
				bouton_3_enfonce = FALSE;
			}

			if (valeur_sw3 == 0 && bouton_3_enfonce == FALSE){
				if (roue_inertie_marche == FALSE){
					roue_inertie_marche = TRUE;
					pwm2_set_PD7(vitesse_inertie);
				}
				else if (roue_inertie_marche == TRUE){
					roue_inertie_marche = FALSE;
					pwm2_set_PD7(0);
				}
				bouton_3_enfonce = TRUE;
			}

			//servomoteur
			if (valeur_bp_joystick == 1 && servo_en_cours == FALSE){
				joystick_enfonce = FALSE;
			}
			if (valeur_bp_joystick == 0 && joystick_enfonce == FALSE){
				servo_en_cours = TRUE;
				servo_etape = 1;
				servo_cycle = 0;
			}
			if (servo_en_cours == TRUE){
				switch (servo_etape){
					case 1:
						pwm1_set_PD5(VALEUR_SERVO_REVIENS);
						servo_cycle++;
						_delay_ms(1);
						if (servo_cycle >= 5){
							servo_etape = 2;
							servo_cycle = 0;
						}
						break;
					case 2:
						pwm1_set_PD5(VALEUR_SERVO_ARME);
						servo_cycle++;
						_delay_ms(1);
						if (servo_cycle >= 5){
							servo_etape = 3;
							servo_cycle = 0;
						}
						break;
					case 3:
						pwm1_set_PD5(VALEUR_SERVO_ATTENTE);
						servo_cycle++;
						_delay_ms(1);
						if (servo_cycle >= 5){
							servo_etape = 0;
							servo_en_cours = FALSE;
							servo_cycle = 0;
						}
						break;
				}
			}
		}
		else if(mannette_connecte == FALSE){
			//Affectation de la vitessem au moteur roues gauches
			pwm0_set_PB3(0);
			//Affectation de la vitessem au moteur roues droites
			pwm0_set_PB4(0);

			pwm2_set_PD6(0);

			//roue inertie
			pwm2_set_PD7(0);

			//servomoteur
			pwm1_set_PD5(VALEUR_SERVO_ATTENTE);

		}
	}
}

/************************************************
            FONCTIONS DE SCALING
*************************************************/

//Cette fonction fait le scaling avant arriere du joystick selon une fonction du 3e degre
double scaleAvantArriere(int entree, int vitesseMax){
	//On commence par le centrer sur 0 
	double deltay = vitesseMax;
	double deltax = vitesseMax-123;
	double y = deltay/deltax * (entree - 123);
	double a;
	double vitesse = 0;
	//on le change pour une fonction de degre 3
	a = vitesseMax/pow(vitesseMax+100, 3);
	
	if (y>0){
		
		vitesse = a*pow(y+100,3);
	}
	if (y< 0){
		
		vitesse = a*pow(y-100,3);
	}

	return vitesse;
}


//Similaire à celle du haut, mais permet un plus grand range de valeurs au milieu du potentiometre
// où la plateforme ne bouge pas
double scaleElevateur(int entree, int vitesseMax){
	//On commence par le centrer sur 0
	double deltay = vitesseMax;
	double deltax = vitesseMax-123;
	double y = deltay/deltax * (entree - 123);
	double a;
	double vitesse = 0;
	//on le change pour une fonction de degre 3
	a = vitesseMax/pow(vitesseMax+100, 3);
	if (y>0){
		vitesse = a*pow(y+100,3);
	}
	if (y < 0){
		vitesse = a*pow(y-100,3);
	}
	//permet a la pateforme de ne pas bouger au milieu dans un range 
	if (entree < 155 && entree > 100 ){
		vitesse = 0;
	}
	
	return vitesse;
}


//Scaling linéraire du joystick droite gauche pour le differentiel
double scaleDroiteGauche(int entree, int differentielMax){
	double deltay = differentielMax*2;
	double deltax = 255;
	double y = deltay/deltax * (entree - 130);
	return y;
}


//Vérifie la connection pour un certain nombre de cycle
void fonctionConnection(int *manette_connecte){

	int i = 0;
	if (uart_rx_buffer_nb_line(UART_0) != 0){
	    *manette_connecte = TRUE;
	    lcd_clear_display();
	    lcd_set_cursor_position(0,0);
	    lcd_write_string("  connecte");
	    i = 0;
	}else{
	    lcd_clear_display();
	    lcd_set_cursor_position(0,0);
	    lcd_write_string("Deconnecte");

	    while (uart_rx_buffer_nb_line(UART_0) == 0){
	        i++;
	        _delay_ms(1);
	        if (i >= 300){
	            *manette_connecte = FALSE;
	            return ;
	        }
	    }
	}
}
