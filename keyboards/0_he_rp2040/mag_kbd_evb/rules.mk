SRC += key_layout.c
SRC += src/matrix.c
SRC += src/mag_analog.c
SRC += src/mag_cal.c
SRC += src/mag_config.c
SRC += src/mag_proc.c
SRC += src/mag_utils.c

LTO_ENABLE = yes

ANALOG_DRIVER_REQUIRED = yes

CONSOLE_ENABLE = yes