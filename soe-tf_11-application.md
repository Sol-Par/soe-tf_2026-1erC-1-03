# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 11: Device Driver - Recepción (Unknown Length & Memory Pool & Interrupt & Gatekeeper Task)**

## 1. Introducción:

En esta actividad se mantiene la arquitectura de recepción mediante **interrupciones, Gatekeeper Task y Memory Pool** desarrollada anteriormente.

La principal diferencia es que ahora la **longitud del mensaje es desconocida a priori**. Por lo tanto, ya no es posible iniciar directamente una recepción indicando la cantidad total de bytes esperados.

Para resolverlo, la Gatekeeper realiza lecturas sucesivas de un byte y finaliza la operación cuando detecta el carácter `'\n'` o cuando se alcanza una longitud máxima definida como protección.

---

## 2. Desarrollo:

### 2.1. Recepción de longitud desconocida

El `request` incorpora dos campos nuevos para controlar la recepción:

```c
typedef struct {
    uint16_t address;
    uint8_t max_length;
    uint8_t received_length;
    TaskHandle_t requester_task;
    uint8_t *p_rx;
} i2c_rx_req_t;
````

* `max_length` establece el límite máximo permitido para el mensaje.
* `received_length` indica cuántos bytes fueron recibidos hasta el momento.

El resto del mecanismo de Queue, Memory Pool y Task Notification se mantiene respecto de las actividades anteriores.

### 2.2. Detección del final del mensaje

Como no se conoce la longitud, la Gatekeeper solicita los datos **de a un byte**:

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

    if (i2c_rx_req->p_rx[i2c_rx_req->received_length] == '\n')
    {
        break;
    }

    i2c_rx_req->received_length++;
}
```

Cada lectura continúa utilizando `HAL_I2C_Mem_Read_IT()`. La Gatekeeper inicia la operación y permanece bloqueada hasta que el callback de recepción libera nuevamente el semáforo.

El carácter `'\n'` funciona como **delimitador de fin de mensaje**. Si éste no aparece, `max_length` evita que la recepción continúe indefinidamente y protege el tamaño del buffer.

### 2.3. Comparación con longitud conocida

| Known Length - Actividad 10                | Unknown Length - Actividad 11                 |
| ------------------------------------------ | --------------------------------------------- |
| La cantidad de bytes se conoce previamente | La longitud se determina durante la recepción |
| Se inicia una única transferencia          | Se realizan lecturas sucesivas                |
| Menor cantidad de interrupciones           | Mayor intervención de CPU                     |
| Implementación más eficiente               | Mayor flexibilidad                            |
| No necesita detectar delimitador           | Finaliza con `'\n'` o `max_length`            |

La principal ventaja es poder procesar mensajes cuyo tamaño no se conoce de antemano.

Como contrapartida, realizar una nueva operación I²C por cada byte introduce más interrupciones, sincronizaciones y transacciones, haciendo esta solución menos eficiente que una recepción de longitud conocida.

### 2.4. Límite del buffer

Al utilizar el contenido recibido posteriormente como string, debe reservarse espacio adicional para el terminador `'\0'`.

Si `max_length` representa la cantidad máxima de datos recibidos, el buffer debería contemplar:

```c
i2c_rx_req.p_rx = pvPortMalloc(i2c_rx_req.max_length + 1);
```

De esta manera, incluso si la recepción alcanza exactamente `max_length`, puede agregarse de forma segura:

```c
i2c_rx_req.p_rx[i2c_rx_req.received_length] = '\0';
```

sin escribir fuera del bloque reservado.

---

## 3. Conclusiones:

1. **Longitud desconocida:** el driver ya no depende de conocer previamente el tamaño del mensaje, utilizando un delimitador y una longitud máxima para determinar el final de la recepción.

2. **Uso de CPU:** la recepción continúa siendo no bloqueante entre eventos, pero requiere una operación e interrupción por cada byte, aumentando el overhead respecto de una transferencia de longitud conocida.

3. **Protección de memoria:** `max_length` limita la cantidad máxima de datos recibidos y el buffer debe reservar espacio adicional para el terminador utilizado al procesar el mensaje como string.
