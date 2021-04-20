/*
 * RECEPTION WIFI
 *
 * Created: 2/12/2021 2:03:27 PM
 * Author : beaul
 */ 
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

#define MODE_SW1 1
#define	MODE_SW2 2
// Décommenter si le bouton sw3 est utilisé pour un mode
//#define	MODE_SW3 3  

//Constantes du mode 1
const int VITESSE_MAX_ROUES_MODE_1 = 255;

//Vitesse max de rotation sur lui meme
const int VITESSE_MAX_ROTATION_MODE_1 = 100;

//le differentiel devrait toujours etre de 10% de la vitesse max selon Youri
const int DIFFERENTIEL_MODE_1 = 25;

//Changer la valeur de la vitesse de la roue d'inertie pendant les tests et recompiler. Restera costante dans la competition
//Nous changerons l'angle de la plateforme seulement

const int VITESSE_INERTIE = 120;

//Changer les valeurs pour changer les angles du servo moteur. 0 degres: 1000, 90 degres: 2000
const int VALEUR_SERVO_ATTENTE = 1500;
const int VALEUR_SERVO_ARME = 1800;
const int VALEUR_SERVO_REVIENS = 1100;

//Elevateur
const int VITESSE_MAX_ELEVATEUR = 255;
	


//Prototypes
double scaleAvantArriere(int entree, int vitesseMax);
double scaleElevateur(int entree, int vitesseMax);

void fonctionConnection(int *manette_connecte);

