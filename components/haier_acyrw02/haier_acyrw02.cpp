#include "haier_acyrw02.h"

namespace esphome {
namespace haier_acyrw02 {

static const char *const TAG = "climate.haier_acyrw02";

void HaierClimate::set_sensor(sensor::Sensor *sensor) { this->sensor_ = sensor; }
void HaierClimate::set_ir_pin(uint16_t pin) { this->ir_pin_ = pin; }

void HaierClimate::setup() {
  Component::setup();

  ac_ = new IRHaierACYRW02(this->ir_pin_);
  if (this->sensor_) {
    this->sensor_->add_on_state_callback([this](float state) {
      this->current_temperature = state;
      this->publish_state();
    });
    this->current_temperature = this->sensor_->state;
  } else {
    this->current_temperature = NAN;
  }

  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->target_temperature = 24;
    this->fan_mode = climate::CLIMATE_FAN_AUTO;
    this->swing_mode = climate::CLIMATE_SWING_OFF;
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  if (isnan(this->target_temperature)) {
    this->target_temperature = 25;
  }

  ac_->begin();
  this->send();

  ESP_LOGD("DEBUG", "Haier A/C remote is in the following state:");
  ESP_LOGD("DEBUG", "  %s\n", ac_->toString().c_str());
}

void HaierClimate::send() {
  if (this->mode == climate::CLIMATE_MODE_OFF) {
    ac_->off();
  } else {
    ac_->on();
    if (this->mode == climate::CLIMATE_MODE_COOL) {
      ac_->setMode(kHaierAcYrw02Cool);
    } else if (this->mode == climate::CLIMATE_MODE_DRY) {
      ac_->setMode(kHaierAcYrw02Dry);
    } else if (this->mode == climate::CLIMATE_MODE_FAN_ONLY) {
      ac_->setMode(kHaierAcYrw02Fan);
    }

    ac_->setTemp(this->target_temperature);

    if (this->fan_mode == climate::CLIMATE_FAN_AUTO) {
      ac_->setFan(kHaierAcYrw02FanAuto);
    } else if (this->fan_mode == climate::CLIMATE_FAN_LOW) {
      ac_->setFan(kHaierAcYrw02FanLow);
    } else if (this->fan_mode == climate::CLIMATE_FAN_MEDIUM) {
      ac_->setFan(kHaierAcYrw02FanMed);
    } else if (this->fan_mode == climate::CLIMATE_FAN_HIGH) {
      ac_->setFan(kHaierAcYrw02FanHigh);
    }

    if (this->swing_mode == climate::CLIMATE_SWING_OFF) {
      ac_->setSwing(kHaierAcYrw02SwingVOff);
    } else if (this->swing_mode == climate::CLIMATE_SWING_VERTICAL) {
      ac_->setSwing(kHaierAcYrw02SwingVAuto);
    }

    ac_->setSleep(this->preset == climate::CLIMATE_PRESET_SLEEP);
    ac_->setHealth(this->preset == climate::CLIMATE_PRESET_COMFORT);
    ac_->setTurbo(this->preset == climate::CLIMATE_PRESET_BOOST);
  }
  ac_->send();
}

climate::ClimateTraits HaierClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_COOL, climate::CLIMATE_MODE_HEAT,
                              climate::CLIMATE_MODE_FAN_ONLY, climate::CLIMATE_MODE_AUTO});
  traits.set_supported_fan_modes(
      {climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH, climate::CLIMATE_FAN_AUTO});
  traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL});
  traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_SLEEP,
                                climate::CLIMATE_PRESET_COMFORT, climate::CLIMATE_PRESET_BOOST});

  traits.set_visual_max_temperature(30);
  traits.set_visual_min_temperature(16);
  traits.set_visual_temperature_step(1);

  return traits;
}

void HaierClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    climate::ClimateMode mode = *call.get_mode();
    this->mode = mode;
  }

  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    this->target_temperature = temp;
  }

  if (call.get_fan_mode().has_value()) {
    climate::ClimateFanMode fan_mode = *call.get_fan_mode();
    this->fan_mode = fan_mode;
  }

  if (call.get_swing_mode().has_value()) {
    climate::ClimateSwingMode swing_mode = *call.get_swing_mode();
    this->swing_mode = swing_mode;
  }

  this->send();

  this->publish_state();

  ESP_LOGD("DEBUG", "Haier A/C remote is in the following state:");
  ESP_LOGD("DEBUG", "  %s\n", ac_->toString().c_str());
}

}  // namespace haier_acyrw02
}  // namespace esphome
