import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_TYPE, DEVICE_CLASS_PROBLEM

from . import BRINK_VENTILATION_ID, BrinkOpenTherm

TYPES = {
    "FILTER_STATUS": DEVICE_CLASS_PROBLEM,
}

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(
    device_class=DEVICE_CLASS_PROBLEM,
).extend(
    {
        cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
        cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    var = await binary_sensor.new_binary_sensor(config)
    func = getattr(parent, f"set_{config[CONF_TYPE].lower()}_binary")
    cg.add(func(var))
