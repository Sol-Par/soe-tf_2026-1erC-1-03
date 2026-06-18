# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 04: Device Driver - Transmisión (Memory Pool, Interrupts & Gatekeeper Task)**

## 1. Introducción
El presente documento detalla la cuarta etapa de desarrollo de la arquitectura del *Device Driver*. El objetivo de esta actividad es la consolidación y fusión de las técnicas implementadas en la Actividad 02 (Asignación dinámica de memoria / *Memory Pool*) y la Actividad 03 (Transmisión asíncrona mediante Interrupciones de hardware).

Esta implementación representa el estándar de la industria para el desarrollo de controladores de periféricos. Al combinar el **paso por referencia** (punteros a memoria dinámica) con rutinas de transmisión **no bloqueantes** (IT), se logra una arquitectura que maximiza de forma simultánea la eficiencia en el uso de la memoria RAM y el tiempo de disponibilidad de la CPU. 

## 2. Desarrollo

El desarrollo se enfocó en integrar la gestión del ciclo de vida de la memoria dinámica con el manejo asíncrono de los eventos de hardware, estableciendo un control estricto a nivel de aplicación para garantizar que no existan condiciones de carrera ni fugas de memoria (*Memory Leaks*).

### 2.1. Fusión de Arquitecturas
Se combinaron los recursos creados en las etapas previas dentro del sistema:
* **Estructura Liviana (`display_msg_t`):** Se mantuvo la estructura optimizada de la Actividad 02, donde las tareas encolan únicamente un puntero (`char *p_text`) y coordenadas, reduciendo drásticamente el peso de las transacciones en la cola `h_display_queue`.
* **Semáforo Binario (`h_i2c_tx_sem`):** Se conservó el semáforo de la Actividad 03 para comunicar la Rutina de Servicio de Interrupción (ISR) del I2C con la *Gatekeeper Task*.

### 2.2. Productores (Task A y Task B)
Las tareas productoras operan generando los datos dinámicos:
1. Solicitan un bloque de memoria al Heap (`pvPortMalloc`).
2. Formatean el texto dentro del bloque asignado.
3. Envían el puntero a la cola. Si el encolamiento falla, ejecutan un `vPortFree` inmediato para prevenir fugas de memoria. Si el envío es exitoso, la tarea transfiere la propiedad del bloque al *Gatekeeper*.

### 2.3. Consumidor (Gatekeeper Task) y Sincronización Explícita
El patrón de diseño establece que la *Gatekeeper Task* maneja activamente el ciclo de vida del dato coordinándose con el driver de bajo nivel (`display.c`):
1. **Recepción:** El *Gatekeeper* desencola el mensaje liviano (puntero a memoria).
2. **Disparo Asíncrono:** Invoca las primitivas de escritura del display. Internamente, el driver solo inicia el envío por hardware (`HAL_I2C_Master_Transmit_IT`) y retorna de forma instantánea.
3. **Sincronización (Espera):** Inmediatamente después, el *Gatekeeper* ejecuta un `xSemaphoreTake`, bloqueando su ejecución y cediendo la CPU a otras tareas mientras el I2C trabaja en segundo plano. Al finalizar físicamente, la ISR ejecuta `xSemaphoreGiveFromISR` y el *Gatekeeper* se destraba.
4. **Liberación de Memoria (Punto Crítico):** Al destrabarse el semáforo, el *Gatekeeper* asume de forma segura que el hardware liberó el bus de datos. En la línea contigua, invoca `vPortFree()` destruyendo la memoria apuntada y cerrando el ciclo de vida del buffer sin riesgos de sobreescritura.

## 3. Conclusiones

La implementación de la Actividad 04 demuestra una arquitectura robusta, eficiente y de alta legibilidad. Se concluye que:

1. **Eficiencia Total:** El uso de punteros minimiza las copias en RAM del RTOS, mientras que las interrupciones aseguran que la CPU dedique su tiempo exclusivamente a la lógica de las tareas productoras, sin esperas activas.
2. **Robustez por Sincronización a Nivel de Aplicación:** Agrupar secuencialmente el `xSemaphoreTake` y el `vPortFree` dentro del mismo bloque de código en el *Gatekeeper* mejora notablemente la trazabilidad del sistema. Garantiza visual y operativamente que la memoria dinámica jamás será destruida o reciclada por FreeRTOS mientras el periférico siga transmitiendo bits físicamente, evitando fallos críticos (*HardFaults*).
3. **Escalabilidad del Driver:** Al mantener el bloqueo y la gestión de memoria centralizados en la tarea de FreeRTOS, el driver de bajo nivel queda puramente dedicado a la manipulación de registros de hardware.
