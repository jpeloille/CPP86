//
// Created by jpeloille on 4/4/21.
//

#include <cstdio>
#include "intel.h"
#include "iapx86.h"

int cycles = 0;
uint8_t opcode;
uint32_t eaaddr;
uint32_t easeg;
uint8_t rm,reg,mod,rmdat;
uint8_t mem[0xFFFFF];

iapx86::iapx86()
{
     instDecoder[0xEA] = &iapx86::JMP;
}

iapx86::~iapx86()
{

}

void iapx86::fetchea()
    {
        rmdat=readmemb(CS<<4 + IP); IP++;
        reg=(rmdat>>3)&7;
        mod=rmdat>>6;
        rm=rmdat&7;
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
                eaaddr=readmemb(CS<<4 + IP); IP++;
                if (eaaddr&0x80) eaaddr|=0xFF00;
                switch (rm)
                {
                    case 0: eaaddr+=BX+SI; easeg=DS; cycles-=11; break;
                    case 1: eaaddr+=BX+DI; easeg=DS; cycles-=12; break;
                    case 2: eaaddr+=BP+SI; easeg=SS; cycles-=12; break;
                    case 3: eaaddr+=BP+DI; easeg=SS; cycles-=11; break;
                    case 4: eaaddr+=SI; easeg=DS; cycles-=9; break;
                    case 5: eaaddr+=DI; easeg=DS; cycles-=9; break;
                    case 6: eaaddr+=BP; easeg=SS; cycles-=9; break;
                    case 7: eaaddr+=BX; easeg=DS; cycles-=9; break;
                }
                break;

            case 2:
                eaaddr=getImmediateWord();
                switch (rm)
                {
                    case 0: eaaddr+=BX+SI; easeg=DS; cycles-=11; break;
                    case 1: eaaddr+=BX+DI; easeg=DS; cycles-=12; break;
                    case 2: eaaddr+=BP+SI; easeg=SS; cycles-=12; break;
                    case 3: eaaddr+=BP+DI; easeg=SS; cycles-=11; break;
                    case 4: eaaddr+=SI; easeg=DS; cycles-=9; break;
                    case 5: eaaddr+=DI; easeg=DS; cycles-=9; break;
                    case 6: eaaddr+=BP; easeg=SS; cycles-=9; break;
                    case 7: eaaddr+=BX; easeg=DS; cycles-=9; break;
                }
                break;
        }
        eaaddr&=0xFFFF;
    }

uint8_t iapx86::readmemb(uint32_t addr)
    {
        cycles--;
        return mem[addr];
    }

uint16_t iapx86::getImmediateWord()
    {
        uint8_t low = mem[CS<<4 | IP];
        IP++;

        uint8_t  high = mem[CS << 4 | IP];
        IP++;

        return (high << 8 | low);
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
    printf("Cycles = %02i\n", cycles);
    while (cycles>0)
    {
        opcode=readmemb(CS<<4 | IP);
        printf("%05X - ", CS<<4 | IP);
        IP++;
        printf("%02X %02i\n", opcode, cycles);
        (this->*instDecoder[opcode])();
    }
}

uint8_t iapx86::JMP()
{
    printf("Instruction 0xEA ! %01X\n", cycles);
    return 0;
}

