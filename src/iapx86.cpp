//
// Created by Julien Peloille on 4/4/21.
// jpeloille@hiram-dev.com
//

/*
 * Physical Address = Segment << 4 | Offset.
 */

#include <cstdio>
#include "intel.h"
#include "iapx86.h"

int cycles = 0;
uint8_t opcode;
uint32_t eaaddr;
uint32_t easeg;
uint8_t rm,reg,mod,ModRM;
uint8_t mem[0xFFFFF];

uint8_t bLeftOperand;
uint8_t bRightOperand;
uint8_t bResult;

uint16_t wLeftOperand;
uint16_t wRightOperand;
uint16_t  wResult;

uint16_t wOffset;
uint16_t wSegbase;
uint8_t  disp8;
uint16_t disp16;

uint8_t pzsTable8[256];
uint8_t pzsTable16[65536];

uint8_t startCycleCarryFlag;

uint16_t iCS; //Current instruction Code Stack;
uint16_t iIP; //current instruction Instruction Pointer;

void populatePzsTables()
{
    //Pour éviter de consommer du temps on génére dans une table les valeurs
    //des drapeaux du signe, de la parité et du zéro. Ainsi, lors des opérations
    //arithmétiques et logiques il y aura un accès direct au sein de ce tableau
    //avec pout entrée le résultat.

    int c,d;

    for (c=0;c<256;c++)
    {
        pzsTable8[c] = 0;

        d=0;
        if (c&1) d++;
        if (c&2) d++;
        if (c&4) d++;
        if (c&8) d++;
        if (c&16) d++;
        if (c&32) d++;
        if (c&64) d++;
        if (c&128) d++;

        if (d%2==0) pzsTable8[c]=P_FLAG;
        else pzsTable8[c]=0;

        if (!c) pzsTable8[c]|=Z_FLAG;
        if (c&0x80) pzsTable8[c]|=S_FLAG;
    }

    for (c=0;c<65536;c++)
    {
        d=0;

        if (c&1) d++;
        if (c&2) d++;
        if (c&4) d++;
        if (c&8) d++;
        if (c&16) d++;
        if (c&32) d++;
        if (c&64) d++;
        if (c&128) d++;

        if (c&256) d++;
        if (c&512) d++;
        if (c&1024) d++;
        if (c&2048) d++;
        if (c&4096) d++;
        if (c&8192) d++;
        if (c&16384) d++;
        if (c&32768) d++;

        if (d%2==0) pzsTable16[c]=P_FLAG;
        else pzsTable16[c]=false;

        if (!c) pzsTable16[c]|=Z_FLAG;
        if (c&0x8000) pzsTable16[c]|=S_FLAG;

    }

}

