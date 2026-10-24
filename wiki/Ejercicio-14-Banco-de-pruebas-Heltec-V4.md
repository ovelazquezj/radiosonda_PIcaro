# Ejercicio 14 — 🧪 Banco de pruebas local: gateway de 1 canal Heltec V4 + ChirpStack en Docker

Montas en tu mesa un **banco de pruebas LoRaWAN completo y personal**: **ChirpStack corriendo en tu PC**
con Docker y una **Heltec WiFi LoRa 32 V4** haciendo de **gateway de 1 canal**, conectada por WiFi a tu
PC. Sirve para **probar tu radiosonda sin depender de ningún servidor ni gateway externo**, en casa o en
el laboratorio y sin internet una vez instalado. El firmware viene **precompilado** y el gateway se
configura **desde el celular**: la placa abre su propia red WiFi con una página donde escribes tu WiFi y
la IP de tu PC, y lo guarda en su memoria.

> **Carpeta del ejercicio:** [`specs/exercises/14_heltec-v4-gateway-1canal/`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/14_heltec-v4-gateway-1canal) · **Plataforma:** **Heltec WiFi LoRa 32 V4** (ESP32-S3 + **SX1262** + amplificador de RF + OLED), firmware **One Channel Hub** de Semtech adaptado por PiCARO · **ChirpStack v4** en Docker

| | |
|---|---|
| Qué demuestra | La cadena LoRaWAN **completa bajo tu control**: nodo → gateway → servidor de red → aplicación. Instalas el LNS, grabas y configuras el gateway y lo registras |
| Hardware | **Heltec WiFi LoRa 32 V4** + antena 915 MHz + cable USB-C de datos; un PC con Docker; un celular; tu **radiosonda** |
| ¿Join / ChirpStack? | ✅ ChirpStack **local** (el del [Ejercicio 00](Ejercicio-00-ChirpStack)), región **us915_1**; el gateway entra por **Semtech UDP 1700** |
| Dato / observable | Gateway **Online** en ChirpStack; en la OLED: EUI, IP, canal y contadores `up`/`dn`; tramas en *LoRaWAN frames* |
| Firmware | [`artifacts/picaro-gw-heltec-v4_us915-fsb2.bin`](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/14_heltec-v4-gateway-1canal/artifacts) — se graba con `esptool` en `0x0` |

**Parámetros de radio (por defecto):** **US915** · **sub-banda 2** · canal fijo **903.9 MHz** · **SF10 /
BW 125 kHz** (= DR0 de la radiosonda) · downlinks en cualquier frecuencia/SF que ordene ChirpStack.

> ⚠️ **Es un gateway de 1 canal.** La radiosonda salta entre los 8 canales de la sub-banda 2, así que el
> gateway oye en promedio **1 de cada 8 uplinks** y el join puede tardar varios intentos. Es suficiente
> para comprobar que tu radiosonda se une, transmite y decodifica bien; no para medir tasa de entrega.

## 📖 Documentos

- 🔧 **[Guía paso a paso](How-To-Banco-de-pruebas-local-Heltec-V4)** — instalar ChirpStack con Docker,
  grabar el firmware, configurar el gateway desde el celular, darlo de alta en ChirpStack y **usarlo cada
  día** con tu radiosonda. Incluye tabla de problemas y soluciones.
- 📐 **[Memoria técnica](Memoria-tecnica-Gateway-Heltec-V4)** — el problema que resuelve, arquitectura,
  hardware (amplificador GC1109/KCT8103L), cambios al firmware, pruebas, seguridad y cómo recompilar.
  Pensada para que la integres en la **memoria técnica de tu proyecto**.

## 🗺️ Resumen del procedimiento

| Parte | Qué haces | Cuándo |
|---|---|---|
| A | Instalar Docker y arrancar ChirpStack (`docker compose up -d` en `00_chirpstack-docker`); abrir el firewall (UDP 1700) | Una vez |
| B | Grabar el firmware: `py -m esptool --chip esp32s3 --port COMx write-flash 0x0 picaro-gw-heltec-v4_us915-fsb2.bin` | Una vez |
| C | Conectarte desde el celular a la red **`PiCARO-GW-xxxxxx`** (clave `picaro1234`), abrir **http://192.168.4.1** y poner tu WiFi y la IP de tu PC | Una vez por red |
| D | En ChirpStack → **Gateways → Add gateway** con el **Gateway EUI** que muestra la OLED | Una vez |
| E | Encender Docker y la Heltec, comprobar **Online** y probar la radiosonda | Cada sesión |

## 📎 Archivos

- `README.md` — la guía paso a paso (misma que la [página How-To](How-To-Banco-de-pruebas-local-Heltec-V4)).
- `MEMORIA_TECNICA.md` — la memoria técnica (misma que la [página de la wiki](Memoria-tecnica-Gateway-Heltec-V4)).
- `artifacts/` — firmware listo para grabar, suma SHA-256 y licencia de Semtech.
- `firmware/` — parche sobre One Channel Hub v1.1.1 y configuración de compilación.
- `img/` — capturas de ChirpStack y de la página del gateway.

> *Basado en One Channel Hub © Semtech (Clear BSD). Radio SX1262 · US915 sub-banda 2 · Semtech UDP · ChirpStack v4.*

> ◀ [Ejercicio 13 — Estación meteo con CubeCell](Ejercicio-13-CubeCell-Estacion-Meteo)

---
_Docs © Omar Velazquez · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)_
