//
// Created by jpeloille on 4/4/21.
//

#ifndef X86___IAPX86_H
#define X86___IAPX86_H

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c %c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define WORD_TO_BINARY_PATTERN "%c%c%c%c %c%c%c%c - %c%c%c%c %c%c%c%c"
#define WORD_TO_BINARY(word) \
  (word & 0x8000 ? '1' : '0'), \
  (word & 0x4000 ? '1' : '0'), \
  (word & 0x2000 ? '1' : '0'), \
  (word & 0x1000 ? '1' : '0'), \
  (word & 0x0800 ? '1' : '0'), \
  (word & 0x0400 ? '1' : '0'), \
  (word & 0x0200 ? '1' : '0'), \
  (word & 0x0100 ? '1' : '0'), \
                             \
  (word & 0x80 ? '1' : '0'), \
  (word & 0x40 ? '1' : '0'), \
  (word & 0x20 ? '1' : '0'), \
  (word & 0x10 ? '1' : '0'), \
  (word & 0x08 ? '1' : '0'), \
  (word & 0x04 ? '1' : '0'), \
  (word & 0x02 ? '1' : '0'), \
  (word & 0x01 ? '1' : '0')

class iapx86
{

private:
    void (iapx86::*instDecoder[0xFF])();

/* Instructions set methods */
private:
    /*
     * Mnemonic notation :
     * ib : immediate byte         rb : register byte
     * iw : immediate word         rw : register word
     * mb : memory byte            rmb : register or memory byte
     * mw : memory word            rmb : register or memory byte
     * sr : segment register
     */

    void ADD_EAb_REGb();
    void ADD_EAw_REGw();
    void ADD_REGb_EAb();
    void ADD_REGw_EAw();
    void ADD_AL_Data8();
    void ADD_AX_Data16();
    void PUSH_ES();
    void POP_ES();
    void OR_EAb_REGb();
    void OR_EAw_REGw();
    void OR_REGb_EAb();
    void OR_REGw_EAw();
    void OR_AL_Data8();
    void OR_AX_Data16();
    void PUSH_CS();

    void ADC_EAb_REGb();
    void ADC_EAw_REGw();
    void ADC_REGb_EAb();
    void ADC_REGw_EAw();
    void ADC_AL_Data8();
    void ADC_AX_Data16();
    void PUSH_SS();
    void POP_SS();
    void SBB_EAb_REGb();
    void SBB_EAw_REGw();
    void SBB_REGb_EAb();
    void SBB_REGw_EAw();
    void SBB_AL_Data8();
    void SBB_AX_Data16();
    void PUSH_DS();
    void POP_DS();

    void mov_AL_ib();
    void mov_rmw_sr();
    void mov_sr_rmw();

    void JMP_NEAR_RELATIVE();
    void JMP_NEAR_INDIRECT();
    void JMP_FAR_DIRECT();
    void JMP_FAR_INDIRECT();

/* Arithmetics methods */
private:
    uint8_t Add8(uint8_t leftOperand, uint8_t rightOperand);
    uint8_t Adc8(uint8_t leftOperand, uint8_t rightOperand);
    uint16_t Add16(uint16_t leftOperand, uint16_t rightOperand);
    uint16_t Adc16(uint16_t leftOperand, uint16_t rightOperand);
    uint8_t Sub8(uint8_t leftOperand, uint8_t rightOperand);
    uint8_t Sbb8(uint8_t leftOperand, uint8_t rightOperand);
    uint16_t Sub16(uint16_t leftOperand, uint16_t rightOperand);
    uint16_t Sbb16(uint16_t leftOperand, uint16_t rightOperand);

    uint8_t Inc8(uint8_t leftOperand, uint8_t rightOperand);
    uint16_t Inc16(uint16_t leftOperand, uint16_t rightOperand);
    uint8_t Dec8(uint8_t leftOperand, uint8_t rightOperand);
    uint16_t Dec16(uint16_t leftOperand, uint16_t rightOperand);

    uint8_t  Or8(uint8_t leftOperand, uint8_t rightOperand);
    uint16_t Or16(uint16_t leftOperand, uint16_t rightOperand);



private:
    void FetchAndDecode_ModRMByte();

    // R/W directly to the selected register //
    inline uint8_t ReadByte_Register();
    inline uint16_t ReadWord_Register();
    void WriteByte_Register(uint8_t data);
    void WriteWord_Register(uint16_t data);

    // R/W switching Effective Address //
    inline uint8_t ReadByte_EffectiveAddress();
    inline uint16_t ReadWord_EffectiveAddress();
    inline void WriteByte_EffectiveAddress(uint8_t data);
    inline void WriteWord_EffectiveAddress(uint16_t data);

    // R/W to memory using Physical Address //
    uint8_t ReadByte_Memory(uint32_t physicalAddress);
    uint16_t ReadWord_Memory(uint32_t physicalAddress);
    void WriteByte_Memory(uint32_t physicalAddress, uint8_t data);
    void WriteWord_Memory(uint32_t physicalAddress, uint16_t data);

    uint8_t  getImmediateByte();
    uint16_t getImmediateWord();



    static int loadbios();


public:
    iapx86();
    ~iapx86();

    void cpuReset();
    void exec86(int requestedCycles);
    void DebugToScreen();

    void PrintFlags();
};

#endif //X86___IAPX86_H
