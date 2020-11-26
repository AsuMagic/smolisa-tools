from nmigen import *
from nmigen.sim import Simulator
from control import *
from ram import *
from regfile import *
from alu import *

from smolisa_py.examples.loop import asm as loop_asm

# TODO: this might contain more logic than expected from a data path
class DataPath(Elaboratable):
    def __init__(self, init_rom):
        self.bus = Record(ControlBus)

        self.ctrl = Control()
        self.ram = RAM(1024, init_rom)
        self.regs = RegFile()
        self.alu = ALU()

    def resolve_reg_addr(self, m, reg_addr, reg_addr_src):
        with m.Switch(reg_addr_src):
            with m.Case(RegAddrSrc.R1):
                m.d.comb += reg_addr.eq(self.ctrl.opcode[4:8])
            with m.Case(RegAddrSrc.R2):
                m.d.comb += reg_addr.eq(self.ctrl.opcode[8:12])
            with m.Case(RegAddrSrc.R3):
                m.d.comb += reg_addr.eq(self.ctrl.opcode[12:16])
            with m.Case(RegAddrSrc.REF_IP):
                m.d.comb += reg_addr.eq(0xE)

    def resolve_alu_src(self, m, alu_data, alu_data_src):
        with m.Switch(alu_data_src):
            with m.Case(AluDataSrc.IP):
                m.d.comb += alu_data.eq(self.regs.rip)
            with m.Case(AluDataSrc.REG1):
                m.d.comb += alu_data.eq(self.regs.rdata1)
            with m.Case(AluDataSrc.REG2):
                m.d.comb += alu_data.eq(self.regs.rdata2)
            with m.Case(AluDataSrc.TWO):
                m.d.comb += alu_data.eq(2)

    def elaborate(self, platform):
        m = Module()

        m.submodules.ctrl = self.ctrl
        m.submodules.ram = self.ram
        m.submodules.regs = self.regs
        m.submodules.alu = self.alu

        imm8 = self.ctrl.opcode[8:16]

        # control datapath
        m.d.comb += [
            self.bus.eq(self.ctrl.bus),
            self.ctrl.mem_data.eq(self.ram.rdata)
        ]

        # register datapath
        with m.If(self.bus.reg_we):
            self.resolve_reg_addr(m, self.regs.waddr, self.bus.reg_waddr_src)
            with m.Switch(self.bus.reg_data_src):
                with m.Case(RegDataSrc.IMM8_LOW):
                    m.d.comb += [
                        self.regs.wdata[0:8].eq(imm8),
                        self.regs.wmask.eq(0b01)
                    ]
                with m.Case(RegDataSrc.IMM8_HIGH):
                    m.d.comb += [
                        self.regs.wdata[8:16].eq(imm8),
                        self.regs.wmask.eq(0b10)
                    ]
                with m.Case(RegDataSrc.MEM8):
                    m.d.comb += [
                        self.regs.wdata[0:8].eq(self.ram.rdata[0:8]),
                        self.regs.wmask.eq(0b01)
                    ]
                with m.Case(RegDataSrc.MEM16):
                    m.d.comb += [
                        self.regs.wdata.eq(self.ram.rdata),
                        self.regs.wmask.eq(0b11)
                    ]
                with m.Case(RegDataSrc.ALU):
                    m.d.comb += [
                        self.regs.wdata.eq(self.alu.out),
                        self.regs.wmask.eq(0b11)
                    ]
                    pass

        self.resolve_reg_addr(m, self.regs.raddr1, self.bus.reg_addr1_src)
        self.resolve_reg_addr(m, self.regs.raddr2, self.bus.reg_addr2_src)

        # memory datapath
        with m.If(self.bus.mem_we):
            with m.If(self.bus.mem_half_write):
                m.d.comb += self.ram.wmask.eq(0b01)
            with m.Else():
                m.d.comb += self.ram.wmask.eq(0b11)
        with m.Else():
            m.d.comb += self.ram.wmask.eq(0b00)
        
        with m.Switch(self.bus.mem_addr_src):
            with m.Case(MemAddrSrc.IP):
                m.d.comb += self.ram.addr.eq(self.regs.rip)
            with m.Case(MemAddrSrc.REG1):
                m.d.comb += self.ram.addr.eq(self.regs.rdata1)
        
        with m.Switch(self.bus.mem_data_src):
            with m.Case(MemDataSrc.IP):
                m.d.comb += self.ram.wdata.eq(self.regs.rip)
            with m.Case(MemDataSrc.REG2):
                m.d.comb += self.ram.wdata.eq(self.regs.rdata2)

        # alu datapath
        m.d.comb += [
            self.alu.op.eq(self.bus.alu_op),
            self.bus.alu_flag_zero.eq(self.alu.flag_zero)
        ]
        self.resolve_alu_src(m, self.alu.a, self.bus.alu_a_src)
        self.resolve_alu_src(m, self.alu.b, self.bus.alu_b_src)

        return m
    
    def testbench(self):
        for _ in range(100):
            yield

dut = DataPath(loop_asm.as_int16_list(1024))

sim = Simulator(dut)
sim.add_clock(1e-6)
sim.add_sync_process(dut.testbench)
with sim.write_vcd("datapath.vcd"):
    sim.run()