Robot Sumo Grupo 1

Proyecto final de Fundamentos de IoT 
Ingeniería Civil Informática 
Universidad Autónoma de Chile

Robot sumo controlado desde el celular. Un Arduino UNO R4 WiFi crea su propia red WiFi (punto de acceso), maneja dos motores DC a través de un puente H y lee un sensor de movimiento MPU-6050. El robot sirve una página web (dashboard) con los controles y un gráfico en tiempo real del sensor. No necesita internet.

Integrantes:

Benjamín Alarcón
Alexander Sandoval
Sebastian Yevenes
Yordan Hernandez

Profesores:
Camilo Fuentes 
Fernando Morales


## 1. Componentes

| Componente | Detalle |
|---|---|
| Microcontrolador | Arduino UNO R4 WiFi |
| Puente H | Módulo tipo L298N (2 canales) |
| Motores | 2 motores DC con caja reductora |
| Sensor | MPU-6050 (acelerómetro + giroscopio, I2C) |
| Batería | 12 V, alimenta los motores |

---

## 2. Esquema de conexiones

### 2.1 Puente H ↔ Arduino

| Señal del puente H | Pin del Arduino | Tipo | Función |
|---|---|---|---|
| ENA | 5 | PWM | Velocidad del motor A |
| IN1 | 2 | Digital | Dirección del motor A |
| IN2 | 3 | Digital | Dirección del motor A |
| ENB | 6 | PWM | Velocidad del motor B |
| IN3 | 4 | Digital | Dirección del motor B |
| IN4 | 7 | Digital | Dirección del motor B |

Motor A: (completar: lado izquierdo o derecho del robot) · Motor B: (completar)

Los pines se definen como constantes al inicio de la clase `Motores` en el código.

### 2.2 MPU-6050 ↔ Arduino

| Pin del MPU-6050 | Pin del Arduino | Función |
|---|---|---|
| VCC | 3,3 V o 5 V | Alimentación |
| GND | GND | Tierra |
| SDA | SDA (junto a AREF, o A4) | Datos I2C |
| SCL | SCL (junto a AREF, o A5) | Reloj I2C |
| AD0 | Sin conectar | Dirección I2C 0x68 |

Los pines XDA, XCL e INT del módulo no se usan.

### 2.3 Alimentación

| Conexión | Detalle |
|---|---|
| Batería 12 V → puente H | Alimenta los motores, de forma independiente a la lógica |
| Alimentación del Arduino | (completar: por VIN o jack desde la batería) |
| GND común | Se unen el GND de la batería, del puente H y del Arduino |

> No se alimentan los motores desde el pin 5V del Arduino.

---

## 3. Cómo cargar el código

1. Instalar en el Arduino IDE el paquete **Arduino UNO R4 Boards** (Gestor de placas).
2. Seleccionar la placa **Arduino UNO R4 WiFi** y el puerto correspondiente.
3. Abrir `RobotSumo/RobotSumo.ino`. La carpeta debe llamarse igual que el archivo y tener **un solo `.ino`**.
4. Compilar y subir. Las bibliotecas `WiFiS3` y `Wire` vienen incluidas con la placa, no hay que instalar nada más.

**Importante:** al encender, el robot debe estar **plano y quieto** unos 2 segundos, porque el sensor se calibra en ese momento.

---

## 4. Cómo usarlo

1. Encender el robot.
2. En el celular, conectarse a la red WiFi **`Robot Sumo Grupo 1`** (clave: `RobotSumo1`). El celular puede avisar que no hay internet: elegir quedarse conectado.
3. Abrir en el navegador **`http://192.168.4.1`**.
4. Usar las flechas para mover el robot, el botón ■ para detenerlo y el control deslizante para ajustar la velocidad (0 a 255).
5. El gráfico muestra la aceleración en los ejes X, Y y Z del robot (en g) de los últimos 30 segundos. Debajo se ven los ángulos Pitch y Roll.

En `RobotSumo.ino` se puede cambiar el nombre y la clave de la red, y también hacer que el robot se conecte a una red existente en vez de crear la suya (`CREAR_RED`).

---

## 5. Rutas del servidor (API)

| Ruta | Respuesta | Descripción |
|---|---|---|
| `/` | HTML | Dashboard |
| `/cmd?d=W&v=150` | `ok` | Mueve el robot. `d` = dirección, `v` = velocidad (0–255) |
| `/data` | JSON | Datos del sensor |
| Cualquier otra | 404 | Ruta no existente |

Direcciones de `/cmd`: `W` adelante · `S` atrás · `A` izquierda · `D` derecha · `F` detener.

Ejemplo de respuesta de `/data`:

```json
{"ok":true,"pitch":1.2,"roll":-0.8,"x":0.02,"y":-0.01,"z":1.00}
```

| Campo | Significado | Unidad |
|---|---|---|
| `ok` | `false` si el sensor no responde | — |
| `pitch` | Cabeceo (inclinación adelante/atrás) | grados |
| `roll` | Alabeo (inclinación a los lados) | grados |
| `x`, `y`, `z` | Aceleración: adelante, izquierda y arriba del robot | g |

---

## 6. Cómo funciona

### Recorrido de un comando
1. Se presiona un botón del dashboard.
2. El JavaScript envía `fetch("/cmd?d=W&v=150")` al Arduino, y lo repite cada 150 ms mientras el botón está presionado.
3. El servidor interpreta la ruta y llama a `Motores::mover()`.
4. Esa función pone los pines IN1–IN4 en la dirección correcta y escribe la velocidad con `analogWrite()` en ENA y ENB.
5. El puente H entrega la corriente de la batería a los motores.

Al soltar el botón se envía el comando de detener. Si el robot deja de recibir comandos por **500 ms** (por ejemplo, si se pierde la conexión), los motores se detienen solos.

### Recorrido de los datos del sensor
1. El Arduino lee el MPU-6050 por I2C cada 10 ms.
2. Convierte los valores a unidades físicas (g y °/s).
3. Calcula Pitch y Roll mezclando acelerómetro y giroscopio (filtro complementario), para que los ángulos sean estables.
4. El dashboard pide `/data` cada 500 ms y dibuja los valores en un gráfico `canvas`, sin librerías externas.

### Orientación del sensor
El sensor está pegado de canto en la pared frontal del chasis. Al encender, el código detecta qué eje del sensor mira hacia arriba y ajusta todo al sistema del robot (X adelante, Y izquierda, Z arriba). Si los signos salen invertidos, se cambia `EJE_ADELANTE` a `-3` al inicio del código.

---

## 7. Estructura del código

| Clase | Responsabilidad |
|---|---|
| `Motores` | Pines, dirección, velocidad por PWM y parada de seguridad |
| `Inclinometro` | Lectura del MPU-6050, calibración, orientación y filtro de ángulos |
| `ServidorWeb` | Punto de acceso WiFi, rutas HTTP y respuesta JSON |

El dashboard (HTML, CSS y JavaScript) está dentro de la constante `PAGINA` y pesa unos 6 KB.

---

## 8. Problemas frecuentes

| Síntoma | Causa probable |
|---|---|
| "Sensor MPU6050 no detectado" | Cable SDA/SCL suelto o mal conectado, o sensor sin alimentación |
| La red WiFi desaparece al mover el robot | Caída de voltaje al arrancar los motores: revisar la alimentación del Arduino |