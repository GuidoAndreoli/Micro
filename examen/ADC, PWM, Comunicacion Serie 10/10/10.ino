#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// === UART ===
void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0);                    // Habilita transmisión
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   // 8 bits de datos
}

void uart_send(char data) {
    while (!(UCSR0A & (1 << UDRE0)));         // Espera buffer vacío
    UDR0 = data;
}

void uart_send_string(const char* str) {
    while (*str) {
        uart_send(*str++);
    }
}

// === ADC ===
void adc_init() {
    ADMUX = (1 << REFS0);                     // AVcc como referencia
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Prescaler 64
}

uint16_t read_adc(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); // Selección del canal
    ADCSRA |= (1 << ADSC);                    // Inicia conversión
    while (ADCSRA & (1 << ADSC));             // Espera que termine
    return ADC;
}

// === PWM ===
void pwm_init() {
    DDRD |= (1 << PD6);                       // OC0A como salida
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00); // Fast PWM, no inversor
    TCCR0B = (1 << CS01);                     // Prescaler 8
}

void pwm_set_duty(uint8_t duty) {
    OCR0A = duty;
}

// === MAIN ===
int main() {
    char buffer[64];
    uint16_t ldr_ext, ldr_int;
    uint8_t duty;

    uart_init(103);     // 9600 baudios con F_CPU = 16MHz
    adc_init();
    pwm_init();

    while (1) {
        ldr_ext = read_adc(0); // LDR exterior: ADC0
        ldr_int = read_adc(1); // LDR interior: ADC1

        // Ajuste del duty cycle: inversamente proporcional a la luz exterior
        if (ldr_ext < 900) {
            duty = 255 - (ldr_ext / 4);  // De 255 a ~30
        } else {
            duty = 0;
        }

        pwm_set_duty(duty);

        // Enviar por UART
        snprintf(buffer, sizeof(buffer), "LDR Ext: %u | LDR Int: %u | Duty: %u%%\r\n",
                 ldr_ext, ldr_int, (duty * 100) / 255);
        uart_send_string(buffer);

        _delay_ms(500);
    }

    return 0;
}
