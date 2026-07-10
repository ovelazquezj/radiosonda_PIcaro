# How-To · Dar de alta un LR1110 nuevo (tus propias DevEUI / JoinEUI / AppKey)

Guía **end-to-end** para poner en marcha un nodo **LR1110** con **credenciales tuyas** (no las de
laboratorio que traen los ejercicios): **generarlas → ponerlas en el código → compilar → flashear →
darlo de alta en ChirpStack → verificar el join**.

> Aplica a los nodos **LR1110** (Nucleo-L476RG, ejercicios 01–04). Para un **TTGO** el flujo es
> parecido **pero el orden de bytes del DevEUI/JoinEUI se invierte** — ver
> [Compilar el TTGO en Arduino](How-To-Compilar-el-TTGO-en-Arduino).

## Las tres credenciales (qué son)

| Credencial | Tamaño | ¿Secreta? | Notas |
|-----------|--------|-----------|-------|
| **DevEUI** | 8 bytes (EUI-64) | No | **Identificador único** del dispositivo. Cambia SIEMPRE por uno nuevo/único. |
| **JoinEUI** (AppEUI) | 8 bytes | No | Identifica al *join server*. Puede ser **común** a varios nodos (el proyecto usa `AA BB CC DD EE FF 00 00`). |
| **AppKey** | 16 bytes | **SÍ** | Clave raíz del cifrado. **Genérala aleatoria** y no la publiques. |

> En LoRaWAN **1.0.x** solo se usa el **AppKey** (no hay NwkKey aparte). Ojo con esto en ChirpStack (Paso 4).

---

## Paso 1 · Genera tus credenciales

Todo en **hexadecimal, orden MSB** (big-endian; el más significativo primero).

- **AppKey (16 bytes, aleatorio):**
  ```bash
  openssl rand -hex 16
  # ejemplo -> 8f3c1a...  (16 bytes = 32 dígitos hex). Guárdalo en un sitio seguro.
  ```
- **DevEUI (8 bytes, ÚNICO):** en un laboratorio puedes elegirlo tú; lo importante es que **no se
  repita** con otro device del mismo ChirpStack. Un patrón cómodo es un prefijo fijo + un contador:
  ```
  DevEUI = AA BB CC DD 00 00 00 42     (tu prefijo + un número que vayas incrementando)
  ```
  > En una red real, el DevEUI debería salir de un bloque **EUI-64** asignado (OUI IEEE). Para el
  > laboratorio, basta con que sea único.
- **JoinEUI (8 bytes):** puedes reutilizar el común del proyecto `AA BB CC DD EE FF 00 00`, o el tuyo.

**Truco — convierte un hex a array C** (para pegarlo en el Paso 2):
```bash
echo "aabbccdd00000042" | sed 's/../0x&, /g'
# -> 0xaa, 0xbb, 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x42,
```

> 🔑 **Anota las 3 credenciales**: las necesitas **idénticas** en el código (Paso 2) y en ChirpStack
> (Paso 4). Cualquier diferencia = el join falla.

---

## Paso 2 · Cámbialas en el código (`example_options.h`)

Edita el `example_options.h` **de tu ejercicio**:

| Ejercicio | Archivo a editar |
|-----------|------------------|
| 01, 03 | `lbm_examples/main_examples/example_options.h` |
| 02, 04 | `lbm_applications/3_geolocation_on_lora_edge/main_geolocation/example_options.h` |

Sustituye los tres arrays y la región (formato real del fichero):

```c
#define USER_LORAWAN_DEVICE_EUI                        \
    {                                                  \
        0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x00, 0x00, 0x42 \
    }
#define USER_LORAWAN_JOIN_EUI                          \
    {                                                  \
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00 \
    }
#define USER_LORAWAN_APP_KEY                                                                           \
    {                                                                                                  \
        0x8F, 0x3C, /* ...los 16 bytes de tu AppKey... */                                              \
    }

/* La región del firmware DEBE coincidir con la del device profile en ChirpStack */
#define MODEM_EXAMPLE_REGION SMTC_MODEM_REGION_US_915   // o SMTC_MODEM_REGION_EU_868
```

> ⚠️ **Orden de bytes (nota clave para LR1110).** Aquí el **DevEUI/JoinEUI van en MSB**, **igual** que
> como se introducen en ChirpStack → se pegan **tal cual, sin invertir**. (Esto es al revés que en el
> TTGO/LMIC, donde el DevEUI/JoinEUI se invierten. No mezcles los dos flujos.)
>
> ℹ️ Con la crypto **SOFT** (por defecto en estos ejemplos), las credenciales que pones a mano aquí
> son **las que realmente se usan** para el join.

