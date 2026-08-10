# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 07: Device Driver - Recepción (Known Length & Polling & Gatekeeper Task)**

## 1. Introducción:

En esta actividad se implementó la primera etapa del driver de **recepción**, utilizando una memoria externa conectada mediante I²C.

En recepción es necesario considerar dónde almacenar los datos recibidos y cómo sincronizar el acceso al periférico con las distintas tareas del sistema. En este caso, aunque los datos no llegan espontáneamente sino que son solicitados por el microcontrolador, varias tareas pueden requerir lecturas sobre el mismo dispositivo.

Para centralizar el acceso al I²C se utilizó una **Gatekeeper Task**. Las tareas de aplicación generan solicitudes de lectura y las envían mediante una cola. Luego permanecen bloqueadas hasta que la Gatekeeper completa la operación y las notifica.

En esta primera implementación la lectura física se realiza mediante **Polling**, utilizando `HAL_I2C_Mem_Read()`.

---

## 2. Desarrollo:

### 2.1. Solicitudes de recepción

Cada operación se representa mediante una estructura que contiene la dirección de memoria, la cantidad de bytes, la tarea solicitante y un buffer donde almacenar los datos recibidos.

```c
typedef struct {
    uint16_t address;
    uint8_t length;
    TaskHandle_t requester_task;
    uint8_t rx_buffer[MAX_MSG_LEN];
} i2c_rx_req_t;
````

Las tareas `Task A` y `Task B` generan sus propias solicitudes y envían un puntero a la Gatekeeper mediante una cola:

```c
i2c_rx_req_t *p_req = &i2c_rx_req;

xQueueSend(h_i2c_queue, &p_req, pdMS_TO_TICKS(10));
```

De esta manera no es necesario copiar toda la estructura y su buffer dentro de la cola.

### 2.2. Bloqueo de las tareas solicitantes

Luego de enviar la solicitud, la tarea se bloquea esperando que la lectura termine:

```c
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
```

Esto evita que la tarea realice una espera activa. Mientras la operación está siendo procesada, FreeRTOS puede ejecutar otras tareas.

Una vez recibida la notificación, los datos almacenados en `rx_buffer` pueden ser procesados normalmente.

### 2.3. Gatekeeper Task

La Gatekeeper es la única tarea que accede directamente al periférico I²C.

```c
for (;;)
{
    if (xQueueReceive(h_i2c_queue,
                      &i2c_rx_req,
                      portMAX_DELAY) == pdPASS)
    {
        i2c_mem_read(&hi2c2,
                     0xA0,
                     i2c_rx_req->address,
                     I2C_MEMADD_SIZE_16BIT,
                     i2c_rx_req->rx_buffer,
                     i2c_rx_req->length,
                     HAL_MAX_DELAY);

        xTaskNotifyGive(i2c_rx_req->requester_task);
    }
}
```

La cola permite serializar las solicitudes de las diferentes tareas, evitando accesos simultáneos al bus.

Al terminar la lectura, la Gatekeeper utiliza:

```c
xTaskNotifyGive(i2c_rx_req->requester_task);
```

para desbloquear específicamente a la tarea que realizó la solicitud.

### 2.4. Lectura mediante Polling

El driver utiliza la función bloqueante de la HAL:

```c
HAL_I2C_Mem_Read(
    hi2c,
    device_address,
    memory_address,
    memory_add_size,
    p_rx,
    size,
    timeout);
```

Por lo tanto, mientras las tareas solicitantes permanecen correctamente bloqueadas por FreeRTOS, la **Gatekeeper permanece ocupada durante toda la transferencia I²C**.

Esta limitación en el uso de CPU será el principal punto a mejorar al evolucionar el driver hacia una implementación basada en interrupciones.

---

## 3. Conclusiones:

1. **Uso de CPU:** las tareas solicitantes pueden permanecer bloqueadas mientras esperan los datos, aunque la Gatekeeper todavía realiza la recepción mediante Polling.

2. **Sincronización:** la cola y la Gatekeeper permiten serializar las solicitudes de distintas tareas y evitar accesos concurrentes al periférico I²C.

3. **Protección de memoria:** cada solicitud mantiene su propio buffer de recepción, que solamente es utilizado por la tarea una vez que la Gatekeeper notifica que la operación terminó.

```
```

...
