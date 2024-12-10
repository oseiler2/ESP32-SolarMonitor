#pragma once

#include <globals.h>
#include <config.h>
#include <float.h>
#include <ArduinoJson.h>

#define CONFIG_PARAM_UNCHANGED    (0)
#define CONFIG_PARAM_UPDATED      (1)
#define CONFIG_PARAM_OUT_OF_RANGE (-1)
#define CONFIG_PARAM_NOT_PRESENT  (-2)
#define CONFIG_PARAM_WRONG_TYPE   (-3)

template <typename C, typename F = uint8_t>
class ConfigParameterBase {
public:
  virtual uint8_t getMaxStrLen(void) = 0;
  virtual const char* getId() = 0;
  virtual const char* getLabel() = 0;
  virtual void print(const C config, char* str) = 0;
  virtual int8_t save(C& config, const char* str) = 0;
  virtual bool isNumber() = 0;
  virtual bool isFraction() = 0;
  virtual bool isBoolean() = 0;
  virtual bool canBeZero() = 0;
  virtual void getMinimum(char* str) = 0;
  virtual void getMaximum(char* str) = 0;
  virtual void setToDefault(C& config) = 0;
  virtual String toString(const C config) = 0;
  virtual void toJson(const C config, DynamicJsonDocument* doc) = 0;
  virtual int8_t fromJson(C& config, DynamicJsonDocument* doc, bool useDefaultIfNotPresent) = 0;
  virtual bool isRebootRequiredOnChange() = 0;
  virtual bool isEnum() = 0;
  virtual const char** getEnumLabels(void) = 0;
  virtual u_int16_t getValueOrdinal(const C config) = 0;
  virtual F getFunction() = 0;
};

template <typename C, typename T, typename F = uint8_t>
class ConfigParameter :public ConfigParameterBase<C, F> {
public:
  ConfigParameter(const char* id, const char* label, T C::* valuePtr, uint8_t maxStrLen, F function = 0, bool rebootRequiredOnChange = false);
  virtual ~ConfigParameter() = default;
  uint8_t getMaxStrLen(void) override;
  const char* getId() override;
  const char* getLabel() override;
  bool isNumber() override;
  bool isFraction() override;
  bool isBoolean() override;
  bool canBeZero()override;
  T* getValuePtr();
  void getMinimum(char* str) override;
  void getMaximum(char* str) override;
  String toString(const C config) override;
  bool isRebootRequiredOnChange() override;
  bool isEnum() override;
  const char** getEnumLabels(void) override;
  F getFunction() override;

protected:
  const char* id;
  const char* label;
  T C::* valuePtr;
  uint8_t maxStrLen;
  bool rebootRequiredOnChange;
  F function;
};

