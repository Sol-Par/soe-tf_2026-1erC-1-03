/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"

/********************** macros and definitions *******************************/
#define G_TASK_B_CNT_INI	0ul

#define TASK_B_DEL_ZERO		(pdMS_TO_TICKS(0ul))
#define TASK_B_DEL_MAX		(pdMS_TO_TICKS(500ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_b_wait_250mS			= "   ==> Task    B - Wait:   500mS";

/********************** external data declaration ****************************/
uint32_t g_task_b_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_b(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_b_cnt = G_TASK_B_CNT_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	// Declaramos la instancia del request a solicitar.
	i2c_rx_req_t i2c_rx_req;

	i2c_rx_req.address = 0x00A0;
	i2c_rx_req.length = 32;
	i2c_rx_req.requester_task = xTaskGetCurrentTaskHandle();

	i2c_rx_req_t *p_req = &i2c_rx_req;

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_b_cnt++;

		i2c_rx_req.rx = pvPortMalloc(MAX_MSG_LEN);

		if(i2c_rx_req.rx != NULL)
		{
			if (xQueueSend(h_i2c_queue, &p_req, pdMS_TO_TICKS(10)) != pdPASS)
			{
				vPortFree(i2c_rx_req.rx);
				LOGGER_INFO("Task B: Cola llena, request descartada y memoria liberada");
			}
			else
			{
				LOGGER_INFO("Task B: Request encolada");

				// Esperamos la notificación del gatekeeper.
				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

				LOGGER_INFO("Task B: Mensaje recibido");

				// Mostrar el mensaje que se recibió.
				LOGGER_LOG("\n--------------------------------\n");
				LOGGER_LOG("\n%s\n", i2c_rx_req.rx);
				LOGGER_LOG("\n--------------------------------\n");

				i2c_rx_req.address += i2c_rx_req.length;

				if (i2c_rx_req.address > 0x0100)
				{
					i2c_rx_req.address = 0x00A0;
				}

				// Una vez que el mensaje se terminó de leer, liberar el espacio.
				vPortFree(i2c_rx_req.rx);
			}
		}
		else
		{
			LOGGER_INFO("Task B: ¡Error! Out of Heap Memory");
		}

    	/* Print out: Wait 250mS */
		LOGGER_INFO(p_task_b_wait_250mS);
		vTaskDelay(TASK_B_DEL_MAX);
	}
}

/********************** end of file ******************************************/
