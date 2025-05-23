#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// Valores de frecuencia (TOP para OCR0A) y etiquetas
const uint8_t freq_vals[] = {249, 124, 62};         // ~1kHz, 2kHz, 4kHz
const char* freq_labels[] = {"1 kHz", "2 kHz", "4 kHz"};

// Duty cycles y etiquetas
const uint8_t duty_vals[] = {25, 50, 75};
const char* duty_labels[] = {"25%", "50%", "75%"};

// Índices
uint8_t freq_idx = 0;
uint8_t duty_idx = 0;

// UART
void uart_init(unsigned int ubrr) {
    UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_send(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_send_string(const char* str) {
    while (*str) uart_send(*str++);
}

void uart_send_line(const char* str) {
    uart_send_string(str);
    uart_send('\r');
    uart_send('\n');
}

void uart_report() {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Freq: %s | Duty: %s",
             freq_labels[freq_idx], duty_labels[duty_idx]);
    uart_send_line(buffer);
}

// PWM usando Timer0
void pwm_init() {
    DDRD |= (1 << PD6); // Pin 6 como salida
    TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1); // Fast PWM, no inversor
    TCCR0B = (1 << WGM02) | (1 << CS01); // Prescaler 8
}

void pwm_update(uint8_t top, uint8_t duty) {
    OCR0A = top;
    OCR0B = (top * duty) / 100;
}

// Botones: pin 7 (PD7), pin 8 (PB0)
void buttons_init() {
    DDRD &= ~(1 << PD7); PORTD |= (1 << PD7); // Entrada con pull-up
    DDRB &= ~(1 << PB0); PORTB |= (1 << PB0); // Entrada con pull-up
}

uint8_t button_pressed(volatile uint8_t *pin, uint8_t bit) {
    if (!(*pin & (1 << bit))) {
        _delay_ms(20); // debounce
        if (!(*pin & (1 << bit))) return 1;
    }
    return 0;
}

int main(void) {
    pwm_init();
    buttons_init();
    uart_init(103); // 9600 baudios para F_CPU = 16MHz

    pwm_update(freq_vals[freq_idx], duty_vals[duty_idx]);
    uart_report();

    while (1) {
        if (button_pressed(&PIND, PD7)) {
            freq_idx = (freq_idx + 1) % 3;
            pwm_update(freq_vals[freq_idx], duty_vals[duty_idx]);
            uart_report();
            while (!(PIND & (1 << PD7))); // espera que se suelte
        }

        if (button_pressed(&PINB, PB0)) {
            duty_idx = (duty_idx + 1) % 3;
            pwm_update(freq_vals[freq_idx], duty_vals[duty_idx]);
            uart_report();
            while (!(PINB & (1 << PB0))); // espera que se suelte
        }
    }
}
