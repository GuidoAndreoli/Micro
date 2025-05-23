.include "m328pdef.inc"

.org 0x0000
    RJMP Inicio
.org INT0addr
    RJMP RSI_0

Inicio:
    SEI                  ; Habilitamos interrupciones globales

    ; Configurar puerto B como salida (LEDs)
    LDI R16, 0xFF
    OUT DDRB, R16

    ; Activar pull-up en PD2 (INT0)
    SBI PORTD, 2

    ; Inicializar pila
    LDI R17, HIGH(RAMEND)
    OUT SPH, R17
    LDI R17, LOW(RAMEND)
    OUT SPL, R17

    ; Habilitar solo INT0
    LDI R18, 0x01
    OUT EIMSK, R18

    ; Configurar INT0 para flanco de subida (puedes cambiar a 0x02 para bajada)
    LDI R19, 0x01
    STS EICRA, R19

    ; Inicializar contador
    LDI R20, 0x00

Loop:
    RJMP Loop

; Rutina de interrupción INT0: contar hasta 3 pulsos, luego encender LED en PB0
RSI_0:
    INC R20            ; Incrementar contador
    CPI R20, 3
    BRNE Salir_0
    SBI PORTB, 0       ; Encender LED en PB0
Salir_0:
    RCALL Mseg
    RETI

; Subrutina de retardo (anti-rebote)
Mseg:
    LDI r21, 21
    LDI r22, 75
    LDI r23, 189
L1:
    DEC r23
    BRNE L1
    DEC r22
    BRNE L1
    DEC r21
    BRNE L1
    NOP
    RET

