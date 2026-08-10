# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 10: Device Driver - Recepción (Known Length & Memory Pool & Interrupt & Gatekeeper Task)**

## 1. Introducción:

En esta actividad se combina la recepción mediante **interrupciones** desarrollada en la Actividad 09 con el esquema de **Memory Pool** utilizado previamente en la Actividad 08.

La arquitectura de recepción no cambia: las tareas envían solicitudes mediante una Queue, la Gatekeeper inicia la transferencia I²C mediante interrupciones y permanece bloqueada hasta recibir el semáforo de finalización.

La modificación se concentra nuevamente en la gestión del buffer de recepción, que pasa de ser fijo a reservarse dinámicamente desde el heap de FreeRTOS.

---

## 2. Desarrollo:

### 2.1. Incorporación del Memory Pool

Al igual que en la Actividad 08, el buffer deja de estar contenido directamente dentro del `request`:

```c
typedef struct {
    uint16_t address;
    uint8_t length;
    TaskHandle_t requester_task;
    uint8_t *p_rx;
} i2c_rx_req_t;
````

Cada tarea reserva el buffer antes de encolar la solicitud:

```c
i2c_rx_req.p_rx = pvPortMalloc(MAX_MSG_LEN);

if (i2c_rx_req.p_rx != NULL)
{
    ...
}
```

La diferencia respecto de la Actividad 08 es que ahora este buffer se utiliza en una **transferencia asíncrona por interrupciones**.

### 2.2. Memory Pool sobre recepción por interrupciones

La Gatekeeper utiliza directamente el buffer reservado para iniciar la lectura:

```c
i2c_mem_read(&hi2c2,
             0xA0,
             i2c_rx_req->address,
             I2C_MEMADD_SIZE_16BIT,
             i2c_rx_req->p_rx,
             i2c_rx_req->length);

xSemaphoreTake(h_i2c_rx_sem, portMAX_DELAY);

xTaskNotifyGive(i2c_rx_req->requester_task);
```

Como la transferencia continúa ejecutándose luego de que `HAL_I2C_Mem_Read_IT()` retorna, el buffer debe permanecer válido hasta que la ISR indique que la recepción finalizó.

Por este motivo, la tarea solicitante libera la memoria únicamente después de recibir su notificación:

```c
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

/* Procesamiento de los datos recibidos */

vPortFree(i2c_rx_req.p_rx);
```

Así, la propiedad del buffer queda asociada a la operación completa de recepción y se evita liberarlo mientras el periférico todavía puede estar escribiendo sobre él.

### 2.3. Evolución respecto de las actividades anteriores

Esta actividad puede interpretarse como la combinación de las dos mejoras desarrolladas previamente:

| Actividad | Transferencia      | Buffer          |
| --------- | ------------------ | --------------- |
| 07        | Polling            | Fijo            |
| 08        | Polling            | Memory Pool     |
| 09        | Interrupciones     | Fijo            |
| **10**    | **Interrupciones** | **Memory Pool** |

De esta manera se conservan simultáneamente:

* El mejor aprovechamiento de CPU obtenido mediante interrupciones.
* La flexibilidad en la administración de buffers obtenida mediante memoria dinámica.

Como contrapartida, el driver debe controlar correctamente la vida útil de los bloques reservados, evitando tanto **memory leaks** como liberaciones prematuras durante una transferencia asíncrona.

---

## 3. Conclusiones:

1. **Uso de CPU:** se mantienen los beneficios de la recepción mediante interrupciones, permitiendo que la Gatekeeper permanezca bloqueada mientras avanza la transferencia.

2. **Gestión de memoria:** se incorpora el mismo esquema de Memory Pool analizado en la Actividad 08, evitando mantener buffers fijos permanentemente asignados.

3. **Sincronización y protección:** al tratarse de una transferencia asíncrona, el buffer debe conservarse hasta recibir la confirmación de finalización, garantizando que la memoria no sea liberada mientras el periférico todavía la está utilizando.
