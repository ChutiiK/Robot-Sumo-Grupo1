// Robot Sumo Grupo 1 - Arduino UNO R4 WiFi
// Control por WiFi desde el celular + inclinómetro MPU6050.
// Clases: Motores, Inclinometro, ServidorWeb.

#include <WiFiS3.h>
#include <Wire.h>

// ================= CONFIGURACIÓN =================
const char *RED_WIFI   = "Robot Sumo Grupo 1";
const char *CLAVE_WIFI = "RobotSumo1";   // mínimo 8 caracteres
const bool  CREAR_RED  = true;           // true : el robot crea su propia red (IP 192.168.4.1)
                                         // false: se conecta a una red existente
const int   EJE_ADELANTE = 3;        // eje del sensor que apunta al FRENTE del robot:
                                         // 1=+X, 2=+Y, 3=+Z (con signo negativo = al revés)

// ================= DASHBOARD =================
const char PAGINA[] = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>Robot Sumo Grupo 1</title>
<style>
:root{
--marco:#244765; --pantalla:#D9EAF2;
--azul:#397F9B; --rojo:#E58A73; --verde:#57977C;
--verde-claro:#DCEFE5; --gris:#42596B;
}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{
margin:0; min-height:100vh; padding:12px;
display:flex; align-items:center; justify-content:center;
font-family:"Segoe UI",system-ui,sans-serif;
background:#C5D9E3;
}
.cuadro{
width:100%; max-width:380px; padding:16px;
background:var(--pantalla);
border:10px solid var(--marco); border-radius:36px;
}
.titulo{
background:var(--azul); color:#fff; text-align:center;
font-weight:700; font-size:1.3rem; padding:16px; border-radius:12px;
}
.estado{
width:70%; margin:18px auto; padding:8px; text-align:center;
border-radius:10px; font-size:1.1rem;
background:var(--verde-claro); color:var(--verde);
}
.estado.off{background:#f8d7d3; color:var(--rojo)}
.control{
display:grid; gap:10px; grid-template-columns:repeat(3,1fr);
grid-template-areas:". up ." "left stop right" ". down .";
}
.control button{
aspect-ratio:1; border:none; border-radius:16px;
background:var(--azul); color:#fff; font-size:2rem;
touch-action:none; user-select:none; -webkit-user-select:none;
}
.control button:active{filter:brightness(1.3)}
.up{grid-area:up} .down{grid-area:down}
.left{grid-area:left} .right{grid-area:right}
.stop{grid-area:stop; background:var(--rojo)!important}
.velocidad,.datos{margin:20px 8px 0; color:var(--gris); font-size:1.1rem}
.velocidad input{width:100%; accent-color:var(--azul); margin-top:10px}
#grafico{
width:100%; height:auto; margin-top:10px;
background:#fff; border:1px solid #d5dde8; border-radius:12px;
}
.titulo-grafico{margin:20px 8px 0; color:var(--gris); font-size:1rem; font-weight:600}
.leyenda{display:flex; flex-direction:column; gap:2px; margin:6px 8px 0; font-size:.9rem}
.leyenda .x{color:#c0392b} .leyenda .y{color:#2e8b57} .leyenda .z{color:#065a82}
</style>
</head>
<body>
<div class="cuadro">
<div class="titulo">Robot Sumo Grupo 1</div>
<div class="estado off" id="estado">● Desconectado</div>
<div class="control">
<button class="up"    data-cmd="W">▲</button>
<button class="left"  data-cmd="A">◀</button>
<button class="stop"  data-cmd="F">■</button>
<button class="right" data-cmd="D">▶</button>
<button class="down"  data-cmd="S">▼</button>
</div>
<div class="velocidad">
Velocidad: <span id="valorVel">150</span>
<input type="range" id="velocidad" min="0" max="255" value="150">
</div>
<div class="datos" id="datos">Inclinación: --</div>
<div class="titulo-grafico">Aceleración por eje (g) en los últimos 30 s</div>
<canvas id="grafico" width="320" height="180"></canvas>
<div class="leyenda">
<span class="x">● X: adelante / atrás</span>
<span class="y">● Y: izquierda / derecha</span>
<span class="z">● Z: arriba / abajo (en reposo ≈ 1 g)</span>
</div>
</div>
<script>
class Robot {
enviar(cmd, vel) {
return fetch(`/cmd?d=${cmd}&v=${vel}`).catch(() => {});
}
async datos() {
try {
const r = await fetch("/data");
return r.ok ? await r.json() : null;
} catch {
return null;
}
}
}
class Grafico {
constructor(canvas, maxPuntos = 60) {
this.ctx = canvas.getContext("2d");
this.w = canvas.width;
this.h = canvas.height;
this.max = maxPuntos;
this.limite = 2;
this.colores = ["#c0392b", "#2e8b57", "#065a82"];
this.series = [[], [], []];
}
agregar(valores) {
valores.forEach((v, i) => {
this.series[i].push(v);
if (this.series[i].length > this.max) this.series[i].shift();
});
this.dibujar();
}
dibujar() {
const c = this.ctx;
c.clearRect(0, 0, this.w, this.h);
c.strokeStyle = "#cbd5e1";
c.lineWidth = 1;
c.beginPath();
c.moveTo(0, this.h / 2);
c.lineTo(this.w, this.h / 2);
c.stroke();
c.fillStyle = "#64748b";
c.font = "10px sans-serif";
c.fillText("+2 g", 4, 11);
c.fillText("0", 4, this.h / 2 - 3);
c.fillText("-2 g", 4, this.h - 4);
this.series.forEach((s, i) => this.linea(s, this.colores[i]));
}
linea(datos, color) {
const c = this.ctx;
c.strokeStyle = color;
c.lineWidth = 2;
c.beginPath();
datos.forEach((v, i) => {
const g = Math.max(-this.limite, Math.min(this.limite, v));
const x = i * this.w / (this.max - 1);
const y = this.h / 2 - g * (this.h / 2) / this.limite;
i ? c.lineTo(x, y) : c.moveTo(x, y);
});
c.stroke();
}
}
class Dashboard {
constructor(robot) {
this.robot = robot;
this.grafico = new Grafico(document.getElementById("grafico"));
this.estado = document.getElementById("estado");
this.datos = document.getElementById("datos");
this.slider = document.getElementById("velocidad");
this.valorVel = document.getElementById("valorVel");
this.slider.addEventListener("input", () => {
this.valorVel.textContent = this.slider.value;
});
document.querySelectorAll(".control button").forEach(b => this.enlazar(b));
setInterval(() => this.actualizar(), 500);
}
enlazar(boton) {
const cmd = boton.dataset.cmd;
let timer = null;
const enviar = () => this.robot.enviar(cmd, this.slider.value);
const soltar = () => {
if (!timer) return;
clearInterval(timer);
timer = null;
this.robot.enviar("F", 0);
};
boton.addEventListener("pointerdown", e => {
e.preventDefault();
enviar();
if (cmd !== "F") timer = setInterval(enviar, 150);
});
["pointerup", "pointerleave", "pointercancel"]
.forEach(ev => boton.addEventListener(ev, soltar));
}
async actualizar() {
const d = await this.robot.datos();
this.estado.textContent = d ? "● Conectado" : "● Desconectado";
this.estado.classList.toggle("off", !d);
if (d && d.ok) {
this.datos.textContent = `Pitch: ${d.pitch}° | Roll: ${d.roll}°`;
this.grafico.agregar([d.x, d.y, d.z]);
} else if (d) {
this.datos.textContent = "Sensor MPU6050 no detectado";
} else {
this.datos.textContent = "Inclinación: --";
}
}
}
new Dashboard(new Robot());
</script>
</body>
</html>
)HTML";

