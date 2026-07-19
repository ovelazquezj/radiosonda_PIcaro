# Ejercicio 11 · Dashboard contra un servidor remoto (MQTT sobre TLS)

El mismo dashboard del [Ejercicio 10](Ejercicio-10-Dashboard-Mission-Control), pero conectado a un
ChirpStack **en Internet** en lugar del que corre en tu portátil: **MQTT cifrado con TLS**, usuario y
contraseña propios, y una **ACL** que hace que cada equipo lea solo sus datos. El paso a paso está en
el [`README.md` del ejercicio](https://github.com/ovelazquezj/radiosonda_PIcaro/tree/master/specs/exercises/11_dashboard-servidor-remoto).

> ⚠️ Los datos de servidor de la guía son **ficticios** (`example.org`, un dominio reservado por
> [RFC 2606](https://www.rfc-editor.org/rfc/rfc2606) que nadie puede registrar). Tu instructor te da
> los reales.

## 🔗 Qué cambia respecto al ej.10

```
  ej.10   dashboard ──MQTT :1883, anónimo, en claro──► ChirpStack en TU portátil
  ej.11   dashboard ──MQTT :8883, TLS + usuario/contraseña──► ChirpStack en Internet
```

El dashboard es el mismo y los datos llegan igual. Lo que cambia está por debajo: el broker ya no es
tuyo, va cifrado y exige identificarte.

## 🔐 Por qué un broker público no puede ser como el del laboratorio

- **En claro no vale.** Por el 1883 el usuario y la contraseña viajan legibles. Por eso un servidor
  serio solo abre el **8883 con TLS**.
- **Anónimo tampoco.** Si cualquiera puede conectarse, cualquiera lee la telemetría de todos — y
  puede **publicar** downlinks a dispositivos ajenos.
- **Un usuario por equipo, no uno compartido.** Con credenciales compartidas no se puede revocar el
  acceso de un solo equipo.
- **ACL por aplicación.** Es lo que impide de verdad que un equipo lea lo de otro. En el ejercicio se
  comprueba en la práctica: con el `app_id` de otro equipo el dashboard **conecta pero no recibe nada**.

## 🧠 La idea que se llevan

**ChirpStack no guarda histórico de payloads.** Publica cada uplink por MQTT y se olvida. El
histórico lo construye el consumidor —en este caso, la SQLite del dashboard—. Si tu dashboard no está
corriendo, esos datos no los recupera nadie.

Es la diferencia entre un *bus de mensajes* y una *base de datos*, y explica por qué el dashboard
tiene su propio almacenamiento local.

## 🛠️ Diagnóstico: leer el error correcto

| Mensaje | Dónde falló | Qué revisar |
|---|---|---|
| `not authorised` | Autenticación | Usuario y contraseña |
| `SSLCertVerificationError` | TLS, **antes** de autenticar | Reloj del sistema; proxy que inspecciona TLS |
| Conecta pero no llega nada | Ni TLS ni auth: es el **topic** | El `app_id`, o simplemente no hay uplinks |
| Se queda «conectando…» | Red | Puerto **8883 de salida** bloqueado en tu red |

Distinguir un fallo de **TLS** de uno de **credenciales** es la habilidad práctica de este ejercicio.

---
_Docs © Omar Velazquez · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)_
