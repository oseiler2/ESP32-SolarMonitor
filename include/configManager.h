#pragma once

#include <Arduino.h>
#include <config.h>
#include <configParameter.h>
#include <vector>

extern Config config;

void setupConfigManager();
void getDefaultConfiguration(Config& config);
boolean validateConfiguration();
boolean loadConfiguration(Config& config);
boolean saveConfiguration(const Config config);
void logConfiguration(const Config config);
void printConfigurationFile();

std::vector<ConfigParameterBase<Config>*> getConfigParameters();