template <typename C, typename T, typename F = uint8_t>
class NumberConfigParameter :public ConfigParameter<C, T, F> {
public:
  NumberConfigParameter(const char* id, const char* label, T C::* valuePtr, T defaultValue, uint8_t maxStrLen, T min, T max, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  ~NumberConfigParameter();
  bool isNumber() override;
  void print(const C config, char* str) override;
  void getMinimum(char* str) override;
  void getMaximum(char* str) override;
  bool canBeZero() override;
  void setToDefault(C& config) override;
  int8_t save(C& config, const char* str) override;
  void toJson(const C config, DynamicJsonDocument* doc) override;
  int8_t fromJson(C& config, DynamicJsonDocument* doc, bool useDefaultIfNotPresent = false) override;
  u_int16_t getValueOrdinal(const C config) override;

protected:
  bool allowZeroValue;
  T defaultValue;
  T minValue;
  T maxValue;
};

template <typename C, typename T, typename F = uint8_t>
class UnsignedNumberConfigParameter :public NumberConfigParameter<C, T, F> {
public:
  UnsignedNumberConfigParameter(const char* id, const char* label, T C::* valuePtr, T defaultValue, uint8_t maxStrLen, T min, T max, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  ~UnsignedNumberConfigParameter();
  void print(const C config, char* str) override;
};

template <typename C, typename F = uint8_t>
class Uint8ConfigParameter :public UnsignedNumberConfigParameter<C, uint8_t, F> {
public:
  Uint8ConfigParameter(const char* id, const char* label, uint8_t C::* valuePtr, uint8_t defaultValue, uint8_t min = 0, uint8_t max = 255, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  Uint8ConfigParameter(const char* id, const char* label, uint8_t C::* valuePtr, uint8_t defaultValue, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  ~Uint8ConfigParameter();
};

template <typename C, typename F = uint8_t>
class Uint16ConfigParameter :public UnsignedNumberConfigParameter<C, uint16_t, F> {
public:
  Uint16ConfigParameter(const char* id, const char* label, uint16_t C::* valuePtr, uint16_t defaultValue, uint16_t min = 0, uint16_t max = 65535, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  Uint16ConfigParameter(const char* id, const char* label, uint16_t C::* valuePtr, uint16_t defaultValue, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  ~Uint16ConfigParameter();
};

template <typename C, typename F = uint8_t>
class Uint32ConfigParameter :public UnsignedNumberConfigParameter<C, uint32_t, F> {
public:
  Uint32ConfigParameter(const char* id, const char* label, uint32_t C::* valuePtr, uint32_t defaultValue, uint32_t min = 0, uint32_t max = 0xffffffff, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  Uint32ConfigParameter(const char* id, const char* label, uint32_t C::* valuePtr, uint32_t defaultValue, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  ~Uint32ConfigParameter();
};

template <typename C, typename F = uint8_t>
class Int8ConfigParameter :public NumberConfigParameter<C, int8_t, F> {
public:
  Int8ConfigParameter(const char* id, const char* label, int8_t C::* valuePtr, int8_t defaultValue, int8_t min = -128, int8_t max = 127, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  Int8ConfigParameter(const char* id, const char* label, int8_t C::* valuePtr, int8_t defaultValue, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  ~Int8ConfigParameter();
};

template <typename C, typename F = uint8_t>
class Int32ConfigParameter :public NumberConfigParameter<C, int32_t, F> {
public:
  Int32ConfigParameter(const char* id, const char* label, int32_t C::* valuePtr, int32_t defaultValue, int32_t min = -2147483647, int32_t max = 2147483646, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  Int32ConfigParameter(const char* id, const char* label, int32_t C::* valuePtr, int32_t defaultValue, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  ~Int32ConfigParameter();
};

template <typename C, typename F = uint8_t>
class FloatConfigParameter :public NumberConfigParameter<C, float, F> {
public:
  FloatConfigParameter(const char* id, const char* label, const char* printFormatting, float C::* valuePtr, float defaultValue, float min = FLT_MIN, float max = FLT_MAX, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  FloatConfigParameter(const char* id, const char* label, const char* printFormatting, float C::* valuePtr, float defaultValue, F function = 0, bool allowZeroValue = false, bool rebootRequiredOnChange = false);
  ~FloatConfigParameter();
  void print(const C config, char* str) override;
  bool isFraction() override;
  void getMinimum(char* str) override;
  void getMaximum(char* str) override;
private:
  const char* printFormatting;
};

template <typename C, typename F = uint8_t>
class BooleanConfigParameter :public ConfigParameter<C, bool, F> {
public:
  BooleanConfigParameter(const char* id, const char* label, bool C::* valuePtr, bool defaultValue, F function = 0, bool rebootRequiredOnChange = false);
  ~BooleanConfigParameter();
  void print(const C config, char* str) override;
  bool isBoolean() override;
  void setToDefault(C& config) override;
  void toJson(const C config, DynamicJsonDocument* doc) override;
  int8_t fromJson(C& config, DynamicJsonDocument* doc, bool useDefaultIfNotPresent = false) override;
  u_int16_t getValueOrdinal(const C config) override;
  int8_t save(C& config, const char* str) override;

protected:
  bool defaultValue;
};

template <typename C, typename F = uint8_t>
class CharArrayConfigParameter :public ConfigParameter<C, char, F> {
public:
  CharArrayConfigParameter(const char* id, const char* label, char C::* valuePtr, const char* defaultValue, uint8_t maxLen, F function = 0, bool rebootRequiredOnChange = false);
  ~CharArrayConfigParameter();
  void print(const C config, char* str) override;
  void setToDefault(C& config) override;
  void toJson(const C config, DynamicJsonDocument* doc) override;
  int8_t fromJson(C& config, DynamicJsonDocument* doc, bool useDefaultIfNotPresent = false) override;
  u_int16_t getValueOrdinal(const C config) override;
  int8_t save(C& config, const char* str) override;

protected:
  const char* defaultValue;
};

/*
template<typename E>
using enumToString_t = const char* (*)(E value);

template<typename E>
using stringToEnum_t = E(*)(const char*);
*/

template <typename C, typename B, typename E, typename F = uint8_t>
class EnumConfigParameter :public NumberConfigParameter<C, B, F> {
public:
  EnumConfigParameter(const char* id, const char* label, E C::* valuePtr, E defaultValue, const char* enumLabels[], E min, E max, F function = 0, bool rebootRequiredOnChange = false);
  ~EnumConfigParameter();
  void print(const C config, char* str) override;
  const char** getEnumLabels(void) override;
  bool isEnum() override;
  bool isNumber() override;
  u_int16_t getValueOrdinal(const C config) override;
  int8_t save(C& config, const char* str) override;

protected:
  const char** enumLabels;
};
