# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 04: Device Driver - Transmisión (Memory Pool, Interrupts & Gatekeeper Task)**

## 1. Introducción
El presente documento detalla la cuarta de desarrollo de la arquitectura del *Device Driver*. El objetivo de esta actividad es la consolidación y fusión de las técnicas implementadas en la Actividad 02 (*Memory Pool* mediante asignación dinámica) y la Actividad 03 (Interrupciones de hardware con sincronización por semáforo).

Al combinar el **paso por referencia** (punteros a memoria dinámica) con rutinas de transmisión **no bloqueantes** (IT), se logra una arquitectura que maximiza de forma simultánea la eficiencia en el uso de la memoria RAM y el tiempo de disponibilidad de la CPU.

## 2. Desarrollo

El desarrollo se enfocó en integrar la gestión del ciclo de vida de la memoria dinámica con el manejo asíncrono de los eventos de hardware, asegurando que no existan condiciones de carrera ni fugas de memoria (*Memory Leaks*).

### 2.1. Fusión de Arquitecturas
Se combinaron los recursos creados en las etapas previas dentro del sistema:
* **Estructura Liviana (`display_msg_t`):** Se mantuvo la estructura optimizada de la Actividad 02, donde las tareas encolan únicamente un puntero (`char *p_text`) y las coordenadas de destino, reduciendo el tamaño de la cola `h_display_queue`.
* **Semáforo Binario (`h_i2c_tx_sem`):** Se conservó el semáforo de sincronización de la Actividad 03 para comunicar la Rutina de Servicio de Interrupción (ISR) del I2C con la capa de abstracción de hardware.

### 2.2. Productores (Task A y Task B)
Las tareas productoras operan con la misma lógica de la Actividad 02, ignorando completamente que la transmisión física se realiza por interrupciones:
1. Solicitan un bloque de memoria al Heap (`pvPortMalloc`).
2. Formatean el texto dentro del bloque asignado.
3. Envían el puntero a la cola. Si la cola está llena, ejecutan un `vPortFree` inmediato para prevenir fugas de memoria, reteniendo la propiedad del bloque. Si el envío es exitoso, transfieren la propiedad al *Gatekeeper*.

### 2.3. Consumidor (Gatekeeper Task) y Driver de Hardware
La *Gatekeeper Task* y el driver de bajo nivel (`display.c`) interactúan de la siguiente manera para garantizar una transmisión segura:
1. **Recepción:** El *Gatekeeper* desencola el mensaje liviano (puntero a memoria dinámica).
2. **Transmisión y Bloqueo:** Invoca las primitivas de escritura del display. Internamente, el driver inicia el envío por hardware (`HAL_I2C_Master_Transmit_IT`) y ejecuta un `xSemaphoreTake`, bloqueando al *Gatekeeper* y cediendo la CPU.
3. **Notificación ISR:** El hardware finaliza la transmisión y dispara la interrupción. La ISR invoca `xSemaphoreGiveFromISR`, desbloqueando al *Gatekeeper*.
4. **Liberación de Memoria (Punto Crítico):** Una vez que el driver retorna (confirmando que el semáforo fue tomado y el hardware liberó el bus), el *Gatekeeper* asume de forma segura que los datos ya fueron enviados físicamente. En este instante, y solo en este instante, invoca `vPortFree()` para destruir la memoria apuntada, cerrando el ciclo de vida del dato.

## 3. Conclusiones

1. **Eficiencia Total:** El sistema es altamente eficiente tanto en espacio como en tiempo. El uso de punteros minimiza las transacciones de RAM del RTOS, mientras que las interrupciones aseguran que la CPU dedique su tiempo de procesamiento exclusivamente a la lógica de aplicación, sin esperas activas por hardware.
2. **Sincronización Crítica de Memoria:** Se comprobó que la liberación de memoria dinámica (`vPortFree`) debe estar rígidamente sincronizada con los eventos físicos del periférico. Delegar el bloqueo del semáforo a la capa del driver garantiza que la memoria nunca se libere antes de tiempo, previniendo fallos críticos (*HardFaults*) y corrupción de datos.
3. **Modularidad y Abstracción:** El diseño mantiene un alto nivel de desacoplamiento. Las tareas de aplicación se limitan a generar datos, el *Gatekeeper* administra el flujo de punteros, y el driver encapsula los detalles de sincronización de hardware. Esta separación de responsabilidades facilita el mantenimiento y la escalabilidad del firmware en proyectos de mayor envergadura.
