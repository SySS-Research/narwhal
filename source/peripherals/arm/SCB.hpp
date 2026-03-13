#pragma once

#include <Peripheral.hpp>
#include <arm.hpp>

namespace emu
{

class SCB : public Peripheral {
public:
    SCB();
    virtual ~SCB();

    virtual uint32_t Read(uint32_t offset, uint32_t size) override;
    virtual void Write(uint32_t offset, uint32_t size, uint32_t value) override;

    uint32_t GetVTOR() const { return mVTOR; };
    void SetVTOR(uint32_t vtor) { mVTOR = vtor; };

    // TODO use this and the other bits
    void SetVECTACTIVE(uint8_t exceptionNumber) { mICSR.setVECTACTIVE(exceptionNumber); }

public:
    class ICSR : public arm::RegisterBase
    {
    public:
        inline ICSR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(NMIPENDSET, 31);
        REGISTER_BASE_BIT(PENDSVSET, 28);
        REGISTER_BASE_BIT(PENDSVCLR, 27);
        REGISTER_BASE_BIT(PENDSTSET, 26);
        REGISTER_BASE_BIT(PENDSTCLR, 25);
        REGISTER_BASE_BIT(ISRPREEMPT, 23);
        REGISTER_BASE_BIT(ISRPENDING, 22);
        REGISTER_BASE_RANGE(VECTPENDING, 20, 12);
        REGISTER_BASE_BIT(RETTOBASE, 11);
        REGISTER_BASE_RANGE(VECTACTIVE, 8, 0);
    };

    class AIRCR : public arm::RegisterBase
    {
    public:
        inline AIRCR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_RANGE(VECTKEY, 31, 16);
        REGISTER_BASE_BIT(ENDIANNESS, 15);
        REGISTER_BASE_RANGE(PRIGROUP, 10, 8);
        REGISTER_BASE_BIT(SYSRESETREQ, 2);
        REGISTER_BASE_BIT(VECTCLRACTIVE, 1);
        REGISTER_BASE_BIT(VECTRESET, 0);
    };

    class SHCSR : public arm::RegisterBase
    {
    public:
        inline SHCSR(uint32_t value) : RegisterBase(value) {}

        REGISTER_BASE_BIT(USGFAULTENA, 18);
        REGISTER_BASE_BIT(BUSFAULTENA, 17);
        REGISTER_BASE_BIT(MEMFAULTENA, 16);

        REGISTER_BASE_BIT(SVCALLPENDED, 15);
        REGISTER_BASE_BIT(BUSFAULTPENDED, 14);
        REGISTER_BASE_BIT(MEMFAULTPENDED, 13);
        REGISTER_BASE_BIT(USGFAULTPENDED, 12);

        REGISTER_BASE_BIT(SYSTICKACT, 11);
        REGISTER_BASE_BIT(PENDSVACT, 10);
        REGISTER_BASE_BIT(MONITORACT, 8);
        REGISTER_BASE_BIT(SVCALLACT, 7);
        REGISTER_BASE_BIT(USGFAULTACT, 3);
        REGISTER_BASE_BIT(BUSFAULTACT, 1);
        REGISTER_BASE_BIT(MEMFAULTACT, 0);
    };

    const AIRCR& GetAIRCR() const { return mAIRCR; }

protected:
    uint32_t mVTOR;
    ICSR mICSR;
    AIRCR mAIRCR;
};

} // namespace emu
