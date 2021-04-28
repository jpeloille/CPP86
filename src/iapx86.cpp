//
// Created by jpeloille on 4/4/21.
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

uint16_t wOffset;
uint16_t wSegbase;
uint8_t  disp8;
uint16_t disp16;

iapx86::iapx86()
{
    instDecoder[0x00] = &iapx86::add_rmb_rb;
    instDecoder[0xB0] = &iapx86::mov_AL_ib;
    instDecoder[0x8C] = &iapx86::mov_rmw_sr;
    instDecoder[0x8E] = &iapx86::mov_sr_rmw;
    instDecoder[0xEA] = &iapx86::JMP_FAR_DIRECT;
}

iapx86::~iapx86()
{

}

void iapx86::decode_ModRM()
    {
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

inline uint8_t iapx86::ReadByte_EffectiveAddress()
{
    if (mod==3)
        return (rm&4)?iapx86_Registers[rm&3].b.h:iapx86_Registers[rm&3].b.l;

    cycles-=3;
    return ReadByte_Memory(easeg<<4 | eaaddr);
}

inline uint16_t iapx86::ReadWord_EffectiveAddress()
{
    if (mod==3)
        return iapx86_Registers[rm].w;

    cycles-=3;
    return  ReadWord_Memory(easeg <<4 | eaaddr);
}

void iapx86::WriteByte_Register(uint8_t data)
{
    if (reg&4) iapx86_Registers[reg&3].b.h=data;
    else iapx86_Registers[reg&3].b.l=data;
}

uint8_t iapx86::ReadByte_Memory(uint32_t addr)
{
    cycles--;
    return mem[addr];
}

inline uint16_t  iapx86::ReadWord_Memory(uint32_t addr)
{
    uint8_t low = mem[addr];
    cycles--;

    uint8_t  high = mem[addr++];
    cycles--;

    /*
    printf(" %02X %00X", low, high);
    return (high << 8 | low);
     */
}

uint8_t  iapx86::getImmediateByte()
{
    uint8_t byte = mem[CS <<4 | IP];
    IP++; cycles--;
    printf(" %02X", byte);
    return byte;
}

uint16_t iapx86::getImmediateWord()
    {
        uint8_t low = mem[CS << 4 | IP];
        IP++; cycles--;

        uint8_t  high = mem[CS << 4 | IP];
        IP++; cycles--;

        printf(" %02X %00X", low, high);
        return (high << 8 | low);
    }

void ComputeAdd8_Flags(uint8_t leftOperand, uint8_t rightOperand)
{
    uint16_t result = (uint16_t)leftOperand + (uint16_t)rightOperand;
    flags&=~0x8D5;
    flags|=znptable8[result&0xFF];
    if (result&0x100) flags|=C_FLAG;
    if (!((leftOperand^rightOperand)&0x80)&&((leftOperand^result)&0x80)) flags|=V_FLAG;
    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)      flags|=A_FLAG;
}

int iapx86::loadbios()
    {
        FILE *f=NULL;
        f=fopen("../rom/OHPC.ROM","rb");
        if (!f) return 0;
        fread(mem+0xFE000,8192,1,f);
        fclose(f);

        uint32_t adress = CS<<4 | IP;
        printf("Bios loaded.");
        printf(" (%05X : %02X) \n",adress, mem[adress]);
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

void iapx86::exec86(int requestedCycles)
{
    cycles += requestedCycles;
    while (cycles>0)
    {
        opcode=ReadByte_Memory(CS<<4 | IP);
        printf("\nES %04x - CS %04X - SS %04X - DS %04X -- IP %04X -- Flags %04X", ES, CS, SS, DS, IP,flags);
        printf("\n%04X:%04X ", CS, IP);
        IP++;
        printf("%02X", opcode);
        (this->*instDecoder[opcode])();
    }
}

/* Arithmetics methods - Compute flags */
uint8_t iapx86::Add8(uint8_t leftOpernad, uint8_t righrOperand)
{
    uint16_t result = (uint16_t) leftOpernad + (uint16_t) righrOperand;

}

/*-- 0x00 --*/
uint8_t iapx86::add_rmb_rb()
{
    ModRM=getImmediateByte();
    decode_ModRM();
    bLeftOperand = ReadByte_EffectiveAddress();
    bRightOperand = ReadByte_Register();

    printf(" - Left operand at adress %05X : %02X", easeg<<4 | eaaddr, bLeftOperand);
    printf(" - Right operand : %02X", bRightOperand);
    printf(" - Result : %03X", bRightOperand+=bLeftOperand);
}

uint8_t  iapx86::mov_AL_ib()
{
    AL = getImmediateByte();
    return 0;
}

uint8_t iapx86::mov_rmw_sr()
{
    ModRM=getImmediateByte();
    decode_ModRM();
    iapx86_Registers[rm].w = iapx86_Segments[reg];
    return 0;
}

uint8_t iapx86::mov_sr_rmw()
{
    ModRM=getImmediateByte();
    decode_ModRM();
    iapx86_Segments[reg]=iapx86_Registers[rm].w;
    return 0;
}

uint8_t  iapx86::JMP_NEAR_RELATIVE()
{

}

uint8_t  iapx86::JMP_NEAR_INDIRECT()
{

}

uint8_t iapx86::JMP_FAR_INDIRECT()
{

}

uint8_t iapx86::JMP_FAR_DIRECT()
{
    wOffset = getImmediateWord();
    wSegbase = getImmediateWord();

    CS = wSegbase;
    IP = wOffset;

    return 0;
}

