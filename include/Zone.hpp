// Copyright - Jared Gaillard - 2016
// MIT License
//
// Arduino CZII Project

#ifndef zone_hpp
#define zone_hpp

#include "Util.h"

//
// Storage class for Zone data
//
class Zone {

  public:
    Zone(uint8_t  number) {
      number = number;
    }

    void setCoolSetpoint(uint8_t value) {
      if (coolSetpoint != value)
        modified = true;
      coolSetpoint = value;
    }

    uint8_t getCoolSetpoint() {
      return coolSetpoint;
    }

    void setHeatSetpoint(uint8_t value) {
      if (heatSetpoint != value)
        modified = true;
      heatSetpoint = value;
    }

    uint8_t getHeatSetpoint() {
      return heatSetpoint;
    }

    void setTemperature(float value) {
      if (temperature != value)
        modified = true;
      temperature = value;
    }

    float getTemperature() {
      return temperature;
    }

    void setHumidity(uint8_t value) {
      if (humidity != value)
        modified = true;
      humidity = value;
    }

    uint8_t getHumidity() {
      return humidity;
    }

    void setDamperPosition(uint8_t value) {
      if (damperPosition != value)
        modified = true;
      damperPosition = value;
    }

    uint8_t getDamperPosition() {
      return damperPosition;
    }

    bool isModified() {
      return modified;
    }

    bool setModified(bool value) {
      modified = value;
    }

  private:
    uint8_t zoneNumber     = -1;
    bool modified       = false;

    uint8_t coolSetpoint   = -1;
    uint8_t heatSetpoint   = -1;
    float temperature   = FLOAT_MIN_VALUE;
    uint8_t humidity       = -1;
    uint8_t damperPosition = -1;
};

#endif
