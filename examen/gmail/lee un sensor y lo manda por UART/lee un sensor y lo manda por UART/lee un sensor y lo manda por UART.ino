#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

// Inicializa UART
void UART_init() {
    uint16_t ubrr = F_CPU/16/9600 - 1;
    UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;
    UCSR0B = (1 << TXEN0);  // Habilita transmisión
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8 bits de datos, 1 bit stop
}

// Envía un solo caracter
void UART_sendChar(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

// Envía una cadena
void UART_sendString(const char* str) {
    while (*str) {
        UART_sendChar(*str++);
    }
}

// Inicializa ADC (canal 0)
void ADC_init() {
    ADMUX = (1 << REFS0);  // Referencia AVCC, canal ADC0
    ADCSRA = (1 << ADEN) | (1 << ADPS1) | (1 << ADPS0);  // Habilita ADC, prescaler 8
}

// Lee el canal 0 del ADC
uint16_t ADC_read() {
    ADCSRA |= (1 << ADSC);             // Inicia conversión
    while (ADCSRA & (1 << ADSC));      // Espera fin de conversión
    return ADC;
}

int main(void) {
    char buffer[32];
    UART_init();
    ADC_init();

    while (1) {
        uint16_t adc_val = ADC_read();
        float voltage = adc_val * 5.0 / 1023.0;

        dtostrf(voltage, 4, 2, buffer);  // Convierte float a string

        UART_sendString("ADC: ");
        itoa(adc_val, buffer, 10);
        UART_sendString(buffer);
        UART_sendString(" | Voltaje: ");

        dtostrf(voltage, 4, 2, buffer);
        UART_sendString(buffer);
        UART_sendString(" V\r\n");

        _delay_ms(1000);
    }
}