---

## Paso 3 · Compila

Compila tu ejercicio con la **misma región** que pusiste arriba (detalle en
[Compilar el firmware LR1110](How-To-Compilar-el-firmware)):

```bash
# Ejemplo — ejercicio 02 (geolocalización + BMP280) en US915:
make -C lbm_applications/3_geolocation_on_lora_edge full_lr1110 \
     MODEM_APP=EXAMPLE_GEOLOCATION REGION=US_915
```

El binario queda en `build_lr1110_l4/app_lr1110_<REGION>.bin` (+ `.hex`/`.elf`).

> `REGION=` (compilada) y `MODEM_EXAMPLE_REGION` (Paso 2) deben ser **coherentes**.

---

## Paso 4 · Flashea

Graba el `.bin`/`.hex` en la Nucleo-L476RG → [Flashear y ver la serie](How-To-Flashear-y-ver-la-serie).
Deja abierta la traza serie (**115200 8N1**) para ver el join en el Paso 6.

---

## Paso 5 · Da de alta el device en ChirpStack

Detalle y variantes por API en [Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack). En resumen:

1. **Device profile** (una vez por región): región = **la misma** que el firmware; **MAC version
   LoRaWAN 1.0.4** (o 1.0.3); **Activation = OTAA**.
2. **Add device:** pega el **DevEUI** y el **JoinEUI** en **MSB** (tal cual el Paso 2), y elige el
   device profile de esa región.
3. **Keys (OTAA):** el **AppKey**… ⚠️ **en 1.0.x va en el campo `nwkKey`** (no `appKey`).

Por API (reproducible):
```bash
export TOKEN="TU_API_KEY"; export API="http://localhost:8090"
export APP="<application-id>"; export DP="<device-profile-id>"
DEVEUI="aabbccdd00000042"; JOINEUI="aabbccddeeff0000"; APPKEY="8f3c...tu_appkey"

# Crear el device
curl -s -X POST "$API/api/devices" -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"device":{"devEui":"'"$DEVEUI"'","name":"lr1110-nuevo","applicationId":"'"$APP"'","deviceProfileId":"'"$DP"'","joinEui":"'"$JOINEUI"'"}}'

# Poner el AppKey EN nwkKey (LoRaWAN 1.0.x)
curl -s -X POST "$API/api/devices/$DEVEUI/keys" -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"deviceKeys":{"devEui":"'"$DEVEUI"'","nwkKey":"'"$APPKEY"'"}}'
```

> 💡 Los ejercicios traen un `provision.sh` que hace esto de forma idempotente; puedes copiarlo y
> cambiar sus credenciales por las tuyas.

---

## Paso 6 · Verifica el join

Con el device dado de alta, pulsa **RESET (B2)** en la Nucleo y mira:

- **Traza serie (115200):** debe aparecer **`Joined`**.
- **ChirpStack → device → LoRaWAN frames:** un **Join Request → Join Accept** y luego los uplinks;
  el device pasa a *last seen* reciente.

```bash
# Estado del device por API:
curl -s "$API/api/devices/$DEVEUI" -H "Authorization: Bearer $TOKEN"   # lastSeenAt, deviceStatus
```

---

## Errores típicos (y su causa)

| Síntoma | Causa | Arreglo |
|---------|-------|---------|
| No hace join | **Región** distinta entre firmware y device profile | Que coincidan `MODEM_EXAMPLE_REGION`, `REGION=` y la región del profile |
| `invalid MIC` / no valida | **AppKey** mal, o en el campo equivocado | En 1.0.x va en **`nwkKey`**; revisa los 16 bytes |
| `Unknown device` en logs | DevEUI no registrado o mal escrito | Registra el **DevEUI** correcto (MSB), idéntico al código |
| Join rechazado por EUI | Orden de bytes | En LR1110 el DevEUI/JoinEUI van **MSB** (no invertir; eso es del TTGO) |
| `DevNonce has already been used` | Nonce repetido tras reflashear | Borra y recrea el device en ChirpStack |
| DevEUI duplicado | Reusaste el DevEUI de otro nodo | Usa un **DevEUI único** por dispositivo |

> ⚠️ **No commitees AppKeys reales** en el repo. Trátalas como secretos.

> Relacionado: **[Compilar el firmware](How-To-Compilar-el-firmware)** ·
> **[Flashear y ver la serie](How-To-Flashear-y-ver-la-serie)** ·
> **[Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)** ·
> Volver: **[🏠 Inicio](Home)**
