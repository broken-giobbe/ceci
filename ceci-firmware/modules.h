/**
 * Header file to contain all the module function definition.
 * 
 * There shouldn't be many, since each module is initialized and then relays on qd_sched to do its stuff
 */
#ifndef MODULES_H
#define MODULES_H

/*
 * mod_sensors
 */
void mod_sensors_init(SPIFFSIniFile* conf);

/*
 * mod_thermostat
 */
void mod_thermostat_init(SPIFFSIniFile* conf);

#endif
