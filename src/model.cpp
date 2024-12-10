#include <model.h>
#include <configManager.h>

// Local logging tag
static const char TAG[] = "Model";

Model::Model(modelUpdatedEvt_t _modelUpdatedEvt) {
  this->liveData = {
    .panel_mV = 0,
    .load_mV = 0,
    .battery_mV = 0,
    .panel_mA = 0,
    .load_mA = 0,
    .battery_mA = 0,
    .batterySoc = 0,
    .batteryCurrent_mA = 0
  };
  this->statsData = {
    .panel_min_mV = 0,
    .panel_max_mV = 0,
    .battery_min_mV = 0,
    .battery_max_mV = 0,
    .consumed_Wh = 0,
    .generated_Wh = 0,
    .heatsinkTemp_centiDeg = 0,
    .statusBattery = 0,
    .statusCharger = 0,
    .statusDischarger = 0
  };

  this->modelUpdatedEvt = _modelUpdatedEvt;
}

Model::~Model() {}

void Model::updateStatus() {}

void Model::updateModel(LiveData _liveData) {
  this->liveData = _liveData;
  this->updateStatus();
  modelUpdatedEvt(M_LIVE_DATA);
}

void Model::updateModel(StatsData _statsData) {
  this->statsData = _statsData;
  this->updateStatus();
  modelUpdatedEvt(M_STATS_DATA);
}

void Model::configurationChanged() {
  updateStatus();
  modelUpdatedEvt(M_CONFIG_CHANGED);
}

LiveData Model::getLiveData() {
  return this->liveData;
}

StatsData Model::getStatsData() {
  return this->statsData;
}