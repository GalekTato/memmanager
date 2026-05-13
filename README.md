# Simulador de Administración de Memoria Virtual

Este proyecto es un simulador avanzado de memoria virtual desarrollado en **C++20**. Implementa paginación por demanda, el algoritmo de reemplazo adaptativo **CAR (Clock with Adaptive Replacement)**, un **Despachador Round Robin** para la planificación de procesos, y una **Arquitectura Cliente-Servidor** mediante sockets TCP POSIX. Cuenta con una Interfaz Gráfica de Usuario (GUI) interactiva desarrollada con **Qt6**.

## Características Principales

*   **Paginación por Demanda:** Simulación del mapeo de direcciones lógicas a físicas mediante tablas de páginas y un mapa de bits global.
*   **Algoritmo CAR:** Política híbrida que supera a LRU al mantener un equilibrio dinámico entre la recencia y la frecuencia de acceso utilizando historiales fantasma.
*   **Despacho Round Robin:** Planificación equitativa de procesos basada en un *quantum* configurable, intercalando la ejecución de cadenas de referencia.
*   **Visualización en Tiempo Real (GUI):** Interfaz en Qt6 (estilo Catppuccin) que permite observar el estado de la memoria virtual, la tabla de páginas local y el log de aciertos/fallos paso a paso.
*   **Modo Red (Cliente-Servidor):** Extensión del simulador que ejecuta el gestor de memoria en un servidor TCP multihilo, permitiendo a clientes remotos enviar procesos y recibir asíncronamente sus métricas de ejecución.

## Requisitos del Sistema

*   **Sistema Operativo:** GNU/Linux (recomendado: Arch, Ubuntu 22.04+) o macOS.
*   **Compilador:** GCC 13.0+ o Clang 16.0+ (Soporte completo para C++20).
*   **Framework Gráfico:** Qt 6.4+ (`qt6-base-dev` y herramientas como `qmake6`).
*   **Herramientas de Construcción:** GNU Make.

## Instalación y Compilación

Clona este repositorio y utiliza el script de construcción proporcionado para compilar tanto la interfaz gráfica como los binarios de red.

```bash
git clone https://github.com/GalekTato/memmanager.git
cd memmanager

# Dar permisos de ejecución al script
chmod +x build_all.sh

# Compilar todos los módulos (GUI, Servidor y Cliente)
./build_all.sh
```

El script ejecutará `qmake6` y `make` en los subdirectorios correspondientes, generando los ejecutables en las carpetas `gui/` y `network/`.

## Uso del Simulador

El proyecto puede operarse de dos formas: localmente con interfaz gráfica o mediante la terminal en modo cliente-servidor.

### 1. Modo Interfaz Gráfica (Local)

Ejecuta el simulador interactivo:

```bash
./gui/memmanager_gui
```

**Flujo de uso básico:**
1. En el panel **PROCESOS**, configura el *Quantum* y el *Ciclo Reloj* (recomendados: 5 y 3).
2. Crea un proceso asignando un PID, el número de páginas requeridas, y una cadena de referencias separada por comas (ej. `0,1,2,0,3`). Presiona **Crear y Asignar**.
3. Haz clic en el botón **▶ Paso** para avanzar la simulación instrucción por instrucción.
4. Observa el estado en la "Memoria Virtual Global" y las traducciones en la "Tabla de Páginas".
5. Si ocurre un fallo de página, el sistema te alertará. Al finalizar todos los procesos, se mostrará un reporte de métricas.

### 2. Modo Cliente-Servidor (Red)

Este modo ejecuta la simulación en segundo plano. Necesitarás dos terminales.

**Terminal 1 (Servidor):**
Inicia el servidor especificando un puerto (por defecto 9000).

```bash
cd network
./memserver 9000
```

**Terminal 2 (Cliente):**
Envía la solicitud de un proceso al servidor. El formato es: `./memclient <host> <puerto> <pid> <num_páginas> <referencias>`

```bash
cd network
./memclient 127.0.0.1 9000 100 10 0,1,2,0,1,4,5
```

El cliente recibirá un `OK` y esperará a que el servidor (que procesa mediante Round Robin junto a otras peticiones concurrentes) termine la ráfaga, devolviendo un reporte con los fallos de página y tiempos.

## Documentación Técnica

El repositorio incluye un extenso reporte técnico en formato LaTeX (`reporte_proyecto.tex`) que abarca el marco teórico, diseño del modelo de clases (UML), explicaciones del algoritmo CAR, y análisis de resultados de pruebas (ej. mitigación de Thrashing). También se incluye `diagrama_uml.txt` con la representación del sistema en formato Mermaid.

## Licencia

Este proyecto fue desarrollado con propósitos académicos para la materia de Sistemas Operativos II en la Benemérita Universidad Autónoma de Puebla (BUAP).
