//
// InstructionDecoder.cpp
// Classix
//
// Copyright (C) 2013 Félix Cloutier
//
// This file is part of Classix.
//
// Classix is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// Classix is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// Classix. If not, see http://www.gnu.org/licenses/.
//

#include <sstream>
#include "InstructionDecoder.h"

namespace
{
	using namespace PPCVM::Disassembly;
	
	const char* conditions[] = {
		"lt", "gt", "eq", "so",
		"ge", "le", "ne", "ns"
	};
	
	OpcodeArgument spr(uint16_t reg)
	{
		return OpcodeArgument::SPR(reg);
	}
	
	OpcodeArgument g(uint8_t reg)
	{
		return OpcodeArgument::GPR(reg);
	}
	
	OpcodeArgument f(uint8_t reg)
	{
		return OpcodeArgument::FPR(reg);
	}
	
	OpcodeArgument cr(uint8_t cr)
	{
		return OpcodeArgument::CR(cr);
	}
	
	OpcodeArgument hex(uint32_t h)
	{
		return OpcodeArgument::Literal(h);
	}
	
	OpcodeArgument hex(int32_t h)
	{
		return OpcodeArgument::Literal(h);
	}
	
	OpcodeArgument offset(int32_t offset)
	{
		return OpcodeArgument::Offset(offset);
	}
	
	bool HasFlag(int value, int mask)
	{
		return (value & mask) == mask;
	}
	
	std::string branchName(int bo, int bi)
	{
		std::string ctrCondition = HasFlag(bo, 0b100) ? "" : HasFlag(bo, 0b10) ? "dz" : "dnz";
		if (HasFlag(bo, 0b10000))
		{
			return ctrCondition;
		}
		else
		{
			int conditionIndex = (bi & 3) | (!HasFlag(bo, 0b1000) << 2);
			return ctrCondition + conditions[conditionIndex];
		}
	}
	
	std::string opX(const std::string& x, bool setsFlags)
	{
		if (!setsFlags)
			return x;
		return x + '.';
	}
	
	std::string opXO(std::string x, bool setsFlags, bool overflows)
	{
		if (overflows)
			x += 'o';
		if (setsFlags)
			x += '.';
		return x;
	}
	
	std::string opLK(std::string x, bool lk)
	{
		if (lk)
			return x + 'l';
		return x;
	}
}

namespace PPCVM
{
	namespace Disassembly
	{
		DisassembledOpcode InstructionDecoder::Decode(PPCVM::Instruction i)
		{
			InstructionDecoder decoder;
			decoder.Dispatch(i);
			return decoder.Opcode;
		}
		
		std::string InstructionDecoder::ToString(PPCVM::Instruction i)
		{
			return (std::stringstream() << Decode(i)).str();
		}
		
#pragma mark -
#define IMPL(x)	void InstructionDecoder::x(PPCVM::Instruction i)
#define BODY(x, c)	IMPL(x) { c; }
#define OP(name)	BODY(name, Emit(i, #name))
		void InstructionDecoder::unknown(PPCVM::Instruction i)
		{
			Emit(i, ".word", hex(i.hex));
		}
		
#pragma mark -
#pragma mark Floating Point Instructions
#define FX2(name)	IMPL(name##x) { Emit(i, opX(#name, i.RC), f(i.RD), f(i.RB)); }
#define FX3(name)	IMPL(name##x) { Emit(i, opX(#name, i.RC), f(i.RD), f(i.RA), f(i.RB)); }
#define FX4(name)	IMPL(name##x) { Emit(i, opX(#name, i.RC), f(i.RD), f(i.RA), f(i.RC), f(i.RB)); }
#define FX2S(name)	FX2(name) FX2(name##s)
#define FX3S(name)	FX3(name) FX3(name##s)
#define FX2Z(name)	FX2(name) FX2(name##z)
#define FX4S(name)	FX4(name) FX4(name##s)
		
		FX2(fabs);
		FX3S(fadd);
		
		IMPL(fcmpo)
		{
			Emit(i, "fcmpo", cr(i.CRFD), f(i.RA), f(i.RB));
		}
		
