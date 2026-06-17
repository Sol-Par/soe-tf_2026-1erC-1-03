# Reporte de Desarrollo - Trabajo Práctico Final: Device Drivers en RTOS
**Actividad 01: Device Driver - Transmisión (Polling & Gatekeeper Task)**

## 1. Introducción
El presente documento detalla el diseño e implementación de la primera etapa del trabajo práctico final, cuyo objetivo es el desarrollo de un *Device Driver* para un Sistema Operativo en Tiempo Real (FreeRTOS). El hardware objetivo es una placa de desarrollo STM32 Nucleo-F103RB programada mediante STM32CubeIDE.

Para esta actividad (Actividad 01), se planteó la implementación de un canal de transmisión de datos hacia un periférico utilizando el método de **Polling** a nivel de hardware, combinado con el patrón de diseño **Gatekeeper Task** a nivel del RTOS. El periférico seleccionado para validar este comportamiento fue un Display LCD conectado mediante el bus **I2C** (a través de un expansor IO PCF8574).

El propósito principal de esta arquitectura es garantizar que el acceso al periférico (recurso compartido) sea seguro y libre de condiciones de carrera cuando múltiples tareas de la aplicación intentan transmitir datos simultáneamente.

## 2. Desarrollo

El desarrollo se centró en la creación de una arquitectura de paso de mensajes, donde las tareas de aplicación delegan el control del hardware a una única tarea administradora.

### 2.1. Recursos Creados y Modificados
Para soportar la comunicación entre las tareas y el driver, se implementaron los siguientes recursos en el sistema:

* **Estructura de Datos (`display_msg_t`):** Se definió en `app.h` un tipo de dato personalizado que encapsula la información que las tareas desean enviar al display. Esta estructura incluye las coordenadas de destino (fila `y`, columna `x`) y un buffer de texto (`text`) formateado.
* **Cola de Mensajes (`h_display_queue`):** Se instanció una *Queue* de FreeRTOS con capacidad para 8 mensajes. Este es el mecanismo principal de sincronización y transferencia de datos entre las tareas productoras y la tarea consumidora.
* **Resolución de Conflictos de Compilación:** Se corrigió una discrepancia de identificadores de tipo ("conflicting type qualifiers") en el contador de *ticks* del sistema (`g_app_tick_cnt`), agregando el calificador `volatile` en `app.c` para asegurar su correcta evaluación durante las interrupciones.

### 2.2. Implementación de Tareas

El flujo del programa se distribuyó en tres hilos de ejecución principales:

#### Tareas de Aplicación (Task A y Task B)
Los archivos `task_a.c` y `task_b.c` fueron modificados para actuar como **productores** de información. 
* **Task A:** Incrementa su contador interno, formatea un mensaje con `snprintf` indicando su estado, y lo envía a la cola para ser impreso en la *Línea 1* del display. Luego, cede la CPU bloqueándose por 250 milisegundos.
* **Task B:** Realiza una acción idéntica, pero dirige su mensaje a la *Línea 2* del display y se bloquea por un tiempo mayor (2.5 segundos), evidenciando la capacidad del RTOS para planificar tareas con distintas frecuencias sin bloquear el sistema.

#### Gatekeeper Task
Se creó la tarea delegada `gatekeeper_task`. Sus responsabilidades exclusivas son:
1.  Inicializar el hardware del display al arrancar.
2.  Bloquearse indefinidamente (`portMAX_DELAY`) esperando la llegada de datos en la cola `h_display_queue`, cediendo el tiempo de CPU al resto del sistema mientras no hay actividad.
3.  Al recibir un mensaje, extraer las coordenadas y el texto, e invocar las primitivas de escritura del periférico.

### 2.3. Driver del Periférico (Display I2C)
El código de bajo nivel para el control del display fue **importado de proyectos anteriores**. Se configuró para operar sobre el bus I2C utilizando las librerías HAL de STM32. Para cumplir con el requerimiento de la Actividad 01 (**Polling**), la transmisión de bytes hacia el expansor PCF8574 se realiza de forma bloqueante (`HAL_I2C_Master_Transmit`), donde la CPU retiene el control hasta que la transferencia del byte finaliza en el bus de hardware.

## 3. Conclusiones

La implementación de la Actividad 01 demostró de manera exitosa la viabilidad y robustez del patrón *Gatekeeper Task* en un entorno concurrente. 

A partir de los resultados observados, se concluye que:
1.  **Exclusión Mutua Segura:** Al restringir las llamadas a las funciones del HAL del I2C únicamente al Gatekeeper, se eliminó por completo el riesgo de que la Tarea A y la Tarea B corrompan los datos en la pantalla si intentan escribir al mismo tiempo.
2.  **Desacoplamiento:** Las tareas de aplicación ahora ignoran los detalles de implementación del hardware. Solo necesitan conocer el formato del mensaje y la interfaz de la cola, lo que facilita la mantenibilidad del código.
3.  **Base para futuras mejoras:** Si bien la transmisión por *Polling* es ineficiente desde el punto de vista del uso de la CPU durante el envío de cada byte, este diseño modular deja el sistema perfectamente preparado para las siguientes etapas evolutivas del trabajo práctico. Las próximas actividades (como la inclusión de *Memory Pools* para gestión dinámica de RAM, y el reemplazo del *Polling* por Interrupciones o *DMA*) podrán integrarse modificando únicamente el funcionamiento interno de la Gatekeeper Task, sin necesidad de alterar la lógica de las Tareas A y B.
