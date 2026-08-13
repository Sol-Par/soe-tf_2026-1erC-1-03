# FIUBA - Electrónica - Sistemas Operativos Embebidos

## Trabajo Final - Device Driver de FreeRTOS

![Banner](Recursos/banner.png)

### 2026-1erC - 1-03

### Integrantes del grupo:

| Padrón | Apellidos, Nombres | Fecha | Deadline |
| :----- | :--------------------- | :------: | :-------: |
| 110901 | Chechko, Víctor Nicolás | - | 12 AGO 26 |
| 109308 | Marconi Casares, Lourdes | - | 12 AGO 26 |
| 109324 | Solari Parravicini, Facundo | - | 12 AGO 26 |

---
### Objetivos

El objetivo principal de este Trabajo Final es diseñar e implementar la evolución de un Device Driver modular en FreeRTOS sobre microcontroladores STM32. A lo largo de 14 actividades, se integran paulatinamente distintas estrategias de comunicación física (Polling, Interrupciones y DMA) junto con técnicas de gestión de memoria dinámica (*Memory Pool* por referencia) y primitivas de sincronización (*Gatekeeper Task*, Colas y Semáforos).

---
### Entorno de Desarrollo y Hardware

* **Microcontrolador / Placa:** STM32 (NUCLEO-F446RE) / Núcleo ARM Cortex-M.
* **Periférico / Salida:** Display LCD (con adaptador I2C PCF8574) / Memoria EEPROM I2C.
* **Sistema Operativo:** FreeRTOS Kernel.
* **IDE & Cadena de Herramientas:** STM32CubeIDE / GCC ARM Embedded Toolchain.
* **Perfilado y Medición:** Unidad DWT (*Data Watchpoint and Trace*) para la medición precisa de tiempos de ejecución en microsegundos.

---
### Resumen de Actividades

#### Transmisión

* **[Actividad 01](./soe-tf_01-application.md) (Polling):** Transmisión de mensajes paso por valor utilizando colas de FreeRTOS (`xQueue`). La `Gatekeeper Task` ejecuta la transferencia mediante espera activa (*Polling*) sobre el bus.
* **[Actividad 02](./soe-tf_02-application.md) (Memory Pool & Polling):** Transmisión paso por referencia. Las tareas asignan memoria dinámica (`pvPortMalloc`), envían el puntero por la cola y el `Gatekeeper` transmite por Polling para luego liberar la memoria (`vPortFree`).
* **[Actividad 03](./soe-tf_03-application.md) (Interrupt):** Transmisión paso por valor asistida por interrupciones (IT). La `Gatekeeper Task` inicia la transferencia y se bloquea en un semáforo binario hasta que la ISR de fin de transmisión lo entrega.
* **[Actividad 04](./soe-tf_04-application.md) (Memory Pool & Interrupt):** Transmisión por interrupciones combinada con paso por referencia. La liberación de memoria dinámica se realiza en el `Gatekeeper` recién al despertar tras la interrupción del hardware.
* **[Actividad 05](./soe-tf_05-application.md) (DMA):** Transmisión por Direct Memory Access (DMA) con paso por valor. El hardware descarga a la CPU de la transferencia byte a byte, notificando su finalización mediante la ISR de DMA.
* **[Actividad 06](./soe-tf_06-application.md) (Memory Pool & DMA):** Máxima optimización en transmisión. Combina el paso por referencia con transferencias por DMA, sincronizando la liberación del bloque del *Heap* mediante semáforos e interrupciones de DMA.

#### Recepción

* **[Actividad 07](./soe-tf_07-application.md) (Known Length & Polling):** Recepción de tramas de longitud fija mediante espera activa (*Polling*), utilizando paso por valor para comunicar los datos recibidos.
* **[Actividad 08](./soe-tf_08-application.md) (Known Length & Memory Pool & Polling):** Recepción de longitud conocida por Polling. Se solicita memoria en el receptor/Gatekeeper para almacenar los datos recibidos y transmitir su puntero por la cola.
* **[Actividad 09](./soe-tf_09-application.md) (Known Length & Interrupt):** Recepción de tramas de longitud fija por interrupción. La tarea se bloquea hasta recibir la notificación de hardware de buffer lleno.
* **[Actividad 10](./soe-tf_10-application.md) (Known Length & Memory Pool & Interrupt):** Recepción de longitud conocida combinando interrupciones de hardware con asignación dinámica de memoria (*Memory Pool*) por referencia.
* **[Actividad 11](./soe-tf_11-application.md) (Unknown Length & Memory Pool & Interrupt):** Recepción de tramas de longitud variable por interrupción (detección de línea libre / *IDLE Line*). Se asigna memoria dinámica dinámicamente según la cantidad real de bytes recibidos.
* **[Actividad 12](./soe-tf_12-application.md) (Known Length & DMA):** Recepción de longitud fija utilizando DMA para escribir datos directamente en memoria RAM sin intervención de la CPU durante la transferencia.
* **[Actividad 13](./soe-tf_13-application.md) (Known Length & Memory Pool & DMA):** Recepción por DMA a longitud fija utilizando bloques de memoria dinámica gestionados mediante paso por referencia.
* **[Actividad 14](./soe-tf_14-application.md) (Unknown Length & Memory Pool & DMA):** Máxima optimización en recepción. Integración de DMA con interrupción por *IDLE Line* y gestión de memoria dinámica para capturar tramas de tamaño variable con costo mínimo de CPU.
---

