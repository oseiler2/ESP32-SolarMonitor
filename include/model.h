#pragma once

#include <globals.h>

const float NaN = sqrt(-1);

typedef enum {
  M_NONE = 0,
  M_CONFIG_CHANGED = 1 << 0,
  M_LIVE_DATA = 1 << 1,
  M_STATS_DATA = 1 << 2
} Measurement;

typedef void (*modelUpdatedEvt_t)(uint16_t mask);

struct LiveData {
  uint16_t panel_mV;
  uint16_t load_mV;
  uint16_t battery_mV;
  uint16_t panel_mA;
  uint16_t load_mA;
  uint16_t battery_mA;
  uint16_t batterySoc;
  uint16_t batteryCurrent_mA;
};

struct StatsData {
  uint16_t panel_min_mV;
  uint16_t panel_max_mV;
  uint16_t battery_min_mV;
  uint16_t battery_max_mV;
  uint16_t consumed_Wh;
  uint16_t generated_Wh;
  uint16_t heatsinkTemp_centiDeg;
  uint16_t statusBattery;
  uint16_t statusCharger;
  uint16_t statusDischarger;
};

class Model {
public:

  Model(modelUpdatedEvt_t modelUpdatedEvt);
  ~Model();

  LiveData getLiveData();
  StatsData getStatsData();

  void updateModel(LiveData liveData);
  void updateModel(StatsData statsData);
  void configurationChanged();

private:

  LiveData liveData;
  StatsData statsData;
  modelUpdatedEvt_t modelUpdatedEvt;
  void updateStatus();

};
