/*
 * File:   main_preLab.c
 * Author: Javier Alejandro Pérez Marín
 * 
 * Pic maestro y esclavo que se define por SS (RA5) en high es maestro,
 * donde el maestro lee mediante el ADC un potenciómetro y mediante SPI
 * pasa al esclavo este valor para indicarlo con leds en PORTD
 *
 * Created on 10 de mayo de 2022, 02:16 PM
 */

// PIC16F887 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

/*
 * CONSTANTES
 */
#define _XTAL_FREQ 1000000

/*
 * VARIABLES
 */
uint8_t val_pot = 0; //Valor del potenciómetro

/*
 * PROTOTIPO DE FUNCIÓN
 */
void setup(void);

void __interrupt() isr(void){
    if(PORTAbits.RA5){ // Si es PIC maestro
        //Interrupción del ADC
        if (PIR1bits.ADIF){
            if (ADCON0bits.CHS == 0){ //Se verifica canal AN0        
                val_pot = ADRESH; //Se guardan los 8 bits de conversión

                if(SSPSTATbits.BF){
                    SSPBUF = val_pot;
                } 
            }
            PIR1bits.ADIF = 0; // Limpiamos bandera ADC
        }
    }
    else{ //Esclavo
    //Se revisa interrupción SPI
        if (PIR1bits.SSPIF){
            PORTD = SSPBUF; //Se pasa valor recibido al PORTD
            PIR1bits.SSPIF = 0; //Limpieza de bandera del SPI
        }
    }
    return;
}

void main(void) {  
   
    setup(); // Se pasa a configurar PIC
        
    while(1)
    {  
        if(ADCON0bits.GO == 0){ // Si no hay proceso de conversión
            ADCON0bits.GO = 1; // Se inicia proceso de conversión
        }
     } 
}


void setup(void){
    ANSEL = 0x01; //Se configura PORTA0/AN0 como entrada analógica
    ANSELH = 0;   //I/O DIGITALES
    
    OSCCONbits.IRCF = 0b100; //Oscilador interno de 1 MHz
    OSCCONbits.SCS = 1; //Oscilador interno
    
    TRISA = 0b00100001; //PORTA0 y PORTA5 como INPUT    
    PORTA = 0;    //CLEAR DE PUERTO A  
    
    TRISD = 0; //PORTD como OUTPUT
    PORTD = 0; //CLEAR DE PUERTO D
    
    if(PORTAbits.RA5){
        TRISC = 0b00010000; //SDI entrada, SCK y SD0 como salida
        PORTC = 0; 

        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0000;   //SPI Maestro, con Fosc/4 a 250 kbits/s
        SSPCONbits.CKP = 0;         //Se configura que el reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       //Se habilita pines de SPI
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        //Se configura que el dato sea enviado cada flanco de subida
        SSPSTATbits.SMP = 1;        //Se configura que el dato sea al final del pulso
        SSPBUF = val_pot;           // Se envía un dato inicial
        
        //Config ADC
        ADCON0bits.ADCS = 0b01; // Fosc/8
        ADCON1bits.VCFG0 = 0;  // Referencia VDD
        ADCON1bits.VCFG1 = 0;  // Referencia VSS
        ADCON0bits.CHS = 0b0000; // Se selecciona PORTA0/AN0
        ADCON1bits.ADFM = 0; // Se indica que se tendrá un justificado a la izquierda
        ADCON0bits.ADON = 1; // Se habilita el modulo ADC
        __delay_us(40);     // Delay para sample time
        
        //Config interrupciones
        INTCONbits.GIE = 1; //Se habilitan interrupciones globales
        PIE1bits.ADIE = 1;  //Se habilita interrupcion del ADC
        PIR1bits.ADIF = 0; // Limpieza de bandera del ADC
        INTCONbits.PEIE = 1; // Se habilitan interrupciones de periféricos
    }
    else{
        TRISC = 0b00011000; // -> SDI y SCK entradas, SD0 como salida
        PORTC = 0;
        
        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0100;   // -> SPI Esclavo, SS hablitado
        SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
        SSPSTATbits.SMP = 0;        // -> Dato al final del pulso de reloj
        
        PIE1bits.SSPIE = 1;   // Se habilita interrupción del SPI
        PIR1bits.SSPIF = 0;  // Limpieza de bandera del SPI
        INTCONbits.PEIE = 1;// Se habilitan interrupciones de periféricos
        INTCONbits.GIE = 1;//Se habilitan interrupciones globales
    }
 }