		IMPL(fcmpu)
		{
			Emit(i, "fcmpu", cr(i.CRFD), f(i.RA), f(i.RB));
		}
		
		FX2Z(fctiw);
		FX3S(fdiv);
		FX4S(fmadd);
		FX2(fmr);
		FX4S(fmsub);
		FX3S(fmul);
		FX2(fnabs);
		FX2(fneg);
		FX4S(fnmadd);
		FX4S(fnmsub);
		FX2(fres);
		FX2(frsp);
		FX2(frsqrte);
		FX4(fsel);
		FX2S(fsqrt);
		FX2S(fsub);
		
#pragma mark -
#pragma mark Integer Instructions
#define IADA(name)	BODY(name##x, Emit(i, opXO(#name, i.RC, i.OE), g(i.RD), g(i.RA)))
#define ILRR(name, a, b) BODY(name##x, Emit(i, opX(#name, i.RC), g(i.R##a), g(i.R##b)))
#define ILRRR(name, a, b, c) BODY(name##x, Emit(i, opX(#name, i.RC), g(i.R##a), g(i.R##b), g(i.R##c)))
#define ILDAB(name) ILRRR(name, D, A, B)
#define IADAB(name)	IMPL(name##x) { Emit(i, opXO(#name, i.RC, i.OE), g(i.RD), g(i.RA), g(i.RB)); }
#define ILASB(name)	BODY(name##x, Emit(i, opX(#name, i.RC), g(i.RA), g(i.RS), g(i.RB)))
#define IADAI(name, dname)	BODY(name, Emit(i, #dname, g(i.RD), g(i.RA), hex(i.SIMM_16)))
#define ICMP(name, to)	IMPL(name) { if (i.L) unknown(i); else if (i.CRFD == 0) Emit(i, #name "d", g(i.RA), to); else if (i.CRFD == 3) Emit(i, #name "w", g(i.RA), to); else Emit(i, #name, cr(i.CRFD), g(i.RA), to); }
#define IROT(name)	BODY(name##x, Emit(i, opX(#name, i.RC), g(i.RA), g(i.RS), hex(i.SH), hex(i.MB), hex(i.ME)))
		
		IADAB(addc);
		IADAB(adde);
		IMPL(addi)
		{
			if (i.RA == 0)
				Emit(i, "li", g(i.RD), hex(i.SIMM_16));
			else
				Emit(i, "addi", g(i.RD), g(i.RA), hex(i.SIMM_16));
		}
		
		IADAI(addic, addic);
		IADAI(addic_rc, addic.);
		IADAI(addis, addis);
		IADA(addme);
		IADA(addze);
		IADAB(add);
		ILASB(and);
		ILASB(andc);
		BODY(andi_rc, Emit(i, "andi.", g(i.RA), g(i.RS), hex(i.UIMM)));
		BODY(andis_rc, Emit(i, "andis.", g(i.RA), g(i.RS), hex(i.UIMM)));
		
		ICMP(cmp, g(i.RB));
		ICMP(cmpi, hex(i.SIMM_16));
		ICMP(cmpl, g(i.RB));
		ICMP(cmpli, hex(i.UIMM));
		
		ILRR(cntlzw, A, S);
		IADAB(divwu);
		IADAB(divw);
		
		ILASB(eqv);
		ILRR(extsb, A, S);
		ILRR(extsh, A, S);
		
		ILDAB(mulhw);
		ILDAB(mulhwu);
		
		IADAI(mulli, mulli);
		IADAB(mullw);
		ILASB(nand);
		IADA(neg);
		ILASB(nor);
		IMPL(orx)
		{
			if (i.RS == i.RB)
				Emit(i, "mr", g(i.RA), g(i.RB));
			else
				Emit(i, opX("or", i.RC), g(i.RA), g(i.RS), g(i.RB));
		}
		
		ILASB(orc);
		IMPL(ori)
		{
			if (i.RA == 0 && i.RS == 0 && i.UIMM == 0)
				Emit(i, "nop");
			else
				Emit(i, "ori", g(i.RA), g(i.RS), hex(i.UIMM));
		}
		BODY(oris, Emit(i, "oris", g(i.RA), g(i.RS), hex(i.UIMM)));
		
