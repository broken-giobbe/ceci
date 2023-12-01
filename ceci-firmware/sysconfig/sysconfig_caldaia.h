#ifndef SYSCONFIG_H
#define SYSCONFIG_H

/**
 * Config for mod_relay.
 * 
 * If HAS_MOD_RELAY is defined the application will embed and load this module.
 */
#define HAS_MOD_RELAY

// Port where the heating control realy is connected
// D7 for hardware version 1.x
// D8 for hardware version 2.x
#define HEATER_PORT D8

/*
 * Default size in bytes for various text buffers. Set it big enough ;)
 */
#define TXT_BUF_SIZE 128

/*
 * Name for the node config file in the SPIFFS
 */
#define CONFIG_FILENAME "/config-caldaia.ini"

#endif
