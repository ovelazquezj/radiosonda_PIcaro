# How-To · Flashear la Nucleo y ver la traza serie

Graba el `.bin`/`.hex` (que compilaste en
[How-To Compilar el firmware](How-To-Compilar-el-firmware)) en la **Nucleo-L476RG + shield LR1110** y
observa el arranque/join por el puerto serie.

## Hardware
Nucleo-L476RG con el shield LR1110, cable USB al conector **ST-LINK**.

## Opción A · Arrastrar y soltar (recomendada en Windows/WSL)
WSL2 no ve el USB por defecto, así que lo más simple es flashear desde **Windows**:

1. Conecta la Nucleo a un USB de **Windows** → se monta el disco **`NODE_L476RG`**.
2. **Copia el `.hex`** al disco (el `.hex` lleva la dirección embebida; es el más fiable para
   arrastrar-y-soltar).
3. El LED del ST-LINK parpadea y el disco se **re-monta** solo = programado y reiniciado.
4. Si aparece `FAIL.TXT`, ábrelo: dice el motivo (suele ser firmware ST-LINK antiguo → actualízalo
   con STM32CubeProgrammer).

> Para llegar a los `artifacts/` desde el Explorador de Windows: `\\wsl$\` → tu distro → la ruta del
> repo. O ejecuta `explorer.exe .` desde la carpeta en la terminal WSL.

## Opción B · Línea de comandos
```bash
# st-flash (Linux/macOS con USB, o WSL con usbipd):
st-flash --connect-under-reset write artifacts/<binario>.bin 0x08000000

# STM32CubeProgrammer (Windows/Linux, el más robusto):
STM32_Programmer_CLI -c port=SWD mode=UR -w artifacts/<binario>.hex 0x08000000 -rst
```

## Ver la traza serie
Terminal serie a **115200 8N1** sobre el VCP del ST-LINK (`/dev/ttyACM0`, `COMx`…):
```bash
tio -b 115200 /dev/ttyACM0        # o picocom/minicom; en Windows: PuTTY/Tera Term
```
Tras flashear, pulsa **RESET (B2)** para reiniciar el firmware y ver el arranque + el join.

## Problemas típicos
| Síntoma | Causa | Arreglo |
|---------|-------|---------|
| Caracteres basura en la serie | baud mal puesto | fija **115200 8N1**, sin control de flujo |
| `FAIL.TXT` en el disco | firmware ST-LINK antiguo | actualízalo con STM32CubeProgrammer |
| El `.hex` no funciona pero el `.bin` sí (o viceversa) | dirección/base incorrecta al escribir | con `.bin` usa base `0x08000000`; con `.hex` no hace falta base |
| No monta `NODE_L476RG` | cable de solo-carga o puerto USB del micro | usa el USB del **ST-LINK** y un cable de datos |

> Ya flasheado y con join OK, provisiona/consume en ChirpStack →
> **[How-To Provisionar en ChirpStack](How-To-Provisionar-en-ChirpStack)** →
