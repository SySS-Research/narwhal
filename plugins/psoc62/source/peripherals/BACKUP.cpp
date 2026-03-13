#include "BACKUP.hpp"

namespace psoc
{

BACKUP::BACKUP() : Peripheral()
{
}

BACKUP::~BACKUP()
{
}

uint32_t BACKUP::Read(uint32_t offset, uint32_t size)
{
    switch (offset) {
        case 0x10: { // BACKUP_STATUS
            STATUS status(0);
            status.setWCO_OK();
            return status;
        }
    };

    EMU_LOG_WARN("Unknown read at offset 0x{:x}, size = {}", offset, size);
    return 0;
}

void BACKUP::Write(uint32_t offset, uint32_t size, uint32_t value)
{
    switch (offset) {
        default:
            EMU_LOG_WARN("Unknown write at offset 0x{:x}, size = {}, value = 0x{:x}", offset, size, value);
            break;
    }
}

} // namespace psoc
