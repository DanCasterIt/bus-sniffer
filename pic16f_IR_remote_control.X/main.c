#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <stdint.h>
#include <pic16f876a.h>
#include <string.h>

#define _XTAL_FREQ 20000000
#define LENGHT 48
#define BUFLEN LENGHT

// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

void setup_peripheals(void);
void putch(char c);
char read_char(void);
void read_line(char * s, int max_len);

int main(int argc, char** argv) {
    char c, number[10];
    uint8_t i, e, val, array[BUFLEN], a, b, cycles;
    uint16_t arrayv[BUFLEN];
    setup_peripheals();
    printf("RESET\n\r");

    while (1) {
        PORTAbits.RA1 = 0;
        PORTAbits.RA2 = 0;
        printf("Registrare(e), riprodurre(i) o test(t): ");
        c = read_char();
        printf("%c\n\r", c);
        if (c == 'e' || c == 'i' || c == 't') {
            if (c == 'e') {
                printf("Registrando...\n\r");
                T1CONbits.TMR1ON = 1;
                for (i = 0; i < LENGHT; i++) {
                    val = PORTAbits.RA0;
                    TMR1 = 0;
                    PIR1bits.TMR1IF = 0;
                    array[i] = !PORTAbits.RA0;
                    PORTAbits.RA1 = !PORTAbits.RA0;
                    PORTAbits.RA2 = !PORTAbits.RA0;
                    while (PORTAbits.RA0 == val && PIR1bits.TMR1IF == 0);
                    arrayv[i] = TMR1;
                    //if(PIR1bits.TMR1IF == 1)    arrayv[i] = 0xFFFF;
                }
                PORTAbits.RA1 = 0;
                PORTAbits.RA2 = 0;
                T1CONbits.TMR1ON = 0;
                for (i = 0; i < LENGHT; i++)    {
                    printf("[%2u] %5u-%u,\t\t", i,  arrayv[i], array[i]);
                    arrayv[i] = 0xFFFF - arrayv[i];
                    printf("%5u-%u,\n\r", arrayv[i], array[i]);
                }
                printf("Selezioare il periodo.\n\r");
                printf("Inizio: ");
                read_line(number, 10);
                a = atoi(number);
                printf("Fine: ");
                read_line(number, 10);
                b = atoi(number);
                b =  b - a + 1;
                for(i = 0; i < b; i++)   array[i] = array[i + a];
                for(i = 0; i < b; i++)   arrayv[i] = arrayv[i + a];
                for(i = 0; i < b; i++)  printf("[%2u] %5u-%u,\n\r", i,  arrayv[i], array[i]);
            }
            if (c == 'i') {
                printf("Digitare quante volte riprodurre la sequenza: ");
                read_line(number, 10);
                cycles = atoi(number);
                printf("Riproducendo...\n\r");
                T1CONbits.TMR1ON = 1;
                for (e = 0; e < cycles; e++) {
                    for (i = 0; i < b; i++) {
                        PIR1bits.TMR1IF = 0;
                        TMR1 = arrayv[i];
                        PORTAbits.RA1 = array[i];
                        PORTAbits.RA2 = array[i];
                        while (PIR1bits.TMR1IF == 0);
                    }
                }
                T1CONbits.TMR1ON = 0;
            }
            if (c == 't') {
                printf("Resettare per uscire.\n\r");
                while (1) {
                    PORTAbits.RA1 = !PORTAbits.RA0;
                    PORTAbits.RA2 = !PORTAbits.RA0;
                }
            }
        } else printf("ERRORE: digitare 'e' o 'i'.\n\r");
    }
}

void setup_peripheals() {
    //I/O
    PORTA = 0;
    ADCON1bits.PCFG = 0x06; //Setting all the analog ports as digital inputs
    TRISB = 0; // all tris-b-a-c as output
    TRISA = 0x01; //except RA0
    TRISC = 0;

    //USART setup
    TRISCbits.TRISC6 = 0; // TX as output
    TRISCbits.TRISC7 = 1; // RX as input
    PORTCbits.RC6 = 1;

    TXSTAbits.SYNC = 0; // Async operation
    TXSTAbits.TX9 = 0; // No tx of 9th bit
    TXSTAbits.TXEN = 1; // Enable transmitter
    RCSTAbits.RC9 = 0; // No rx of 9th bit
    // Setting for 19200 BPS
    //BAUDCONbits.BRG16 = 0;   // Divisor at 8 bit
    TXSTAbits.BRGH = 0; // No high-speed baudrate
    SPBRG = 15; // divisor value for 19200

    RCSTAbits.CREN = 1; // Enable receiver
    RCSTAbits.SPEN = 1; // Enable serial port

    PIE1bits.TXIE = 0; // disable TX hardware interrupt
    PIE1bits.RCIE = 0; // disable RX hardware interrupt

    //TIMER1 (16-bit)
    T1CONbits.T1CKPS = 0b11; // prescaler 1:8 fosc/4
    T1CONbits.T1OSCEN = 1;
    T1CONbits.TMR1CS = 0;
    T1CONbits.TMR1ON = 0;
    TMR1 = 0;

    PIE1bits.TMR1IE = 0;
    PIR1bits.TMR1IF = 0;
}

void putch(char c) {
    // wait the end of transmission
    while (TXSTAbits.TRMT == 0) {
    };
    TXREG = c; // send the new byte
}

char read_char(void) {
    while (PIR1bits.RCIF == 0) {
        if (RCSTAbits.OERR == 1) {
            RCSTAbits.OERR = 0;
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
        }
    }
    return RCREG;
}

void read_line(char * s, int max_len) {
    int i = 0;
    for (;;) {
        char c = read_char();
        if (c == 13) {
            putchar(c);
            putchar(10);
            s[i] = 0;
            return;
        } else if (c == 127 || c == 8) {
            if (i > 0) {
                putchar(c);
                putchar(' ');
                putchar(c);
                --i;
            }
        } else if (c >= 32) {
            if (i < max_len) {
                putchar(c);
                s[i] = c;
                ++i;
            }
        }
    }
}