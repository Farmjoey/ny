# NOVA RIDE OS – milepæl 1 (hardware-probe)
STATUS: IKKE kompileret og IKKE testet på hardware. Alle pins er antagelser for CYD (ESP32-2432S028R-familien).

## Byg (Windows, PlatformIO)
    pip install platformio
    pio run -e cyd_st7789      # prøv først denne
    pio run -e cyd_ili9341     # hvis billedet er forkert/hvidt/spejlet

## Flash via USB
    pip install esptool
    python -m esptool --chip esp32 --port COMx write_flash 0x0 NOVA_RIDE_cyd_st7789.factory.bin
(eller kun app: 0x10000 firmware.bin, hvis bootloader/partitioner allerede er flashet)

## Hvad testen viser
Chip, flash, PSRAM, MAC, farvebjælker (rød/grøn/blå), touch-rådata og knapper til invert/rotation/RGB-BGR.
Send mig skærmbilledet + seriel log (115200), så fastlægger vi den endelige profil.
