import emu

def crc16_8005(data: bytes, crc: int = 0x0000) -> int:
    for b in data:
        crc ^= b << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x8005) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    return crc

class HMIPCoproc(emu.Device):
    def __init__(self):
        super().__init__()

        self.tx_connector = emu.Connector("tx", "emu.usart.data:v1")
        self.register_output_connector(self.tx_connector)

        self.register_input_connector(emu.Connector("rx", "emu.usart.data:v1", callback=self.tx_callback))

        self.packet_buffer = b""
        self.packet_len = 0
        self.escape = False

    def init(self):
        pass

    def draw_node(self):
        pass

    def send_data(self, data):
        if crc16_8005(data[1:], 0xffff) != 0:
            emu.print("HMIPCoproc: Sending data with invalid checksum")

        emu.print("HMIPCoproc: Command sent: " + data.hex())

        for b in data:
            self.tx_connector.call_callback(b, 0, b"")

    def match_packet_count(self, cmd, data):
        # remove checksum
        data = bytearray(data[:-2])
        # match packet counter
        data[4] = cmd[1]
        # append new checksum
        data += crc16_8005(data[1:], 0xffff).to_bytes(2, "big")
        return data

    def send_response(self, cmd, data):
        self.send_data(self.match_packet_count(cmd, data))

    def on_command_received(self, cmd: bytes):
        emu.print("HMIPCoproc: Command received: " + cmd.hex())

        # --snip--
        # Handle commands here

    def tx_callback(self, arg0, arg1, dataArg):
        # emu.print("HMIPCoproc: " + hex(arg0))

        if arg0 == 0xfd: # start of command
            self.packet_buffer = b""
            return 0
        elif arg0 == 0xfc: # escape byte
            self.escape = True
            return 0

        if self.escape:
            arg0 |= 0x80
            self.escape = False

        # add byte to packet buffer
        self.packet_buffer += bytes([arg0])

        # byte 1 and 2 are length
        if len(self.packet_buffer) == 2: 
            self.packet_len = self.packet_buffer[0] << 8 | self.packet_buffer[1]

        # if we received the 2 checksum bytes too we're done
        if len(self.packet_buffer) > 2 and len(self.packet_buffer) == 2 + self.packet_len + 2:
            if crc16_8005(self.packet_buffer, 0xffff) != 0:
                emu.print("HMIPCoproc: Received command with invalid checksum")
                return 0

            self.on_command_received(self.packet_buffer[2:2+self.packet_len])

        return 0

emu.register_device("emu.py.devices.hmip_coproc", lambda config : HMIPCoproc())

def start_timer_callback():
    # emu.print("Increasing timer delay")
    emu.get_emulator().set_register(emu.Register.R1, emu.get_emulator().get_register(emu.Register.R1) + 1000)

def on_start(emulator: emu.Emulator):
    # emulator.add_breakpoint(0x08020de4, 4, start_timer_callback)
    # emulator.add_breakpoint(0x08029510, 4, start_timer_callback)
    for addr in [0x08031ebe, 0x08032d8a, 0x080323b2]:
        emulator.add_breakpoint(addr, 4, start_timer_callback)

emu.on_start(on_start)
