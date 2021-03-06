#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <stdint.h>
#include <pic16f876a.h>
#include <string.h>

#define _XTAL_FREQ 20000000
//#define LENGHT 0x1FFFF //23LCV1024 maximum address value
#define LENGHT 1024

#define SPI_RX_IN_PROGRESS 0x0
#define DUMMY_DATA 0x0

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
uint8_t SPI1_Exchange8bit(uint8_t data);
uint8_t SPI1_Exchange8bitBuffer(uint8_t *dataIn, uint8_t bufLen, uint8_t *dataOut);
void RAM_sequential_SPI_write(volatile unsigned char* latch, uint8_t pin_number, uint24_t address, uint8_t *data_out, uint8_t data_out_size);
void RAM_sequential_SPI_read(volatile unsigned char* latch, uint8_t pin_number, uint24_t address, uint8_t *data_in, uint8_t data_in_size);
void interrupt ISR(void);

char c, number[10];
uint24_t i, e, a, b, cycles;
uint16_t tvalue;
uint8_t bvalue;

int main(int argc, char** argv) {
    setup_peripheals();
    printf("RESET\n\r");
    PORTCbits.RC0 = 1; //SS0 high by default
    PORTCbits.RC1 = 1; //SS1 high by default
    PORTCbits.RC2 = 1; //SS2 high by default
    for(i=0; i<10; i++)  {
        bvalue = i << 2;
        printf("tvalue = %d\t", (int)bvalue);
        RAM_sequential_SPI_write(&PORTC, 2, i, (uint8_t*) & bvalue, 1);
        printf("%d\t", (int)bvalue);
        RAM_sequential_SPI_read(&PORTC, 2, i, (uint8_t*) & bvalue, 1);
        printf("%d\n\r", (int)bvalue);
    }
    while (1) {
        PORTAbits.RA1 = 0;
        PORTAbits.RA2 = 0;
        printf("Registrare(e), riprodurre(i) o test(t): ");
        c = read_char();
        printf("%c\n\r", c);
        if (c == 'e' || c == 'i' || c == 't') {
            if (c == 'e') {
                printf("Registrando...\n\r");
                bvalue = 0;
                tvalue = 0;
                i = 0;
                TMR1 = 0;
                INTCONbits.RBIF = 0;
                INTCONbits.RBIE = 1;
                PIR1bits.TMR1IF = 0;
                T1CONbits.TMR1ON = 1;
                //array[i] = ~PORTBbits.RB4;
                bvalue = ~PORTBbits.RB4;
                RAM_sequential_SPI_write(&PORTC, 2, i + 2, &bvalue, 1);
                PORTAbits.RA1 = ~PORTBbits.RB4;
                PORTAbits.RA2 = ~PORTBbits.RB4;
                while (i < LENGHT);
                PORTAbits.RA1 = 0;
                PORTAbits.RA2 = 0;
                for (i = 0; i < LENGHT; i += 3) {
                    RAM_sequential_SPI_read(&PORTC, 2, i, (uint8_t*) & tvalue, 2);
                    RAM_sequential_SPI_read(&PORTC, 2, i + 2, &bvalue, 1);
                    printf("[%4u] %5u-%u,\t\t", (unsigned int) i, (unsigned int) tvalue, (unsigned int) bvalue);
                    tvalue = 0xFFFF - tvalue;
                    RAM_sequential_SPI_write(&PORTC, 2, i, (uint8_t*) & tvalue, 2);
                    printf("%5u-%u\n\r", (unsigned int) tvalue, (unsigned int) bvalue);
                }
                printf("Selezioare il periodo.\n\r");
                printf("Inizio: ");
                read_line(number, 10);
                a = atoi(number);
                printf("Fine: ");
                read_line(number, 10);
                b = atoi(number);
                b = b - a + 1;
                for (i = 0; i < b; i += 3) {
                    RAM_sequential_SPI_read(&PORTC, 2, i + a, (uint8_t*) & tvalue, 2);
                    RAM_sequential_SPI_read(&PORTC, 2, (i + a) + 2, &bvalue, 1);
                    RAM_sequential_SPI_write(&PORTC, 2, i, (uint8_t*) & tvalue, 2);
                    RAM_sequential_SPI_write(&PORTC, 2, i + 2, &bvalue, 1);
                    printf("[%4u] %5u-%u,\t\t", (unsigned int) i, (unsigned int) (0xFFFF - tvalue), (unsigned int) bvalue);
                    printf("%5u-%u\n\r", (unsigned int) tvalue, (unsigned int) bvalue);
                }
            }
            if (c == 'i') {
                printf("Digitare quante volte riprodurre la sequenza: ");
                read_line(number, 10);
                cycles = atoi(number);
                printf("Riproducendo...\n\r");
                i = 0;
                e = 0;
                PIR1bits.TMR1IF = 0;
                T1CONbits.TMR1ON = 1;
                while (e < cycles);
                T1CONbits.TMR1ON = 0;
            }
            if (c == 't') {
                printf("Resettare per uscire.\n\r");
                while (1) {
                    PORTAbits.RA1 = ~PORTBbits.RB4;
                    PORTAbits.RA2 = ~PORTBbits.RB4;
                }
            }
        } else printf("ERRORE: digitare 'e', 'i' o 't'.\n\r");
    }
}

