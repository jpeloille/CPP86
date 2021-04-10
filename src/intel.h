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

wordRegister iapx86_Registers[7];

#define IP instructionPointer
uint16_t instructionPointer;

#define ES iapx86_Segments[0]
#define CS iapx86_Segments[1]
#define SS iapx86_Segments[2]
#define DS iapx86_Segments[3]

uint16_t iapx86_Segments[3];

#endif //X86___INTEL_H
