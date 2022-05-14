/*
 * File:   main_Postlab_MASTER.c
 * Author: Javier Alejandro Pérez Marín 
 * 
 * Master que mediante comunicación SPI full duplex envía ADRESH a MCU2 y
 * recibe un contador de botones del MCU3
 *
 * Created on 13 de mayo de 2022, 11:59 AM
 */

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

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000
/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
uint8_t val_pot = 0; //Variable para guardar valor del potenciómetro

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);

/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    //Interrupción del ADC
    if (PIR1bits.ADIF){
        if (ADCON0bits.CHS == 0){ //Se verifica canal AN0        
            val_pot = ADRESH; //Se guardan los 8 bits de conversión
        }
        PIR1bits.ADIF = 0; // Limpiamos bandera ADC
    }
    
    return;
}
/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){        
        if(ADCON0bits.GO == 0){ // Si no hay proceso de conversión
            ADCON0bits.GO = 1; // Se inicia proceso de conversión
            __delay_us(40); // Sample time Delay
        }
        
        // Se da un pulso de reloj para generar Start del pic MCU2
        PORTAbits.RA6 = 1; // Se deshabilita el ss del esclavo MCU3
        __delay_ms(50); //Delay para que MCU3 note el cambio
        PORTAbits.RA7 = 0; // Se habilita nuevamente el escalvo MCU2
        __delay_ms(50); //Delay para que MCU2 note el cambio
        
        // Enviamos el dato del potenciómetro 
        SSPBUF = val_pot;   // Cargamos valor del ADRESH
        while(!SSPSTATbits.BF){}// Esperamos a que termine el envio
        
        //Se da señal de stop del MCU2 terminando la lectura
        PORTAbits.RA7 = 1; // Deshabilitamos el ss del esclavo
       __delay_ms(50);  // Delay para que el PIC MCU2 pueda detectar el cambio en el pin
       
        // Se da un pulso de reloj para generar Start del pic MCU3
        PORTAbits.RA6 = 0; // Se habilita el ss del esclavo MCU3
        __delay_ms(50); // Delay para que el PIC MCU3 pueda detectar el cambio en el pin
        
        // Se manda un dato para activar MCU3
        SSPBUF = 0xFF; // Master inicia la comunicación y prende el clock
        while(!SSPSTATbits.BF){} // Esperamos a que reciba datos
        PORTD = SSPBUF; //Se guarda valor de contador recibido.
        
        __delay_ms(50); //Delay 
    }
    return;
}
/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    ANSEL = 0x01; //Se configura PORTA0/AN0 como entrada analógica
    ANSELH = 0;   //I/O DIGITALES
    
    TRISA = 0x01; //PORTA0 como INPUT    
    PORTA = 0;    //CLEAR DE PUERTO A
    
    
    TRISD = 0; //PORTD como OUTPUT
    PORTD = 0; //CLEAR de PORTD
        
    OSCCONbits.IRCF = 0b100; // 1MHz
    OSCCONbits.SCS = 1; // Reloj interno
    
    // Configuracion de SPI
    // Configs de Maestro
    
    TRISC = 0b00010000; // SDI entrada, SCK y SD0 como salida
    PORTC = 0;

    // SSPCON <5:0>
    SSPCONbits.SSPM = 0b0000;  // SPI Maestro, Reloj -> Fosc/4 (250kbits/s)
    SSPCONbits.CKP = 0; // Reloj inactivo en 0
    SSPCONbits.SSPEN = 1; // Habilitamos pines de SPI
    // SSPSTAT<7:6>
    SSPSTATbits.CKE = 1; // Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 1;  // Dato al final del pulso de reloj
    SSPBUF = 100;  // Enviamos un dato inicial
    
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
