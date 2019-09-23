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
volatile uint16_t pwm_reg;

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

void update_pwm_reg() {
    cli();
    pwm_reg = ((uint32_t)(TMP_LOW_CNT * 400) / (235 - temp.digit));
    sei();
}

void read_and_update() {
    therm_convert();
    while (!therm_read_temperature());
    update_pwm_reg();    
}

ISR(TIM0_COMPB_vect) {
    OCR0A = pwm_reg;
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

    /* Timer init
     * PWM, Phase & Freq. Correct, TOP = OCR0A, 1:8 prescaler
     * Counting up: Set OC0A/OC0B on compare match
     * Counting down: Clear OC0A/OC0B on compare match
     */
    TCCR0A = (1 << WGM00) | (1 << COM0A0) | (1 << COM0A1);
    TCCR0B = (1 << WGM03) | (1 << CS01);
    TCCR0C = 0;
    TIMSK0 = (1 << OCIE0B); // Enable OC0B interrupt (at BOTTOM)
}

int main(void) {
    init();    

    // Initial temp reading
    read_and_update();
    OCR0A = pwm_reg;
    OCR0B = TMP_LOW_CNT;
    
    PRR = (1 << PRTIM0); // Enable Timer0

    while (1) {
        read_and_update();
    }
}