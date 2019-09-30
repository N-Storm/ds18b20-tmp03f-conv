/* 
 * File:   ds18b20-tmp03f-conv.h
 * Author: nstorm
 */

#include <stdint.h>

#ifndef DS18B20_TMP03F_CONV_H
#define	DS18B20_TMP03F_CONV_H

/* Определяем куда подключен датчик */
#define THERM_PORT PORTB
#define THERM_DDR DDRB
#define THERM_PIN PINB
#define THERM_DQ PB2
/* Макросы для "дергания ноги" и изменения режима ввода/вывода */
#define THERM_INPUT_MODE() THERM_DDR &= ~(1 << THERM_DQ)
#define THERM_OUTPUT_MODE() THERM_DDR |= (1 << THERM_DQ)
#define THERM_LOW() THERM_PORT &= ~(1 << THERM_DQ)
#define THERM_HIGH() THERM_PORT |= (1 << THERM_DQ)

// команды управления датчиком
#define THERM_CMD_CONVERTTEMP 0x44
#define THERM_CMD_RSCRATCHPAD 0xBE
#define THERM_CMD_WSCRATCHPAD 0x4E
#define THERM_CMD_CPYSCRATCHPAD 0x48
#define THERM_CMD_RECEEPROM 0xB8
#define THERM_CMD_RPWRSUPPLY 0xB4
#define THERM_CMD_SEARCHROM 0xF0
#define THERM_CMD_READROM 0x33
#define THERM_CMD_MATCHROM 0x55
#define THERM_CMD_SKIPROM 0xCC
#define THERM_CMD_ALARMSEARCH 0xEC

#define THERM_DECIMAL_STEPS_12BIT 625 //.0625

#define TMP_LOW_US 12000
#define TMP_LOW_CNT (uint16_t)(TMP_LOW_US/2)

typedef struct {
    int8_t digit;
    uint16_t decimal;    
} temp_t;

#endif	/* DS18B20_TMP03F_CONV_H */

