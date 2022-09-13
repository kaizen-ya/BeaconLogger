set PORT=COM3
set APP=M5Core2BeaconLogger

.\esptool.exe --port %PORT% --baud 921600 ^
     --chip esp32  --before default_reset --after hard_reset ^
     write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect ^
     0x10000 ./%APP%.ino.m5stack_core2.bin

pause
