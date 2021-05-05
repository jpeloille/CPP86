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
uint16_t wLeftOperand;
uint16_t wRightOperand;

uint16_t wOffset;
uint16_t wSegbase;
uint8_t  disp8;
uint16_t disp16;

uint8_t paritytable_8bits[256];
uint16_t znptable16[65536];



void generate_parity_table()
{
    int c,d;
    for (c=0;c<256;c++)
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
        if (d%2==0)
            paritytable_8bits[c]=true;
        else
            paritytable_8bits[c]=false;
    }
    paritytable_8bits[0]=0;
}

iapx86::iapx86()
{
    generate_parity_table();

    instDecoder[0x00] = &iapx86::ADD_EAb_REGb;
    instDecoder[0x01] = &iapx86::ADD_EAw_REGw;
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
//====================================================================================================================//

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
//====================================================================================================================//

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

    printf(" %02X %02X", low, high);
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

    printf(" %02X %02X", low, high);
    return (high << 8 | low);
}

void ComputeAdd8_Flags(uint8_t leftOperand, uint8_t rightOperand)
{
    uint16_t result = leftOperand + rightOperand;
    uint8_t byteResult = (uint8_t)result;

    signFlag = 1 == ((byteResult >> 7) & 1);
    carryFlag = result > 0xFF;
    zeroFlag = 0 == byteResult;

    if (!((leftOperand^rightOperand)&0x80)&&((leftOperand^result)&0x80))
        overflowFlag = true;
    else
        overflowFlag = false;

    if (((leftOperand&0xF)+(rightOperand&0xF))&0x10)
        auxiliaryCarryFlag = true;
    else
        auxiliaryCarryFlag = false;

    parityFlag = paritytable_8bits[(uint8_t)result & 0xFF];
}

int iapx86::loadbios()
    {
        FILE *f=NULL;
        f=fopen("../rom/OHPC02.ROM","rb");
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
        //printf("\nES %04x - CS %04X - SS %04X - DS %04X -- IP %04X -- Flags %04X", ES, CS, SS, DS, IP,flags);
        printf("\n%04X:%04X ", CS, IP);
        IP++;
        printf("%02X", opcode);
        (this->*instDecoder[opcode])();
    }
}

void printFlags()
{
    printf("Zero flag : %01X", zeroFlag);
    printf( " - Carry flag : %01X", carryFlag);
    printf(" - Sign flag : %01X", signFlag);
    printf(" - Parity flag : %01X", parityFlag);
    printf(" - Auxiliary carry flag: %01X", auxiliaryCarryFlag);
    printf(" - Overflow flag : %01X\n", overflowFlag);
}

/* Arithmetics methods - Compute flags */


/*-- 0x00 --*/
void iapx86::ADD_EAb_REGb()
{
    ModRM=getImmediateByte();
    decode_ModRM();
    bLeftOperand = ReadByte_EffectiveAddress();
    bRightOperand = ReadByte_Register();
    WriteByte_EffectiveAddress(bLeftOperand + bRightOperand);
    ComputeAdd8_Flags(bLeftOperand,bRightOperand);
    cycles-=((mod==3)?3:24);

    printf(" - Left operand at adress %05X : %02X", easeg<<4 | eaaddr, bLeftOperand);
    printf(" - Right operand : %02X", bRightOperand);
    uint8_t result8 = bRightOperand + bLeftOperand;
    printf(" - Result : %02X", result8);
    uint16_t resul16 = bLeftOperand+bRightOperand;
    printf(" - Resilt 16 : %04X\n", resul16);
    printFlags();

}

void iapx86::ADD_EAw_REGw()
{
    ModRM=getImmediateByte();
    decode_ModRM();
    wLeftOperand=ReadWord_EffectiveAddress();
    wRightOperand=ReadWord_Register();
    
}


void  iapx86::mov_AL_ib()
{
    AL = getImmediateByte();
}

void iapx86::mov_rmw_sr()
{
    ModRM=getImmediateByte();
    decode_ModRM();
    iapx86_Registers[rm].w = iapx86_Segments[reg];
}

void iapx86::mov_sr_rmw()
{
    ModRM=getImmediateByte();
    decode_ModRM();
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



