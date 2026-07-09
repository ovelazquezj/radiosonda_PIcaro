# 1 · ¿Qué es LoRa?

**LoRa** (*Long Range*) es una **técnica de modulación de radio** creada por Semtech. Es la **capa
física** (PHY): define *cómo* se transmiten los bits por el aire, no *qué* significan. Usa una
modulación llamada **CSS** (*Chirp Spread Spectrum*), muy robusta frente al ruido y a señales
débiles.

## Sus tres ideas clave
- **Alcance largo:** varios kilómetros en ciudad, decenas en campo abierto/línea de vista.
- **Bajo consumo:** un nodo puede durar **años** con una pila.
- **Baja tasa de datos:** manda **poquitos bytes** de vez en cuando (no es Wi-Fi ni streaming).

## Bandas de frecuencia (sub-GHz, ISM)
LoRa opera en bandas **libres** sub-GHz, distintas según la región:
- **EU868** (~863–870 MHz) — Europa.
- **US915** (~902–928 MHz) — América.
- Otras: AU915, AS923, IN865, KR920…

> ⚠️ La región del **dispositivo** y la del **gateway/servidor** deben coincidir, o no se comunican.

## El compromiso: Spreading Factor (SF)
El **SF** (de SF7 a SF12) decide el equilibrio entre alcance y velocidad:

| SF | Alcance | Velocidad | Tiempo en aire | Consumo por trama |
|----|---------|-----------|----------------|-------------------|
| **SF7** | menor | mayor | corto | menor |
| **SF12** | mayor | menor | largo | mayor |

Más SF = llega más lejos pero **tarda más** en enviar y **gasta más** batería. Los servidores usan
**ADR** (*Adaptive Data Rate*) para elegir el SF óptimo automáticamente.

## LoRa ≠ LoRaWAN (no los confundas)
- **LoRa** = la **radio** (capa física, el "cómo suena" la señal).
- **LoRaWAN** = el **protocolo de red** que va **encima** de LoRa (direcciones, seguridad,
  gateways, servidor). Lo vemos en la **[siguiente página](Qué-es-LoRaWAN)**.

Analogía: LoRa es como las **ondas de radio**; LoRaWAN es el **idioma y las reglas** para que
muchos dispositivos hablen ordenadamente por esas ondas.

> Siguiente: **[¿Qué es LoRaWAN?](Qué-es-LoRaWAN)** →
