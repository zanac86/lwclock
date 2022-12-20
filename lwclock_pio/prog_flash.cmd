esptool erase_flash
esptool --after no_reset --baud 921600 write_flash 0x000000 .pio\build\d1_mini\firmware.bin
esptool --after hard_reset --baud 921600 write_flash 0x200000 .pio\build\d1_mini\littlefs.bin
