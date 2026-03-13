import emu
import imgui

class Button(emu.Device):
    def __init__(self, config):
        super().__init__()

        self.connector = emu.Connector("output", "emu.gpio:v1")
        self.register_output_connector(self.connector)

        self.last_button_state = False
        self.low_active = False

        if "low_active" in config and config["low_active"] == "true":
            self.low_active = True

    def init(self):
        if self.low_active:
            # Initialize to high for low active
            self.connector.call_callback(1, 0, b"")

    def draw_node(self):
        imgui.button("Press")
        pressed = imgui.is_item_active()
        if pressed != self.last_button_state:
            if self.low_active:
                value = 0 if pressed else 1
            else:
                value = 1 if pressed else 0
            self.connector.call_callback(value, 0, b"")
            self.last_button_state = pressed

emu.register_device("emu.py.devices.button", lambda config : Button(config))
