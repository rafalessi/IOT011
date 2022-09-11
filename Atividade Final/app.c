#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/* Lib for isaplha and isdigit */
#include <ctype.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Local includes. */
#include "console.h"

#define TASK1_PRIORITY 0
#define TASK2_PRIORITY 0
#define TASK3_PRIORITY 3
#define TASK4_PRIORITY 1
#define TASK5_PRIORITY 4
#define TASK6_PRIORITY 5
#define TASK7_PRIORITY 6

#define BLACK "\033[30m" /* Black */
#define RED "\033[31m"   /* Red */
#define GREEN "\033[32m" /* Green */
#define BLUE "\033[34m" /* Green */
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
    500};
st_led_param_t red = {
    13,
    RED,
    1000};

st_led_param_t blue = {
    10,
    BLUE,
    200};

TaskHandle_t greenTask_hdlr, redTask_hdlr, led_hdlr;

QueueHandle_t structQueue = NULL; // Queue for receiving characters from user
QueueHandle_t structQueue_translation = NULL; // Queue for translation character
QueueHandle_t structQueue_translated = NULL; // Queue for translated character
QueueHandle_t structQueue_led_blinker = NULL; // Queue for translated character

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
        
        if (key > 0)
        {   // Filter not Alphabet and numeric characters to avoid mistranslantion
            if (key == '\n') // Enter key to conclude input of user
            {
                stop = 1;
            }   
            else if (!isdigit(key) && !isalpha(key) && key != ' '){ // Filter Space bar key and non alphabet and digit inputs
                gotoxy(1,6);
                printf("Only use Alphanumeric Characters and spaces!! Please start again. \n");
                vTaskResume(redTask_hdlr);
                stop = 1;
            }
            if (xQueueSend(structQueue, &key, 0) != pdTRUE)
            {
                
            }
            if (xQueueSend(structQueue_translation, &key, 0) != pdTRUE)
            {
                //
            }
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

static void prvTask_led_morse(void *pvParameters)
{
    uint32_t notificationValue;

    // pvParameters contains LED params
    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();

    const int CURSOR_Y3 = 6;
    int cursor_x3 = 0;

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
                vTaskDelay(100 / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);

                gotoxy(led->pos, 2);
                printf("%s ", BLACK);
                fflush(stdout);
                vTaskDelay(200 / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);

            }
            if (notificationValue == 2)
            {
                gotoxy(led->pos, 2);
                printf("%s⬤", led->color);
                fflush(stdout);
                vTaskDelay(600 / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);

                gotoxy(led->pos, 2);
                printf("%s ", BLACK);
                fflush(stdout);
                vTaskDelay(200 / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);
                
            }
            if (notificationValue == 3)
            {

                gotoxy(led->pos, 2);
                printf("%s ", BLACK);
                fflush(stdout);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                // vTaskDelayUntil(&xLastWakeTime, led->period_ms / portTICK_PERIOD_MS);
            }
        }        
    }
}

