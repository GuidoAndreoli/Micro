.include "m328pdef.inc"

.org 0x0000
    RJMP RESET
.org 0x001A               ; TIMER1_OVF vector
    RJMP TIMER1_OVF_ISR

; Tabla de valores para display de 7 segmentos (ánodo común)
SEGMENT_TABLE:
    .db 0b11000000 ; 0
    .db 0b11111001 ; 1
    .db 0b10100100 ; 2
    .db 0b10110000 ; 3
    .db 0b10011001 ; 4
    .db 0b10010010 ; 5
    .db 0b10000010 ; 6
    .db 0b11111000 ; 7
    .db 0b10000000 ; 8
    .db 0b10010000 ; 9

; ========================
RESET:
    ; Limpiar R1 para usarlo como __zero_reg__
    CLR R1

    ; --- Configurar display en PORTD como salida ---
    LDI R16, 0xFF
    OUT DDRD, R16

    ; --- Configurar PB3 (OC2A) como salida para 1kHz ---
    SBI DDRB, PB3

    ; --- Inicializar índice del display ---
    LDI R17, 0

    ; --- Timer1: configurar para interrupción por overflow (~1s) ---
    LDI R18, LOW(0xC30F)
    STS TCNT1L, R18
    LDI R18, HIGH(0xC30F)
    STS TCNT1H, R18

    ; Habilitar interrupción por overflow
    LDI R16, (1 << TOIE1)
    STS TIMSK1, R16

    ; Prescaler = 1024
    LDI R16, (1 << CS12) | (1 << CS10)
    STS TCCR1B, R16

    ; --- Timer2: configurar para generar 1 kHz en PB3 ---
    ; Modo CTC, toggle OC2A
    LDI R16, (1 << COM2A0) | (1 << WGM21)
    STS TCCR2A, R16
    LDI R16, (1 << CS22)               ; prescaler = 64
    STS TCCR2B, R16
    LDI R16, 124
    STS OCR2A, R16

    ; --- Habilitar interrupciones globales ---
    SEI

LOOP:
    RJMP LOOP

; ========================
TIMER1_OVF_ISR:
    ; Mostrar número actual
    LDI ZH, HIGH(SEGMENT_TABLE << 1)
    LDI ZL, LOW(SEGMENT_TABLE << 1)
    ADD ZL, R17
    ADC ZH, R1              ; usar R1 como zero_reg
    LPM R18, Z
    OUT PORTD, R18

    ; Incrementar índice y reiniciar si llega a 10
    INC R17
    CPI R17, 20
    BRLO CONT
    LDI R17, 0
CONT:
    ; Reiniciar TCNT1 para overflow cada ~1s
    LDI R18, LOW(0xC30F)
    STS TCNT1L, R18
    LDI R18, HIGH(0xC30F)
    STS TCNT1H, R18

    RETI

