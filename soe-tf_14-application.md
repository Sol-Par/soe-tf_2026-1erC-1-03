# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 14: Device Driver - Recepción (Unknown Length & Memory Pool & DMA & Gatekeeper Task)**

## 1. Introducción:

En esta actividad se combina la recepción de **longitud desconocida** desarrollada en la Actividad 11 con el esquema de **DMA y Memory Pool** de la Actividad 13.

La arquitectura general se mantiene sin cambios: las tareas reservan sus buffers, generan requests y la Gatekeeper centraliza las operaciones sobre el I²C. La diferencia está en que ahora la longitud no se conoce previamente, por lo que la recepción DMA debe repetirse hasta detectar el final del mensaje.

---

## 2. Desarrollo:

### 2.1. Unknown Length mediante DMA

El `request` conserva la información necesaria para controlar una recepción de longitud variable:

```c
typedef struct {
    uint16_t address;
    uint16_t max_length;
    uint16_t received_length;
    TaskHandle_t requester_task;
    uint8_t *p_rx;
} i2c_rx_req_t;
````

Al igual que en la Actividad 11, la Gatekeeper recibe los datos **de a un byte**, hasta encontrar el carácter de terminación `'\0'` o alcanzar `max_length`.

La diferencia es que cada lectura utiliza ahora DMA:

```c
while (i2c_rx_req->received_length < i2c_rx_req->max_length)
{
    i2c_mem_read(&hi2c2,
                 0xA0,
                 i2c_rx_req->address + i2c_rx_req->received_length,
                 I2C_MEMADD_SIZE_16BIT,
                 &i2c_rx_req->p_rx[i2c_rx_req->received_length],
                 1);

    xSemaphoreTake(h_i2c_rx_sem, portMAX_DELAY);

    if (i2c_rx_req->p_rx[i2c_rx_req->received_length] == '\0')
    {
        break;
    }

    i2c_rx_req->received_length++;
}
```

El driver continúa utilizando:

```c
HAL_I2C_Mem_Read_DMA(...)
```

y el callback de finalización libera el semáforo para permitir que la Gatekeeper avance con el siguiente byte.

### 2.2. DMA con longitud desconocida

Esta implementación combina correctamente ambos mecanismos, aunque aparece un compromiso importante.

DMA obtiene su mayor ventaja cuando puede transferir **bloques completos de datos** con mínima intervención de CPU. Al desconocerse la longitud, en esta solución se inicia una transferencia DMA independiente por cada byte.

| Actividad | Transferencia  | Longitud    | Buffer          |
| --------- | -------------- | ----------- | --------------- |
| 11        | Interrupciones | Unknown     | Memory Pool     |
| 13        | DMA            | Known       | Memory Pool     |
| **14**    | **DMA**        | **Unknown** | **Memory Pool** |

Por lo tanto, la Actividad 14 mantiene la flexibilidad de detectar dinámicamente el final del mensaje, pero requiere mayor cantidad de inicializaciones de DMA, interrupciones de finalización y sincronizaciones con la Gatekeeper que una transferencia DMA de longitud conocida.

Una alternativa más eficiente para mensajes de mayor tamaño sería recibir bloques de varios bytes mediante DMA y buscar posteriormente el delimitador dentro del bloque.

### 2.3. Gestión del buffer

El Memory Pool funciona de la misma manera que en la Actividad 13: el buffer se reserva antes de encolar el request y permanece válido durante toda la recepción.

```c
i2c_rx_req.p_rx = pvPortMalloc(i2c_rx_req.max_length);
```

Una vez detectado el final del mensaje y recibida la notificación de la Gatekeeper, la tarea procesa los datos y libera el bloque:

```c
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

/* Procesamiento del mensaje */

vPortFree(i2c_rx_req.p_rx);
```

`max_length` funciona además como protección frente a un mensaje sin terminador. Para una implementación robusta, si el buffer será utilizado como string también debe garantizarse un espacio para `'\0'` en caso de alcanzar la longitud máxima.

---

## 3. Conclusiones:

1. **Longitud desconocida:** se combina la detección dinámica del final del mensaje con la recepción mediante DMA, manteniendo un límite máximo para proteger el buffer.

2. **Uso de CPU:** DMA continúa evitando que la CPU copie directamente los datos, aunque su ventaja se reduce al realizar transferencias independientes de sólo un byte.

3. **Gestión de memoria:** el Memory Pool permite conservar buffers únicamente durante cada recepción, cuya vida útil se mantiene hasta que la Gatekeeper confirma que la operación completa finalizó.
