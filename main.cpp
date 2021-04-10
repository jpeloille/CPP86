#include <iostream>


#include "src/iapx86.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    //cpuReset();
    iapx86 cpu;
    cpu.cpuReset();
    cpu.exec86(100);
return 0;
}