### Funciones de Driver Implementadas
El driver expone una API estandarizada encargada de gestionar la comunicación física a través del bus I2C. Estas funciones abstraen tanto la escritura en el display LCD como la lectura de bloques de datos en la memoria EEPROM, integrando internamente las llamadas a la HAL y los mecanismos de sincronización (espera activa, interrupciones o DMA).

```c
// Función del driver I2C para imprimir un mensaje
// en la posición (x,y) de una pantalla LCD.
void i2c_lcd_puts_x_y(I2C_LCD_HandleTypeDef *lcd, int col, int row, char *str)
```

```c
// Función del driver I2C para leer una memoria.
HAL_StatusTypeDef i2c_mem_read(I2C_HandleTypeDef *hi2c, uint16_t device_address, uint16_t memory_address, uint16_t memory_add_size, uint8_t *p_rx, uint16_t size, uint32_t timeout)
```

---

### Cuadro Comparativo
A continuación se presentan las métricas de rendimiento obtenidas durante la caracterización de cada actividad. El **Tiempo Bloqueante** representa el tiempo durante el cual la CPU permanece dedicada exclusivamente a la operación de I/O (impedida de ejecutar otras tareas), mientras que el **Tiempo Total** contempla la duración completa de la transacción física sobre el bus.

#### Transmisión

| Actividad | Tipo            | Espera Bloqueante [μs/byte] | Tiempo Total [μs/byte] | Eficiencia CPU | Manejo de Memoria    |
| :-------: | :-------------: | :-------------------------: | :--------------------: | :------------: | :------------------: |
| 01        | Polling         | 465                         | 473                    | Baja           | Inseguro             |
| 02        | Polling         | 462                         | 473                    | Baja           | Seguro (Memory Pool) |
| 03        | Interrupt       | 4                           | 488                    | Media          | Inseguro             |
| 04        | Interrupt       | 4                           | 489                    | Media          | Seguro (Memory Pool) |
| 05        | DMA             | 9                           | 367                    | Alta           | Inseguro             |
| 06        | DMA             | 9                           | 368                    | Alta           | Seguro (Memory Pool) |

<br>

#### Recepción

| Actividad | Tipo                     | Espera Bloqueante [μs/byte] | Tiempo Total [μs/byte] | Eficiencia CPU | Manejo de Memoria    |
| :-------: | :----------------------: | :-------------------------: | :--------------------: | :------------: | :------------------: |
| 07        | Polling Known Length     | 102                         | 103                    | Baja           | Inseguro             |
| 08        | Polling Known Length     | 102                         | 102                    | Baja           | Seguro (Memory Pool) |
| 09        | Interrupt Known Length   | 4                           | 101                    | Media          | Inseguro             |
| 10        | Interrupt Known Length   | 4                           | 100                    | Media          | Seguro (Memory Pool) |
| 11        | Interrupt Unknown Length | 5                           | 550                    | Media          | Seguro (Memory Pool) |
| 12        | DMA Known Length         | 6                           | 31                     | Alta           | Inseguro             |
| 13        | DMA Known Length         | 6                           | 35                     | Alta           | Seguro (Memory Pool) |
| 14        | DMA Unknown Length       | 8                           | 584                    | Alta           | Seguro (Memory Pool) |

<br>

Se concluye a partir de los resultados que la comunicación por _polling_ resulta ser la más ineficiente en términos de uso de la CPU, presentando un tiempo bloqueante mucho mayor al de las otras alternativas. En este sentido, se puede observar que la espera bloqueante coincide prácticamente con el tiempo total del proceso.

En las actividades que aprovechan el uso de interrupciones y del módulo de DMA, se midieron esperas bloqueantes ínfimas y tiempos totales comparables o menores a los casos de _polling_, la única excepción siendo el tiempo total observado en las actividades 11 y 14. La desmejora en estos casos se atribuye al modo en que se lee de a 1 byte a la vez, dado el desconocimiento a priori de la longitud del mensaje a recibir.

