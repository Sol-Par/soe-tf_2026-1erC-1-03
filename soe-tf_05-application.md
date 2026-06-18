# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 05: Device Driver - Transmisión (DMA & Gatekeeper Task)**

## 1. Introducción
El presente documento detalla la quinta etapa del desarrollo del *Device Driver*. El objetivo es llevar la arquitectura a su máxima eficiencia posible mediante la implementación de **DMA (Direct Memory Access)** para la transmisión de datos hacia el periférico I2C, manteniendo la gestión de memoria dinámica (*Memory Pool*) y la sincronización a nivel de tarea (*Gatekeeper Task*).

En la etapa anterior, basada en interrupciones (IT), la CPU debía intervenir byte a byte para transferir la información desde la memoria RAM hacia el registro de datos del hardware. Al introducir DMA, esta transferencia física se delega por completo a un procesador secundario dedicado. La CPU solo interviene para iniciar la transacción y es notificada (vía interrupción) únicamente cuando el bloque completo de datos ha sido transmitido.

## 2. Desarrollo

La transición hacia DMA demostró la robustez del diseño propuesto. Al mantener la lógica de control y sincronización centralizada en la *Gatekeeper Task*, las modificaciones se limitaron exclusivamente a la capa inferior de hardware.

### 2.1. Configuración de Hardware (STM32CubeIDE)
Para habilitar la transferencia autónoma, se configuró el controlador DMA del microcontrolador:
1. Se asignó un canal DMA al periférico I2C1 (Tx) en modo *Normal* (transferencia única, no circular).
2. Se habilitó el incremento automático de la dirección de origen (*Memory Increment*) para que el DMA barra secuencialmente el buffer dinámico de caracteres.
3. Se enlazó la interrupción global del canal DMA para notificar la finalización.

### 2.2. Disparo Asíncrono (Driver de Bajo Nivel)
La capa de abstracción (`display.c`) fue actualizada para utilizar la API de DMA:
* La función `HAL_I2C_Master_Transmit_IT` fue reemplazada por `HAL_I2C_Master_Transmit_DMA`. Al invocar esta primitiva, el driver configura el hardware DMA con la dirección del puntero y la longitud del texto, iniciando la transferencia en paralelo e inmediatamente retornando el control a la tarea que lo invocó.

### 2.3. Sincronización y Protección de RAM (Gatekeeper Task)
La arquitectura de sincronización explícita en el *Gatekeeper* resultó ser crítica y perfecta para el comportamiento del DMA:
1. El *Gatekeeper* invoca las primitivas del display (que disparan el DMA en segundo plano).
2. **Espera Activa:** En la línea siguiente, el *Gatekeeper* ejecuta `xSemaphoreTake(h_i2c_tx_sem, portMAX_DELAY)`, cediendo el 100% de la CPU al RTOS.
3. **Notificación ISR:** El controlador DMA mueve los bytes de forma autónoma. Al finalizar, dispara la interrupción de *Transfer Complete*. El *Callback* ejecuta `xSemaphoreGiveFromISR()`, destrabando el semáforo.
4. **Liberación Segura:** Al destrabarse el semáforo, el *Gatekeeper* avanza a la siguiente línea y ejecuta `vPortFree()`. Esto garantiza matemáticamente que la memoria RAM dinámica nunca será alterada ni destruida mientras el controlador DMA se encuentra leyéndola.

## 3. Conclusiones

La implementación de la transmisión mediante DMA permitió extraer las siguientes conclusiones:

1. **Offloading Total de CPU:** El uso de DMA reduce drásticamente las interrupciones generadas (de una por byte a una por trama entera). Esto minimiza el *overhead* del Kernel de FreeRTOS por cambios de contexto, logrando que el tiempo de CPU esté casi íntegramente dedicado a la ejecución de las tareas productoras.
2. **Escalabilidad y Abstracción:** El hecho de haber migrado el mecanismo subyacente del hardware (de Polling a IT, y de IT a DMA) sin alterar en absoluto la lógica de generación de datos de `Task A` y `Task B`, valida contundentemente la modularidad y el bajo acoplamiento del diseño implementado.
