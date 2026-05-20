import emu

def function_callback():
    emu.print("Function hit, skipping...")

    # Return from function
    emulator = emu.get_emulator()
    emulator.set_register(emu.Register.PC, emulator.get_register(emu.Register.LR) | 1)

def patch_out_function(emulator: emu.Emulator, name: str):
    address = emulator.get_symbol_address(name)
    if address:
        emulator.add_breakpoint(address, 1, function_callback)
        emu.print(f"Breakpoint {name} added at address 0x{address:08x}")
    else:
        emu.print("Failed to find symbol " + name)

def on_start(emulator: emu.Emulator):
    patch_out_function(emulator, "sha_sync_sha256_start")
    patch_out_function(emulator, "sha_sync_sha256_update")
    patch_out_function(emulator, "sha_sync_sha256_finish")
    patch_out_function(emulator, "pukcc_sha256_compute")

emu.on_start(on_start)
