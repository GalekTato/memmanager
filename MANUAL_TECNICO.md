# Manual Técnico - Simulador de Administración de Memoria Virtual

## 1. Introducción
Este simulador es una herramienta educativa avanzada diseñada para demostrar conceptos fundamentales de sistemas operativos, específicamente en el área de gestión de memoria virtual. Implementa técnicas modernas como la paginación por demanda, algoritmos de reemplazo adaptativos y comunicación en red.

## 2. Arquitectura del Sistema
El sistema está dividido en tres capas principales:

### 2.1. Capa de Lógica de Memoria (`include/`)
- **MemoryManager**: El núcleo del simulador. Gestiona la memoria virtual mediante un mapa de bits y controla la carga/descarga de páginas en la RAM física simulada.
- **ReplacementPolicy (Interfaz)**: Define el contrato para las políticas de reemplazo de páginas.
- **HybridPolicy (Algoritmo CAR)**: Implementa el algoritmo *Clock with Adaptive Replacement*. Este algoritmo utiliza cuatro colas:
    - `T1` y `T2`: Páginas en RAM (recientes y frecuentes).
    - `B1` y `B2`: Historiales "fantasma" para páginas recientemente expulsadas, permitiendo que el algoritmo se adapte dinámicamente a la carga de trabajo.
- **Process**: Clase que encapsula la información de un proceso, incluyendo su Tabla de Páginas local y su cadena de referencias.

### 2.2. Capa de Despacho (`Dispatcher`)
- **Round Robin**: Utiliza una cola de listos (`readyQueue_`) y un *quantum* configurable. Cada proceso ejecuta una cantidad limitada de referencias antes de ser rotado al final de la cola, permitiendo una simulación multihilo cooperativa.

### 2.3. Capa de Interfaz y Red (`gui/` y `network/`)
- **Qt6 GUI**: Proporciona una visualización en tiempo real del estado de la memoria, el log de eventos y el control paso a paso.
- **TCP Sockets**: Implementa un modelo servidor multihilo que procesa peticiones de clientes remotos que envían procesos para ser simulados.

## 3. Detalles de Implementación: Algoritmo CAR
El algoritmo CAR combina las ventajas de LRU (recencia) y LFU (frecuencia) mediante un mecanismo de reloj.
- **Adaptabilidad**: El parámetro `p` se ajusta automáticamente basándose en los aciertos en los historiales fantasma (`B1` y `B2`).
- **Resiliencia**: Es resistente al problema de "escaneo de memoria" que degrada el rendimiento de LRU puro.

## 4. Estructura de Archivos
```text
memmanager/
├── include/           # Lógica del motor de memoria
│   ├── MemoryManager.hpp
│   ├── HybridPolicy.hpp
│   └── Dispatcher.hpp
├── gui/              # Interfaz gráfica (Qt6)
│   ├── MainWindow.cpp
│   └── main_gui.cpp
├── network/          # Módulo de red
│   ├── Server.cpp
│   ├── Client.cpp
│   └── server_main.cpp
└── tests/            # Pruebas unitarias
```

## 5. Requisitos Técnicos
- **Compilador**: Soporte para C++20 (GCC 13+).
- **Librerías**: Qt 6.4+.
- **Sistema Operativo**: Linux (Probado en Arch y Ubuntu).

---

# Guía de Pruebas y Resultados Esperados

## 1. Preparación del Entorno
Asegúrese de que el script de construcción sea ejecutable y compile el proyecto:
```bash
chmod +x build_all.sh
./build_all.sh
```

## 2. Pruebas en Modo Local (GUI)
### Procedimiento:
1. Inicie la aplicación: `./gui/memmanager_gui`
2. Configure los parámetros:
   - **Quantum**: 5
   - **Ciclo Reloj**: 3
3. Cree un proceso de prueba:
   - **PID**: 1
   - **Páginas**: 10
   - **Referencias**: `0,1,2,0,1,3,4,0,1`
4. Use el botón **"Paso"** para observar la carga de páginas.

### Resultados Esperados:
- Las páginas `0, 1, 2` causarán **Page Faults** iniciales (indicados en el log).
- El segundo acceso a `0` y `1` debe ser un **Hit** (ya están en RAM).
- La visualización de la "Memoria Virtual Global" debe mostrar los bloques ocupados por el PID 1.

## 3. Pruebas en Modo Servidor (Red)
Este modo permite enviar procesos desde diferentes terminales o máquinas.

### Paso 1: Iniciar el Servidor
```bash
cd network
./memserver 9000
```

### Paso 2: Enviar Procesos desde Clientes
Abra otra terminal y envíe varios procesos:
```bash
cd network
# Cliente 1
./memclient 127.0.0.1 9000 101 15 0,1,2,3,4,0,1,2
# Cliente 2 (Simultáneo)
./memclient 127.0.0.1 9000 102 10 5,6,5,6,7,8
```

### Resultados Esperados:
- El servidor mostrará en la consola la llegada de cada proceso y su ejecución intercalada (Round Robin).
- El cliente recibirá un mensaje de finalización con el reporte:
    - **Page Faults**: Conteo de fallos (ej. 5 fallos para el primer proceso).
    - **Waiting Time**: Tiempo que el proceso estuvo en la cola de listos.
    - **Finish Time**: Tiempo global de finalización.

## 4. Escenarios de Validación
- **Thrashing**: Si se cargan muchos procesos con muchas páginas, el algoritmo CAR debería mostrar cómo expulsa páginas de forma inteligente (frecuencia vs recencia) para estabilizar el sistema.
- **Segmentación**: Intentar acceder a una página mayor al número de páginas asignadas (`segfault`) debe ser manejado por el sistema sin crashear.
