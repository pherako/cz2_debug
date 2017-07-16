// Copyright - Jared Gaillard - 2016
// MIT License
//
// Arduino CZII Project

#ifndef ComfortZoneII_hpp
#define ComfortZoneII_hpp

//#include <ArduinoJson.h>
//#include <Arduino.h>
//#include <TimeLib.h>
#include "Zone.hpp"
#include "RingBuffer.hpp"
#include "Util.h"

#define TABLE1_TEMPS_ROW        16
#define TABLE1_TIME_ROW         18

class ComfortZoneII
{
  public:
    ComfortZoneII(uint8_t numberZones);
    Zone* getZone(uint8_t zoneIndex);
    int update(RingBuffer* ringBuffer);
    bool isZoneModified();
    void clearZoneModified();
    bool isStatusModified();
    void clearStatusModified();
    //String toZoneJson();
    //String toStatusJson();

    // CZII frame
    static const uint8_t DEST_ADDRESS_POS     = 0;
    static const uint8_t SOURCE_ADDRESS_POS   = 2;
    static const uint8_t DATA_LENGTH_POS      = 4;
    static const uint8_t FUNCTION_POS         = 7;
    static const uint8_t DATA_START_POS       = 8;
    static const uint8_t TBL_POS              = DATA_START_POS + 1;
    static const uint8_t ROW_POS              = DATA_START_POS + 2;

    static const uint8_t MIN_MESSAGE_SIZE     = 11;

    // CZII Function Codes
    static const uint8_t RESPONSE_FUNCTION    = 6;
    static const uint8_t READ_FUNCTION        = 11;
    static const uint8_t WRITE_FUNCTION       = 12;

  private:

    int dbg_lvl = 3;
    Zone* zones[8];
    uint8_t NUMBER_ZONES;
    bool statusModified;

    uint8_t controllerState = -1;
    uint8_t lat_Temp_f = -1;
    float outside_Temp_f = FLOAT_MIN_VALUE;
    float outside_Temp2_f = FLOAT_MIN_VALUE;
    //TimeElements time;

    bool isValidTemperature(float value);
    float getTemperatureF(uint8_t highByte, uint8_t lowByte);
    void updateZoneInfo            ( RingBuffer* ringBuffer);
    void updateOutsideHumidityTemp ( RingBuffer* ringBuffer);
    void updateOutsideTemp         ( RingBuffer* ringBuffer);
    void updateControllerState     ( RingBuffer* ringBuffer);
    void updateZoneSetpoints       ( RingBuffer* ringBuffer);
    void updateTime                ( RingBuffer* ringBuffer);
    void updateZone1Info           ( RingBuffer* ringBuffer);
    void updateDamperPositions     ( RingBuffer* ringBuffer);
    void updateZoneTemp            ( RingBuffer* ringBuffer, uint8_t zoneStart);

    //void addJson(JsonObject& jsonObject, String key, uint8_t value);
    //void addJson(JsonObject& jsonObject, String key, float value);

    void setControllerState(uint8_t value);
    void setLatTemperature(uint8_t value);
    void setOutsideTemperature(float value);
    void setOutsideTemperature2(float value);
    void setDayTime(uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

    enum ControllerStateFlags {
      DeHumidify          = 1 << 7,
      Humidify            = 1 << 6,
      Fan_G               = 1 << 5,
      ReversingValve      = 1 << 4,
      AuxHeat2_W2         = 1 << 3,
      AuxHeat1_W1         = 1 << 2,
      CompressorStage2_Y2 = 1 << 1,
      CompressorStage1_Y1 = 1 << 0
    };
};

#endif
