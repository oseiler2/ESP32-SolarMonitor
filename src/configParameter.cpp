#include <configParameter.h>
#include <type_traits>
#include <bits/basic_string.h>

// Local logging tag
static const char TAG[] = "ConfigParameter";

// -------------------- generic -------------------
template <typename C, typename T, typename F>
ConfigParameter<C, T, F>::ConfigParameter(const char* _id, const char* _label, T C::* _valuePtr, uint8_t _maxStrLen, F _function, bool _rebootRequiredOnChange) {
  this->id = _id;
  this->label = _label;
  this->valuePtr = _valuePtr;
  this->maxStrLen = _maxStrLen;
  this->rebootRequiredOnChange = _rebootRequiredOnChange;
  this->function = _function;
}

template <typename C, typename T, typename F>
uint8_t ConfigParameter<C, T, F>::getMaxStrLen(void) {
  return this->maxStrLen;
}

template <typename C, typename T, typename F>
const char* ConfigParameter<C, T, F>::getId() {
  return this->id;
}

template <typename C, typename T, typename F>
const char* ConfigParameter<C, T, F>::getLabel() {
  return this->label;
}

template <typename C, typename T, typename F>
bool ConfigParameter<C, T, F>::isNumber() {
  return false;
}

template <typename C, typename T, typename F>
bool ConfigParameter<C, T, F>::isFraction() {
  return false;
}

template <typename C, typename T, typename F>
bool ConfigParameter<C, T, F>::isBoolean() {
  return false;
}

template <typename C, typename T, typename F>
bool ConfigParameter<C, T, F>::canBeZero() {
  return false;
}

template <typename C, typename T, typename F>
T* ConfigParameter<C, T, F>::getValuePtr() {
  return this->valuePtr;
}

template <typename C, typename T, typename F>
void ConfigParameter<C, T, F>::getMinimum(char* str) { str[0] = 0; }

template <typename C, typename T, typename F>
void ConfigParameter<C, T, F>::getMaximum(char* str) { str[0] = 0; }

template <typename C, typename T, typename F>
String ConfigParameter<C, T, F>::toString(const C config) {
  char buffer[this->getMaxStrLen()] = { 0 };
  this->print(config, buffer);
  return String(buffer);
}

template <typename C, typename T, typename F>
bool ConfigParameter<C, T, F>::isRebootRequiredOnChange() {
  return this->rebootRequiredOnChange;
}

template <typename C, typename T, typename F>
bool ConfigParameter<C, T, F>::isEnum() {
  return false;
}

template <typename C, typename T, typename F>
const char** ConfigParameter<C, T, F>::getEnumLabels(void) { return nullptr; }

template <typename C, typename T, typename F>
F ConfigParameter<C, T, F>::getFunction() { return this->function; }

