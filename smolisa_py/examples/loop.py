from smolisa_py.asm.assembler import *

asm = Asm()

asm.at(0x0000, [
    LI(RG0, low("loop")),
    Label("loop"),
    OR(RIP, RG0, RG0)
])

if __name__ == "__main__":
    asm.to_rom()