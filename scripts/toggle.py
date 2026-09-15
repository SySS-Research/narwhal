import emu
import imgui

class Toggle(emu.Device):
    def __init__(self, config):
        super().__init__()

        self.connector = emu.Connector("output", "emu.gpio:v1")
        self.register_output_connector(self.connector)

        self.toggle_state = False
        self.low_active = False

        if "low_active" in config and config["low_active"] == "true":
            self.low_active = True

    def init(self):
        if self.low_active:
            # Initialize to high for low active
            self.connector.call_callback(1, 0, b"")

    def draw_node(self):
        pressed = imgui.checkbox("Toggle", self.toggle_state)
        self.toggle_state = pressed[1]
        if pressed[0]:
            if self.low_active:
                value = 0 if self.toggle_state else 1
            else:
                value = 1 if self.toggle_state else 0
            self.connector.call_callback(value, 0, b"")

emu.register_device("emu.py.devices.toggle", lambda config : Toggle(config))
