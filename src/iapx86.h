//
// Created by jpeloille on 4/4/21.
//

#ifndef X86___IAPX86_H
#define X86___IAPX86_H


class iapx86
{
private:
    uint8_t (iapx86::*instDecoder[0xFF])();

private:
    uint8_t JMP();

private:
    void fetchea();
    uint8_t readmemb(uint32_t addr);
    uint16_t getImmediateWord();
    static int loadbios();


public:
    iapx86();
    ~iapx86();

    void cpuReset();
    void exec86(int requestedCycles);
};

#endif //X86___IAPX86_H
