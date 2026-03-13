import emu

def stop_fuzzing_callback():
    emu.print("Breakpoint hit, stopping fuzzing...")
    emu.get_emulator().stop()

def on_fuzzing_start(emulator: emu.Emulator, input: bytes):
    emulator.add_breakpoint(0x800054e, 4, stop_fuzzing_callback)

    emulator.write_memory(0x24000000, len(input).to_bytes(4, "little"))
    emulator.write_memory(0x24000004, input)

# emu.on_fuzzing_start(on_fuzzing_start)

# This block loads fuzzing data from a file
# def on_start(emulator: emu.Emulator):
#     with open("afl_output/default/crashes/id:000000,sig:01,src:000002,time:18552,execs:2102,op:havoc,rep:1", "rb") as f:
#         input = f.read()
#         on_fuzzing_start(emulator, input)

# emu.on_start(on_start)