// ================= MOTORES =================
class Motores {
public:
  void iniciar() {
    const uint8_t pines[] = {ENA, IN1, IN2, ENB, IN3, IN4};
    for (uint8_t p : pines) pinMode(p, OUTPUT);
    parar();
  }

  // dir: W adelante, S atrás, A izquierda, D derecha, F frenar
  void mover(char dir, int vel) {
    vel = constrain(vel, 0, 255);
    ultimoComando = millis();
    switch (dir) {
      case 'W': lado(ENA, IN1, IN2,  vel); lado(ENB, IN3, IN4,  vel); break;
      case 'S': lado(ENA, IN1, IN2, -vel); lado(ENB, IN3, IN4, -vel); break;
      case 'A': lado(ENA, IN1, IN2,  vel); lado(ENB, IN3, IN4, -vel); break;
      case 'D': lado(ENA, IN1, IN2, -vel); lado(ENB, IN3, IN4,  vel); break;
      default:  parar(); return;
    }
    enMovimiento = true;
  }

  void parar() {
    lado(ENA, IN1, IN2, 0);
    lado(ENB, IN3, IN4, 0);
    enMovimiento = false;
  }

  // Seguridad: si se pierde la señal, el robot se detiene.
  void vigilar() {
    if (enMovimiento && millis() - ultimoComando > 500) parar();
  }

private:
  // Cambia los pines según tu cableado (ENA y ENB deben ser PWM).
  static const uint8_t ENA = 5, IN1 = 2, IN2 = 3;
  static const uint8_t ENB = 6, IN3 = 4, IN4 = 7;

