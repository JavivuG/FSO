# Fundamentos de Sistemas Operativos

Un minishell con `fork`/`exec` y un decodificador de imágenes ASCII concurrente con hilos POSIX y semáforos, escritos en C.

> Asignatura **Fundamentos de Sistemas Operativos** · 2º curso, 1º cuatrimestre · curso 2022-2023 · Grado en Ingeniería Informática, Universidad de Valladolid.

## Qué hace

- **Entrega 1 — minishell:** muestra el prompt `minishell>> `, lee una orden de la entrada estándar y la ejecuta en un proceso hijo. Solo admite `ls` y `cat` con un parámetro como máximo; avisa de que ignora el resto de parámetros y termina con `salir`.
- **Entrega 2 — decodificador concurrente:** lee un fichero codificado (una línea por carácter con `fila,columna,código`), reconstruye la imagen ASCII y la escribe en un fichero de salida y por pantalla.
- La entrega 2 reparte el trabajo entre un hilo lector, N hilos decodificadores y un hilo dibujante.
- Descarta las líneas fuera de rango y los caracteres no imprimibles, y escribe en un fichero de estadísticas cuántas líneas y caracteres ha procesado cada hilo.

## Tecnologías

- C (GCC), llamadas al sistema POSIX (`read`, `write`, `fork`, `execvp`, `wait`)
- `pthread` y semáforos POSIX (`semaphore.h`)

## Aspectos técnicos destacados

- **Lectura byte a byte con `read(0, ...)`** en el minishell, en lugar de `stdio`, hasta encontrar el salto de línea.
- **Modelo productor-consumidor con buffer circular** de tamaño configurable: el hilo lector produce y los decodificadores consumen, sincronizados con los semáforos `hay_espacio` y `hay_dato`.
- **Reparto de columnas entre hilos:** cada decodificador atiende un rango `[col_ini, col_fin)` y los sobrantes se reparten entre los primeros hilos. Un decodificador que lee un dato de otra columna lo devuelve sin consumirlo.
- **Marca de fin de datos:** el lector escribe `fil = -1` en el buffer para que todos los decodificadores terminen.
- **Lista enlazada compartida** protegida por `mutex_lista`. El dibujante espera en `despierta_dibu` cada carácter nuevo y compone la imagen en una matriz dinámica.
- **Escritura exclusiva** de las estadísticas en el fichero de datos mediante el semáforo `escritor`.

## Estructura del repositorio

```
FSO/
├── Entrega1/
│   └── p1-fso.c        # minishell (ls / cat)
└── Entrega2/
    ├── p2-fso.c        # decodificador concurrente
    ├── csmile.txt      # imagen codificada de ejemplo
    └── cdarth.txt      # imagen codificada de ejemplo
```

## Requisitos

- Linux o cualquier sistema POSIX
- GCC con soporte para `pthread`

## Cómo compilar y ejecutar

Entrega 1:

```bash
gcc -o p1 Entrega1/p1-fso.c
./p1
minishell>> ls -l
minishell>> salir
```

Entrega 2. Se ejecuta con cinco parámetros: fichero codificado, fichero de salida, tamaño del buffer, número de hilos decodificadores y fichero de estadísticas.

```bash
gcc -pthread -o p2 Entrega2/p2-fso.c
./p2 Entrega2/csmile.txt salida.txt 5 3 datos.txt
```

Ambos comandos se han probado con GCC en Linux: la entrega 2 reconstruye la imagen de `csmile.txt` y genera las estadísticas de cada hilo.

## Autoría

Trabajo en grupo (grupo de laboratorio X2). Según la cabecera de los ficheros fuente, sus autores son Javier García González y Diego Stefano Wortman Rueda.

---

Forma parte de [UVa-Projects](https://github.com/JavivuG/UVa-Projects).