		IROT(rlwimi);
		
		// this would need so many unit tests my head hurts
		// http://publib.boulder.ibm.com/infocenter/pseries/v5r3/index.jsp?topic=/com.ibm.aix.aixassem/doc/alangref/fixed_rotate_shift32.htm
		IMPL(rlwinmx)
		{
			if (i.ME == 31)
			{
				if (i.MB == 0)
				{
					if (i.SH < 16)
						Emit(i, opX("rotlwi", i.RC), g(i.RA), g(i.RS), hex(i.SH));
					else
						Emit(i, opX("rotrwi", i.RC), g(i.RA), g(i.RS), hex(32 - i.SH));
				}
				else
				{
					if (i.SH == 0)
						Emit(i, opX("clrlwi", i.RC), g(i.RA), g(i.RS), hex(i.MB));
					else
					{
						if (32 - i.MB == i.SH)
							Emit(i, opX("srwi", i.RC), g(i.RA), g(i.RS), hex(i.MB));
						else
						{
							uint32_t n = 32 - i.MB;
							Emit(i, opX("extrwi", i.RC), g(i.RA), g(i.RS), hex(n), hex(i.SH - n));
						}
					}
				}
			}
			else
			{
				if (i.MB == 0)
				{
					if (i.SH == 0)
						Emit(i, opX("clrrwi", i.RC), g(i.RA), g(i.RS), hex(31 - i.ME));
					else
					{
						if (31 - i.ME == i.SH)
							Emit(i, opX("slwi", i.RC), g(i.RA), g(i.RS), hex(i.SH));
						else
							Emit(i, opX("extlwi", i.RC), g(i.RA), g(i.RS), hex(i.SH), hex(i.ME + 1));
					}
				}
				else
				{
					if (i.MB == 32 - i.SH)
					{
						uint32_t n = i.ME + 1 - i.MB;
						Emit(i, opX("inslwi", i.RC), g(i.RA), g(i.RS), hex(n), hex(i.MB));
					}
					else if (31 - i.ME == i.MB)
					{
						uint32_t b = i.SH + i.MB;
						Emit(i, opX("clrslwi", i.RC), g(i.RA), g(i.RS), hex(b), hex(i.MB));
					}
					else
					{
						uint32_t b = i.MB;
						uint32_t n = i.ME + 1 - b;
						if (i.SH == 32 - (b + n))
							Emit(i, opX("insrwi", i.RC), g(i.RA), g(i.RS), hex(n), hex(b));
						else
							Emit(i, opX("rlwinm", i.RC), g(i.RA), g(i.RS), hex(i.SH), hex(i.MB), hex(i.ME));
					}
				}
			}
		}
		
		IROT(rlwnm);
		
		ILASB(slw);
		ILASB(sraw);
		BODY(srawix, Emit(i, opX("srawi", i.RC), g(i.RA), g(i.RS), hex(i.SH)));
		ILASB(srw);
		
		IADAB(subf);
		IADAB(subfc);
		IADAB(subfe);
		BODY(subfic, Emit(i, "subfic", g(i.RD), g(i.RA), hex(i.SIMM_16)));
		IADA(subfme);
		IADA(subfze);
		
		IMPL(tw)
		{
			if (i.TO == 4)
				Emit(i, "tweq", g(i.RA), g(i.RB));
			else if (i.TO == 5)
				Emit(i, "twge", g(i.RA), g(i.RB));
			else if (i.TO == 31 && i.RA == 0 && i.RB == 0)
				Emit(i, "trap");
			else
				Emit(i, "tw", hex(i.TO), g(i.RA), g(i.RB));
		}
		
		IMPL(twi)
		{
			if (i.TO == 8)
				Emit(i, "twgti", g(i.RA), hex(i.SIMM_16));
			else if (i.TO == 6)
				Emit(i, "twllei", g(i.RA), hex(i.SIMM_16));
			else
				Emit(i, "twi", hex(i.TO), g(i.RA), hex(i.SIMM_16));
		}
		