iapx86::iapx86()
{
    populatePzsTables();

    // Populate instDecoder table :
    /* 0x00 - 0x0F ==================================*/
    instDecoder[0x00] = &iapx86::ADD_EAb_REGb;
    instDecoder[0x01] = &iapx86::ADD_EAw_REGw;
    instDecoder[0x02] = &iapx86::ADD_REGb_EAb;
    instDecoder[0x03] = &iapx86::ADD_REGw_EAw;
    instDecoder[0x04] = &iapx86::ADD_AL_Data8;
    instDecoder[0x05] = &iapx86::ADD_AX_Data16;
    instDecoder[0x06] = &iapx86::PUSH_ES;
    instDecoder[0x07] = &iapx86::POP_ES;
    instDecoder[0x08] = &iapx86::OR_EAb_REGb;
    instDecoder[0x09] = &iapx86::OR_EAw_REGw;
    instDecoder[0x0A] = &iapx86::OR_REGb_EAb;
    instDecoder[0x0B] = &iapx86::OR_REGw_EAw;
    instDecoder[0x0C] = &iapx86::OR_AL_Data8;
    instDecoder[0x0D] = &iapx86::OR_AX_Data16;
    instDecoder[0x0E] = &iapx86::PUSH_CS;

    /* 0x10 - 0x1F ==================================*/
    instDecoder[0x10] = &iapx86::ADC_EAb_REGb;
    instDecoder[0x11] = &iapx86::ADC_EAw_REGw;
    instDecoder[0x12] = &iapx86::ADC_REGb_EAb;
    instDecoder[0x13] = &iapx86::ADC_REGw_EAw;
    instDecoder[0x14] = &iapx86::ADC_AL_Data8;
    instDecoder[0x15] = &iapx86::ADC_AX_Data16;
    instDecoder[0x16] = &iapx86::PUSH_SS;
    instDecoder[0x17] = &iapx86::POP_SS;
    /*instDecoder[0x18] = &iapx86::SBB_EAb_REGb;
    instDecoder[0x19] = &iapx86::SBB_EAw_REGw;
    instDecoder[0x1A] = &iapx86::SBB_REGb_EAb;
    instDecoder[0x1B] = &iapx86::SBB_REGw_EAw;
    instDecoder[0x1C] = &iapx86::SBB_AL_Data8;
    instDecoder[0x1D] = &iapx86::SBB_AX_Data16;
    instDecoder[0x1E] = &iapx86::PUSH_DS;
    instDecoder[0x1f] = &iapx86::PUSH_DS;*/


    /* 0xB0 - 0xBF ==================================*/
    instDecoder[0xB0] = &iapx86::mov_AL_ib;

    /* 0x80 - 0x8F ==================================*/
    instDecoder[0x8C] = &iapx86::mov_rmw_sr;
    instDecoder[0x8E] = &iapx86::mov_sr_rmw;

    /* 0xE0 - 0xEF ==================================*/
    instDecoder[0xEA] = &iapx86::JMP_FAR_DIRECT;
}

iapx86::~iapx86()
{

}

int iapx86::loadbios()
{
    FILE *f=NULL;
    f=fopen("../rom/OHPC_00.ROM","rb");
    if (!f) return 0;
    fread(mem+0xFE000,8192,1,f);
    fclose(f);

    uint32_t adress = CS<<4 | IP;
    printf("Bios loaded.");
    return 1;
}

void iapx86::cpuReset()
{
    ES = 0x0000;
    CS = 0xFFFF;
    SS = 0x0000;
    DS = 0x0000;
    IP = 0x0000;

    loadbios();
}

//= Fetch & decode ModRM byte ========================================================================================//
void iapx86::FetchAndDecode_ModRMByte()
    {
        ModRM=getImmediateByte();

        reg=(ModRM>>3)&7;
        mod=ModRM>>6;
        rm=ModRM&7;

        switch (mod)
        {
            case 0:
                switch (rm)
                {
                    case 0: eaaddr=BX+SI; easeg=DS; cycles-=7; break;
                    case 1: eaaddr=BX+DI; easeg=DS; cycles-=8; break;
                    case 2: eaaddr=BP+SI; easeg=SS; cycles-=8; break;
                    case 3: eaaddr=BP+DI; easeg=SS; cycles-=7; break;
                    case 4: eaaddr=SI; easeg=DS; cycles-=5; break;
                    case 5: eaaddr=DI; easeg=DS; cycles-=5; break;
                    case 6: eaaddr=getImmediateWord(); easeg=DS; cycles-=6; break;
                    case 7: eaaddr=BX; easeg=DS; cycles-=5; break;
                }
                break;

            case 1:
                disp8=getImmediateByte();
                //if (disp8&0x80) disp8|=0xFF00;
                switch (rm)
                {
                    case 0: eaaddr=BX+SI+disp8; easeg=DS; cycles-=11; break;
                    case 1: eaaddr=BX+DI+disp8; easeg=DS; cycles-=12; break;
                    case 2: eaaddr=BP+SI+disp8; easeg=SS; cycles-=12; break;
                    case 3: eaaddr=BP+DI+disp8; easeg=SS; cycles-=11; break;
                    case 4: eaaddr=SI+disp8; easeg=DS; cycles-=9; break;
                    case 5: eaaddr=DI+disp8; easeg=DS; cycles-=9; break;
                    case 6: eaaddr=BP+disp8; easeg=SS; cycles-=9; break;
                    case 7: eaaddr=BX+disp8; easeg=DS; cycles-=9; break;
                }
                break;

            case 2:
                disp16=getImmediateWord();
                switch (rm)
                {
                    case 0: eaaddr=BX+SI+disp16; easeg=DS; cycles-=11; break;
                    case 2: eaaddr=BP+SI+disp16; easeg=SS; cycles-=12; break;
                    case 3: eaaddr=BP+DI+disp16; easeg=SS; cycles-=11; break;
                    case 4: eaaddr=SI+disp16; easeg=DS; cycles-=9; break;
                    case 5: eaaddr=DI+disp16; easeg=DS; cycles-=9; break;
                    case 6: eaaddr=BP+disp16; easeg=SS; cycles-=9; break;
                    case 7: eaaddr=BX+disp16; easeg=DS; cycles-=9; break;
                }
                break;
        }
        eaaddr&=0xFFFF;
        //printf(" - (%02X-%02X-%02X)", mod, reg, rm);
    }

