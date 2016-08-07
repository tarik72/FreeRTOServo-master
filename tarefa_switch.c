//*****************************************************************************
//
// tarefa_switch.c - Tarefa para processar os botoes.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "tarefa_switch.h"
#include "tarefa_servo.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define TAMANHO_PILHA_TAREFA_SWITCH        128         // Tamanho da pilha em palavras

extern xQueueHandle g_FilaServo;
extern xSemaphoreHandle g_pUARTSemaphore;

//********************************************************************************
// Essa tarefa le os estados dos botoes e passa tal informacao para tarefa Servo.
//********************************************************************************
static void TarefaSwitch(void *pvParameters)
{
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;
    uint8_t ui8CurButtonState, ui8PrevButtonState;
    uint8_t ui8Message;

    ui8CurButtonState = ui8PrevButtonState = 0;

    ui16LastTime = xTaskGetTickCount();

    while(1)
    {
        ui8CurButtonState = ButtonsPoll(0, 0);

        //
        // Checa se o estado anterior eh igual ao atual.
        //
        if(ui8CurButtonState != ui8PrevButtonState)
        {
            ui8PrevButtonState = ui8CurButtonState;

            //
            // Checa se a mudanca do estado eh devido ao
            // botao pressionado e nao ao soltar do botao
            //
            if((ui8CurButtonState & ALL_BUTTONS) != 0)
            {
                if((ui8CurButtonState & ALL_BUTTONS) == LEFT_BUTTON)
                {
                    ui8Message = LEFT_BUTTON;

                    //
                    // Projeto o UART de acesso concorrente
                    //
                    xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                    UARTprintf("Botao esquerdo pressionado.\n");
                    xSemaphoreGive(g_pUARTSemaphore);
                }
                else if((ui8CurButtonState & ALL_BUTTONS) == RIGHT_BUTTON)
                {
                    ui8Message = RIGHT_BUTTON;

                    //
                    // Projeto o UART de acesso concorrente
                    //
                    xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                    UARTprintf("Botao direito pressionado.\n");
                    xSemaphoreGive(g_pUARTSemaphore);
                }

                //
                // Passa o valor do botao pressionado a tarefa Servo
                //
                if(xQueueSend(g_FilaServo, &ui8Message, portMAX_DELAY) !=
                   pdPASS)
                {
                    //
                    // Erro: a fila nunca deve estar cheia.
                    //
                    UARTprintf("\nFila cheia. Isto nunca deveria acontecer!\n");
                    while(1)
                    {
                    }
                }
            }
        }

        //
        // Espera um tempo pra checar a volta.
        //
        vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);
    }
}

//*****************************************************************************
//
// Inicializa a tarefa switch
//
//*****************************************************************************
uint32_t InicTarefaSwitch(void)
{
    //
    // Desbloqueia o registrador GPIO do botao direito pra funcionar.
    //
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0xFF;

    //
    // Inicializa os botoes
    //
    ButtonsInit();

    //
    // Cria a tarefa task
    //
    if(xTaskCreate(TarefaSwitch, (signed portCHAR *)"Switch",
                   TAMANHO_PILHA_TAREFA_SWITCH, NULL, tskIDLE_PRIORITY +
                   PRIORIDADE_TAREFA_SWITCH, NULL) != pdTRUE)
    {
        return(1);
    }

    return(0);
}
