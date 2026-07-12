<!--
  PLANTILLA de README de ejercicio — radiosonda_PIcaro
  Copia esta carpeta a NN_nombre/ y rellena cada sección.
  Objetivo: una GUÍA PASO A PASO que lleve de la mano a un estudiante que empieza DESDE CERO.
  Reglas de oro:
    · Cada paso: acción + comando EXACTO (indica la CARPETA desde donde se ejecuta) + SALIDA ESPERADA.
    · No des nada por sabido: enlaza a la Wiki para lo genérico (instalar toolchain/Arduino, flashear).
    · Usa valores REALES del código/credenciales, nunca inventados. Marca lo aproximado como "algo así".
    · Borra estos comentarios <!-- --> al terminar.
-->
# Ejercicio NN — <Título corto> (<qué logras en 3-5 palabras>)

> **En una frase:** <qué consigue el estudiante al terminar>.
> **Plataforma:** <radio · placa · firmware>. **Banda:** US915 (por defecto; EU868 como alternativa).

## 🎯 Qué vas a conseguir
<El resultado concreto y OBSERVABLE. Si hay salida final (JSON, OLED, traza), enséñala aquí como
"preview" de la meta, para que el estudiante sepa hacia dónde va.>

```
<ejemplo del resultado final: p.ej. el JSON decodificado que verá en ChirpStack>
```

## 🧰 Antes de empezar
Marca esta lista **antes** de seguir (si te falta algo, no continúes):

- [ ] **ChirpStack corriendo** → [Ejercicio 00](../00_chirpstack-docker/) · [Wiki: ChirpStack en 5 min](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/ChirpStack-en-5-minutos)
- [ ] **Tus IDs de ChirpStack exportados** (los creas en el ej.00): `TOKEN`, `TENANT`, `APP` — ver [Ejercicio 00 · crea tu tenant/app/API key](../00_chirpstack-docker/)
- [ ] **Gateway** de tu banda con cobertura *(solo si el nodo hace join)* → [Ejercicio 07](../07_esp-1ch-gateway/) o uno comercial
- [ ] **Hardware:** <lista concreta>
- [ ] **Herramientas:** <toolchain ARM / Arduino IDE+librerías / Python+deps> → [Wiki: Requisitos e instalación](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki/How-To-Requisitos-e-instalación)

> ℹ️ Los comandos `./*.sh` y `python3` corren en **bash** (Linux/macOS o **WSL** en Windows).

## 📟 Hardware y conexiones
<Tabla de pines / cableado / diagrama ASCII. Si la placa es integrada y no hay que cablear, dilo:
"Placa integrada — no hay que cablear nada. ⚠️ Nunca enciendas la radio sin antena.">

## 🪜 Paso a paso

### 1. <Acción del paso>
<Qué haces y por qué, en 1-2 frases.>
```bash
# desde specs/exercises/NN_nombre/
<comando exacto>
```
**Salida esperada:**
```
<las líneas que el estudiante debe ver — copiadas del comportamiento real>
```

### 2. <Siguiente acción>
<...repite el patrón: objetivo → comando/acción exacta → salida esperada...>

## ✅ Cómo saber que funcionó
- [ ] <checkpoint observable 1 — p.ej. en la traza serie aparece `Joined`>
- [ ] <checkpoint 2 — p.ej. en ChirpStack el device muestra *last seen* y frames>
- [ ] <checkpoint 3 — p.ej. llega un JSON por MQTT con el `object` decodificado>

## 🛠️ Si algo falla
| Síntoma | Causa probable | Arreglo |
|---|---|---|
| <lo que ve> | <por qué> | <qué hacer> |

## 📤 Los datos (payload)
- **fPort:** <n>
- **Formato:** <tabla de bytes / estructura>
- **Decodificación:** <codec / ejemplo hex → valores>

## ➡️ Navegación
- ⬅️ Anterior: [Ejercicio NN-1 · …](../NN-1_.../)
- ➡️ Siguiente: [Ejercicio NN+1 · …](../NN+1_.../)
- 🏠 [Índice de ejercicios](../README.md) · 📚 [Wiki](https://github.com/ovelazquezj/radiosonda_PIcaro/wiki)

## 📎 Referencia
- **Credenciales:** [`credentials.json`](credentials.json)
- **Archivos de esta carpeta:** <mapa breve de qué es cada uno>
- **Guías comunes:** [Compilar](../COMMON_BUILD.md) · [Flashear](../COMMON_FLASH.md) · [ChirpStack API](../COMMON_CHIRPSTACK_API.md)

---
> 📄 Material educativo bajo **CC BY 4.0** © **Omar Velazquez** — ver [`LICENSE-CC-BY-4.0.md`](../../../LICENSE-CC-BY-4.0.md).
