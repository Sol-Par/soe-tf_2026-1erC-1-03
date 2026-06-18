# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 06: Device Driver - Transmisión (Memory Pool & DMA & Gatekeeper Task)**

## 1. Introducción
El presente documento detalla la sexta etapa del desarrollo de la arquitectura del *Device Driver*. El objetivo principal de esta actividad es fusionar la gestión de memoria dinámica (*Memory Pool*) implementada originalmente en las Actividades 02 y 04, con la descarga de procesamiento de hardware (DMA) introducida en la Actividad 05.

Esta combinación representa el punto máximo de optimización para la transmisión de datos: el **paso por referencia** minimiza las copias de información dentro de la memoria RAM administrada por FreeRTOS, mientras que el **DMA** garantiza que el procesador no pierda ciclos de reloj ejecutando bucles de transmisión o atendiendo múltiples interrupciones por cada byte enviado.

## 2. Desarrollo

El desarrollo se caracterizó por la integración de módulos ya probados en etapas anteriores, demostrando la alta cohesión y el bajo acoplamiento del diseño del sistema.

### 2.1. Reestructuración de Datos (Herencia de la Actividad 02 y 04)
Se revirtió la estructura del mensaje `display_msg_t` para abandonar el paso por valor y volver a utilizar un puntero (`char *p_text`). Las tareas productoras (`Task A` y `Task B`) retomaron exactamente el mismo código empleado en la Actividad 04:
1. Solicitan memoria mediante `pvPortMalloc()`.
2. Encolan el puntero transfiriendo su propiedad a la *Gatekeeper Task*.
3. Aplican prevención estricta de *Memory Leaks* liberando el bloque (`vPortFree()`) en caso de que la cola de mensajes se encuentre llena.

### 2.2. Disparo Asíncrono (Herencia de la Actividad 05)
La capa de abstracción del hardware (`display.c`) mantuvo la configuración implementada en la Actividad 05. En lugar de transmitir por interrupciones, el driver configura el controlador DMA mediante la API `HAL_I2C_Master_Transmit_DMA`, proporcionándole la dirección de memoria dinámica y la longitud del texto, para luego retornar el control a la tarea de manera instantánea.

### 2.3. Sincronización Crítica de Memoria (Gatekeeper Task)
La sincronización a nivel de aplicación, consolidada en la Actividad 05, demostró ser el patrón arquitectónico ideal para proteger las transferencias por DMA. La *Gatekeeper Task* opera como garante de la integridad de la RAM:
1. **Disparo:** Invoca al driver, lo que inicia la lectura del DMA desde el bloque de memoria dinámica asignado.
2. **Freno de Tarea:** Ejecuta `xSemaphoreTake(h_i2c_tx_sem, portMAX_DELAY)`, cediendo la CPU.
3. **Estabilidad de RAM:** Durante el tiempo de bloqueo, el controlador DMA lee directamente del Heap de FreeRTOS. Como la tarea dueña del puntero (Gatekeeper) está dormida, es imposible que la memoria sea modificada o destruida accidentalmente.
4. **Liberación:** Al dispararse la interrupción de *DMA Transfer Complete*, la ISR entrega el semáforo. El *Gatekeeper* despierta y, teniendo confirmación absoluta de que el hardware físico liberó el bus, ejecuta el `vPortFree()` correspondiente.

## 3. Conclusiones

La implementación de la Actividad 06 consolida la arquitectura de transmisión del *Device Driver*, permitiendo extraer las siguientes observaciones:

1. **Integridad de Datos:** Al igual que se observó en la Actividad 04 con las interrupciones, el patrón de "Transferencia de Propiedad" se adaptó perfectamente al comportamiento del DMA. El bloqueo explícito en el *Gatekeeper Task* antes de liberar la memoria es la única garantía de que el DMA no lea punteros colgantes o memoria reasignada (basura).
2. **Eficiencia Extrema:** Las tareas productoras manejan punteros ligeros, el RTOS mueve solo 4 bytes por mensaje a través de la cola, y el hardware transmite de manera autónoma. La CPU queda dedicada casi en su totalidad a la lógica de negocio y planificación del sistema.
3. **Diseño Modular:** La transición entre mecanismos de comunicación física (Polling $\rightarrow$ IT $\rightarrow$ DMA) a lo largo de las actividades requirió cero alteraciones en la lógica de las tareas productoras, validando el éxito de la abstracción de capas en este Trabajo Práctico.
