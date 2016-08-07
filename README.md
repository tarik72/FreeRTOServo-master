# FreeRTOServo
This application demonstrates the use of FreeRTOS tasks to control a servo using a Tiva C board from TI. Once FreeRTOS
is uploaded to the board, you can start playing with the OS features. 

## Moving the servo
To select the direction, the user presses one of the buttons on the board: `SW1` to move the pointer to the left and `SW2` to move the pointer to the right.

## More details

The application uses FreeRTOS to coordinate the tasks properly. The tasks are:

1. Servo: acts in the PWM to control the servomechanism;
2. Switch: monitors the buttons being pressed and passes that information to the Servo task.

Moreover, the other resources used are:

* A **queue** that makes possible the exchange of information between the Servo and Switch task;
* A **semaphore** that protects the UART of being accessed by multiple tasks at the same time;
* A **delay** that blocks the tasks when they are not being executed.



##IDE

P.S.: This project was created using Code Composer Studio, so make sure you use it to compile the code.
