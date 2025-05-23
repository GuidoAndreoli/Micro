#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>

// UART
void UART_init() {
    uint16_t ubrr = F_CPU / 16 / 9600 - 1;
    UBRR0H = (ubrr >> 8);
    UBRR0L = ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_sendChar(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void UART_sendString(const char *str) {
    while (*str) UART_sendChar(*str++);
}

// DHT11 - PD4
#define DHT_PORT PORTD
#define DHT_DDR  DDRD
#define DHT_PIN  PIND
#define DHT_INPUT_PIN  PD4

uint8_t dht_data[5];

void DHT_request() {
    DHT_DDR |= (1 << DHT_INPUT_PIN);
    DHT_PORT &= ~(1 << DHT_INPUT_PIN);
    _delay_ms(20);
    DHT_PORT |= (1 << DHT_INPUT_PIN);
    _delay_us(40);
    DHT_DDR &= ~(1 << DHT_INPUT_PIN);
}

bool DHT_response() {
    _delay_us(40);
    if ((DHT_PIN & (1 << DHT_INPUT_PIN))) return false;
    _delay_us(80);
    if (!(DHT_PIN & (1 << DHT_INPUT_PIN))) return false;
    _delay_us(80);
    return true;
}

uint8_t DHT_readByte() {
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        while (!(DHT_PIN & (1 << DHT_INPUT_PIN)));  // Espera a que suba
        _delay_us(30);
        if (DHT_PIN & (1 << DHT_INPUT_PIN))
            data |= (1 << (7 - i));
        while (DHT_PIN & (1 << DHT_INPUT_PIN));     // Espera a que baje
    }
    return data;
}

bool DHT_getData(uint8_t *humedad, uint8_t *temperatura) {
    DHT_request();
    if (!DHT_response()) return false;

    for (int i = 0; i < 5; i++)
        dht_data[i] = DHT_readByte();

    uint8_t suma = dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3];
    if (suma != dht_data[4]) return false;

    *humedad = dht_data[0];
    *temperatura = dht_data[2];
    return true;
}

int main(void) {
    UART_init();
    uint8_t hum, temp;
    char buffer[16];

    while (1) {
        if (DHT_getData(&hum, &temp)) {
            UART_sendString("Temp: ");
            itoa(temp, buffer, 10);
            UART_sendString(buffer);
            UART_sendString(" C | Hum: ");
            itoa(hum, buffer, 10);
            UART_sendString(buffer);
            UART_sendString(" %\r\n");
        } else {
            UART_sendString("Error al leer DHT11\r\n");
        }
        _delay_ms(2000);
    }
}
