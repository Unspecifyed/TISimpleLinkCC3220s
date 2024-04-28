/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* Morse code definitions */
#define DOT_DELAY       500000  // 500ms delay for a dot
#define DASH_DELAY      1500000 // 1500ms delay for a dash
#define CHAR_DELAY      3000000 // 3000ms delay between characters
#define WORD_DELAY      6000000 // 6000ms delay between words

typedef enum {
    SOS_S,
    SOS_O,
    SOS_S2,
    OK_O,
    OK_K
} MorseState;

MorseState currentState = SOS_S;
bool isSystemOn = true;

void gpioButtonFxn0(uint_least8_t index)
{
    isSystemOn = !isSystemOn;  // Toggle system state
}

void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    if (!isSystemOn) {
        // Turn off LEDs and reset state
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
        GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
        currentState = SOS_S;
        return;
    }

    static int dotDashCount = 0;
    static int charCount = 0;
    static int wordCount = 0;

    switch (currentState) {
        case SOS_S:
        case SOS_S2:
            if (dotDashCount < 3) {
                GPIO_toggle(CONFIG_GPIO_LED_0);  // Toggle green LED
                Timer_setPeriod(myHandle, DOT_DELAY);
                dotDashCount++;
            } else {
                dotDashCount = 0;
                charCount++;
                if (charCount < 3) {
                    Timer_setPeriod(myHandle, CHAR_DELAY);
                } else {
                    charCount = 0;
                    wordCount++;
                    if (wordCount < 2) {
                        Timer_setPeriod(myHandle, WORD_DELAY);
                    } else {
                        wordCount = 0;
                        if (currentState == SOS_S) {
                            currentState = SOS_O;
                        } else {
                            currentState = SOS_S2;
                        }
                    }
                }
            }
            break;

        case SOS_O:
            if (dotDashCount < 3) {
                GPIO_toggle(CONFIG_GPIO_LED_1);  // Toggle red LED
                Timer_setPeriod(myHandle, DASH_DELAY);
                dotDashCount++;
            } else {
                dotDashCount = 0;
                charCount++;
                if (charCount < 3) {
                    Timer_setPeriod(myHandle, CHAR_DELAY);
                } else {
                    charCount = 0;
                    wordCount++;
                    if (wordCount < 2) {
                        Timer_setPeriod(myHandle, WORD_DELAY);
                    } else {
                        wordCount = 0;
                        currentState = OK_O;
                    }
                }
            }
            break;

        case OK_O:
            if (dotDashCount < 3) {
                GPIO_toggle(CONFIG_GPIO_LED_0);  // Toggle green LED
                GPIO_toggle(CONFIG_GPIO_LED_1);  // Toggle red LED
                Timer_setPeriod(myHandle, DOT_DELAY);
                dotDashCount++;
            } else {
                dotDashCount = 0;
                charCount++;
                if (charCount < 2) {
                    Timer_setPeriod(myHandle, CHAR_DELAY);
                } else {
                    charCount = 0;
                    wordCount++;
                    if (wordCount < 1) {
                        Timer_setPeriod(myHandle, WORD_DELAY);
                    } else {
                        wordCount = 0;
                        currentState = OK_K;
                    }
                }
            }
            break;

        case OK_K:
            if (dotDashCount < 3) {
                GPIO_toggle(CONFIG_GPIO_LED_0);  // Toggle green LED
                GPIO_toggle(CONFIG_GPIO_LED_1);  // Toggle red LED
                Timer_setPeriod(myHandle, DOT_DELAY);
                dotDashCount++;
            } else {
                dotDashCount = 0;
                charCount++;
                if (charCount < 3) {
                    Timer_setPeriod(myHandle, CHAR_DELAY);
                } else {
                    charCount = 0;
                    wordCount++;
                    if (wordCount < 2) {
                        Timer_setPeriod(myHandle, WORD_DELAY);
                    } else {
                        wordCount = 0;
                        currentState = SOS_S;
                    }
                }
            }
            break;
    }
}

void initTimer(void)
{
    Timer_Handle timer0;
    Timer_Params params;

    Timer_init();
    Timer_Params_init(&params);
    params.period = DOT_DELAY;  // Initial delay for a dot
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;
    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer0 == NULL) {
        /* Failed to initialize timer */
        while (1) {}
    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }
}

void *mainThread(void *arg0)
{
    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /* Initialize and start the timer */
    initTimer();

    return (NULL);
}
