Trabajo final de la materia Sistemas de Control, año 2025, 5to año Ing. Electrónica (UTN-FRSFCO).

El repositorio contiene el firmware utilizado para el control PID de un motor de corriente continua *Dunkermotoren GR63x55*. 

El algoritmo PID implementado en código se obtuvo a través de la caracterización del motor. Se encontrar los parámetros del motor de forma experimental, y se calculó su función de transferencia. Luego, se simuló su respuesta, y se diseño un controlador PID continuo de lazo cerrado. Finalmente, se discretizó y se implemento en un microcontrolador RP2040.

La implementación del algoritmo se realizó en el lenguaje de programación C, utilizando la capa de abstracción de hardware Pico-SDK.