  unsigned long ultimoComando = 0;
  bool enMovimiento = false;

  void lado(uint8_t en, uint8_t a, uint8_t b, int v) {
    digitalWrite(a, v > 0);
    digitalWrite(b, v < 0);
    analogWrite(en, abs(v));
  }
};

// ================= INCLINÓMETRO (MPU6050) =================
// Mezcla acelerómetro (estable en el tiempo, ruidoso al moverse) con
// giroscopio (suave al moverse, pero se desvía solo): filtro complementario.
// Funciona con el sensor montado en cualquier posición: al encender (robot
// plano y quieto) detecta cuál eje del sensor mira hacia arriba.
// Ejes del robot: X = adelante, Y = izquierda, Z = arriba.
class Inclinometro {
public:
  void iniciar() {
    Wire.begin();
    intentarConectar();
  }

  // Llamar en cada vuelta del loop: actualiza los valores cada 10 ms.
  void actualizar() {
    if (!presente) {                     // reintenta cada 1 s si no hay sensor
      if (millis() - ultimoIntento > 1000) {
        ultimoIntento = millis();
        intentarConectar();
      }
      return;
    }
    unsigned long ahora = micros();
    if (ahora - ultimaLectura < 10000UL) return;

    float a[3], g[3];
    if (!leerCrudo(a, g)) {              // un fallo suelto (ruido) no cuenta como desconexión
      ultimaLectura = ahora;
      if (++fallos > 50) presente = false;
      return;
    }
    fallos = 0;
    for (int k = 0; k < 3; k++) g[k] -= sesgo[k];

    // Pasar del sistema del sensor al del robot
    float ax = proyectar(F, a), ay = proyectar(L, a), az = proyectar(U, a);
    float gx = proyectar(F, g), gy = proyectar(L, g);

    float pitchAcel = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
    float rollAcel  = atan2(ay, az) * 180.0 / PI;
    float dt = (ahora - ultimaLectura) / 1000000.0;
    ultimaLectura = ahora;

    if (!iniciado || dt > 0.5) {         // primera lectura: partir del acelerómetro
      pitch = pitchAcel;
      roll  = rollAcel;
      sx = ax; sy = ay; sz = az;
      iniciado = true;
      return;
    }
    pitch = PESO_GIRO * (pitch + gy * dt) + (1 - PESO_GIRO) * pitchAcel;
    roll  = PESO_GIRO * (roll  + gx * dt) + (1 - PESO_GIRO) * rollAcel;

    sx += SUAVIZADO * (ax - sx);         // aceleración suavizada (para el gráfico)
    sy += SUAVIZADO * (ay - sy);
    sz += SUAVIZADO * (az - sz);
  }

  // Devuelve false si el sensor no responde. x, y, z en g.
  bool leer(float &p, float &r, float &x, float &y, float &z) {
    p = pitch; r = roll;
    x = sx;    y = sy;    z = sz;
    return presente && iniciado;
  }

private:
  uint8_t dir = 0x68;                        // dirección I2C (0x68, o 0x69 si AD0 está a 3.3V)
  static constexpr float PESO_GIRO = 0.98;   // más alto = más suave, pero más desvío
  static constexpr float SUAVIZADO = 0.1;    // 0.05 = muy suave, 0.5 = reacciona rápido

