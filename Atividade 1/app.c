#include <stdio.h>
#include <pthread.h>
#include <stdlib.h> 

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Local includes. */
#include "console.h"

#define TASK1_PRIORITY 0
#define TASK2_PRIORITY 0
#define TASK3_PRIORITY 0
#define TASK4_PRIORITY 1

#define BLACK "\033[30m" /* Black */
#define RED "\033[31m"   /* Red */
#define GREEN "\033[32m" /* Green */
#define DISABLE_CURSOR() printf("\e[?25l")
#define ENABLE_CURSOR() printf("\e[?25h")

#define clear() printf("\033[H\033[J")
#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))

typedef struct
{
    int pos;
    char *color;
    int period_ms;
} st_led_param_t;

st_led_param_t green = {
    6,
    GREEN,
    250};
st_led_param_t red = {
    13,
    RED,
    100};



TaskHandle_t greenTask_blinking_hdlr, greenTask_off_hdlr, redTask_hdlr,redTask_blink_hdlr, redTask_off_hdlr, redTask_on_hdlr;

#include <termios.h>

static void prvTask_getChar(void *pvParameters)
{
    char key;
    int n;

    /* I need to change  the keyboard behavior to
    enable nonblock getchar */
    struct termios initial_settings,
        new_settings;

    tcgetattr(0, &initial_settings);

    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 1;

    tcsetattr(0, TCSANOW, &new_settings);
    /* End of keyboard configuration */
    for (;;)
    {
        int stop = 0;
        key = getchar();

        switch (key)
        {
        case '+':
            vTaskResume(greenTask_blinking_hdlr);
            xTaskNotify(redTask_off_hdlr,01UL, eSetValueWithOverwrite);
            break;
        case '1':
            xTaskNotify(redTask_blink_hdlr,01UL, eSetValueWithOverwrite);
            break;
        case '*':
            xTaskNotify(redTask_on_hdlr,01UL, eSetValueWithOverwrite);
            vTaskSuspend(greenTask_blinking_hdlr);
            xTaskNotify(greenTask_off_hdlr,01UL, eSetValueWithOverwrite);
            break;
        case 'k':
            stop = 1;
            break;
        }
        if (stop)
        {
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    tcsetattr(0, TCSANOW, &initial_settings);
    ENABLE_CURSOR();
    exit(0);
    vTaskDelete(NULL);
}
static void prvTask_led_blink(void *pvParameters)
{
    uint32_t notificationValue;

    // pvParameters contains LED params
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {  
        if (xTaskNotifyWait(
                ULONG_MAX,
                ULONG_MAX,
                &notificationValue,
                portMAX_DELAY))
        {
            if (notificationValue == 1)
            {
                gotoxy(led->pos, 2);
                printf("%s⬤", led->color);
                fflush(stdout);
                vTaskDelay(led->period_ms / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);

                gotoxy(led->pos, 2);
                printf("%s ", BLACK);
                fflush(stdout);
                vTaskDelay(led->period_ms / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);
            }
        }        
    }

}

static void prvTask_led_on(void *pvParameters)
{
    uint32_t notificationValue;

    // pvParameters contains LED params
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {  
        if (xTaskNotifyWait(
                ULONG_MAX,
                ULONG_MAX,
                &notificationValue,
                portMAX_DELAY))
        {
            if (notificationValue == 1)
            {
                gotoxy(led->pos, 2);
                printf("%s⬤", led->color);
                fflush(stdout);
                vTaskDelay(led->period_ms / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);
            }
        }        
    }

}

static void prvTask_led_off(void *pvParameters)
{
    uint32_t notificationValue;

    // pvParameters contains LED params
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {  
        if (xTaskNotifyWait(
                ULONG_MAX,
                ULONG_MAX,
                &notificationValue,
                portMAX_DELAY))
        {
            if (notificationValue == 1)
            {
                gotoxy(led->pos, 2);
                printf("%s ", BLACK);
                fflush(stdout);
                vTaskDelay(led->period_ms / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);
            }
        }        
    }

}

static void prvTask_led_blinking(void *pvParameters)
{
    // pvParameters contains LED params
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        // console_print("@");
        gotoxy(led->pos, 2);
        printf("%s⬤", led->color);
        fflush(stdout);
        vTaskDelay(led->period_ms / portTICK_PERIOD_MS);
        // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);
        
        gotoxy(led->pos, 2);
        printf("%s ", BLACK);
        fflush(stdout);
        vTaskDelay(led->period_ms / portTICK_PERIOD_MS);
        // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}


void app_run(void)
{

    clear();
    DISABLE_CURSOR();
    printf(
        "╔═════════════════╗\n"
        "║                 ║\n"
        "╚═════════════════╝\n");

    xTaskCreate(prvTask_led_blinking, "LED_green_blinking", configMINIMAL_STACK_SIZE, &green, TASK1_PRIORITY, &greenTask_blinking_hdlr);
    xTaskCreate(prvTask_led_off, "LED_green_off", configMINIMAL_STACK_SIZE, &green, TASK3_PRIORITY, &greenTask_off_hdlr);
    xTaskCreate(prvTask_led_off, "LED_red_off", configMINIMAL_STACK_SIZE, &red, TASK3_PRIORITY, &redTask_off_hdlr);
    //xTaskCreate(prvTask_led_blinking, "LED_red", configMINIMAL_STACK_SIZE, &red, TASK2_PRIORITY, &redTask_hdlr); 
    xTaskCreate(prvTask_led_blink, "LED_red_blink", configMINIMAL_STACK_SIZE, &red, TASK3_PRIORITY, &redTask_blink_hdlr);
    xTaskCreate(prvTask_led_on, "LED_red_on", configMINIMAL_STACK_SIZE, &red, TASK3_PRIORITY, &redTask_on_hdlr);
    
    xTaskCreate(prvTask_getChar, "Get_key", configMINIMAL_STACK_SIZE, NULL, TASK4_PRIORITY, NULL);


    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following
     * line will never be reached.  If the following line does execute, then
     * there was insufficient FreeRTOS heap memory available for the idle and/or
     * timer tasks      to be created.  See the memory management section on the
     * FreeRTOS web site for more details. */
    for (;;)
    {
    }
}