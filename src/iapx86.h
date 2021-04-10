//
// Created by jpeloille on 4/4/21.
//

#ifndef X86___IAPX86_H
#define X86___IAPX86_H


class iapx86
{
private:
    uint8_t (iapx86::*instDecoder[0xFF])();

 /*
  * Mnemonic notation :
  * ib : immediate byte         rb : register byte
  * iw : immediate word         rw : register word
  * mb : memory byte            rmb : register or memory byte
  * mw : memory word            rmb : register or memory byte
  * sr : segment register
  */

private:
    uint8_t mov_rmw_sr();
    uint8_t mov_sr_rmw();

    uint8_t JMP_NEAR_RELATIVE();
    uint8_t JMP_NEAR_INDIRECT();
    uint8_t JMP_FAR_DIRECT();
    uint8_t JMP_FAR_INDIRECT();

private:
    void fetchea();
    uint8_t readmemb(uint32_t addr);
    uint8_t  getImmediateByte();
    uint16_t getImmediateWord();
    static int loadbios();


public:
    iapx86();
    ~iapx86();

    void cpuReset();
    void exec86(int requestedCycles);
};

#endif //X86___IAPX86_H
