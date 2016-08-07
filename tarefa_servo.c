//*****************************************************************************
// tarefa_servo.c - Tarefa para mover o servo
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "tarefa_servo.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//*****************************************************************************
// Inclusao das bibliotecas pro PWM.
//*****************************************************************************
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"

//*****************************************************************************
// Frequencia base de 55HZ escolhida para controlar o servo.
//*****************************************************************************
#define PWM_FREQUENCY 55

//*****************************************************************************
// Tamanho da pilha para tarefa SERVO.
//*****************************************************************************
#define TAMANHOPILHASERVO	128         // Tamanho da pilha em palavras.

//*****************************************************************************
// Tamanho do item e da pilha para a mensagem SERVO na fila.
//*****************************************************************************
#define TAMANHO_ITEM_SERVO	sizeof(uint8_t)
#define TAMANHO_FILA_SERVO	5

//*****************************************************************************
// Fila que mantem as mensagens enviadas para tarefa SERVO.
//*****************************************************************************
xQueueHandle g_FilaServo;

extern xSemaphoreHandle g_pUARTSemaphore;

//*****************************************************************************
//
// Esta tarefa faz o ponteiro do servo se movimentar para a esquerda ou direita.
// O usuario seleciona o lado a partir dos botoes SW1 ou SW2
//
//*****************************************************************************
static void
TarefaServo(void *pvParameters)
{
	// Variaveis para programar PWM (Tipo volateis: compilador nao elimina-as)
	volatile uint32_t ui32Load;
	volatile uint32_t ui32PWMClock;
	volatile uint8_t ui8Adjust;

	ui8Adjust = 83; //83: posicao central pra criar um pulso de 1.5mS do PWM.

    uint8_t i8Message;

    ui32PWMClock = SysCtlClockGet() / 64;	//	divisor 64 roda o clock a 625kHz.
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load);

    ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, ui8Adjust * ui32Load / 1000);
    ROM_PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_0);

    while(1)
    {
        if(xQueueReceive(g_FilaServo, &i8Message, 0) == pdPASS)
        {
            if(i8Message == LEFT_BUTTON)
            {
            	//Movimento sendo definido PWM
            	ui8Adjust += 15;
            	if (ui8Adjust > 155) ui8Adjust = 155;

            	//	Ajuste do tamanho do pulso PWM
            	ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, ui8Adjust * ui32Load / 1000);

            	xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            	UARTprintf("Servo moveu-se para a posicao %d.\n\n", ui8Adjust);
            	xSemaphoreGive(g_pUARTSemaphore);
            }

            if(i8Message == RIGHT_BUTTON)
            {
            	//	Movimento sendo definido PWM
            	ui8Adjust -= 15;
            	if (ui8Adjust < 40) ui8Adjust = 40;
            	//	Ajuste do tamanho do pulso PWM
            	ROM_PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, ui8Adjust * ui32Load / 1000);
                //
                // Proteger o MUTEX do UART
            	xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            	UARTprintf("Servo moveu-se para a posicao %d.\n\n", ui8Adjust);
            	xSemaphoreGive(g_pUARTSemaphore);
            }
        }

    }
}

//*****************************************************************************
//
// Initializes the LED task.
//
//*****************************************************************************
uint32_t
InicTarefaServo(void)
{
    //
    // Cria uma fila para envio de mensagens pra tarefa Servo
    //
    g_FilaServo = xQueueCreate(TAMANHO_FILA_SERVO, TAMANHO_ITEM_SERVO);

    //
    // Cria a tarefa Servo
    //
    if(xTaskCreate(TarefaServo, (signed portCHAR *)"SERVOf", TAMANHOPILHASERVO, NULL,
                   tskIDLE_PRIORITY + PRIORIDADE_TAREFA_SERVO, NULL) != pdTRUE)
    {
        return(1);
    }

    return(0);
}