  bool presente = false, iniciado = false, calibrado = false;
  uint8_t fallos = 0;
  float pitch = 0, roll = 0;
  float sx = 0, sy = 0, sz = 0;
  float sesgo[3] = {0, 0, 0};                // error del giroscopio en reposo
  int F[3] = {1, 0, 0};                      // adelante, izquierda y arriba del robot
  int L[3] = {0, 1, 0};                      // escritos con los ejes del sensor
  int U[3] = {0, 0, 1};
  unsigned long ultimaLectura = 0, ultimoIntento = 0;

  // Busca el sensor en 0x68 y 0x69 y lo saca del modo sleep.
  void intentarConectar() {
    presente = false;
    const uint8_t dirs[] = {0x68, 0x69};
    for (uint8_t d : dirs) {
      Wire.beginTransmission(d);
      Wire.write(0x6B);
      Wire.write(0);
      if (Wire.endTransmission() == 0) { dir = d; presente = true; break; }
    }
    if (!presente) {
      Serial.println("MPU6050 no responde: revisa SDA, SCL, VCC y GND");
      return;
    }
    fallos = 0;
    iniciado = false;
    if (!calibrado) calibrar();          // solo la primera vez: el robot debe estar plano y quieto
  }

  static float proyectar(const int e[3], const float v[3]) {
    return e[0] * v[0] + e[1] * v[1] + e[2] * v[2];
  }

  int16_t leer16() {
    int16_t alto = Wire.read();
    return (alto << 8) | Wire.read();
  }

  // Lee acelerómetro (en g) y giroscopio (en °/s), ejes del sensor.
  bool leerCrudo(float a[3], float g[3]) {
    Wire.beginTransmission(dir);
    Wire.write(0x3B);                    // primer registro del acelerómetro
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom(dir, (uint8_t)14);
    if (Wire.available() < 14) return false;

    for (int k = 0; k < 3; k++) a[k] = leer16() / 16384.0;   // ±2 g
    leer16();                                                // temperatura (no se usa)
    for (int k = 0; k < 3; k++) g[k] = leer16() / 131.0;     // ±250 °/s
    return true;
  }

  // Promedia 200 lecturas en reposo: error del giroscopio y dirección de la gravedad.
  void calibrar() {
    float sa[3] = {0, 0, 0}, sg[3] = {0, 0, 0}, a[3], g[3];
    int n = 0;
    for (int i = 0; i < 200; i++) {
      if (leerCrudo(a, g)) {
        for (int k = 0; k < 3; k++) { sa[k] += a[k]; sg[k] += g[k]; }
        n++;
      }
      delay(3);
    }
    if (n == 0) return;
    for (int k = 0; k < 3; k++) { sesgo[k] = sg[k] / n; sa[k] /= n; }
    definirEjes(sa);
    calibrado = true;
  }

  // "Arriba" = eje del sensor que marca ±1 g. "Adelante" viene de EJE_ADELANTE.
  // "Izquierda" sale del producto cruz (arriba × adelante).
  void definirEjes(const float a[3]) {
    int arriba = 0;
    for (int k = 1; k < 3; k++) if (fabs(a[k]) > fabs(a[arriba])) arriba = k;
    int frente = abs(EJE_ADELANTE) - 1;
    if (frente == arriba) {
      Serial.println("Aviso: EJE_ADELANTE coincide con el eje vertical, revisa la configuración");
      frente = (arriba + 1) % 3;
    }
    for (int k = 0; k < 3; k++) { F[k] = 0; U[k] = 0; }
    U[arriba] = (a[arriba] > 0) ? 1 : -1;
    F[frente] = (EJE_ADELANTE > 0) ? 1 : -1;
    L[0] = U[1] * F[2] - U[2] * F[1];
    L[1] = U[2] * F[0] - U[0] * F[2];
    L[2] = U[0] * F[1] - U[1] * F[0];

    Serial.print("Sensor -> arriba: ");    Serial.print(etiqueta(U));
    Serial.print("  adelante: ");          Serial.print(etiqueta(F));
    Serial.print("  izquierda: ");         Serial.println(etiqueta(L));
  }