double scaleDroiteGauche(int entree, int differentielMax);
int main(void)
{		

//---------------------------
//VARIABLES
//------------------------------

//Valeur int recues de la manette
int valeur_axe_x = 0;
int valeur_axe_y = 0;
int valeur_pot = 0;
int valeur_sw1 = 0;
int valeur_sw2 = 0;
int valeur_sw3 = 0;
int valeur_bp_joystick = 0;

double vitesse=0;
double differentiel=0;
double vitesseRD=0;
double vitesseRG=0;

double vitesseElevateur = 0;

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
	
	
//Initilisation de sa valeur avec l'angle en attente
int valeur_servo = VALEUR_SERVO_ATTENTE;
	

//Etat 1 au debut sans appuyer sur aucun bouton
int state = MODE_SW1;

// Création d'un tableau pour la réception de données de la manette.
char string_recu[33];
//-----------------------------------------------------------------
//-------------------------------------------------------

  //Initialisation des sorties et du MLI

	pwm0_init();
	pwm1_init(20000);
	pwm2_init();
	
	//Initialisation du module UART0
	uart_init(UART_0);
	sei();

  //Initialisation Lcd

	lcd_init();

//*************************************************
//AFFECTATION DES VALEURS AUX MOTEURS (INITIALES)
//*************************************************
	vitesseRG = abs(0);
	vitesseRD = abs(0);
			
			
	//Affcetation de la vitessem au moteur roues gauches
	pwm0_set_PB3(0);
	//Affcetation de la vitessem au moteur roues droites
	pwm0_set_PB4(0);
			
	//Elevateur 
	vitesseElevateur = 0;
	pwm2_set_PD6(0);
			
	//roue inertie
	pwm2_set_PD7(0);
			
	//servomoteur
	pwm1_set_PD5(VALEUR_SERVO_ATTENTE);


	//---------------------------------------------------
	
	
	
	while (1)
	{
		

		//Verification si la mannette est connectee 
		fonctionConnection(&mannette_connecte);
		
		
		
		
		//Verifier l'etat de la file rx, si elle est pleine on rentre
		if (mannette_connecte == TRUE){
			
			uart_get_line(UART_0, string_recu, 17);
			
			//Remettre a zero a chaque bouble while les valeurs construites de UART
			valeur_axe_x = 0;
			valeur_axe_y = 0;
			valeur_pot = 0;
			valeur_sw1 = 0;
			valeur_sw2 = 0;
			valeur_sw3 = 0;
			valeur_bp_joystick = 0;
			
			
			state = 1;

			//Remettre a zero les valeurs de moteurs
			vitesse = 0;
			vitesseRD = 0;
			vitesseRG= 0;
			differentiel= 0;
			vitesseElevateur = 0;
			valeur_servo = VALEUR_SERVO_ATTENTE;

		
			//********************************************
			//construction des valeurs numeriques recues du transmetteur
			//*******************************************
			valeur_axe_x += 100* char_to_uint(string_recu[1]) +  10* char_to_uint(string_recu[2])+  char_to_uint(string_recu[3]);
			valeur_axe_y += 100* char_to_uint(string_recu[4]) +  10* char_to_uint(string_recu[5])+  char_to_uint(string_recu[6]);
			valeur_pot += 100* char_to_uint(string_recu[7]) +  10* char_to_uint(string_recu[8])+  char_to_uint(string_recu[9]);
			valeur_sw1 = string_recu[10]- 48;
			valeur_sw2 = string_recu[11]- 48;
			valeur_sw3 = string_recu[12]- 48;
			valeur_bp_joystick = string_recu[13]- 48;
		} 
			
			//********************************************
			// TEST AFFICHAGE CONSTRUCTION VALEUR
			//********************************************
			//char str_test [32];
			//sprintf(str_test,"%3d%3d%3d%d%d%d%d", valeur_axe_x , valeur_axe_y, valeur_pot, valeur_sw1, valeur_sw2, valeur_sw3, valeur_bp_joystick);
			//lcd_set_cursor_position(0,1);
			//lcd_write_string(str_test);
			
			//********************************************
			// Gestion de la machine d'�tat selon le mode
			//********************************************
			if (valeur_sw1 == 0){
				state = MODE_SW1;
				lcd_clear_display();
			}
			if (valeur_sw2 == 0){
				state = MODE_SW2;
				lcd_clear_display();
			}
			/*if (valeur_sw3 == 0){
				state = MODE_SW3;
				lcd_clear_display();
			}*/
                            //********************************************
                            // MACHINE DETAT SELON LE MODE
                            //********************************************
			
			switch(state){

				//MODE NORMAL 
				case MODE_SW1:
				lcd_set_cursor_position(0,0);
				lcd_write_string("Mode normal  ");
	
				
				//Scaling de la vitesse
				vitesse = scaleAvantArriere(valeur_axe_x,VITESSE_MAX_ROUES_MODE_1);
				differentiel = scaleDroiteGauche(valeur_axe_y, DIFFERENTIEL_MODE_1);	
				
			
				
				//Affectation du differentiel et du sens de rotation 
				if (vitesse > 0){
					vitesseRD = vitesse - differentiel;
					vitesseRG = vitesse + differentiel;
					//Roues gauches U7 et U9-----------------------------
					//Affectation de la bonne direction sur les pont en H respectif
					PORTB = set_bit(PORTB, PB1);	
					//Roues droites U6 et U8------------------------------
					//Affectation de la bonne durection sur les pont en H respectif
					PORTB = clear_bit(PORTB, PB2);
				}
				if (vitesse < 0){
						vitesseRD = vitesse + differentiel;
						vitesseRG = vitesse - differentiel;
					//Roues gauches U7 et U9-----------------------------
					//Affectation de la bonne direction sur les pont en H respectif
					PORTB = clear_bit(PORTB, PB1);
					//Roues droites U6 et U8------------------------------
					//Affectation de la bonne durection sur les pont en H respectif
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
				
				
				//Cas special permettant de faire tournern le vehicule sur lui meme
				else if (valeur_axe_x < 145 && valeur_axe_x > 110){
					vitesse = scaleDroiteGauche(valeur_axe_y, VITESSE_MAX_ROTATION_MODE_1);
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
			
			
			///////////////////////////////////////////////////
			//				SERVO MOTEUR
			///////////////////////////////////////////
			//Verifiction de la valeur du jystick pour actionner le servomoteur si besoin
			if (valeur_bp_joystick == 0){
				//Actionner le sevo moteur
				valeur_servo = VALEUR_SERVO_ARME;
			}
			
	
				
				break;


      case MODE_SW2:

      //AJOUTER ICI LE MODE 2

      break;
	/*
      case MODE_SW3:

      AJOUTER ICI LE MODE 3
    
      break;
        //FIN DE LA MACHINE DETAT */

			}

		 //*************************************************
		//AFFECTATION DES VALEURS AUX MOTEURS
		//*************************************************
		
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
				
				//roue intertie
				
			
				if(valeur_sw3 == 1){
					bouton_3_enfonce = FALSE;
				}
				
				if (valeur_sw3 == 0 && bouton_3_enfonce == FALSE){
					if (roue_inertie_marche == FALSE){
						roue_inertie_marche = TRUE;
						pwm2_set_PD7(VITESSE_INERTIE);
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
				//Affcetation de la vitessem au moteur roues gauches
				pwm0_set_PB3(0);
				//Affcetation de la vitessem au moteur roues droites
				pwm0_set_PB4(0);
				
				
				pwm2_set_PD6(0);
				
				//roue inertie
				pwm2_set_PD7(0);
				
				//servomoteur
				pwm1_set_PD5(VALEUR_SERVO_ATTENTE);
				
			}
			
			
			
		//*************************************************
      //AFFICHAGE LCD 
      //*************************************************
			char strRD[10];
			char strRG[10];
			char strelev[10];
			uint8_to_string(strRG, vitesseRG);
			uint8_to_string(strRD, vitesseRD);
			uint8_to_string(strelev, vitesseElevateur);
			
			lcd_set_cursor_position(0, 1);
			lcd_write_string("RG:");
			lcd_set_cursor_position(3, 1);
			lcd_write_string(strRG);
			lcd_set_cursor_position(6, 1);
			lcd_write_string("RD:");
			lcd_set_cursor_position(9, 1);
			lcd_write_string(strRD);
			lcd_set_cursor_position(12, 1);
			lcd_write_string(strelev);
		}
			
		
}

//*************************************************
//FONCTIONS DE SCALING 
//*************************************************


//Cette fonction fait le scaling avant arriere du joystick selon unbe fonction du 3e degre
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


//Cette fonction est similaire a celle du haut, mais permet un plus grand range de valewurs au milieu du potentiometre //ou la plateforme ne bouge pas 
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


//Scaling linearie du joystick droite gauche pour le differentiel 
double scaleDroiteGauche(int entree, int differentielMax){
	double deltay = differentielMax*2;
	double deltax = 255;
	double y = deltay/deltax * (entree - 130);
	return y;
}

//Fonction qui verifie la connection pour un certain nombre de cycle 
void fonctionConnection(int *manette_connecte){
	int i = 0;
		if (uart_rx_buffer_nb_line(UART_0) != 0){
			*manette_connecte = TRUE;
			lcd_set_cursor_position(0,0);
			lcd_write_string("  connecte");
			i = 0;
		}else{
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
	
	


