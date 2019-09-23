/*
 * File:   ds18b20-tmp03f-conv.c
 * Author: nstorm
 */

#define F_CPU 8000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "ds18b20-tmp03f-conv.h"

// Global temp
temp_t temp;

// сброс датчика
uint8_t therm_reset() {
    uint8_t i;
    // опускаем ногу вниз на 480uS
    THERM_LOW();
    THERM_OUTPUT_MODE();
    _delay_us(480); // замените функцию задержки на свою
    // подымаем линию на 60uS
    THERM_INPUT_MODE();
    _delay_us(60);
    // получаем значение на линии в период 480uS
    i = (THERM_PIN & (1 << THERM_DQ));
    _delay_us(420);
    // возвращаем значение (0=OK, 1=датчик не найден)
    return i;
}


// функция отправки бита
void therm_write_bit(uint8_t bit) {
    // опускаем на 1uS
    THERM_LOW();
    THERM_OUTPUT_MODE();
    _delay_us(1);
    // если хотим отправить 1, поднимаем линию (если нет, оставляем как есть)
    if (bit) THERM_INPUT_MODE();
    // ждем 60uS и поднимаем линию
    _delay_us(60);
    THERM_INPUT_MODE();
}

// чтение бита
uint8_t therm_read_bit(void) {
    uint8_t bit = 0;
    // опускаем на 1uS
    THERM_LOW();
    THERM_OUTPUT_MODE();
    _delay_us(1);
    // поднимаем на 14uS
    THERM_INPUT_MODE();
    _delay_us(14);
    // читаем состояние
    if (THERM_PIN & (1 << THERM_DQ)) bit = 1;
    // ждем 45 мкс и возвращаем значение
    _delay_us(45);
    return bit;
}

// читаем байт
uint8_t therm_read_byte(void) {
    uint8_t i = 8, n = 0;
    while (i--) {
        // сдвигаем в право на 1 и сохраняем следующее значение
        n >>= 1;
        n |= (therm_read_bit() << 7);
    }
    return n;
}

// отправляем байт
void therm_write_byte(uint8_t byte) {
    uint8_t i = 8;
    while (i--) {
        // отправляем бит и сдвигаем вправо на 1
        therm_write_bit(byte & 1);
        byte >>= 1;
    }
}

void therm_convert() {
    therm_reset();
    therm_write_byte(THERM_CMD_SKIPROM);
    therm_write_byte(THERM_CMD_CONVERTTEMP);
}

// читаем температуру с датчика
bool therm_read_temperature() {
    uint8_t temperature[2];

    if (!therm_read_bit())
        return false;

    therm_reset();
    therm_write_byte(THERM_CMD_SKIPROM);
    therm_write_byte(THERM_CMD_RSCRATCHPAD);

    temperature[0] = therm_read_byte();
    temperature[1] = therm_read_byte();
    therm_reset();

    temp.digit = temperature[0] >> 4;
    temp.digit |= (temperature[1] & 0x7) << 4;

    temp.decimal = temperature[0] & 0xF;
    temp.decimal *= THERM_DECIMAL_STEPS_12BIT;
    
    return true;
}

void init() {
	cli(); // Disable interrupts
	CCP = 0xD8; // Magic number to enable configuration access
	CLKMSR = 0b00; // Internal osc
	_NOP();
    _NOP();
    _NOP();
	CCP = 0xD8;
	CLKPSR = 0; // Prescaler 1:1 = 8 MHz system clock
	ACSR |= (1 << 7); // ACD (ACD: Analog Comparator Disable = 1)
	PRR = 0b11; // Power Reduction Register
	sei(); // Enable interrupts

	THERM_DDR = 0xFF;

	// PCICR = (1 << PCIE0); // Enable port change interrupt
	// PCMSK = (1 << PCINT0) | (1 << PCINT1); // Enable interrupt on PB0, PB1 change
	// EICRA |= (1 << ISC01) | (0 << ISC00); // The falling edge of INT0 generates an interrupt request.
	// EIMSK |= (1 << INT0); // Enable INT0
}

int main(void) {
    init();    

    // Initial temp reading
    therm_convert();
    while (!therm_read_temperature());

    while (1) {
    }
}