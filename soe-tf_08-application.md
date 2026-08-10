# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 08: Device Driver - Recepción (Known Length & Memory Pool & Polling & Gatekeeper Task)**

## 1. Introducción:

En esta actividad se mantiene la arquitectura desarrollada anteriormente en la Actividad 07: las tareas generan solicitudes de lectura, las envían mediante una cola y la **Gatekeeper Task** realiza el acceso al periférico I²C mediante Polling.

La modificación principal se encuentra en la **gestión de los buffers de recepción**. En la Actividad 07 cada `request` contenía internamente un buffer de tamaño fijo. En esta versión, el buffer se reserva dinámicamente desde el heap administrado por FreeRTOS mediante `pvPortMalloc()` y se libera con `vPortFree()` una vez procesados los datos.

De esta manera, la memoria utilizada para recepción existe solamente durante el tiempo necesario para completar cada operación.

---

## 2. Desarrollo:

### 2.1. Cambio en la estructura de recepción

Anteriormente el buffer formaba parte directamente de la estructura:

```c
uint8_t rx_buffer[MAX_MSG_LEN];
````

En esta versión se reemplazó por un puntero (` uint8_t *p_rx`), quedando la nueva estructura:

```c
typedef struct {
    uint16_t address;
    uint8_t length;
    TaskHandle_t requester_task;
    uint8_t *p_rx;
} i2c_rx_req_t;
```

Esto reduce el tamaño fijo de cada `request` y permite separar la información de control de la memoria utilizada para almacenar los datos.

| Actividad 07                        | Actividad 08                              |
| ----------------------------------- | ----------------------------------------- |
| Buffer embebido en el `request`     | Buffer reservado dinámicamente            |
| Memoria ocupada permanentemente     | Memoria ocupada sólo durante la recepción |
| Tamaño fijo                         | Mayor flexibilidad                        |
| Sin posibilidad de fallo de reserva | La reserva puede fallar                   |

### 2.2. Reserva dinámica del buffer

Antes de enviar una solicitud, cada tarea reserva memoria mediante:

```c
i2c_rx_req.p_rx = pvPortMalloc(MAX_MSG_LEN);

if (i2c_rx_req.p_rx != NULL)
{
    /* Enviar request */
}
else
{
    LOGGER_INFO("Task A: ¡Error! Out of Heap Memory");
}
```

La comprobación contra `NULL` es necesaria porque la memoria disponible en un sistema embebido es limitada. En esta configuración, FreeRTOS dispone de un heap de:

```c
#define configTOTAL_HEAP_SIZE ((size_t)4096)
```

Una vez reservado el buffer, el puntero continúa viajando dentro de la misma solicitud enviada a la Gatekeeper.

### 2.3. Uso del buffer por la Gatekeeper

La lógica general de la Gatekeeper no cambia. La diferencia es que ahora la función de lectura recibe el puntero al bloque reservado dinámicamente:

```c
i2c_mem_read(&hi2c2,
             0xA0,
             i2c_rx_req->address,
             I2C_MEMADD_SIZE_16BIT,
             i2c_rx_req->p_rx,
             i2c_rx_req->length,
             HAL_MAX_DELAY);
```

Al finalizar la lectura se mantiene el mecanismo de sincronización utilizado anteriormente:

```c
xTaskNotifyGive(i2c_rx_req->requester_task);
```

Por lo tanto, la tarea solicitante continúa bloqueada hasta que la Gatekeeper termina de escribir sobre el buffer.

### 2.4. Liberación de memoria

Una vez recibida la notificación y procesados los datos, la tarea libera el bloque:

```c
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

LOGGER_LOG("\n%s\n", i2c_rx_req.p_rx);

/* El buffer ya no es necesario */
vPortFree(i2c_rx_req.p_rx);
```

También se contempla el caso en que la cola esté llena. Si el `request` no puede ser enviado, el buffer reservado debe liberarse inmediatamente para evitar una pérdida de memoria:

```c
if (xQueueSend(h_i2c_queue, &p_req, pdMS_TO_TICKS(10)) != pdPASS)
{
    vPortFree(i2c_rx_req.p_rx);
    LOGGER_INFO("Task A: Cola llena, request descartada y memoria liberada");
}
```

Este punto es importante porque toda llamada exitosa a `pvPortMalloc()` debe tener asociada una posterior llamada a `vPortFree()`.

### 2.5. Ventajas y desventajas

La modificación afecta principalmente a la administración de memoria, el mecanismo de recepción continúa siendo Polling.

| Ventajas                                                             | Desventajas                                                      |
| -------------------------------------------------------------------- | ---------------------------------------------------------------- |
| Los buffers existen únicamente cuando son necesarios.                | `pvPortMalloc()` puede fallar si no existe memoria disponible.   |
| El `request` deja de contener un buffer fijo.                        | Se agrega el costo de reservar y liberar memoria.                |
| Permite manejar buffers de manera más flexible.                      | Una liberación incorrecta puede producir memory leaks.           |
| Facilita futuras arquitecturas con múltiples buffers en circulación. | El uso repetido de memoria dinámica puede generar fragmentación. |

---

## 3. Conclusiones:

1. **Uso de CPU:** esta actividad no modifica todavía el mecanismo de transferencia; la recepción continúa utilizando Polling, por lo que la mejora está enfocada principalmente en la gestión de memoria.

2. **Tiempos de bloqueo:** se conserva la sincronización mediante Queue + Gatekeeper + Task Notification, manteniendo a las tareas solicitantes bloqueadas hasta que sus datos estén disponibles.

3. **Protección de memoria:** los buffers pasan a reservarse dinámicamente y deben liberarse explícitamente una vez utilizados, permitiendo un uso más flexible de la memoria pero requiriendo controlar correctamente errores de reserva y posibles memory leaks.

```
```
