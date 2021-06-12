//
// Created by jpeloille on 4/4/21.
//

#ifndef X86___INTEL_H
#define X86___INTEL_H

#include <stdint.h>

#define AX iapx86_Registers[0].w
#define CX iapx86_Registers[1].w
#define DX iapx86_Registers[2].w
#define BX iapx86_Registers[3].w
#define SP iapx86_Registers[4].w
#define BP iapx86_Registers[5].w
#define SI iapx86_Registers[6].w
#define DI iapx86_Registers[7].w

#define AL iapx86_Registers[0].b.l
#define CL iapx86_Registers[1].b.l
#define DL iapx86_Registers[2].b.l
#define BL iapx86_Registers[3].b.l

#define AH iapx86_Registers[0].b.h
#define CH iapx86_Registers[1].b.h
#define DH iapx86_Registers[2].b.h
#define BH iapx86_Registers[3].b.h

typedef union
{
    uint16_t w;
    struct
    {
        uint8_t l,h;
    } b;
} wordRegister;

wordRegister iapx86_Registers[8];

#define IP instructionPointer
uint16_t instructionPointer;

#define ES iapx86_Segments[0]
#define CS iapx86_Segments[1]
#define SS iapx86_Segments[2]
#define DS iapx86_Segments[3]
uint16_t iapx86_Segments[4];

bool parityFlag;
bool carryFlag;
bool auxiliaryCarryFlag;
bool directionflag;
bool interruptEnableFlag;
bool overflowFlag;
bool signFlag;
bool trapFlag;
bool zeroFlag;

/* FLAGS 0X:0X:0X:0X:(OF):(DF):(lF):(TF) - (SF):(ZF):0X:(AF):0X:(PF):0X:(CF) */
/* FLAGS 15:14:13:12:(11):(10):(09):(08) - (07):(06):05:(04):03:(02):01:(00) */

#define C_FLAG  0x0001
#define P_FLAG  0x0004
#define A_FLAG  0x0010
#define Z_FLAG  0x0040
#define S_FLAG  0x0080
#define T_FLAG  0x0100
#define I_FLAG  0x0200
#define D_FLAG  0x0400
#define O_FLAG  0x0800

uint16_t iapx86_FlagsRegister;

#endif //X86___INTEL_H
