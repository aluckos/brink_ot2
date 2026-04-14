#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

class BrinkOpenTherm;

// Number: główny suwak wentylacji (OT ID 71)
class BrinkNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_{nullptr};
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override;
};

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot{nullptr};
  int pin_in{0}, pin_out{0};

  // sterowanie (ID 71)
  float target_ventilation_{25.0f};

  // pomocnicze do 2-bajtowych TSP
  uint8_t tsp_low_byte_{0};

  // --- encje ESPHome (wsk. ustawiane z pythonowych platform) ---
  // Temperatury (OT IDs 80-83)
  sensor::Sensor *t_supply_in_sensor{nullptr};    // ID 80
  sensor::Sensor *t_supply_out_sensor{nullptr};   // ID 81
  sensor::Sensor *t_exhaust_in_sensor{nullptr};   // ID 82
  sensor::Sensor *t_exhaust_out_sensor{nullptr};  // ID 83

  // przepływ (u Ciebie: TSP 52/53 jako 2 bajty)
  sensor::Sensor *current_flow_sensor{nullptr};

  // Dodatkowe TSP 2-bajtowe
  sensor::Sensor *cpid_sensor{nullptr};  // 64/65
  sensor::Sensor *cpod_sensor{nullptr};  // 66/67

  sensor::Sensor *u1_sensor{nullptr};  // 0/1
  sensor::Sensor *u2_sensor{nullptr};  // 2/3
  sensor::Sensor *u3_sensor{nullptr};  // 4/5

  // Dodatkowe TSP 1-bajtowe
  sensor::Sensor *u4_sensor{nullptr};  // 6 (wartość /2 => °C)
  sensor::Sensor *u5_sensor{nullptr};  // 7 (wartość /2 => °C)
  sensor::Sensor *i1_sensor{nullptr};  // 9 (wartość -100)

  // Bypass status jako tekst i jako liczba
  sensor::Sensor *bypass_status_sensor{nullptr};        // surowy 0/1/2
  text_sensor::TextSensor *bypass_status_text{nullptr}; // opis

  // Filtr + status komunikacji
  binary_sensor::BinarySensor *filter_status_binary{nullptr};
  text_sensor::TextSensor *status_text_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }

  // --- settery sensorów (muszą odpowiadać nazwom z sensor.py) ---
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; }
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; }

  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }

  void set_cpid_sensor(sensor::Sensor *s) { cpid_sensor = s; }
  void set_cpod_sensor(sensor::Sensor *s) { cpod_sensor = s; }

  void set_u1_sensor(sensor::Sensor *s) { u1_sensor = s; }
  void set_u2_sensor(sensor::Sensor *s) { u2_sensor = s; }
  void set_u3_sensor(sensor::Sensor *s) { u3_sensor = s; }
  void set_u4_sensor(sensor::Sensor *s) { u4_sensor = s; }
  void set_u5_sensor(sensor::Sensor *s) { u5_sensor = s; }
  void set_i1_sensor(sensor::Sensor *s) { i1_sensor = s; }

  void set_bypass_status_sensor(sensor::Sensor *s) { bypass_status_sensor = s; }
  void set_bypass_status_text(text_sensor::TextSensor *s) { bypass_status_text = s; }

  void set_filter_status_binary(binary_sensor::BinarySensor *s) { filter_status_binary = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor = s; }

  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override;
  void update() override;

 protected:
  // Prosta „kolejka” polling: odpytywanie po jednym elemencie na update()
  // żeby nie zajechać OT i zachować cykliczną komunikację.
  uint8_t step_{0};

  inline void publish_bypass_text_(uint8_t v) {
    if (!bypass_status_text) return;
    switch (v) {
      case 0:
        bypass_status_text->publish_state("closed");
        break;
      case 1:
        bypass_status_text->publish_state("automatic");
        break;
      case 2:
        bypass_status_text->publish_state("inlet_minimum");
        break;
      default:
        bypass_status_text->publish_state("unknown");
        break;
    }
  }
};

// --- IMPLEMENTACJA ---

static BrinkOpenTherm *global_brink_ot = nullptr;

static void IRAM_ATTR handleInterrupt() {
  if (global_brink_ot != nullptr && global_brink_ot->ot != nullptr) {
    global_brink_ot->ot->handleInterrupt();
  }
}

inline void BrinkOpenTherm::setup() {
  global_brink_ot = this;
  ot = new OpenTherm(pin_in, pin_out);
  ot->begin(handleInterrupt);
}

inline void BrinkNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_ != nullptr) {
    this->parent_->target_ventilation_ = value;
  }
}

