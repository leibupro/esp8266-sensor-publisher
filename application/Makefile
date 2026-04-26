
include ../common.mk

SHELL := /usr/bin/env bash

TOOLCHPFX := xtensa-lx106-elf-
CC := $(TOOLCHPFX)gcc
LD := $(CC)
ESPT := esptool

W := -Wall \
     -Wextra \
     -Wpedantic \
     -Wconversion \
     -Wsign-conversion \
     -Wsign-compare \
     -Wcast-align=strict \
     -Wfloat-equal \
     -Wlogical-op \
     -Werror

#
# Need to disable pedandic warnings again
# because some sdk headers would issue:
# error: ISO C forbids zero-size array
#
WN := -Wno-frame-address \
      -Wno-pedantic

CF := -g3 -O0 -std=gnu11 $(W) $(WN) -ffreestanding -mlongcalls

INCDIRS := freertos/include \
           freertos/port/esp8266/include \
           freertos/port/esp8266/include/freertos \
           lwip/port/esp8266/include \
           lwip/include/apps \
           lwip/include/apps/sntp \
           freertos/include/freertos/private \
           vfs/include \
           esp_common/include \
           heap/port/esp8266/include \
           heap/include \
           esp8266/include \
           esp8266/include/rom \
           log/include \
           tcpip_adapter/include \
           esp_event/include \
           nvs_flash/include \
           spi_flash/include \
           lwip/lwip/src/include \
           mqtt/esp-mqtt/include 
INCDIRS := $(addprefix ./sdk_headers/, $(INCDIRS))
INCDIRS += include

DEFINES := $(addprefix -D, \
    USING_IBUS_FASTER_GET \
    __ESP_FILE__=__FILE__ \
)

LF := -nostdlib \
      -Wl,--gc-sections \
      -Wl,-static \
      -Wl,--no-check-sections \
      -Wl,-EL

LIBDIR := ./lib
LDDIR := ./ld

LIBDIRS := $(LDDIR) \
           $(LIBDIR)
LIBDIRS := $(addprefix -L, $(LIBDIRS))

LD_SCRIPTS := $(addprefix -T , \
    esp8266_out.ld \
    esp8266.project.ld \
    esp8266.rom.ld \
    esp8266.peripherals.ld \
)

#
# Forced symbols. Took over from an SDK build
# linker invocation ...
#
comma := ,
FSYMS := $(addprefix -Wl$(comma)-u$(comma), \
    call_user_start_cpu0 \
    esp_app_desc \
    call_user_start \
    g_esp_sys_info \
    __errno \
    pthread_include_pthread_impl \
    pthread_include_pthread_cond_impl \
    pthread_include_pthread_local_storage_impl \
)

#
# Library group. The search area ...
#
LDLIBS := -Wl,--start-group $(addprefix -l, \
    app_update \
    bootloader_support \
    coap \
    console \
    esp-tls \
    esp-wolfssl \
    esp8266 \
    gcc \
    hal \
    core \
    net80211 \
    phy \
    rtc \
    clk \
    pp \
    smartconfig \
    ssc \
    espnow \
    esp_common \
    esp_event \
    esp_gdbstub \
    esp_http_client \
    esp_http_server \
    esp_https_ota \
    esp_ringbuf \
    fatfs \
    freemodbus \
    freertos \
    heap \
    http_parser \
    jsmn \
    json \
    libsodium \
    log \
    lwip \
    mbedtls \
    mdns \
    mqtt \
    newlib \
    c_nano \
    m \
    nvs_flash \
    openssl \
    protobuf-c \
    protocomm \
    pthread \
    spi_flash \
    spi_ram \
    spiffs \
    tcp_transport \
    tcpip_adapter \
    vfs \
    wear_levelling \
    wifi_provisioning \
    wpa_supplicant \
    stdc++ \
    gcov \
) \
-Wl,--end-group

SRCDIR := ./src
OBJDIR := ./obj
BINDIR := ./bin

INCLUDES := $(addprefix -I, $(INCDIRS))

SRC := dht.c \
       wifi.c \
       mqtt.c \
       main.c

OBJ := $(addprefix $(OBJDIR)/, $(SRC:.c=.o))

ELF := $(BINDIR)/senpub.elf
BINARY := $(BINDIR)/senpub.bin

MKDIRS := $(OBJDIR) $(BINDIR)

VPATH := $(SRCDIR)

.PHONY: all flash clean


all: $(MKDIRS) $(BINARY)


$(MKDIRS):
	mkdir $@


flash: $(BINARY)
	$(ESPT) --chip esp8266 \
        --port /dev/ttyUSB0 \
        --baud 115200 \
        --before default-reset \
        --after hard-reset \
        write-flash \
        --flash-mode dio \
        --flash-freq 40m \
        --flash-size 2MB \
        0x10000 \
        $<


$(BINARY): $(ELF)
	$(ESPT) --chip esp8266 elf2image \
        --flash-mode dio \
        --flash-freq 40m \
        --flash-size 2MB \
        --version 3 \
        -o $@ \
        $<


$(ELF): $(OBJ)
	$(LD) $(LF) $(LIBDIRS) $(LD_SCRIPTS) $(FSYMS) $^ $(LDLIBS) -o $@


$(OBJDIR)/%.o: %.c
	$(CC) $(CF) $(DEFINES) $(INCLUDES) -c $< -o $@


clean:
	-rm -rf $(MKDIRS)

