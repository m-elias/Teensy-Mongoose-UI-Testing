#pragma once

#define MG_ARCH MG_ARCH_NEWLIB           // Use ARM toolchain
#define MG_ENABLE_TCPIP 1                // Enable built-in network stack
#define MG_ENABLE_DRIVER_IMXRT 1         // Enable RTxx driver
#define MG_ENABLE_CUSTOM_MILLIS 1        // Let user to implement mg_millis()
#define MG_ENABLE_CUSTOM_RANDOM 1        // Ask user to implement mg_random()
#define MG_ENABLE_PACKED_FS 1            // Enable packed filesystem
#define MG_TCPIP_PHY_ADDR 0              // PHY address
#define MG_OTA MG_OTA_RT1060             // Enable OTA
#define MG_TLS MG_TLS_BUILTIN            // Enable built-in TLS

// Match Teensy's linker script - required for OTA to work
#define MG_IRAM __attribute__((noinline, section(".fastrun")))

// For static IP configuration, define MG_TCPIP_{IP,MASK,GW}
// By default, those are set to zero, meaning that DHCP is used
//
// #define MG_TCPIP_IP MG_IPV4(192, 168, 0, 10)
// #define MG_TCPIP_GW MG_IPV4(192, 168, 0, 1)
// #define MG_TCPIP_MASK MG_IPV4(255, 255, 255, 0)

// Construct MAC address from the MCU unique ID
#define MG_OCOTP_FUSES ((volatile uint32_t *) 0x401f4410)
#define MG_SET_MAC_ADDRESS(mac)                                             \
  do {                                                                      \
    mac[0] = 2;                                                             \
    mac[1] = (MG_OCOTP_FUSES[0] >> 0) & 255;                                \
    mac[2] = (MG_OCOTP_FUSES[0] >> 10) & 255;                               \
    mac[3] = ((MG_OCOTP_FUSES[0] >> 19) ^ (MG_OCOTP_FUSES[4] >> 19)) & 255; \
    mac[4] = (MG_OCOTP_FUSES[4] >> 10) & 255;                               \
    mac[5] = (MG_OCOTP_FUSES[4] >> 0) & 255;                                \
  } while (0)