static void prvTask_translator (void *pvParameters)
{
    (void)pvParameters;
    char key;

    int position; // integer for position in alphabet
    char character; // temporary variable for alphabet lookup in upper case
    char *morse_character;

    int k=0;
    int l=0;

    // Array with alphabet in Morse Code
    char *morse_alphabet[] = {
        ".-",  // a
        "-...",  // b
        "-.-.",  // c
        "-..",  // d
        ".",  // e
        "..-.",  // f
        "--.",  // g
        "....",  // h
        "..",  // i
        ".---",  // j
        "-.-",  // k
        ".-..",  // l
        "--",  // m
        "-.",  // n
        "---",  // o
        ".--.",  // p
        "--.-",  // q
        ".-.",  // r
        "...",  // s
        "-",  // t
        "..-",  // u
        "...-",  // v
        ".--",  // w
        "-..-",  // x
        "-.--",  // y
        "--.." };  // z
    char *morse_digit[] = {
        "-----", // 0
        ".----",// 1
        "..---", // 2
        "...--", // 3
        "....-",// 4
        ".....", // 5
        "-....", // 6
        "--...", // 7
        "---..", // 8
        "----."}; // 9

    for (;;)
    {
        if (xQueueReceive(structQueue_translation, &key, portMAX_DELAY) == pdPASS)
            {
                // Start checking if is alphabet to find location in array
                if( isalpha(key))
                {
                    character = toupper(key); // as user could inserted capital letter and it can be read as different character, set to Capital all character.
                    position = character - 'A'; // Find position in alphabet      
                    morse_character = morse_alphabet[position];
                }
                // Then it's a digit or space, because bad input was already filtered before in logger
                else if (isdigit(key))
                {
                    position = key - '0'; // Find position in numbers     
                    morse_character = morse_digit[position];
                } 
                else // It is a space bar. Consider it as a long pause. But it still not working properly
                    morse_character = ' ';
            }
        ;
        // Routine to send each char in the buffer. But 
        for (int i=0; i< strlen(morse_character); i++){
            char *character = morse_character[i];
            if (xQueueSend(structQueue_led_blinker, &character, 0) != pdTRUE)
            {
                gotoxy(1,6);
                printf("Error. Queue to LED not transmitted. Part of the character %s not sent", morse_character); // Still trying to understand why not every queue is sent to LED Blinker..
                // Probably is the queue that has a overflow, because it happens after 10 characters
            }
            if (xQueueSend(structQueue_translated, &character, 0) != pdTRUE)
            {
                //
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
         
         vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void prvTask_logger(void *pvParameters)
{
    (void)pvParameters;
    char key;
    char morse_character;

    const int CURSOR_Y = 4;
    int cursor_x = 0;
    for (;;)
    {
        if (xQueueReceive(structQueue, &key, portMAX_DELAY) == pdPASS)
        {
            gotoxy(cursor_x++, CURSOR_Y);
            printf("%c", key);
            fflush(stdout);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void prvTask_coder(void *pvParameters)
{
    (void)pvParameters;
    char key;
    char morse_character;

    const int CURSOR_Y2 = 5;
    int cursor_x2 = 0;
    
    for (;;)
    {
        if (xQueueReceive(structQueue_translated, &morse_character, portMAX_DELAY) == pdPASS)
        {
            gotoxy(cursor_x2++, CURSOR_Y2);
            printf("%c", morse_character);
            fflush(stdout);

            if (morse_character == '.'){
                xTaskNotify(led_hdlr,01UL, eSetValueWithOverwrite);
            }
            else if (morse_character == '-'){
                xTaskNotify(led_hdlr,02UL, eSetValueWithOverwrite);
            }
            else {
                xTaskNotify(led_hdlr,03UL, eSetValueWithOverwrite); 
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void app_run(void)
{
    structQueue = xQueueCreate(100, // Queue length
                               1); // Queue item size

    if (structQueue == NULL)
    {
        printf("Fail on create queue\n");
        exit(1);
    }

    structQueue_translation = xQueueCreate(100, // Queue length
                                           1); // Queue item size. Check if it works with more than one character

    if (structQueue_translation == NULL)
    {
        printf("Fail on create queue\n");
        exit(1);
    }

    structQueue_translated = xQueueCreate(100, // Queue length
                                           1); // Queue item size. Check if it works with more than one character

    if (structQueue_translated == NULL)
    {
        printf("Fail on create queue\n");
        exit(1);
    }

    structQueue_led_blinker = xQueueCreate(100, // Queue length
                                           1); // Queue item size. Check if it works with more than one character

    if (structQueue_led_blinker  == NULL)
    {
        printf("Fail on create queue\n");
        exit(1);
    }

    clear();
    DISABLE_CURSOR();
    printf(
        "╔═══Morse Code════╗\n"
        "║                 ║\n"
        "╚═══Type slowly═══╝\n");

    xTaskCreate(prvTask_logger, "logger", configMINIMAL_STACK_SIZE, NULL, TASK4_PRIORITY, NULL);
    xTaskCreate(prvTask_coder, "coder", configMINIMAL_STACK_SIZE, NULL, TASK4_PRIORITY, NULL);
    xTaskCreate(prvTask_translator, "translator", configMINIMAL_STACK_SIZE, NULL, TASK5_PRIORITY, NULL);
    xTaskCreate(prvTask_led_morse, "LED", configMINIMAL_STACK_SIZE, &blue, TASK6_PRIORITY, &led_hdlr);
    xTaskCreate(prvTask_getChar, "Get_key", configMINIMAL_STACK_SIZE, NULL, TASK7_PRIORITY, NULL);

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