El método de transmisión y recepción por DMA se destaca por sobre la vía de IT en mayor medida cuando los mensajes a comunicar poseen una longitud considerable en bytes. Esto se debe a que, mientras que las interrupciones producen un uso periódico de la CPU que crece proporcionalmente con el número de bytes, la comunicación por DMA únicamente requiere dos intervenciones independientemente de la longitud, siendo estas cuando comienza la transferencia y cuando termina.

---

### Obtención de Tiempos

Para medir el tiempo transcurrido entre dos instrucciones o eventos, se realizó el siguiente procedimiento:

Para medir el tiempo transcurrido entre eventos, se empleó la unidad **DWT (Data Watchpoint and Trace)** propia del núcleo ARM Cortex-M. Este módulo cuenta con un contador de ciclos de reloj de alta resolución (`CYCCNT`) que permite calcular con precisión de microsegundos la latencia y el tiempo bloqueante de la CPU en cada operación del driver.

### Procedimiento de Medición

1. **Inicialización:** Se habilita e inicializa el contador de ciclos DWT antes de comenzar la ejecución de las tareas.
2. **Reset:** Inmediatamente antes de iniciar la función a caracterizar (o la transmisión/recepción), se reinicia el contador mediante `cycle_counter_reset()`.
3. **Captura:** Una vez finalizado el bloque de código o liberado el recurso, se obtiene el tiempo transcurrido con `cycle_counter_get_time_us()`.

```c
// Asegurarse de incluir el archivo de DWT (Data Watchpoint and Trace).
#include "dwt.h"

// Reiniciar el contador en el punto de inicio.
cycle_counter_reset();

// (lista de instrucciones o eventos a medir)
// ...

// Obtener el contador en el punto final.
uint32_t time = cycle_counter_get_time_us();

// Imprimir el tiempo por consola (o bien colocar un breakpoint
// y leer el valor en 'Expressions').
LOGGER_INFO("Tiempo = %lu us", time);
```

<br>

Para la medición de las **esperas bloqueantes**, la instrucción a medir fue la primitiva de transmisión/recepción propia de la HAL (`HAL_I2C_Master_Transmit_XX` en un caso y `HAL_I2C_Mem_Read_XX` en el otro).

Para la obtención de los **tiempos totales**, se midió el tiempo de una transmisión/recepción completa, incluyendo el tiempo que transcurre la tarea _gatekeeper_ bloqueada en un semáforo, en los casos en que lo hay. Esta espera, sin embargo, no es bloqueante en tanto que permite que se ejecuten otras tareas (la tarea _IDLE_, en ausencia de otras) mientras finaliza el proceso de comunicación.

En todos los casos, se normalizó el tiempo por el tamaño del mensaje a transmitir/recibir en bytes, para que las comparaciones resulten verdaderamente cualitativas. En el caso de **transmisión** (actividades 01 a 06), el mensaje siempre fue de 18 bytes, incluyendo 1 byte para indicar la posición del cursor y 17 bytes de mensaje. En el caso de **recepción** (actividades 07 a 14) el tamaño del mensaje recibido difiere entre actividades, y es plenamente variable en las actividades 11 y 14. En las actividades donde se recibe por DMA, se priorizó la lectura de mensajes largos para aprovechar de forma más plena la ventaja del método.

---

### Conclusiones 

A lo largo del desarrollo del Trabajo Final se pudieron validar de forma práctica las diferencias de rendimiento entre las distintas arquitecturas de un Device Driver en FreeRTOS:

* **Gestión de Memoria (Paso por Valor vs. Memory Pool):** Pasar datos por valor a través de colas (`xQueue`) es más simple de implementar, pero requiere copiar todo el contenido del buffer en cada envío. La implementación de *Memory Pool* (paso por referencia) mediante `pvPortMalloc` y `vPortFree` demostró ser la alternativa más eficiente, ya que solo se transmiten punteros y se evita sobrecargar la memoria y los ciclos de CPU.
* **Mecanismos de Comunicación (Polling vs. Interrupciones vs. DMA):**
  * **Polling:** Consume el 100% de la CPU en esperas activas, impidiendo que el RTOS aproveche el tiempo para ejecutar otras tareas.
  * **Interrupciones (IT):** Liberan a la CPU durante la transferencia, bloqueando la tarea en un semáforo hasta recibir la notificación del hardware.
  * **DMA:** Ofrece el mejor rendimiento general, permitiendo que la transferencia entre la memoria y el periférico ocurra de forma autónoma sin intervención del procesador.

En conclusión, la integración de *Memory Pool* junto con transferencias asistidas por DMA y sincronización por semáforos/colas consolida la arquitectura ideal para el diseño de controladores en sistemas embebidos de tiempo real, logrando un equilibrio óptimo entre eficiencia de CPU, uso racional de RAM y determinismo en el sistema.