//= R/W Register =====================================================================================================//
inline uint8_t iapx86::ReadByte_Register()
{
    return ((reg&4)?iapx86_Registers[reg&3].b.h:iapx86_Registers[reg&3].b.l);
}

inline uint16_t iapx86::ReadWord_Register()
{
    if (mod==3)
        return iapx86_Registers[rm].w;

    cycles-=3;
    return ReadWord_Memory(easeg << 4 | eaaddr);
}

void iapx86::WriteByte_Register(uint8_t data)
{
    if (reg&4) iapx86_Registers[reg&3].b.h=data;
    else iapx86_Registers[reg&3].b.l=data;
}

void iapx86::WriteWord_Register(uint16_t data)
{
    iapx86_Registers[rm].w = data;
}

//= R/W Effective address ============================================================================================//
inline uint8_t iapx86::ReadByte_EffectiveAddress()
{
    if (mod==3)
        return (rm&4)?iapx86_Registers[rm&3].b.h:iapx86_Registers[rm&3].b.l;

    cycles-=3;
    return ReadByte_Memory((easeg << 4 | eaaddr));
}

inline uint16_t iapx86::ReadWord_EffectiveAddress()
{
    if (mod==3)
        return iapx86_Registers[rm].w;

    cycles-=3;
    return  ReadWord_Memory(easeg <<4 | eaaddr);
}

inline void iapx86::WriteByte_EffectiveAddress(uint8_t data)
{
    if (mod==3)
    {
        if (rm&4) iapx86_Registers[rm&3].b.h=data;
        else iapx86_Registers[rm&3].b.l=data;
    }
    else
    {
        cycles-=2;
        WriteByte_Memory((easeg << 4 | eaaddr),data);
    }
}

inline void iapx86::WriteWord_EffectiveAddress(uint16_t data)
{
    if (mod==3)
    {
        iapx86_Registers[rm&3].w=data;
    }
    else
    {
        WriteWord_Memory(easeg << 4 | eaaddr, data);
    }
}

//= Memory ===========================================================================================================//
uint8_t iapx86::ReadByte_Memory(uint32_t addr)
{
    cycles--;
    return mem[addr];
}

inline uint16_t iapx86::ReadWord_Memory(uint32_t addr)
{
    uint8_t low = mem[addr];
    cycles--;

    uint8_t  high = mem[addr++];
    cycles--;

    return (high << 8 | low);
}

void iapx86::WriteByte_Memory(uint32_t  physicalAddress, uint8_t data)
{
    mem[physicalAddress]=data;
}

