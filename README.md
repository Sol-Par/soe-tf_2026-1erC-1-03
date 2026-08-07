# FIUBA - Electrónica - Sistemas Operativos Embebidos

## Trabajo Final - Device Driver de FreeRTOS

![Banner](Recursos/banner.png)

### 2026-1erC - 1-03

### Integrantes del grupo:

| Padrón | Apellidos, Nombres | Fecha | Deadline |
| :----- | :--------------------- | :------: | :-------: |
| 110901 | Chechko, Víctor Nicolás | - | 12 AGO 26 |
| 109308 | Marconi Casares, Lourdes | - | 12 AGO 26 |
| 109324 | Solari Parravicini, Facundo | - | 12 AGO 26 |

---

### Cuadro Comparativo

#### Transmisión

| Actividad | Tipo            | Tiempo Bloqueante [μS] | Tiempo Total [μS] | Eficiencia CPU | Manejo de memoria    |
| :-------: | :-------------: | :--------------------: | :---------------: | :------------: | :------------------: |
| 01        | Polling         | -                      | -                 | Baja           | Inseguro             |
| 02        | Polling         | -                      | -                 | Baja           | Seguro (memory pool) |
| 03        | Interrupt       | -                      | -                 | Media          | Inseguro             |
| 04        | Interrupt       | -                      | -                 | Media          | Seguro (memory pool) |
| 05        | DMA             | -                      | -                 | Alta           | Inseguro             |
| 06        | DMA             | -                      | -                 | Alta           | Seguro (memory pool) |

<br>

#### Recepción

| Actividad | Tipo                     | Tiempo Bloqueante [μS] | Tiempo Total [μS] | Eficiencia CPU | Manejo de memoria    |
| :-------: | :----------------------: | :--------------------: | :---------------: | :------------: | :------------------: |
| 07        | Polling Known Length     | -                      | -                 | Baja           | Inseguro             |
| 08        | Polling Known Length     | -                      | -                 | Baja           | Seguro (memory pool) |
| 09        | Interrupt Known Length   | -                      | -                 | Media          | Inseguro             |
| 10        | Interrupt Known Length   | -                      | -                 | Media          | Seguro (memory pool) |
| 11        | Interrupt Unknown Length | -                      | -                 | Media          | Seguro (memory pool) |
| 12        | DMA Known Length         | -                      | -                 | Alta           | Inseguro             |
| 13        | DMA Known Length         | -                      | -                 | Alta           | Seguro (memory pool) |
| 14        | DMA Unknown Length       | -                      | -                 | Alta           | Seguro (memory pool) |

---

### Obtención de Tiempos

Para medir el tiempo transcurrido entre dos instrucciones o eventos, se realizó el siguiente procedimiento:

```c
// Asegurarse de incluir el archivo de DWT (Data Watchpoint and Trace).
#include "dwt.h"

# Reiniciar el contador en el punto de inicio.
cycle_counter_reset();

// (lista de instrucciones o eventos a medir)
// ...

// Obtener el contador en el punto final.
uint32_t time = cycle_counter_get_time_us();

// Imprimir el tiempo por consola (o bien colocar un breakpoint
// y leer el valor en 'Expressions').
LOGGER_INFO("Tiempo = %lu us", time);
```

---