inline void BrinkOpenTherm::update() {
  if (ot == nullptr) return;

  // keep-alive (ID 0)
  ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 0, 0x0100));
  if (status_text_sensor != nullptr) status_text_sensor->publish_state("connected");

  unsigned long response = 0;

  // krokami odczytujemy kolejne parametry
  switch (step_) {
    case 0:  // WRITE: wentylacja ID 71
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID) 71,
                                       (unsigned int) target_ventilation_));
      step_++;
      break;

    case 1:  // T1 ID 80
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 80, 0));
      if (response && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
      step_++;
      break;

    case 2:  // T2 ID 81
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 81, 0));
      if (response && t_supply_out_sensor) t_supply_out_sensor->publish_state(ot->getFloat(response));
      step_++;
      break;

    case 3:  // T3 ID 82
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 82, 0));
      if (response && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
      step_++;
      break;

    case 4:  // T4 ID 83
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 83, 0));
      if (response && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state(ot->getFloat(response));
      step_++;
      break;

    // --- TSP 2-bajtowe (two-step: low, high) ---
    case 5:  // FLOW low: TSP 52 (LB)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 52 << 8));
      if (response) tsp_low_byte_ = (uint8_t) (response & 0xFF);
      step_++;
      break;

    case 6:  // FLOW high: TSP 53 (HB)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 53 << 8));
      if (response && current_flow_sensor) {
        current_flow_sensor->publish_state(((uint16_t) (response & 0xFF) << 8) | tsp_low_byte_);
      }
      step_++;
      break;

    case 7:  // Filter: TSP 13
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 13 << 8));
      if (response && filter_status_binary) {
        filter_status_binary->publish_state((response & 0xFF) == 1);
      }
      step_++;
      break;

    // --- bypass status: TSP 55 ---
    case 8:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 55 << 8));
      if (response) {
        uint8_t v = (uint8_t) (response & 0xFF);
        if (bypass_status_sensor) bypass_status_sensor->publish_state(v);
        publish_bypass_text_(v);
      }
      step_++;
      break;

    // --- CPID: 64/65 (2 bytes) ---
    case 9:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 64 << 8));
      if (response) tsp_low_byte_ = (uint8_t) (response & 0xFF);
      step_++;
      break;

    case 10:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 65 << 8));
      if (response && cpid_sensor) cpid_sensor->publish_state(((uint16_t) (response & 0xFF) << 8) | tsp_low_byte_);
      step_++;
      break;

    // --- CPOD: 66/67 (2 bytes) ---
    case 11:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 66 << 8));
      if (response) tsp_low_byte_ = (uint8_t) (response & 0xFF);
      step_++;
      break;

    case 12:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 67 << 8));
      if (response && cpod_sensor) cpod_sensor->publish_state(((uint16_t) (response & 0xFF) << 8) | tsp_low_byte_);
      step_++;
      break;

    // --- U1: 0/1 (2 bytes) ---
    case 13:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 0 << 8));
      if (response) tsp_low_byte_ = (uint8_t) (response & 0xFF);
      step_++;
      break;

    case 14:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 1 << 8));
      if (response && u1_sensor) u1_sensor->publish_state(((uint16_t) (response & 0xFF) << 8) | tsp_low_byte_);
      step_++;
      break;

    // --- U2: 2/3 ---
    case 15:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 2 << 8));
      if (response) tsp_low_byte_ = (uint8_t) (response & 0xFF);
      step_++;
      break;

    case 16:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 3 << 8));
      if (response && u2_sensor) u2_sensor->publish_state(((uint16_t) (response & 0xFF) << 8) | tsp_low_byte_);
      step_++;
      break;

    // --- U3: 4/5 ---
    case 17:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 4 << 8));
      if (response) tsp_low_byte_ = (uint8_t) (response & 0xFF);
      step_++;
      break;

    case 18:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 5 << 8));
      if (response && u3_sensor) u3_sensor->publish_state(((uint16_t) (response & 0xFF) << 8) | tsp_low_byte_);
      step_++;
      break;

    // --- U4 (temp threshold atmo): index 6, value/2 => °C ---
    case 19:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 6 << 8));
      if (response && u4_sensor) u4_sensor->publish_state(((uint8_t) (response & 0xFF)) / 2.0f);
      step_++;
      break;

    // --- U5 (temp threshold indoor): index 7, value/2 => °C ---
    case 20:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 7 << 8));
      if (response && u5_sensor) u5_sensor->publish_state(((uint8_t) (response & 0xFF)) / 2.0f);
      step_++;
      break;

    // --- I1 (imbalance): index 9, value-100 ---
    case 21:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID) 89, 9 << 8));
      if (response && i1_sensor) i1_sensor->publish_state(((int) ((uint8_t) (response & 0xFF))) - 100);
      step_ = 0;  // wracamy do początku
      break;

    default:
      step_ = 0;
      break;
  }
}

}  // namespace brink_ventilation
}  // namespace esphome
