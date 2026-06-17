# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 02: Device Driver - Transmisión (Memory Pool & Polling & Gatekeeper Task)**

## 1. Introducción
El presente documento detalla la evolución de la arquitectura del *Device Driver* desarrollada en la Actividad 01. En esta segunda etapa, el objetivo principal es optimizar el uso de los recursos del microcontrolador (CPU y memoria RAM) mediante la implementación de un mecanismo de gestión de memoria dinámica, cumpliendo con la premisa de la Actividad 02 (incorporación de un *Memory Pool* o asignación dinámica en FreeRTOS).

En la versión anterior, la comunicación entre las tareas y el Gatekeeper se realizaba mediante el **paso por valor**. Esto implicaba que el RTOS debía copiar arreglos enteros de caracteres (buffers) hacia y desde la cola de mensajes, generando un consumo innecesario de ciclos de reloj. 

Para resolver este problema de escalabilidad, se rediseñó el sistema utilizando primitivas de asignación dinámica de FreeRTOS (`pvPortMalloc` y `vPortFree`) para implementar un **paso por referencia**. De esta forma, las tareas ahora solo intercambian punteros (direcciones de memoria), logrando un sistema significativamente más rápido y eficiente.

## 2. Desarrollo

El desarrollo se centró en modificar la estructura de datos compartida y establecer un ciclo de vida estricto para la memoria (Alloc -> Queue -> Free) simulando el comportamiento de un Memory Pool a través del Heap de FreeRTOS.

### 2.1. Refactorización de Recursos
* **Estructura de Datos Liviana (`display_msg_t`):** Se modificó la estructura del mensaje. El buffer de texto estático de 21 bytes fue reemplazado por un puntero a carácter (`char *p_text`). Esto redujo drásticamente el tamaño físico de cada elemento dentro de la cola `h_display_queue`, ya que ahora solo viajan 4 bytes (tamaño de un puntero en la arquitectura ARM Cortex-M) independientemente de la longitud del mensaje.

### 2.2. Implementación de Tareas (Productores)
Las tareas de aplicación (`task_a.c` y `task_b.c`) fueron actualizadas para solicitar y formatear memoria dinámica:
1. **Asignación (Allocate):** Antes de enviar un mensaje, la tarea solicita un bloque de memoria al Heap de FreeRTOS utilizando `pvPortMalloc()`.
2. **Escritura:** Se valida que la asignación haya sido exitosa (puntero distinto de `NULL`) y se escribe el texto formateado directamente en ese bloque de memoria.
3. **Manejo de Errores (Prevención de Memory Leaks):** Se implementó una verificación crítica al enviar el mensaje a la cola (`xQueueSend`). Si la cola está llena y el envío falla, la tarea productora retiene la propiedad de la memoria y ejecuta un `vPortFree()` inmediato para liberar el bloque y evitar fugas de memoria (*Memory Leaks*). Si el envío es exitoso, la tarea transfiere la propiedad del bloque al Gatekeeper.

### 2.3. Tarea Gatekeeper (Consumidor)
La tarea `gatekeeper_task` mantuvo su lógica de transmisión por hardware (Polling vía I2C con esperas activas por SysTick), pero modificó su manejo de datos:
1. **Recepción:** Desencola el mensaje que contiene el puntero al texto.
2. **Transmisión:** Accede a la porción de memoria apuntada para imprimir los caracteres en el display.
3. **Liberación (Free):** Una vez finalizada la transmisión por I2C, el Gatekeeper asume la responsabilidad sobre ese bloque de memoria y ejecuta `vPortFree()` para devolverlo al Heap de FreeRTOS, permitiendo que otras tareas lo reutilicen.

## 3. Conclusiones

La implementación de la Actividad 02 demostró una mejora arquitectónica sustancial en el diseño del driver, arrojando las siguientes conclusiones:

1. **Eficiencia de Procesamiento:** Al reemplazar el paso por valor por el paso por referencia mediante punteros, se eliminó la sobrecarga del RTOS asociada a la copia profunda de arreglos en cada operación de la cola. El sistema ahora es capaz de escalar para transmitir tramas de datos de longitud variable o de gran tamaño sin penalizar el rendimiento de la CPU.
2. **Transferencia de Propiedad (Ownership Transfer):** Se aplicó exitosamente este patrón de diseño concurrente. Quedó establecido que la responsabilidad de liberar la memoria recae siempre en el "dueño" actual del puntero (la tarea productora si falla el encolamiento, o el Gatekeeper una vez consumido el dato).
3. **Determinismo y Estabilidad:** Con la incorporación de las validaciones de retorno tanto en la asignación (`pvPortMalloc`) como en el envío (`xQueueSend`), el sistema quedó protegido contra fallos por agotamiento de Heap o colas saturadas, asegurando la robustez esperada en un entorno de tiempo real.
