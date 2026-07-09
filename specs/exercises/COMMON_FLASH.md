# COMMON — Flashear el binario en la Nucleo-L476RG

Referenciado por todos los ejercicios. Parte de que el `.bin`/`.hex` **ya existe** en la
carpeta `artifacts/` del ejercicio.

## Hardware
Nucleo-L476RG + shield LR1110, cable USB al conector **ST-LINK**.

## Opción recomendada en este entorno (WSL): arrastrar-y-soltar (Windows)
> WSL2 no ve el USB por defecto, así que lo más simple es flashear desde **Windows**.

1. Conecta la Nucleo a un USB de **Windows** → se monta el disco **`NODE_L476RG`**.
2. **Copia el `.hex`** del ejercicio a ese disco (el `.hex` lleva la dirección embebida; es el más
   fiable para arrastrar-y-soltar).
3. El LED del ST-LINK parpadea; el disco se **re-monta** solo = programado y reiniciado.
4. Si aparece un `FAIL.TXT` en el disco, ábrelo: dice el motivo (suele ser firmware ST-LINK antiguo
   → actualízalo con STM32CubeProgrammer).

> Para llegar a los `artifacts/` desde el Explorador de Windows: `\\wsl$\` → tu distro → la ruta del
> repo. (O `explorer.exe .` desde la carpeta en la terminal WSL.)

## Opción por línea de comandos
```bash
# st-flash (Linux/macOS con acceso USB, o WSL con usbipd):
st-flash --connect-under-reset write artifacts/<binario>.bin 0x08000000

# STM32CubeProgrammer (Windows/Linux, el más robusto):
STM32_Programmer_CLI -c port=SWD mode=UR -w artifacts/<binario>.hex 0x08000000 -rst
```

## Ver la traza serie (todas las pruebas)
Terminal serie a **115200 8N1** sobre el VCP del ST-LINK (`/dev/ttyACM0`, `COMx`…):
```bash
tio -b 115200 /dev/ttyACM0     # o picocom/minicom; en Windows: PuTTY/Tera Term
```
Tras flashear, pulsa **RESET (B2)** para reiniciar el firmware.

> Si ves caracteres basura, es **baud mal puesto**: fija 115200 8N1, sin control de flujo.
