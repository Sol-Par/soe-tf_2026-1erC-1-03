# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 03: Device Driver - Transmisión (Interrupt & Gatekeeper Task)**

## 1. Introducción
El presente documento detalla la tercera etapa de desarrollo del *Device Driver* para el RTOS. El objetivo de esta actividad es reemplazar el método de transmisión bloqueante (Polling) implementado en las etapas anteriores, por un mecanismo de comunicación basado en **Interrupciones (IT)**, manteniendo el patrón arquitectónico *Gatekeeper Task*.

Para aislar el objetivo de estudio de esta etapa, se revirtió temporalmente el manejo de memoria al "paso por valor" (buffers estáticos en los mensajes), permitiendo concentrar el diseño exclusivamente en la sincronización entre el hardware I2C y el planificador (Scheduler) de FreeRTOS.

El problema a resolver radica en que las funciones de transmisión por *Polling* retienen el control del microprocesador en un bucle de espera activa (esperando que los bits viajen físicamente por el cable). Al migrar a interrupciones, se busca ceder el 100% de ese tiempo de CPU al RTOS para que ejecute otras tareas mientras el periférico I2C trabaja en segundo plano.

## 2. Desarrollo

El desarrollo consistió en vincular las rutinas de servicio de interrupción (ISR) del hardware STM32 con la API de FreeRTOS mediante primitivas de sincronización.

### 2.1. Creación de Recursos de Sincronización
Para lograr que la tarea administradora se bloquee y despierte a demanda del hardware, se introdujo un nuevo recurso del sistema operativo:
* **Semáforo Binario (`h_i2c_tx_sem`):** Se instanció un semáforo binario durante la inicialización de la aplicación (`app_init`). Este elemento actúa como una señal de finalización entre la interrupción del hardware y la *Gatekeeper Task*.

### 2.2. Modificación del Driver de Bajo Nivel
La capa de abstracción del hardware (`display.c`) fue modificada para operar de manera no bloqueante:
1. Se reemplazó la llamada a `HAL_I2C_Master_Transmit` por su variante asíncrona `HAL_I2C_Master_Transmit_IT`. Esta función delega la transmisión al periférico I2C y retorna el control a la CPU de manera casi instantánea.
2. Inmediatamente después de iniciar la transmisión, el driver invoca la función `xSemaphoreTake(h_i2c_tx_sem, portMAX_DELAY)`. Como el semáforo se encuentra vacío, la *Gatekeeper Task* pasa al estado **Blocked**, permitiendo que FreeRTOS asigne la CPU a las tareas de aplicación (Task A y Task B).

### 2.3. Rutina de Servicio de Interrupción (ISR)
Se implementó el *Callback* de interrupción del HAL de STM32 (`HAL_I2C_MasterTxCpltCallback`). Cuando el periférico I2C finaliza el envío del último bit por hardware, se dispara esta interrupción, ejecutando las siguientes acciones:
1. **Notificación:** Se invoca a la función `xSemaphoreGiveFromISR()`, la cual "entrega" el semáforo binario, indicando que la transmisión finalizó.
2. **Cambio de Contexto Forzado:** Se utilizó la macro `portYIELD_FROM_ISR()` para evaluar si la tarea recién desbloqueada (Gatekeeper) tiene mayor prioridad que la tarea que fue interrumpida. De ser así, se fuerza un cambio de contexto inmediato al salir de la ISR, minimizando la latencia de respuesta.

## 3. Conclusiones

La implementación de la Actividad 03 logró optimizar drásticamente la utilización de la CPU, demostrando el poder de combinar periféricos por hardware con un RTOS. Se concluye que:

1. **Aprovechamiento de CPU:** El microcontrolador ya no desperdicia ciclos de reloj en esperas activas de hardware. El tiempo que antes se perdía enviando datos a 100 kHz por el bus I2C, ahora es capitalizado por el Scheduler para avanzar en la lógica de las tareas productoras.
2. **Sincronización ISR-Task Segura:** Se validó el uso correcto de las APIs específicas para interrupciones de FreeRTOS (aquellas con el sufijo `FromISR`). Utilizar la API regular dentro de un *Callback* habría corrompido el estado interno del Kernel.
3. **Latencia Determinista:** La combinación de la interrupción con el `portYIELD_FROM_ISR` garantiza que el *Gatekeeper* retome el control en el instante exacto en que el bus queda libre, logrando una latencia de atención predecible y propia de un sistema *Hard Real-Time*.