		ILASB(xor);
		BODY(xori, Emit(i, "xori", g(i.RA), g(i.RS), hex(i.UIMM)));
		BODY(xoris, Emit(i, "xori", g(i.RA), g(i.RS), hex(i.UIMM)));
		
#pragma mark -
#pragma mark Load/Store
#define ADDR			offset(i.SIMM_16), i.RA == 0 ? OpcodeArgument() : g(i.RA)
#define ADDRU			offset(i.SIMM_16), g(i.RA)
#define ADDRX			g(i.RB), i.RA == 0 ? OpcodeArgument() : g(i.RA)
#define ADDRUX			g(i.RB), g(i.RA)
#define LOADI_(name, addr)			BODY(name, Emit(i, #name, g(i.RD), addr))
#define LOADI(name)					BODY(name, Emit(i, #name, g(i.RD), ADDR)) LOADI_(name##x, ADDRX) LOADI_(name##u, ADDRU) LOADI_(name##ux, ADDRUX)
#define LOADF_(name, addr)			BODY(name, Emit(i, #name, f(i.RD), addr))
#define LOADF(name)					BODY(name, Emit(i, #name, f(i.RD), ADDR)) LOADF_(name##x, ADDRX) LOADF_(name##u, ADDRU) LOADF_(name##ux, ADDRUX)
		
		OP(eieio);
		LOADI(lbz);
		LOADF(lfd);
		LOADF(lfs);
		LOADI(lha);
		LOADI_(lhbrx, ADDRX);
		LOADI(lhz);
		LOADI_(lmw, ADDR);
		BODY(lswi, Emit(i, "lswi", g(i.RD), g(i.RA), hex(i.NB)));
		BODY(lswx, Emit(i, "lswx", g(i.RD), g(i.RA), g(i.RB)));
		LOADI_(lwarx, ADDRX);
		LOADI_(lwbrx, ADDRX);
		LOADI(lwz);
		LOADI(stb);
		LOADI(stfd);
		BODY(stfiwx, Emit(i, "stfiwx", f(i.RD), g(i.RA), g(i.RB)));
		LOADI(stfs);
		LOADI(sth);
		LOADI_(sthbrx, ADDRX);
		LOADI_(stmw, ADDR);
		BODY(stswi, Emit(i, "stswi", g(i.RD), g(i.RA), hex(i.NB)));
		BODY(stswx, Emit(i, "stswx", g(i.RD), g(i.RA), g(i.RB)));
		LOADI(stw);
		LOADI_(stwbrx, ADDRX);
		BODY(stwcxd, Emit(i, "stwcx.", g(i.RD), ADDRX));
		
#pragma mark -
#pragma mark System Registers
#define CROP(name)	BODY(name, Emit(i, #name, cr(i.CRBD), cr(i.CRBA), cr(i.CRBB)))
		
		CROP(crand);
		CROP(crandc);
		CROP(creqv);
		CROP(crnand);
		CROP(crnor);
		CROP(cror);
		CROP(crorc);
		CROP(crxor);
		OP(isync);
		
		BODY(mcrf, Emit(i, "mcrfs", cr(i.CRFD), cr(i.CRFS)));
		BODY(mcrfs, Emit(i, "mcrfs", cr(i.CRFD), cr(i.CRFS)));
		BODY(mcrxr, Emit(i, "mcrxr", cr(i.CRFD)));
		BODY(mfcr, Emit(i, "mfcr", g(i.RD)));
		BODY(mffsx, Emit(i, opX("mffs", i.RC), f(i.RD)));
		
		IMPL(mfspr)
		{
			OpcodeArgument d = g(i.RD);
			uint8_t spr = static_cast<uint8_t>((i.RB << 5) | i.RA);
			switch (spr)
			{
				case 1: Emit(i, "mfxer", d); return;
				case 4: Emit(i, "mfrtcu", d); return;
				case 5: Emit(i, "mfrtcl", d); return;
				case 8: Emit(i, "mflr", d); return;
				case 9: Emit(i, "mfctr", d); return;
			}
			Emit(i, "mfspr", d, ::spr(spr));
		}
		
