import emu
import imgui

class TestWindow(emu.Window):
    def __init__(self):
        super().__init__("Test Window")

        self.count = 0

    def draw(self):
        imgui.text("Hello world from python")
        imgui.text(f"Count {self.count}")

        if imgui.button("Click me"):
            self.count += 1

emu.register_window(TestWindow())
