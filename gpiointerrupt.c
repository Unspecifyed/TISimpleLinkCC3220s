#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/UART.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* Global Variables */
static uint8_t temperature = 25;  // Initial temperature

/* Function Prototypes */
void gpioButtonFxn0(uint_least8_t index);
void gpioButtonFxn1(uint_least8_t index);
void timerCallback(Timer_Handle myHandle, int_fast16_t status);
void initTimer(void);
void initI2C(void);
void readTemperature(I2C_Handle i2cHandle);
void initUART(void);
void sendDataToServer(UART_Handle uartHandle, const char *data);

/* Timer Interval in microseconds */
#define TIMER_INTERVAL     1000000  // 1 second

/* I2C Configuration */
#define I2C_SLAVE_ADDR      0x48  // I2C slave address of temperature sensor
#define TEMP_REG_ADDR       0x00  // Temperature register address

/* UART Configuration */
#define UART_BAUD_RATE      115200

/* Task Scheduler Interval */
#define TASK_INTERVAL       5000000  // 5 seconds

/*
 * ========================= Task Scheduler Documentation =========================
 *
 * === Algorithm Description ===
 * The task scheduler is implemented as a simple time-based scheduler that executes 
 * tasks at predefined intervals. The scheduler uses the `Task_sleep()` function to 
 * delay the execution of tasks.
 *
 * === Inputs ===
 * - TASK_INTERVAL: The time interval between successive executions of the scheduler, 
 *   defined in microseconds.
 *
 * === Outputs ===
 * - Periodic execution of tasks based on the predefined interval.
 *
 * === Expected Results ===
 * The scheduler should ensure that tasks are executed at regular intervals as defined 
 * by `TASK_INTERVAL`.
 *
 * =============================== Code Documentation ===============================
 *
 * ======== gpioButtonFxn0 ========
 */
void gpioButtonFxn0(uint_least8_t index)
{
    /* Increase temperature */
    if (temperature < 50)
    {
        temperature++;
    }
}

/*
 * ======== gpioButtonFxn1 ========
 */
void gpioButtonFxn1(uint_least8_t index)
{
    /* Decrease temperature */
    if (temperature > 10)
    {
        temperature--;
    }
}

/*
 * ======== timerCallback ========
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    readTemperature(NULL);  // Read temperature
    sendDataToServer(NULL, "Temperature is %d°C\n", temperature);  // Send temperature via UART
}

/*
 * ======== initTimer ========
 */
void initTimer(void)
{
    Timer_Handle timer;
    Timer_Params params;

    Timer_init();
    Timer_Params_init(&params);
    params.period = TIMER_INTERVAL;
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    timer = Timer_open(CONFIG_TIMER_0, &params);

    if (timer == NULL) {
        /* Failed to initialize timer */
        while (1) {}
    }

    if (Timer_start(timer) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }
}

/*
 * ======== initI2C ========
 */
void initI2C(void)
{
    I2C_init();
}

/*
 * ======== readTemperature ========
 */
void readTemperature(I2C_Handle i2cHandle)
{
    uint8_t data;
    I2C_Transaction i2cTransaction;

    i2cTransaction.slaveAddress = I2C_SLAVE_ADDR;
    i2cTransaction.writeBuf = &TEMP_REG_ADDR;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = &data;
    i2cTransaction.readCount = 1;

    if (I2C_transfer(i2cHandle, &i2cTransaction))
    {
        /* Convert the data to temperature */
        temperature = (int8_t)data;
    }
}

/*
 * ======== initUART ========
 */
void initUART(void)
{
    UART_init();
}

/*
 * ======== sendDataToServer ========
 */
void sendDataToServer(UART_Handle uartHandle, const char *data)
{
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.baudRate = UART_BAUD_RATE;
    uartParams.writeDataMode = UART_DATA_BINARY;

    if (uartHandle == NULL)
    {
        uartHandle = UART_open(CONFIG_UART_0, &uartParams);
    }

    if (uartHandle != NULL)
    {
        UART_write(uartHandle, data, strlen(data));
        UART_close(uartHandle);
    }
}

/*
 * ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    I2C_Handle i2cHandle = NULL;
    UART_Handle uartHandle = NULL;

    /* Call driver init functions */
    GPIO_init();
    I2C_init();
    UART_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /* Configure BUTTON1 pin */
    GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_1);

    /* Initialize and start the timer */
    initTimer();

    while (1)
    {
        /* Initialize I2C if not done */
        if (i2cHandle == NULL)
        {
            i2cHandle = I2C_open(CONFIG_I2C_0, NULL);
        }

        /* Initialize UART if not done */
        if (uartHandle == NULL)
        {
            uartHandle = UART_open(CONFIG_UART_0, NULL);
        }

        /* Task Scheduler */
        Task_sleep(TASK_INTERVAL);
    }
}
