# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 13: Device Driver - Recepción (Known Length & Memory Pool & DMA & Gatekeeper Task)**

## 1. Introducción:

En esta actividad se combina la recepción mediante **DMA** desarrollada en la Actividad 12 con el esquema de **Memory Pool** utilizado anteriormente.

La arquitectura general no presenta cambios: la Gatekeeper continúa iniciando la transferencia mediante DMA y permanece bloqueada hasta recibir la señal de finalización. La modificación se concentra únicamente en la gestión del buffer de recepción, que pasa de ser fijo a reservarse dinámicamente desde el heap de FreeRTOS.

De esta forma se combinan las dos mejoras desarrolladas previamente: **menor intervención de CPU mediante DMA** y **mayor flexibilidad en la administración de memoria**.

---

## 2. Desarrollo:

### 2.1. Incorporación del Memory Pool

Al igual que en las actividades anteriores que utilizaron Memory Pool (Actividad 08 y Actividad 10), el buffer fijo se reemplaza por un puntero:

```c
typedef struct {
    uint16_t address;
    uint16_t length;
    TaskHandle_t requester_task;
    uint8_t *p_rx;
} i2c_rx_req_t;
````

Antes de generar una solicitud, cada tarea reserva el espacio necesario:

```c
i2c_rx_req.p_rx = pvPortMalloc(MAX_MSG_LEN);

if (i2c_rx_req.p_rx != NULL)
{
    ...
}
```

En esta implementación se definió:

```c
#define MAX_MSG_LEN 200
```

La Gatekeeper utiliza directamente este bloque como destino de la transferencia DMA:

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

### 2.2. Vida útil del buffer

Al tratarse de una transferencia asíncrona, el bloque de memoria debe permanecer válido mientras el DMA continúa escribiendo sobre él.

Por este motivo, la tarea libera el buffer únicamente después de recibir la notificación de finalización:

```c
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

/* Procesamiento de los datos recibidos */

vPortFree(i2c_rx_req.p_rx);
```

También se mantiene la liberación del buffer si el request no puede ser encolado:

```c
if (xQueueSend(h_i2c_queue, &p_req, pdMS_TO_TICKS(10)) != pdPASS)
{
    vPortFree(i2c_rx_req.p_rx);
}
```

El mecanismo de DMA, el callback de finalización y la sincronización mediante semáforo permanecen iguales a los desarrollados en la Actividad 12.

### 2.3. Evolución del driver

Esta actividad completa la combinación entre los distintos mecanismos estudiados:

| Actividad | Transferencia  | Buffer          |
| --------- | -------------- | --------------- |
| 07        | Polling        | Fijo            |
| 08        | Polling        | Memory Pool     |
| 09        | Interrupciones | Fijo            |
| 10        | Interrupciones | Memory Pool     |
| 12        | DMA            | Fijo            |
| **13**    | **DMA**        | **Memory Pool** |

La principal ventaja es combinar el bajo uso de CPU de DMA con una utilización más flexible de memoria. Como contrapartida, aumenta la responsabilidad del software sobre la vida útil de cada bloque, ya que una liberación prematura podría invalidar el buffer mientras el DMA todavía lo está utilizando.

---

## 3. Conclusiones:

1. **Uso de CPU:** se mantienen los beneficios obtenidos mediante DMA, realizando la transferencia hacia memoria con mínima intervención del procesador.

2. **Gestión de memoria:** los buffers se reservan únicamente cuando se necesita realizar una recepción y se liberan una vez procesados los datos.

3. **Sincronización y protección:** la memoria reservada permanece válida durante toda la transferencia DMA y sólo es liberada después de confirmar su finalización.
   
