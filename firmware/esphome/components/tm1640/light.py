import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID

from . import Tm1640, tm1640_ns

DEPENDENCIES = ["tm1640"]

Tm1640Light = tm1640_ns.class_("Tm1640Light", light.LightOutput)
CONF_TM1640_ID = "tm1640_id"
CONF_DIM_DARK = "dim_dark"
CONF_DIM_DARKER = "dim_darker"

CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(Tm1640Light),
        cv.GenerateID(CONF_TM1640_ID): cv.use_id(Tm1640),
        cv.Optional(CONF_DIM_DARK, default=0.5): cv.percentage,
        cv.Optional(CONF_DIM_DARKER, default=0.25): cv.percentage,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    display = await cg.get_variable(config[CONF_TM1640_ID])
    cg.add(var.set_display(display))
    cg.add(display.set_dim_dark_percent(round(config[CONF_DIM_DARK] * 100)))
    cg.add(display.set_dim_darker_percent(round(config[CONF_DIM_DARKER] * 100)))
