#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "tarefa_servo.h"
#include "tarefa_switch.h"
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
//! Thiago Oliveira Ribeiro
//! ---------------------------------------------------------------------------
//!
//! Esta aplicacao demonstra o uso de tarefas
//!
//! A aplicacao consiste em controlar o micro servo-motor TowerPro a partir
//! da placa Tiva C Series da Texas Instruments.
//!
//! Para selecionar a direcao desejada o usuario deve pressionar um dos botoes
//! disponiveis na placa: SW1 (para mover o ponteiro pra esquerda) ou SW2 (para
//! mover o ponteiro para esquerda).
//!
//! No entanto, a aplicacao utiliza-se do sistema operacional FreeRTOS para
//! coordenar as tarefas de maneira adequada. As tarefas criadas sao as seguintes:
//!
//! - Servo: atua no PWM controlando o servo mecanismo.
//!
//! - Switch: monitora os botoes sendo pressionados e passa a informacao
//!   		  para a tarefa servo.
//!
//! Alem das tuas tarefas mencionadas, a aplicacao tambem faz uso dos seguinte
//! recursos do FreeRTOS:
//!
//! - Uma Fila (Queue) que tornara possivel a transferencia de informacoes entre
//!	  as tasks "switch" e "servo".
//!
//! - Um semaforo (Semaphore) para proteger o recurso, UART, de ser acessado por
//!   multiplas tarefas ao mesmo tempo.
//!
//! - Um atraso (delay) do FreeRTOS para colocar as tarefas em modo bloqueando
//!    quando as mesmas nao estiverem desempenhando nada.
//!
//*****************************************************************************

xSemaphoreHandle g_pUARTSemaphore;		// Mutex que protege o acesso concorrente da UART por multiplas tarefas

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line){}	// Rotina de erro chamada se a biblioteca de drivers der erro.
#endif

//*******************************************************************************
//
// Funcao chamada pelo FreeRTOS quando um erro de estouro de pilha for detectado
//
//*******************************************************************************
void
vApplicationStackOverflowHook(xTaskHandle *pxTask, char *pcTaskName)
{
    while(1)
    {
    }
}

//*****************************************************************************
//
// Funcao que configura UART e seus pinos
//
//*****************************************************************************
void ConfigureUART(void)
{
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// Funcao que configura PWM e seus pinos
//
//*****************************************************************************

void ConfigurePWM(void){
	/* Habilitar o periférico PWM e sua configuração */

	// Setar clock
	ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

	// Habilitar periférico
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// Setar pino
	ROM_GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);
	ROM_GPIOPinConfigure(GPIO_PD0_M1PWM0);

	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
	ROM_GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
	ROM_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

//*****************************************************************************
//
// Inicializacao do FreeRTOS e do conjunto de tarefas iniciais
//
//*****************************************************************************
int main(void)
{
    //
    // Configuracao do clock para rodar a 50MHz.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Inicializacao e configuracao do UART
    //
    ConfigureUART();

    // Inicializacao e configuracao do módulo PWM
    ConfigurePWM();

    //
    // Print demo introduction.
    //
    UARTprintf("\n\nWelcome to the EK-TM4C123GXL FreeRTOS Demo!\n");

    //
    // Criacao do Mutex para coordenar o acesso a UART
    //
    g_pUARTSemaphore = xSemaphoreCreateMutex();

    //
    // Cria a tarefa Servo
    //
    if(InicTarefaServo() != 0)
    {
        while(1)
        {
        }
    }

    //
    // Cria a tarefa Switch.
    //
    if(InicTarefaSwitch() != 0)
    {
        while(1)
        {
        }
    }

    //
    // Inicializa o escalonador de tarefas.
    //
    vTaskStartScheduler();

    //
    // Caso o escalonador retorne alguma coisa, escreve "erro" e entra em loop
    // infinito.
    //

    while(1)
    {
    }
}