void iapx86::WriteWord_Memory(uint32_t physicalAddress, uint16_t data)
{
    mem[physicalAddress]=data;
    mem[physicalAddress++]=(data>>8);
}

//====================================================================================================================//
uint8_t iapx86::getImmediateByte()
{
    uint8_t byte = mem[CS <<4 | IP];
    IP++; cycles--;
    return byte;
}

uint16_t iapx86::getImmediateWord()
{
    uint8_t low = mem[CS << 4 | IP];
    IP++; cycles--;

    uint8_t  high = mem[CS << 4 | IP];
    IP++; cycles--;

    return (high << 8 | low);
}

//= Arithmetics methods ==============================================================================================//
uint8_t iapx86::Add8(uint8_t leftOperand, uint8_t rightOperand)
{
    uint16_t result16 = leftOperand + rightOperand;
    uint8_t result8 = (uint16_t) result16;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x8D5;
    iapx86_FlagsRegister |= pzsTable8[result8 & 0xFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (result16 & 0x100)
        iapx86_FlagsRegister |= C_FLAG;
    if (!((leftOperand^rightOperand)&0x80)&&((leftOperand^result16)&0x80))
        iapx86_FlagsRegister |= O_FLAG;

    return result8;
}

uint8_t iapx86::Adc8(uint8_t leftOperand, uint8_t rightOperand)
{
    uint16_t result16 = leftOperand + rightOperand + startCycleCarryFlag;
    uint8_t result8 = (uint16_t) result16;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x8D5;
    iapx86_FlagsRegister |= pzsTable8[result8 & 0xFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (result16 & 0x100)
        iapx86_FlagsRegister |= C_FLAG;
    if (!((leftOperand^rightOperand)&0x80)&&((leftOperand^result16)&0x80))
        iapx86_FlagsRegister |= O_FLAG;

    return result8;
}

uint16_t iapx86::Add16(uint16_t leftOperand, uint16_t rightOperand)
{
    uint32_t u32Result = leftOperand + rightOperand;
    uint16_t u16Result = (uint16_t)u32Result;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x08D5;
    iapx86_FlagsRegister |= pzsTable16[u16Result&0xFFFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (u32Result & 0x10000)
        iapx86_FlagsRegister |= C_FLAG;
    if (!((leftOperand^rightOperand)&0x8000)&&((leftOperand^u32Result)&0x8000))
        iapx86_FlagsRegister |= O_FLAG;

    return u16Result;
}

uint16_t iapx86::Adc16(uint16_t leftOperand, uint16_t rightOperand)
{
    uint32_t u32Result = leftOperand + rightOperand + startCycleCarryFlag;
    uint16_t u16Result = (uint16_t)u32Result;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x08D5;
    iapx86_FlagsRegister |= pzsTable16[u16Result&0xFFFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (u32Result & 0x10000)
        iapx86_FlagsRegister |= C_FLAG;
    if (!((leftOperand^rightOperand)&0x8000)&&((leftOperand^u32Result)&0x8000))
        iapx86_FlagsRegister |= O_FLAG;

    return u16Result;
}

uint8_t iapx86::Sub8(uint8_t leftOperand, uint8_t rightOperand)
{
    /* SUB (Subtract) :
     * Operation: The source (rightmost) operand is subtracted from the destination (left-
     * most) operand and the result is stored in the destination.
     *
     *  (DEST) <- (LSRC)-(RSRC)
     */

    uint16_t result16 = (uint16_t)leftOperand - (uint16_t)rightOperand;
    uint8_t result8 = (uint8_t)result16;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x8D5;
    iapx86_FlagsRegister |= pzsTable8[result8 & 0xFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (result16 & 0x100)
        iapx86_FlagsRegister |= C_FLAG;
    if (!((leftOperand^rightOperand)&0x80)&&((leftOperand^result16)&0x80))
        iapx86_FlagsRegister |= O_FLAG;

    return result8;
}

uint8_t iapx86::Sbb8(uint8_t leftOperand, uint8_t rightOperand)
{
    /* SBB (Subtract with borrow) :
     * Operation: The source (rightmost) operand is subtracted from the destination (left-
     * most). If the carry flag was set, 1 is subtracted from the above result. The result
     * replaces the original destination operand.
     *
     *  if (CF) = 1 then (DEST) <- (LSRC)-(RSRC)-1
     *      else (DEST) <- (LSRC)-(RSRC)
     */

    uint16_t result16 = (uint16_t)leftOperand - (uint16_t)rightOperand - startCycleCarryFlag;
    uint8_t result8 = (uint8_t)result16;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x8D5;
    iapx86_FlagsRegister |= pzsTable8[result8 & 0xFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (result16 & 0x100)
        iapx86_FlagsRegister |= C_FLAG;
    if (!((leftOperand^rightOperand)&0x80)&&((leftOperand^result16)&0x80))
        iapx86_FlagsRegister |= O_FLAG;

    return result8;
}

uint16_t iapx86::Sub16(uint16_t leftOperand, uint16_t rightOperand)
{
    uint32_t result32 = (uint32_t)leftOperand - (uint32_t)rightOperand;
    uint16_t result16 = (uint16_t)result32;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF : */
    iapx86_FlagsRegister &= ~0x08D5;
    iapx86_FlagsRegister |= pzsTable16[result16&0xFFFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (result32 & 0x10000)
        iapx86_FlagsRegister |= C_FLAG;
    if (!((leftOperand^rightOperand)&0x8000)&&((leftOperand^result32)&0x8000))
        iapx86_FlagsRegister |= O_FLAG;

    return result16;
}

uint16_t iapx86::Sbb16(uint16_t leftOperand, uint16_t rightOperand)
{
    uint32_t result32 = (uint32_t)leftOperand - (uint32_t)rightOperand - startCycleCarryFlag;
    uint16_t result16 = (uint16_t)result32;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF : */
    iapx86_FlagsRegister &= ~0x08D5;
    iapx86_FlagsRegister |= pzsTable16[result16&0xFFFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (result32 & 0x10000)
        iapx86_FlagsRegister |= C_FLAG;
    if (!((leftOperand^rightOperand)&0x8000)&&((leftOperand^result32)&0x8000))
        iapx86_FlagsRegister |= O_FLAG;

    return result16;
}

uint8_t iapx86::Inc8(uint8_t leftOperand, uint8_t rightOperand)
{
    /* INC (Increment destination by 1)
     * Operation: The specified operand is incremented by 1. There is no carry out of the
     * most-significant bit.
     *      (DEST) <- (DEST) + 1
     * CAUTION : 'rightOperand' must be 1.
     */

    uint16_t result16 = leftOperand + rightOperand;
    uint8_t result8 = (uint16_t) result16;

    /* Flags Affected: AF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x8D4;
    iapx86_FlagsRegister |= pzsTable8[result8 & 0xFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (!((leftOperand^rightOperand)&0x80)&&((leftOperand^result16)&0x80))
        iapx86_FlagsRegister |= O_FLAG;

    return result8;
}

uint16_t iapx86::Inc16(uint16_t leftOperand, uint16_t rightOperand)
{
    uint32_t result32 = leftOperand + rightOperand;
    uint16_t result16 = (uint16_t)result32;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x08D4;
    iapx86_FlagsRegister |= pzsTable16[result16&0xFFFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (!((leftOperand^rightOperand)&0x8000)&&((leftOperand^result32)&0x8000))
        iapx86_FlagsRegister |= O_FLAG;

    return result16;
}

uint8_t iapx86::Dec8(uint8_t leftOperand, uint8_t rightOperand)
{
    /* DEC (Decrement destination by one)
     * Operation: The specified operand is reduced by 1.
     *      (DEST) <- (DEST)-1
     * CAUTION : 'rightOperand' must be 1.
     */

    uint16_t result16 = leftOperand - rightOperand;
    uint8_t result8 = (uint16_t) result16;

    /* Flags Affected: AF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x8D4;
    iapx86_FlagsRegister |= pzsTable8[result8 & 0xFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (!((leftOperand^rightOperand)&0x80)&&((leftOperand^result16)&0x80))
        iapx86_FlagsRegister |= O_FLAG;

    return result8;
}

uint16_t iapx86::Dec16(uint16_t leftOperand, uint16_t rightOperand)
{
    uint32_t result32 = leftOperand - rightOperand;
    uint16_t result16 = (uint16_t)result32;

    /* Flags Affected: AF, CF, OF, PF, SF, ZF */
    iapx86_FlagsRegister &= ~0x08D4;
    iapx86_FlagsRegister |= pzsTable16[result16&0xFFFF];
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        iapx86_FlagsRegister |= A_FLAG;
    if (!((leftOperand^rightOperand)&0x8000)&&((leftOperand^result32)&0x8000))
        iapx86_FlagsRegister |= O_FLAG;

    return result16;
}

//= Logics methods ===================================================================================================//
uint8_t iapx86::Bitwise8(uint8_t leftOperand, uint8_t rightOperand)
{
    uint16_t result16 = leftOperand | rightOperand;
    uint8_t result8 = (uint16_t) result16;

    /* Flags Affected: CF, OF, PF, SF, ZF. */
    /* Undefined: AF */
    /* Note : carry and overflow flags are both reset - CF = OF = 0. */

    iapx86_FlagsRegister &= ~0x8C5;
    iapx86_FlagsRegister |= pzsTable8[result8 & 0xFF];

    return result8;
}

uint16_t iapx86::Bitwise16(uint16_t leftOperand, uint16_t rightOperand)
{
    uint16_t wResult = leftOperand | rightOperand;
    uint8_t  bResult = (uint8_t)wResult;
    //parityFlag = paritytable_8bits[(uint8_t)wResult & 0xFF];
    signFlag = 1 == ((wResult >> 15) & 1);
    zeroFlag = 0 == bResult;

    carryFlag = false;
    overflowFlag = false;

    return wResult;
}

//= Main method ======================================================================================================//
void iapx86::exec86(int requestedCycles)
{
    cycles += requestedCycles;
    while (cycles>0)
    {
        opcode=ReadByte_Memory(CS<<4 | IP);
        //printf("\nES %04x - CS %04X - SS %04X - DS %04X -- IP %04X -- Flags %04X", ES, CS, SS, DS, IP,flags);
        iCS = CS; //Saved for debug to screen method.
        iIP = IP; //Saved for debug to screen method.
        IP++;
        startCycleCarryFlag = iapx86_FlagsRegister & C_FLAG;

        (this->*instDecoder[opcode])();
        DebugToScreen();
    }
}

//= Opcodes methods ==================================================================================================//
/*-- 0x00 --*/
void iapx86::ADD_EAb_REGb()
{
    FetchAndDecode_ModRMByte();
    bLeftOperand = ReadByte_EffectiveAddress();
    bRightOperand = ReadByte_Register();
    bResult = Add8(bLeftOperand, bRightOperand);
    WriteByte_EffectiveAddress(bResult);
    cycles-=((mod==3)?3:24);
}

/*-- 0x01 --*/
void iapx86::ADD_EAw_REGw()
{
    FetchAndDecode_ModRMByte();
    wLeftOperand=ReadWord_EffectiveAddress();
    wRightOperand=ReadWord_Register();
    uint16_t u16Result = Add16(wLeftOperand, wRightOperand);
    WriteWord_EffectiveAddress(u16Result);
    cycles-=((mod==3)?3:24);
}

/*-- 0x02 --*/
void iapx86::ADD_REGb_EAb()
{
    FetchAndDecode_ModRMByte();
    bLeftOperand=ReadByte_Register();
    bRightOperand=ReadByte_EffectiveAddress();
    bResult = Add8(bLeftOperand, bRightOperand);
    WriteByte_Register(bResult);
}

/*-- 0x03 --*/
void iapx86::ADD_REGw_EAw()
{
    FetchAndDecode_ModRMByte();
    wLeftOperand = ReadWord_Register();
    wRightOperand = ReadWord_EffectiveAddress();
    uint16_t u16Result = Add16(wLeftOperand, wRightOperand);
    WriteWord_Register(u16Result);

}

/*-- 0x04 --*/
void iapx86::ADD_AL_Data8()
{
    bLeftOperand = AL;
    bRightOperand = getImmediateByte();
    bResult = Add8(bLeftOperand, bRightOperand);
    AL = bResult;

}

/*-- 0x05 --*/
void iapx86::ADD_AX_Data16()
{
    wLeftOperand = AX;
    wRightOperand = getImmediateWord();
    wResult = Add16(wLeftOperand, wRightOperand);
    AX = wResult;

}

/*-- 0x06 --*/
void iapx86::PUSH_ES()
{
    SP -= 2;
    WriteWord_Memory(SS << 4 | SP, ES);
    cycles-=14;
}

/*-- 0x07 --*/
void iapx86::POP_ES()
{
    ES = ReadWord_Memory(SS << 4 | SP);
    SP += 2;
    cycles-=12;
}

/*-- 0x08 --*/
void iapx86::OR_EAb_REGb()
{
    FetchAndDecode_ModRMByte();
    bLeftOperand = ReadByte_EffectiveAddress();
    bRightOperand = ReadByte_Register();
    bResult = Bitwise8(bLeftOperand, bRightOperand);
    WriteByte_EffectiveAddress(bResult);

}

/*-- 0x09 --*/
void iapx86::OR_EAw_REGw()
{
    FetchAndDecode_ModRMByte();
    wLeftOperand = ReadWord_EffectiveAddress();
    wRightOperand = ReadWord_Register();
    wResult = Bitwise16(wLeftOperand, wRightOperand);
    WriteWord_EffectiveAddress(wResult);

}

/*-- 0x0A --*/
void iapx86::OR_REGb_EAb()
{
    FetchAndDecode_ModRMByte();
    bLeftOperand = ReadByte_Register();
    bRightOperand = ReadByte_EffectiveAddress();
    bResult = Bitwise8(bLeftOperand, bRightOperand);
    WriteByte_Register(bResult);

}

/*-- 0x0B --*/
void iapx86::OR_REGw_EAw()
{
    FetchAndDecode_ModRMByte();
    wLeftOperand = ReadWord_Register();
    wRightOperand = ReadWord_EffectiveAddress();
    wResult = Bitwise16(wLeftOperand, wRightOperand);
    WriteWord_Register(wResult);

}

/*-- 0x0C --*/
void iapx86::OR_AL_Data8()
{
    bLeftOperand = AL;
    bRightOperand = getImmediateByte();
    bResult= Bitwise8(bLeftOperand,bRightOperand);
    AL = bResult;

}

/*-- 0x0D --*/
void iapx86::OR_AX_Data16()
{
    wLeftOperand = AX;
    wRightOperand = getImmediateWord();
    wResult = Bitwise16(wLeftOperand, wRightOperand);
    AX = wResult;

}

/*-- 0x0E --*/
void iapx86::PUSH_CS()
{
    SP -= 2;
    WriteWord_Memory(SS << 4 | SP, CS);
    cycles-=14;
}

/*-- 0X10 --*/
void iapx86::ADC_EAb_REGb()
{
    FetchAndDecode_ModRMByte();
    bLeftOperand = ReadByte_EffectiveAddress();
    bRightOperand = ReadByte_Register();
    bResult = Adc8(bLeftOperand, bRightOperand);
    WriteByte_EffectiveAddress(bResult);
    cycles-=((mod==3)?3:24);
}

/*-- 0x11 --*/
void iapx86::ADC_EAw_REGw()
{
    FetchAndDecode_ModRMByte();
    wLeftOperand = ReadWord_EffectiveAddress();
    wRightOperand = ReadWord_Register();
    wResult = Adc16(wLeftOperand, wRightOperand);
    WriteWord_EffectiveAddress(wResult);
    cycles-=((mod==3)?3:24);
}

/*-- 0x12 --*/
void iapx86::ADC_REGb_EAb()
{
    FetchAndDecode_ModRMByte();
    bLeftOperand = ReadByte_Register();
    bRightOperand = ReadByte_EffectiveAddress();
    bResult = Adc8(bLeftOperand, bRightOperand);
    WriteByte_EffectiveAddress(bResult);
    cycles-=((mod==3)?3:24);
}

/*-- 0x13 --*/
void  iapx86::ADC_REGw_EAw()
{
    FetchAndDecode_ModRMByte();
    wLeftOperand = ReadWord_Register();
    wRightOperand = ReadWord_EffectiveAddress();
    wResult = Adc16(wLeftOperand, wRightOperand);
    WriteWord_EffectiveAddress(wResult);
    cycles-=((mod==3)?3:24);
}

/*-- 0x14 --*/
void iapx86::ADC_AL_Data8()
{
    bLeftOperand = AL;
    bRightOperand = getImmediateByte();
    bResult = Adc8(bLeftOperand, bRightOperand);
    AL = bResult;
    cycles-=4;
}

/*-- 0x15 --*/
void iapx86::ADC_AX_Data16()
{
    wLeftOperand = AX;
    wRightOperand = getImmediateWord();
    wResult = Adc16(wLeftOperand, wRightOperand);
    AX = wResult;
    cycles-=4;
}

/*-- 0x16 --*/
void iapx86::PUSH_SS()
{
    SP -= 2;
    WriteWord_Memory(SS << 4 | SP, SS);
    cycles-=14;
}

/*-- 0x17 --*/
void iapx86::POP_SS()
{
    SS = ReadWord_Memory(SS << 4 | SP);
    SP += 2;
    cycles-=12;
}



void  iapx86::mov_AL_ib()
{
    AL = getImmediateByte();
}

void iapx86::mov_rmw_sr()
{
    FetchAndDecode_ModRMByte();
    iapx86_Registers[rm].w = iapx86_Segments[reg];
}

void iapx86::mov_sr_rmw()
{
    FetchAndDecode_ModRMByte();
    iapx86_Segments[reg]=iapx86_Registers[rm].w;
}

void  iapx86::JMP_NEAR_RELATIVE()
{

}

void  iapx86::JMP_NEAR_INDIRECT()
{

}

void iapx86::JMP_FAR_INDIRECT()
{

}

void iapx86::JMP_FAR_DIRECT()
{
    wOffset = getImmediateWord();
    wSegbase = getImmediateWord();

    CS = wSegbase;
    IP = wOffset;
}

void iapx86::PrintFlags()
{
    printf(" - F/REG : "  BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(iapx86_FlagsRegister));
}

void iapx86::DebugToScreen()
{
    printf("\n%04X:%04X ", CS, IP);
    printf("%02X", opcode);

    switch (opcode)
    {
     case 0x00:
        printf(" - (%02X + %02X) = %02X",bLeftOperand, bRightOperand, bResult);
        PrintFlags();
        break;

    case 0x02:
        printf(" - (%02X + %02X) = %02X",bLeftOperand, bRightOperand, bResult);
        PrintFlags();
        break;

    case 0x04:
        printf(" - (%02X + %02X) = %02X",bLeftOperand, bRightOperand, bResult);
        PrintFlags();
        break;

    case 0xEA:
        printf(" - SEGMENT : %04X - OFFSET : %04X",wSegbase, wOffset);
        break;
    }
}



