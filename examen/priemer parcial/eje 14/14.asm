.include "m328pdef.inc"

.org 0x0000
    RJMP Start

.def temp = R16

Start:
    ; Configurar Stack Pointer
    LDI temp, HIGH(RAMEND)
    OUT SPH, temp
    LDI temp, LOW(RAMEND)
    OUT SPL, temp

    ; Configurar PORTB como salida
    LDI temp, 0xFF
    OUT DDRB, temp

Loop:
    ; Aquí va la rutina RSI_0
    LDI R20, 0x01
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x02
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x03
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x04
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x05
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x06
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x07
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x08
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x08
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x07
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x05
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x04
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x02
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x01
    OUT PORTB, R20
    RCALL Delay

    LDI R20, 0x00
    OUT PORTB, R20
    RCALL Delay

    RJMP Loop

; ---------------------------
; Subrutina Delay básica
Delay:
    LDI R21, 50
D1: LDI R22, 200
D2: NOP
    DEC R22
    BRNE D2
    DEC R21
    BRNE D1
    RET