  static String etiqueta(const int e[3]) {
    for (int k = 0; k < 3; k++)
      if (e[k]) return String(e[k] > 0 ? "+" : "-") + "XYZ"[k];
    return "?";
  }
};

// ================= SERVIDOR WEB =================
class ServidorWeb {
public:
  ServidorWeb(Motores &m, Inclinometro &i) : server(80), motores(m), incl(i) {}

  void iniciar(const char *red, const char *clave, bool crearRed) {
    if (WiFi.status() == WL_NO_MODULE) {
      Serial.println("No se encontró el módulo WiFi");
      while (true) {}
    }
    if (crearRed) {
      int estado = WiFi.beginAP(red, clave);
      if (estado != WL_AP_LISTENING) {
        Serial.println("No se pudo crear la red WiFi");
        while (true) {}
      }
      delay(2000);
    } else {
      while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Conectando al WiFi...");
        WiFi.begin(red, clave);
        delay(5000);
      }
    }
    server.begin();
    Serial.print("Abre en el celular: http://");
    Serial.println(WiFi.localIP());
  }

  void atender() {
    WiFiClient c = server.available();
    if (!c) return;
    c.setTimeout(100);                   // evita bloquear el robot

    String linea = c.readStringUntil('\n');   // ej: GET /cmd?d=F&v=150 HTTP/1.1
    while (c.connected() && c.readStringUntil('\n').length() > 1) {}  // saltar cabeceras

    if (linea.startsWith("GET /cmd")) {
      procesarComando(linea);
      responder(c, "text/plain", "ok");
    } else if (linea.startsWith("GET /data")) {
      responder(c, "application/json", datosJson());
    } else if (linea.startsWith("GET / ")) {
      enviarPagina(c);
    } else {
      c.print("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
    }
    c.stop();
  }

private:
  WiFiServer server;
  Motores &motores;
  Inclinometro &incl;

  void procesarComando(const String &linea) {
    int d = linea.indexOf("d=");
    int v = linea.indexOf("v=");
    if (d < 0 || v < 0) return;
    char dir = linea[d + 2];
    if (!strchr("WASDF", dir)) return;   // ignora comandos inválidos
    motores.mover(dir, linea.substring(v + 2).toInt());
  }

  String datosJson() {
    float pitch, roll, x, y, z;
    bool ok = incl.leer(pitch, roll, x, y, z);
    return String("{\"ok\":") + (ok ? "true" : "false") +
           ",\"pitch\":" + String(pitch, 1) +
           ",\"roll\":" + String(roll, 1) +
           ",\"x\":" + String(x, 2) +
           ",\"y\":" + String(y, 2) +
           ",\"z\":" + String(z, 2) + "}";
  }

  void responder(WiFiClient &c, const char *tipo, const String &cuerpo) {
    c.print("HTTP/1.1 200 OK\r\nContent-Type: ");
    c.print(tipo);
    c.print("\r\nConnection: close\r\n\r\n");
    c.print(cuerpo);
  }

  // La página pesa varios KB: se envía en trozos para que no se corte.
  void enviarPagina(WiFiClient &c) {
    c.print("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n");
    const size_t total = strlen(PAGINA);
    for (size_t i = 0; i < total; i += 512) {
      size_t n = (total - i < 512) ? (total - i) : 512;
      c.write((const uint8_t *)PAGINA + i, n);
    }
  }
};

// ================= PROGRAMA PRINCIPAL =================
Motores motores;
Inclinometro inclinometro;
ServidorWeb servidor(motores, inclinometro);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}  // espera máx. 3 s al monitor serie
  motores.iniciar();
  inclinometro.iniciar();
  servidor.iniciar(RED_WIFI, CLAVE_WIFI, CREAR_RED);
}

void loop() {
  servidor.atender();
  motores.vigilar();
  inclinometro.actualizar();
}