// -------------------- number -------------------
template <typename C, typename T, typename F>
NumberConfigParameter<C, T, F>::NumberConfigParameter(const char* _id, const char* _label, T C::* _valuePtr, T _defaultValue, uint8_t _maxStrLen, T _min, T _max, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  ConfigParameter<C, T, F>(_id, _label, _valuePtr, _maxStrLen, _function, _rebootRequiredOnChange) {
  this->minValue = _min;
  this->maxValue = _max;
  if constexpr (std::is_floating_point_v<T>) {
    _ASSERT(_min <= _defaultValue || (_allowZeroValue && (_defaultValue == (T)0)));
    _ASSERT(_defaultValue <= _max || (_allowZeroValue && (_defaultValue == (T)0)));
  } else {
    _ASSERT(_min <= _defaultValue || (_allowZeroValue && (_defaultValue == (T)0)));
    _ASSERT(_defaultValue <= _max || (_allowZeroValue && (_defaultValue == (T)0)));
  }
  this->defaultValue = _defaultValue;
  this->allowZeroValue = _allowZeroValue;
}

template <typename C, typename T, typename F>
NumberConfigParameter<C, T, F>::~NumberConfigParameter() {};

template <typename C, typename T, typename F>
bool NumberConfigParameter<C, T, F>::isNumber() {
  return true;
}

template <typename C, typename T, typename F>
void NumberConfigParameter<C, T, F>::print(const C config, char* str) {
  sprintf(str, "%i", config.*(this->valuePtr));
}

template <typename C, typename T, typename F>
void NumberConfigParameter<C, T, F>::getMinimum(char* str) {
  if constexpr (std::is_unsigned_v<T>) {
    snprintf(str, this->maxStrLen + 1, "%u", this->minValue);
  } else  if constexpr (std::is_signed_v<T>) {
    snprintf(str, this->maxStrLen + 1, "%i", this->minValue);
  } else {
    _ASSERT("CHECK constexpr !!");
  }
}

template <typename C, typename T, typename F>
void NumberConfigParameter<C, T, F>::getMaximum(char* str) {
  if constexpr (std::is_unsigned_v<T>) {
    snprintf(str, this->maxStrLen + 1, "%u", this->maxValue);
  } else  if constexpr (std::is_signed_v<T>) {
    snprintf(str, this->maxStrLen + 1, "%i", this->maxValue);
  } else {
    _ASSERT("CHECK constexpr !!");
  }
}

template <typename C, typename T, typename F>
bool NumberConfigParameter<C, T, F>::canBeZero() {
  return this->allowZeroValue;
}

template <typename C, typename T, typename F>
void NumberConfigParameter<C, T, F>::setToDefault(C& config) {
  config.*(this->valuePtr) = this->defaultValue;
}

template <typename C, typename T, typename F>
void NumberConfigParameter<C, T, F>::toJson(const C config, DynamicJsonDocument* doc) {
  (*doc)[this->getId()] = config.*(this->valuePtr);
};

template <typename C, typename T, typename F>
int8_t NumberConfigParameter<C, T, F>::save(C& config, const char* str) {
  T value;
  if constexpr (std::is_floating_point_v<T>) {
    value = (T)atof(str);
  } else  if constexpr (std::is_unsigned_v<T>) {
    value = (T)std::stoul(str);
  } else  if constexpr (std::is_signed_v<T>) {
    value = (T)std::stol(str);
  } else {
    _ASSERT("CHECK constexpr !!");
  }
  if (config.*(this->valuePtr) == value) return CONFIG_PARAM_UNCHANGED;
  if ((value < this->minValue || value > this->maxValue) && !(this->allowZeroValue && value == (T)0)) {
    if constexpr (std::is_floating_point_v<T>) {
      ESP_LOGI(TAG, "Ignoring JSON value %f for %s outside range [%f,%f]", value, this->getId(), this->minValue, this->maxValue);
    } else if constexpr (std::is_signed_v<T>) {
      ESP_LOGI(TAG, "Ignoring JSON value %i for %s outside range [%i,%i]", value, this->getId(), this->minValue, this->maxValue);
    } else if constexpr (std::is_unsigned_v<T>) {
      if constexpr (std::is_same_v<T, uint32_t>) {
        ESP_LOGI(TAG, "Ignoring JSON value %04x%04x for %s outside range [%04x%04x,%04x%04x]",
          (uint16_t)(value >> 16), (uint16_t)(value & 0x0000fffful),
          this->getId(),
          (uint16_t)(this->minValue >> 16), (uint16_t)(this->minValue & 0x0000fffful),
          (uint16_t)(this->maxValue >> 16), (uint16_t)(this->maxValue & 0x0000fffful));
      } else {
        ESP_LOGI(TAG, "Ignoring JSON value %u for %s outside range [%u,%u]", value, this->getId(), this->minValue, this->maxValue);
      }
    } else {
      ESP_LOGE(TAG, "CHECK constexpr !!");
      ESP_LOGI(TAG, "Ignoring JSON value %s: outside range", this->getId());
    }
    return CONFIG_PARAM_OUT_OF_RANGE;
  }
  config.*(this->valuePtr) = value;
  return CONFIG_PARAM_UPDATED;
}

template <typename C, typename T, typename F>
int8_t NumberConfigParameter<C, T, F>::fromJson(C& config, DynamicJsonDocument* doc, bool useDefaultIfNotPresent) {
  if ((*doc).containsKey(this->getId())) {
    if ((*doc)[(const char*)this->getId()].is<T>()) {
      T value = (*doc)[(const char*)this->getId()].as<T>();
      if ((value < this->minValue || value > this->maxValue) && !(this->allowZeroValue && value == (T)0)) {
        if constexpr (std::is_floating_point_v<T>) {
          ESP_LOGI(TAG, "Ignoring JSON value %f for %s outside range [%f,%f]", value, this->getId(), this->minValue, this->maxValue);
        } else if constexpr (std::is_signed_v<T>) {
          ESP_LOGI(TAG, "Ignoring JSON value %i for %s outside range [%i,%i]", value, this->getId(), this->minValue, this->maxValue);
        } else if constexpr (std::is_unsigned_v<T>) {
          if constexpr (std::is_same_v<T, uint32_t>) {
            ESP_LOGI(TAG, "Ignoring JSON value %04x%04x for %s outside range [%04x%04x,%04x%04x]",
              (uint16_t)(value >> 16), (uint16_t)(value & 0x0000fffful),
              this->getId(),
              (uint16_t)(this->minValue >> 16), (uint16_t)(this->minValue & 0x0000fffful),
              (uint16_t)(this->maxValue >> 16), (uint16_t)(this->maxValue & 0x0000fffful));
          } else {
            ESP_LOGI(TAG, "Ignoring JSON value %u for %s outside range [%u,%u]", value, this->getId(), this->minValue, this->maxValue);
          }
        } else {
          ESP_LOGE(TAG, "CHECK constexpr !!");
          ESP_LOGI(TAG, "Ignoring JSON value %s: outside range", this->getId());
        }
        return CONFIG_PARAM_OUT_OF_RANGE;
      }
      if (config.*(this->valuePtr) != value) {
        config.*(this->valuePtr) = value;
        return CONFIG_PARAM_UPDATED;
      } else {
        return CONFIG_PARAM_UNCHANGED;
      }
    } else {
      return CONFIG_PARAM_WRONG_TYPE;
    }
  } else {
    if (useDefaultIfNotPresent) {
      this->setToDefault(config);
      return CONFIG_PARAM_UPDATED;
    } else {
      return CONFIG_PARAM_NOT_PRESENT;
    }
  }
};

template <typename C, typename T, typename F>
u_int16_t NumberConfigParameter<C, T, F>::getValueOrdinal(const C config) {
  return config.*(this->valuePtr) - minValue;
};
// -------------------- unsigned number -------------------

template <typename C, typename T, typename F>
UnsignedNumberConfigParameter<C, T, F>::UnsignedNumberConfigParameter(const char* _id, const char* _label, T C::* _valuePtr, T _defaultValue, uint8_t _maxStrLen, T _min, T _max, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  NumberConfigParameter<C, T, F>(_id, _label, _valuePtr, _defaultValue, _maxStrLen, _min, _max, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename T, typename F>
UnsignedNumberConfigParameter<C, T, F>::~UnsignedNumberConfigParameter() {};

template <typename C, typename T, typename F>
void UnsignedNumberConfigParameter<C, T, F>::print(const C config, char* str) {
  snprintf(str, this->maxStrLen + 1, "%u", config.*(this->valuePtr));
}


// -------------------- uint8_t -------------------
template <typename C, typename F>
Uint8ConfigParameter<C, F>::Uint8ConfigParameter(const char* _id, const char* _label, uint8_t C::* _valuePtr, uint8_t _defaultValue, uint8_t _min, uint8_t _max, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  UnsignedNumberConfigParameter<C, uint8_t, F>(_id, _label, _valuePtr, _defaultValue, 4, _min, _max, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Uint8ConfigParameter<C, F>::Uint8ConfigParameter(const char* _id, const char* _label, uint8_t C::* _valuePtr, uint8_t _defaultValue, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  Uint8ConfigParameter<C, F>(_id, _label, _valuePtr, _defaultValue, 0, 255, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Uint8ConfigParameter<C, F>::~Uint8ConfigParameter() {};


// -------------------- uint16_t -------------------
template <typename C, typename F>
Uint16ConfigParameter<C, F>::Uint16ConfigParameter(const char* _id, const char* _label, uint16_t C::* _valuePtr, uint16_t _defaultValue, uint16_t _min, uint16_t _max, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  UnsignedNumberConfigParameter<C, uint16_t, F>(_id, _label, _valuePtr, _defaultValue, 6, _min, _max, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Uint16ConfigParameter<C, F>::Uint16ConfigParameter(const char* _id, const char* _label, uint16_t C::* _valuePtr, uint16_t _defaultValue, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  Uint16ConfigParameter<C, F>(_id, _label, _valuePtr, _defaultValue, 0, 65535, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Uint16ConfigParameter<C, F>::~Uint16ConfigParameter() {}

// -------------------- uint32_t -------------------
template <typename C, typename F>
Uint32ConfigParameter<C, F>::Uint32ConfigParameter(const char* _id, const char* _label, uint32_t C::* _valuePtr, uint32_t _defaultValue, uint32_t _min, uint32_t _max, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  UnsignedNumberConfigParameter<C, uint32_t, F>(_id, _label, _valuePtr, _defaultValue, 10, _min, _max, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Uint32ConfigParameter<C, F>::Uint32ConfigParameter(const char* _id, const char* _label, uint32_t C::* _valuePtr, uint32_t _defaultValue, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  Uint32ConfigParameter<C, F>(_id, _label, _valuePtr, _defaultValue, 0, 0xfffffffful, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Uint32ConfigParameter<C, F>::~Uint32ConfigParameter() {}

// -------------------- int8_t -------------------
template <typename C, typename F>
Int8ConfigParameter<C, F>::Int8ConfigParameter(const char* _id, const char* _label, int8_t C::* _valuePtr, int8_t _defaultValue, int8_t _min, int8_t _max, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  NumberConfigParameter<C, int8_t, F>(_id, _label, _valuePtr, _defaultValue, 5, _min, _max, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Int8ConfigParameter<C, F>::Int8ConfigParameter(const char* _id, const char* _label, int8_t C::* _valuePtr, int8_t _defaultValue, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  Int8ConfigParameter<C, F>(_id, _label, _valuePtr, _defaultValue, -128, 127, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Int8ConfigParameter<C, F>::~Int8ConfigParameter() {};

// -------------------- int32_t -------------------
template <typename C, typename F>
Int32ConfigParameter<C, F>::Int32ConfigParameter(const char* _id, const char* _label, int32_t C::* _valuePtr, int32_t _defaultValue, int32_t _min, int32_t _max, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  NumberConfigParameter<C, int32_t, F>(_id, _label, _valuePtr, _defaultValue, 11, _min, _max, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Int32ConfigParameter<C, F>::Int32ConfigParameter(const char* _id, const char* _label, int32_t C::* _valuePtr, int32_t _defaultValue, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  Int32ConfigParameter<C, F>(_id, _label, _valuePtr, _defaultValue, -2147483647, 2147483646, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
Int32ConfigParameter<C, F>::~Int32ConfigParameter() {};


// -------------------- float -------------------
template <typename C, typename F>
FloatConfigParameter<C, F>::FloatConfigParameter(const char* _id, const char* _label, const char* _printFormatting, float C::* _valuePtr, float _defaultValue, float _min, float _max, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  NumberConfigParameter<C, float, F>(_id, _label, _valuePtr, _defaultValue, 10, _min, _max, _function, _allowZeroValue, _rebootRequiredOnChange) {
  this->printFormatting = _printFormatting;
}

template <typename C, typename F>
FloatConfigParameter<C, F>::FloatConfigParameter(const char* _id, const char* _label, const char* _printFormatting, float C::* _valuePtr, float _defaultValue, F _function, bool _allowZeroValue, bool _rebootRequiredOnChange) :
  FloatConfigParameter<C, F>(_id, _label, _printFormatting, _valuePtr, _defaultValue, FLT_MIN, FLT_MAX, _function, _allowZeroValue, _rebootRequiredOnChange) {}

template <typename C, typename F>
FloatConfigParameter<C, F>::~FloatConfigParameter() {};

template <typename C, typename F>
void FloatConfigParameter<C, F>::print(const C config, char* str) {
  sprintf(str, printFormatting, config.*(this->valuePtr));
}

template <typename C, typename F>
void FloatConfigParameter<C, F>::getMinimum(char* str) {
  snprintf(str, this->maxStrLen + 1, this->printFormatting, this->minValue);
}

template <typename C, typename F>
void FloatConfigParameter<C, F>::getMaximum(char* str) {
  snprintf(str, this->maxStrLen + 1, this->printFormatting, this->maxValue);
}

template <typename C, typename F>
bool FloatConfigParameter<C, F>::isFraction() {
  return true;
}


// -------------------- bool -------------------
template <typename C, typename F>
BooleanConfigParameter<C, F>::BooleanConfigParameter(const char* _id, const char* _label, bool C::* _valuePtr, bool _defaultValue, F _function, bool _rebootRequiredOnChange)
  :ConfigParameter<C, bool, F>(_id, _label, _valuePtr, 6, _function, _rebootRequiredOnChange) {
  this->defaultValue = _defaultValue;
}

template <typename C, typename F>
BooleanConfigParameter<C, F>::~BooleanConfigParameter() {}

template <typename C, typename F>
bool BooleanConfigParameter<C, F>::isBoolean() {
  return true;
}

template <typename C, typename F>
void BooleanConfigParameter<C, F>::print(const C config, char* str) {
  sprintf(str, "%s", config.*(this->valuePtr) ? "true" : "false");
}

template <typename C, typename F>
int8_t BooleanConfigParameter<C, F>::save(C& config, const char* str) {
  bool value = strncmp("true", str, 4) == 0 || strncmp("on", str, 2) == 0;
  if (config.*(this->valuePtr) == value) return CONFIG_PARAM_UNCHANGED;
  config.*(this->valuePtr) = value;
  return CONFIG_PARAM_UPDATED;
}

template <typename C, typename F>
void BooleanConfigParameter<C, F>::setToDefault(C& config) {
  config.*(this->valuePtr) = this->defaultValue;
}

template <typename C, typename F>
void BooleanConfigParameter<C, F>::toJson(const C config, DynamicJsonDocument* doc) {
  (*doc)[this->getId()] = config.*(this->valuePtr);
};

template <typename C, typename F>
int8_t BooleanConfigParameter<C, F>::fromJson(C& config, DynamicJsonDocument* doc, bool useDefaultIfNotPresent) {
  if ((*doc).containsKey(this->getId())) {
    if ((*doc)[(const char*)this->getId()].is<bool>()) {
      boolean value = (*doc)[(const char*)this->getId()].as<bool>();
      if (config.*(this->valuePtr) != value) {
        config.*(this->valuePtr) = value;
        return CONFIG_PARAM_UPDATED;
      } else {
        return CONFIG_PARAM_UNCHANGED;
      }
    } else {
      return CONFIG_PARAM_WRONG_TYPE;
    }
  } else {
    if (useDefaultIfNotPresent) {
      this->setToDefault(config);
      return CONFIG_PARAM_UPDATED;
    } else {
      return CONFIG_PARAM_NOT_PRESENT;
    }
  }
};

template <typename C, typename F>
u_int16_t BooleanConfigParameter<C, F>::getValueOrdinal(const C config) {
  return config.*(this->valuePtr);
};

// -------------------- char* -------------------
template <typename C, typename F>
CharArrayConfigParameter<C, F>::CharArrayConfigParameter(const char* _id, const char* _label, char C::* _valuePtr, const char* _defaultValue, uint8_t _maxLen, F _function, bool _rebootRequiredOnChange)
  :ConfigParameter<C, char, F>(_id, _label, _valuePtr, _maxLen, _function, _rebootRequiredOnChange) {
  _ASSERT(strlen(_defaultValue) <= _maxLen);
  this->defaultValue = _defaultValue;
}
template <typename C, typename F>
CharArrayConfigParameter<C, F>::~CharArrayConfigParameter() {}

template <typename C, typename F>
void CharArrayConfigParameter<C, F>::print(const C config, char* str) {
  sprintf(str, "%s", (char*)&(config.*(this->valuePtr)));
}

template <typename C, typename F>
int8_t CharArrayConfigParameter<C, F>::save(C& config, const char* str) {
  bool isSame = strncmp((char*)&(config.*(this->valuePtr)), str, this->maxStrLen) == 0;
  if (isSame) return CONFIG_PARAM_UNCHANGED;
  strncpy((char*)&(config.*(this->valuePtr)), str, min(strlen(str), (size_t)(this->maxStrLen + 1)));
  ((char*)&(config.*(this->valuePtr)))[min(strlen(str), (size_t)(this->maxStrLen + 1))] = 0x00;
  return CONFIG_PARAM_UPDATED;
}

template <typename C, typename F>
void CharArrayConfigParameter<C, F>::setToDefault(C& config) {
  this->save(config, defaultValue);
}

template <typename C, typename F>
void CharArrayConfigParameter<C, F>::toJson(const C config, DynamicJsonDocument* doc) {
  (*doc)[this->getId()] = (char*)&(config.*(this->valuePtr));
};

template <typename C, typename F>
int8_t CharArrayConfigParameter<C, F>::fromJson(C& config, DynamicJsonDocument* doc, bool useDefaultIfNotPresent) {
  if ((*doc).containsKey(this->getId())) {
    if ((*doc)[(const char*)this->getId()].is<const char*>()) {
      const char* fromJson = (*doc)[(const char*)this->getId()].as<const char*>();
      if (strncmp(fromJson, &(config.*(this->valuePtr)), (size_t)(this->maxStrLen + 1)) != 0) {
        strncpy((char*)&(config.*(this->valuePtr)), fromJson, min(strlen(fromJson), (size_t)(this->maxStrLen + 1)));
        ((char*)&(config.*(this->valuePtr)))[min(strlen(fromJson), (size_t)(this->maxStrLen + 1))] = 0x00;
        return CONFIG_PARAM_UPDATED;
      } else {
        return CONFIG_PARAM_UNCHANGED;
      }
    } else {
      return CONFIG_PARAM_WRONG_TYPE;
    }
  } else {
    if (useDefaultIfNotPresent) {
      this->setToDefault(config);
      return CONFIG_PARAM_UPDATED;
    } else {
      return CONFIG_PARAM_NOT_PRESENT;
    }
  }
};

template <typename C, typename F>
u_int16_t CharArrayConfigParameter<C, F>::getValueOrdinal(const C config) {
  _ASSERT(false);
};

// -------------------- enum -------------------

template <typename C, typename B, typename E, typename F>
EnumConfigParameter<C, B, E, F>::EnumConfigParameter(const char* _id, const char* _label, E C::* _valuePtr, E _defaultValue, const char* _enumLabels[], E _min, E _max, F _function, bool _rebootRequiredOnChange) :
  NumberConfigParameter<C, B, F>(_id, _label, (B C::*)_valuePtr, (B)_defaultValue, 0, (B)_min, (B)_max, false, _function, _rebootRequiredOnChange) {
  size_t maxLen = 0;
  for (B i = _min; i < _max;i++) {
    maxLen = max(maxLen, strlen(_enumLabels[i]));
  }
  this->maxStrLen = (uint8_t)maxLen;
  this->enumLabels = _enumLabels;
}

template <typename C, typename B, typename E, typename F>
EnumConfigParameter<C, B, E, F>::~EnumConfigParameter() {};

template <typename C, typename B, typename E, typename F>
void EnumConfigParameter<C, B, E, F>::print(const C config, char* str) {
  sprintf(str, "%s", this->enumLabels[(B)(config.*(this->valuePtr))]);
}

template <typename C, typename B, typename E, typename F>
int8_t EnumConfigParameter<C, B, E, F>::save(C& config, const char* str) {
  for (B i = (B)this->minValue; i <= (B)this->maxValue; i++) {
    ESP_LOGD(TAG, "%u %s ? %s", i, this->enumLabels[i], strncmp(this->enumLabels[i], str, strlen(this->enumLabels[i])) == 0 ? "true" : "false");
    if (strncmp(this->enumLabels[i], str, strlen(this->enumLabels[i])) == 0) {
      if (config.*(this->valuePtr) == (E)i) return CONFIG_PARAM_UNCHANGED;
      config.*(this->valuePtr) = (E)i;
      return CONFIG_PARAM_UPDATED;
    }
  }
  uint16_t value = (uint16_t)atoi(str);
  if (value < this->minValue || value > this->maxValue) {
    ESP_LOGI(TAG, "Ignoring parsed value %d for %s outside range [%u,%u]", value, this->getId(), this->minValue, this->maxValue);
    return CONFIG_PARAM_OUT_OF_RANGE;
  }
  if (config.*(this->valuePtr) == value) return CONFIG_PARAM_UNCHANGED;
  config.*(this->valuePtr) = value;
  return CONFIG_PARAM_UPDATED;
}

template <typename C, typename B, typename E, typename F>
bool EnumConfigParameter<C, B, E, F>::isEnum() {
  return true;
}

template <typename C, typename B, typename E, typename F>
bool EnumConfigParameter<C, B, E, F>::isNumber() {
  return false;
}

template <typename C, typename B, typename E, typename F>
const char** EnumConfigParameter<C, B, E, F>::getEnumLabels(void) {
  return this->enumLabels;
};

template <typename C, typename B, typename E, typename F>
u_int16_t EnumConfigParameter<C, B, E, F>::getValueOrdinal(const C config) {
  return (B)(config.*(this->valuePtr)) - (B)(this->minValue);
};

#include <configParameter.tpp>