		IMPL(mftb)
		{
			OpcodeArgument d = g(i.RD);
			uint8_t tbl = static_cast<uint8_t>((i.RB << 5) | i.RA);
			if (tbl == 268)
				Emit(i, "mftb", d);
			else if (tbl == 269)
				Emit(i, "mftbu", d);
			else
				unknown(i);
		}
		
		BODY(mtcrf, Emit(i, "mtcrf", cr(i.CRM), g(i.RS)));
		BODY(mtfsb0x, Emit(i, opX("mtfsb0", i.RC), cr(i.CRBD)));
		BODY(mtfsb1x, Emit(i, opX("mtfsb0", i.RC), cr(i.CRBD)));
		BODY(mtfsfix, Emit(i, opX("mtfsfi", i.RC), cr(i.CRFD), hex(i.SR)));
		BODY(mtfsfx, Emit(i, opX("mtfsf", i.RC), hex(i.FM), f(i.RB)));
		
		IMPL(mtspr)
		{
			OpcodeArgument d = g(i.RD);
			uint8_t spr = static_cast<uint8_t>((i.RB << 5) | i.RA);
			switch (spr)
			{
				case 1: Emit(i, "mtxer", d); return;
				case 8: Emit(i, "mtlr", d); return;
				case 9: Emit(i, "mtctr", d); return;
			}
			Emit(i, "mtspr", d, ::spr(spr));
		}
		
		OP(rfi);
		OP(rfid);
		OP(sync);
		
#pragma mark -
#pragma mark Branching
		IMPL(bcctrx)
		{
			std::string opcode = opLK("b" + branchName(i.BO, i.BI) + "ctr", i.LK);
			uint8_t crN = i.BI >> 2;
			if (crN == 0)
				Emit(i, opcode);
			else
				Emit(i, opcode, cr(crN));
		}
		
		IMPL(bclrx)
		{
			std::string opcode = opLK("b" + branchName(i.BO, i.BI) + "lr", i.LK);
			uint8_t crN = i.BI >> 2;
			if (crN == 0)
				Emit(i, opcode);
			else
				Emit(i, opcode, cr(crN));
		}
		
		IMPL(bcx)
		{
			OpcodeArgument target = hex(i.BD << 2);
			std::string opcode = opLK("b" + branchName(i.BO, i.BI), i.LK);
			
			if (i.LK) opcode += 'l';
			if (i.AA) opcode += 'a';
			
			uint8_t crN = i.BI >> 2;
			if (crN == 0)
				Emit(i, opcode, target);
			else
				Emit(i, opcode, cr(crN), target);
		}
		
		IMPL(bx)
		{
			std::string suffix;
			if (i.LK) suffix += 'l';
			if (i.AA) suffix += 'a';
			
			Emit(i, "b" + suffix, hex(i.LI << 2));
		}
		
		OP(sc);
		
#pragma mark -
#pragma mark Supervisor Mode
#define DCB(name)	BODY(name, Emit(i, #name, g(i.RA), g(i.RB)))
#define A(name) BODY(name, Emit(i, #name, g(i.RA)))
		DCB(dcba);
		DCB(dcbf);
		DCB(dcbt);
		DCB(dcbst);
		DCB(dcbtst);
		DCB(dcbz);
		DCB(dcbi);
		
		BODY(eciwx, Emit(i, "eciwx", g(i.RD), g(i.RA), g(i.RB)));
		BODY(ecowx, Emit(i, "ecowx", g(i.RS), g(i.RA), g(i.RB)));
		DCB(icbi);
		A(mfmsr);
		BODY(mfsr, Emit(i, "mfsr", g(i.RD), spr(i.SR)));
		BODY(mfsrin, Emit(i, "mfsrin", g(i.RD), g(i.RB)));
		BODY(mtmsr, Emit(i, "mtmsr", g(i.RS)));
		BODY(mtsr, Emit(i, "mtsr", spr(i.SR), g(i.RD)));
		BODY(mtsrin, Emit(i, "mfsrin", g(i.RS), g(i.RB)));
		OP(tlbia);
		OP(tlbie);
		OP(tlbsync);
	}
}
