import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from . import BRINK_VENTILATION_ID, BrinkOpenTherm

TYPES = {
    # Temperatury (OT 80-83)
    "T_SUPPLY_IN": ["Brink Temp Czerpnia (T1)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    "T_SUPPLY_OUT": ["Brink Temp Nawiew (T2)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    "T_EXHAUST_IN": ["Brink Temp Wywiew (T3)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    "T_EXHAUST_OUT": ["Brink Temp Wyrzutnia (T4)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],

    # Przepływ (Twoje TSP 52/53 jako 2 bajty)
    "CURRENT_FLOW": ["Brink Przepływ", "m³/h", 0, None],

    # Ciśnienia (Pa) - CPID/CPOD (TSP 64/65, 66/67)
    "CPID": ["Brink Ciśnienie nawiewu (CPID)", "Pa", 0, None],
    "CPOD": ["Brink Ciśnienie wywiewu (CPOD)", "Pa", 0, None],

    # U1/U2/U3: kroki wydatku (m3/h) – w OpenHAB są 2-bajtowe
    "U1": ["Brink U1 (step 1)", "m³/h", 0, None],
    "U2": ["Brink U2 (step 2)", "m³/h", 0, None],
    "U3": ["Brink U3 (step 3)", "m³/h", 0, None],

    # U4/U5: progi bypassu (°C), w protokole *2, więc w C++ dzielimy przez 2
    "U4": ["Brink U4 (bypass atmo threshold)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    "U5": ["Brink U5 (bypass indoor threshold)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],

    # I1: imbalance (wartość -100)
    "I1": ["Brink I1 (imbalance)", "%", 0, None],

    # Bypass status (0/1/2)
    "BYPASS_STATUS": ["Brink Bypass status", "", 0, None],
}

CONFIG_SCHEMA = sensor.sensor_schema(
    state_class=STATE_CLASS_MEASUREMENT,
).extend(
    {
        cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
        cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    conf_data = TYPES[config[CONF_TYPE]]

    var = await sensor.new_sensor(config)

    # jednostki / dokładność / device class
    if conf_data[1] is not None:
        cg.add(var.set_unit_of_measurement(conf_data[1]))
    cg.add(var.set_accuracy_decimals(conf_data[2]))
    if conf_data[3] is not None:
        cg.add(var.set_device_class(conf_data[3]))

    func = getattr(parent, f"set_{config[CONF_TYPE].lower()}_sensor")
    cg.add(func(var))
