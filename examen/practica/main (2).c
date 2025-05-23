/*
 * main.c
 *
 * Created: 2/21/2025 23:11:07 PM
 *  Author: Guido
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BAUD 9600
#define UBRR ((F_CPU/16/BAUD)-1)

#define FAN_PIN     PD2
#define BUZZER_PIN  PD3
#define DHT_PIN     PD4

#define LCD_I2C_ADDR 0x27      
#define LCD_BACKLIGHT 0x08     
#define LCD_ENABLE 0x04           

typedef struct {
    uint8_t temperatura;
    uint8_t humedad;
} Muestra;

#define EEPROM_START  0
#define MAX_SAMPLES   100

// Variables globales
uint16_t sample_index = 0;      // Índice para almacenar muestras en EEPROM
uint16_t Sample_counter = 0;    // Contador de muestras
volatile uint16_t sample_time;  // Tiempo de muestreo

// Función para verificar si hay datos disponibles en UART
uint8_t uart_available(){
    return (UCSR0A & (1 << RXC0));
}

void i2c_init(void) {
    TWSR = 0;
    TWBR = 72; // ((F_CPU / 100000UL) - 16) / 2
}

void i2c_start(void) {
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
}

void i2c_stop(void) {
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
    _delay_us(10); // Pequeño retardo para asegurarse del stop
}

void i2c_write(uint8_t data) {
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
}

void lcd_send_bits(uint8_t bits, uint8_t mode) {
    i2c_start();
    i2c_write(LCD_I2C_ADDR << 1);
    // Envía el nibble con backlight, modo y pulso de enable
    i2c_write(bits | mode | LCD_BACKLIGHT | LCD_ENABLE);
    _delay_us(1);
    i2c_write(bits | mode | LCD_BACKLIGHT);
    i2c_stop();
}

void lcd_send(uint8_t data, uint8_t mode) {
    uint8_t high_bits = data & 0xF0;
    uint8_t low_bits  = (data << 4) & 0xF0;
    
    lcd_send_bits(high_bits, mode);
    lcd_send_bits(low_bits, mode);
}

void lcd_cmd(uint8_t cmd) {
    lcd_send(cmd, 0);
    _delay_ms(2);
}

void lcd_data(uint8_t data) {
    lcd_send(data, 1);
    _delay_us(30);
}

void lcd_init(void) {
    i2c_init();
    _delay_ms(50);

    // Inicialización en modo 4 bits
    lcd_cmd(0x33);
    lcd_cmd(0x32);
    lcd_cmd(0x28);  // Modo: 4 bits, 2 líneas, 5x8 puntos
    lcd_cmd(0x0C);  // Display encendido, cursor apagado
    lcd_cmd(0x06);  // Modo de entrada: incremento automático
    lcd_cmd(0x01);  // Limpiar display
    _delay_ms(2);
}

void lcd_clear(void) {
    lcd_cmd(0x01);
    _delay_ms(2);
}

void lcd_print(const char *str) {
    while(*str) {
        lcd_data(*str++);
    }
}

void mostrarMensajeLCD(const char *mensaje) {
    char linea1[17] = {0};
    char linea2[17] = {0};
    
    // Extraer hasta 16 caracteres para la primera línea
    strncpy(linea1, mensaje, 16);
    // Extraer los siguientes 16 caracteres (si existen) para la segunda línea
    if(strlen(mensaje) > 16) {
        strncpy(linea2, mensaje + 16, 16);
    }
    
    // Posicionar el cursor en la primera línea y mostrarla
    lcd_cmd(0x80);  // Dirección inicio de la primera línea
    lcd_print(linea1);
    
    // Posicionar el cursor en la segunda línea y mostrarla
    lcd_cmd(0xC0);  // Dirección inicio de la segunda línea
    lcd_print(linea2);
}

void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (3 << UCSZ00);
}

void uart_transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

char uart_receive(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void uart_print(const char *str) {
    while(*str) {
        uart_transmit(*str++);
    }
}

void DHT_start(void) {
    DDRD |= (1 << DHT_PIN);    // Configura el pin como salida
    PORTD &= ~(1 << DHT_PIN);   // Envía nivel bajo
    _delay_ms(20);              // Mantener bajo al menos 18ms
    PORTD |= (1 << DHT_PIN);    // Envía nivel alto
    _delay_us(40);              // Espera 20-40us
}

uint8_t DHT_Respuesta(void) {
    DDRD &= ~(1 << DHT_PIN);  
    _delay_us(40);
    if (!(PIND & (1 << DHT_PIN))) {
        _delay_us(80);
        if (PIND & (1 << DHT_PIN)) {
            _delay_us(80);
            return 1;
        }
    }
    return 0;
}

void DHT_read(uint8_t ListaResultado[5]) {
    for (int j = 0; j < 5; j++) {
        uint8_t result = 0;
        for (int i = 0; i < 8; i++) {
            while (!(PIND & (1 << DHT_PIN))); // Espera inicio del pulso
            _delay_us(30);                    // Espera 30us para determinar el valor
            if (PIND & (1 << DHT_PIN)) {
                result |= (1 << (7 - i));     // Bit '1'
            }
            while (PIND & (1 << DHT_PIN));      // Espera que finalice el pulso
        }
        ListaResultado[j] = result;
    }
}

uint8_t read_dht11(uint8_t *temperature, uint8_t *humidity) {
    uint8_t data[5];
    DHT_start();
    if (!DHT_Respuesta())
        return 0;
    DHT_read(data);
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
        return 0;
    *humidity    = data[0]; // Humedad entera
    *temperature = data[2]; // Temperatura entera
    return 1;
}

void save_sample_to_eeprom(Muestra muestra){
    if(sample_index < MAX_SAMPLES){
        uint16_t addr = EEPROM_START + sample_index * sizeof(Muestra);
        eeprom_write_byte((uint8_t*)addr, muestra.temperatura);
        eeprom_write_byte((uint8_t*)(addr + 1), muestra.humedad);
        sample_index++;
    } else {
        sample_index = 0;
    }
}

void print_eeprom_data(){
    char Cadena[32];
    uart_print("\r\n-------------- Datos EEPROM --------------\r\n");
    sprintf(Cadena, "Tiempo de muestreo actual: %d\n", sample_time);
    uart_print(Cadena);
    for(uint16_t i = 0; i < Sample_counter; i++) {
        uint16_t addr = EEPROM_START + i * sizeof(Muestra);
        uint8_t temp = eeprom_read_byte((uint8_t*)addr);
        uint8_t hum  = eeprom_read_byte((uint8_t*)(addr + 1));
        sprintf(Cadena, "Muestra %d: Temperatura = %d, Humedad = %d\r\n", i+1, temp, hum);
        uart_print(Cadena);
    }
    uart_print("------------------------------------------\r\n");
}

void verificarTemp(uint8_t temperatura) {
    if(temperatura > 30) {
        PORTD |= (1 << FAN_PIN);
        PORTD |= (1 << BUZZER_PIN);
        
        lcd_clear();
        mostrarMensajeLCD("ALERTA:         Temperatura >30C");
        uart_print("Alerta: Temperatura > 30°C\r\n");
    } else {
        PORTD &= ~(1 << FAN_PIN);
        PORTD &= ~(1 << BUZZER_PIN);
    }
}

void cambiarTiempo(){
    uart_print("\nIngrese el nuevo tiempo de muestreo (0-9 seg): ");
    flush_input_buffer();
    char c = uart_receive(); // Se asume que el usuario ingresará un dígito
    if(c >= '0' && c <= '9'){
        sample_time = c - '0';
    }
    uart_print("\nNuevo tiempo de muestreo: ");
    uart_transmit(c);
    lcd_clear();
    mostrarMensajeLCD("Tiempo Muestreo Cambiado");
    _delay_ms(2000);
}

void flush_input_buffer(){
    // Descarta todos los datos que haya en el buffer UART
    while (UCSR0A & (1 << RXC0)) {
        (void) UDR0;
    }
}

void comprobarUart(){
    if(uart_available()){
        char cmd = uart_receive();
        if((cmd == 't') || (cmd == 'T')){
            cambiarTiempo();
        } else if((cmd == 'p') || (cmd == 'P')){
            print_eeprom_data();
        }
    }
    _delay_ms(100);
}

int main(void) {
    uint8_t temperatura = 0, humedad = 0;
    char Cadena[16];
    char cmd;
    Muestra s;
    
    lcd_init();
    uart_init(UBRR);
    lcd_clear();
    lcd_print("Bienvenido!");
    _delay_ms(2000);
    
    DDRD |= (1 << FAN_PIN) | (1 << BUZZER_PIN);
    
    // Mensaje inicial para configurar el tiempo de muestreo
    lcd_clear();
    mostrarMensajeLCD("Visualice la    Consola");
    uart_print("Pulse 'T' para configurar tiempo de muestreo.\r\n");
    uart_print("Si no hay respuesta en 10 seg, se usara 5 seg.\r\n");
    
    // Espera de 10 segundos para detectar comando 'T'
    uint16_t timer = 10000;  // 10000 ms (10s)
    uint16_t contador = 0;
    while(contador < timer){
        if(uart_available()){
            cmd = uart_receive();
            if(cmd == 'T' || cmd == 't'){
                flush_input_buffer();  // Descarta posibles \r o \n
                uart_print("Ingrese nuevo tiempo (0-9 seg): ");
                char c;
                while(1){
                    c = uart_receive();
                    if(c >= '0' && c <= '9'){
                        sample_time = c - '0';
                        uart_print("\nNuevo tiempo de muestreo: ");
                        uart_transmit(c);
                        break;
                    }
                }
                break;
            } else {
                uart_print("\nEntrada no válida. \nIntente de nuevo: ");
            }  
        }
        _delay_ms(100);
        contador += 100;
    }
    if(contador >= timer){
        sample_time = 5;
    }
    
    while(1){
        if(read_dht11(&temperatura, &humedad)){
            // Mostrar datos en LCD
            lcd_clear();
            lcd_cmd(0x80);
            sprintf(Cadena, "Temperatura: %dC", temperatura);
            lcd_print(Cadena);
            lcd_cmd(0xC0);
            sprintf(Cadena, "Humedad: %d%%", humedad);
            lcd_print(Cadena);
            _delay_ms(100);
            
            // Guardar muestra en EEPROM
            s.temperatura = temperatura;
            s.humedad    = humedad;
            save_sample_to_eeprom(s);
            if(Sample_counter < MAX_SAMPLES){
                Sample_counter++;
            }
            
            // Verificar si la temperatura supera 30°C
            verificarTemp(temperatura);
        }
        
        // Revisar comandos vía UART sin bloquear
        comprobarUart();
        
        for(uint16_t i = 0; i < sample_time; i++){
            _delay_ms(1000);
        }
    }
    return 0;
}