void setup_peripheals() {
    //I/O
    PORTA = 0;
    ADCON1bits.PCFG = 0x06; //Setting all the analog ports as digital ports
    // all tris-b-a-c as output
    TRISA = 0;
    TRISB = 0x10; //except RB4
    TRISC = 0x10; //except RC4

    //interrupts configurations
    INTCONbits.GIE = 1; // global enable interrupts
    INTCONbits.PEIE = 1; // peripheral enable interrupts

    //PORTB's interrupt-on-change feature (only from RB4 to RB7).
    INTCONbits.RBIF = 0;
    INTCONbits.RBIE = 0;

    //USART setup
    TRISCbits.TRISC6 = 0; // TX as output
    TRISCbits.TRISC7 = 1; // RX as input
    PORTCbits.RC6 = 1;

    TXSTAbits.SYNC = 0; // Async operation
    TXSTAbits.TX9 = 0; // No tx of 9th bit
    TXSTAbits.TXEN = 1; // Enable transmitter
    RCSTAbits.RC9 = 0; // No rx of 9th bit
    //Setting for 19200 BPS
    TXSTAbits.BRGH = 0; // No high-speed baudrate
    SPBRG = 15; // divisor value for 19200

    RCSTAbits.CREN = 1; // Enable receiver
    RCSTAbits.SPEN = 1; // Enable serial port

    PIE1bits.TXIE = 0; // disable TX hardware interrupt
    PIE1bits.RCIE = 0; // disable RX hardware interrupt

    //TIMER1 (16-bit)
    T1CONbits.T1CKPS = 0b11; // prescaler 1:8 fosc/4
    T1CONbits.T1OSCEN = 0;
    T1CONbits.TMR1CS = 0;
    T1CONbits.TMR1ON = 0;
    TMR1 = 0;

    PIE1bits.TMR1IE = 1;
    PIR1bits.TMR1IF = 0;

    //MSSP in SPI master mode
    //SDO on RC5, SDI on RC4, SCK on RC3
    SSPSTATbits.SMP = 1; //Input data sampled at end of data output time
    SSPSTATbits.CKE = 1; //Transmit occurs on transition from active to Idle clock state
    SSPCONbits.SSPEN = 1; //Synchronous Serial Port Enable bit
    SSPCONbits.CKP = 0; //Idle state for clock is a low level
    SSPCONbits.SSPM = 0; //SPI Master mode, clock = FOSC/4 (5MHz)
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

uint8_t SPI1_Exchange8bit(uint8_t data) {
    // Clear the Write Collision flag, to allow writing
    SSPCONbits.WCOL = 0;

    SSPBUF = data;

    while (SSPSTATbits.BF == SPI_RX_IN_PROGRESS) {
    }

    return (SSPBUF);
}

uint8_t SPI1_Exchange8bitBuffer(uint8_t *dataIn, uint8_t bufLen, uint8_t *dataOut) {
    uint8_t bytesWritten = 0;

    if (bufLen != 0) {
        if (dataIn != NULL) {
            while (bytesWritten < bufLen) {
                if (dataOut == NULL) {
                    SPI1_Exchange8bit(dataIn[bytesWritten]);
                } else {
                    dataOut[bytesWritten] = SPI1_Exchange8bit(dataIn[bytesWritten]);
                }

                bytesWritten++;
            }
        } else {
            if (dataOut != NULL) {
                while (bytesWritten < bufLen) {
                    dataOut[bytesWritten] = SPI1_Exchange8bit(DUMMY_DATA);

                    bytesWritten++;
                }
            }
        }
    }

    return bytesWritten;
}

void RAM_set_SPI_mode(volatile unsigned char* latch, uint8_t pin_number) {
    *latch &= ~(1 << pin_number); //metto CS in low per avviare i trasferimenti
    SPI1_Exchange8bit(0b11111111); //comando RSTIO
    *latch |= (1 << pin_number); //metto CS in high per bloccare i trasferimenti
}

void RAM_sequential_SPI_write(volatile unsigned char* latch, uint8_t pin_number, uint24_t address, uint8_t *data_out, uint8_t data_out_size) {
    uint8_t address_array[3];
    address_array[2] = address;
    address_array[1] = address >> 8;
    address_array[0] = address >> 16;
    *latch &= ~(1 << pin_number); //metto CS in low per avviare i trasferimenti
    SPI1_Exchange8bit(0b00000010); //comando WRITE
    SPI1_Exchange8bitBuffer(address_array, 3, NULL);
    SPI1_Exchange8bitBuffer(data_out, data_out_size, NULL);
    *latch |= (1 << pin_number); //metto CS in high per bloccare i trasferimenti
}

void RAM_sequential_SPI_read(volatile unsigned char* latch, uint8_t pin_number, uint24_t address, uint8_t *data_in, uint8_t data_in_size) {
    uint8_t address_array[3];
    address_array[2] = address;
    address_array[1] = address >> 8;
    address_array[0] = address >> 16;
    *latch &= ~(1 << pin_number); //metto CS in low per avviare i trasferimenti
    SPI1_Exchange8bit(0b00000011); //comando READ
    SPI1_Exchange8bitBuffer(address_array, 3, NULL);
    SPI1_Exchange8bitBuffer(NULL, data_in_size, data_in);
    *latch |= (1 << pin_number); //metto CS in high per bloccare i trasferimenti
}

void interrupt ISR(void) {
    if (c == 'e') {
        tvalue = TMR1;
        RAM_sequential_SPI_write(&PORTC, 2, i, (uint8_t*) & TMR1, 2);
        TMR1 = 0;
        i += 3;
        if (i < LENGHT) {
            bvalue = ~PORTBbits.RB4;
            RAM_sequential_SPI_write(&PORTC, 2, i + 2, &bvalue, 1);
        } else {
            T1CONbits.TMR1ON = 0;
            INTCONbits.RBIE = 0;
        }
        PORTAbits.RA1 = ~PORTBbits.RB4;
        PORTAbits.RA2 = ~PORTBbits.RB4;
        INTCONbits.RBIF = 0;
        PIR1bits.TMR1IF = 0;
    } else {
        if (e < cycles) {
            RAM_sequential_SPI_read(&PORTC, 2, i, (uint8_t*) & TMR1, 2);
            RAM_sequential_SPI_read(&PORTC, 2, i + 2, &bvalue, 1);
            PORTAbits.RA2 = bvalue;
            PORTAbits.RA1 = bvalue;
            i += 3;
            if (i >= b) {
                e++;
                i = 0;
            }
        } else T1CONbits.TMR1ON = 0;
        PIR1bits.TMR1IF = 0;
    }
}