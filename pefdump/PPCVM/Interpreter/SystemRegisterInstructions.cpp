#include "Interpreter.h"

namespace
{
	inline bool GetCRBit(MachineState* state, int bit)
	{
		return (state->cr[bit >> 2] >> (3 - (bit & 3))) & 1;
	}
	
	inline void SetCRBit(MachineState* state, int bit, bool value)
	{
		int bitValue = 8 >> (bit & 3);
		if (value)
			state->cr[bit >> 2] |= bitValue;
		else
			state->cr[bit >> 2] &= ~bitValue;
	}
	
	inline int GetCRField(MachineState* state, int field)
	{
		return state->cr[field];
	}
	
	inline void SetCRField(MachineState* state, int field, uint8_t value)
	{
		state->cr[field] = value;
	}
	
	inline void SetCR(MachineState* state, uint32_t value)
	{
		for (int i = 0; i < 8; i++)
		{
			state->cr[i] = (value >> (28 - i * 4)) & 0xf;
		}
	}
	
	inline uint32_t GetCR(MachineState* state)
	{
		uint32_t cr = state->cr[0] << 28;
		for (int i = 0; i < 8; i++)
		{
			cr |= state->cr[i] << (28 - i * 4);
		}
		return cr;
	}
}

namespace PPCVM
{
	namespace Execution
	{
		void Interpreter::crand(Instruction inst)
		{
			SetCRBit(state, inst.CRBD, GetCRBit(state, inst.CRBA) & GetCRBit(state, inst.CRBB));
		}

		void Interpreter::crandc(Instruction inst)
		{
			SetCRBit(state, inst.CRBD, GetCRBit(state, inst.CRBA) & (1 ^ GetCRBit(state, inst.CRBB)));
		}

		void Interpreter::creqv(Instruction inst)
		{
			SetCRBit(state, inst.CRBD, 1 ^ (GetCRBit(state, inst.CRBA) ^ GetCRBit(state, inst.CRBB)));
		}

		void Interpreter::crnand(Instruction inst)
		{
			SetCRBit(state, inst.CRBD, 1 ^ (GetCRBit(state, inst.CRBA) & GetCRBit(state, inst.CRBB)));
		}

		void Interpreter::crnor(Instruction inst)
		{
			SetCRBit(state, inst.CRBD, 1 ^ (GetCRBit(state, inst.CRBA) | GetCRBit(state, inst.CRBB)));
		}

		void Interpreter::cror(Instruction inst)
		{
			SetCRBit(state, inst.CRBD, (GetCRBit(state, inst.CRBA) | GetCRBit(state, inst.CRBB)));
		}

		void Interpreter::crorc(Instruction inst)
		{
			SetCRBit(state, inst.CRBD, (GetCRBit(state, inst.CRBA) | (1 ^ GetCRBit(state, inst.CRBB))));
		}

		void Interpreter::crxor(Instruction inst)
		{
			SetCRBit(state, inst.CRBD, (GetCRBit(state, inst.CRBA) ^ GetCRBit(state, inst.CRBB)));
		}

		void Interpreter::isync(Instruction inst)
		{
			__sync_synchronize();
		}

		void Interpreter::mcrf(Instruction inst)
		{
			int cr_f = GetCRField(state, inst.CRFS);
			SetCRField(state, inst.CRFD, cr_f);
		}

		void Interpreter::mcrfs(Instruction inst)
		{
			abort();
		}

		void Interpreter::mcrxr(Instruction inst)
		{
			// USES_XER
			SetCRField(state, inst.CRFD, state->xer >> 28); 
			state->xer &= ~0xF0000000; // clear 0-3
		}

		void Interpreter::mfcr(Instruction inst)
		{
			state->gpr[inst.RD] = GetCR(state);
		}

		void Interpreter::mffsx(Instruction inst)
		{
			abort();
		}

		void Interpreter::mfspr(Instruction inst)
		{
			abort();
		}

		void Interpreter::mftb(Instruction inst)
		{
			abort();
		}

		void Interpreter::mtcrf(Instruction inst)
		{
			uint32_t crm = inst.CRM;
			if (crm == 0xFF)
			{
				SetCR(state, state->gpr[inst.RS]);
			}
			else
			{
				//TODO: use lookup table? probably not worth it
				uint32_t mask = 0;
				for (int i = 0; i < 8; i++) {
					if (crm & (1 << i))
						mask |= 0xF << (i*4);
				}
				SetCR(state, (GetCR(state) & ~mask) | (state->gpr[inst.RS] & mask));
			}
		}

		void Interpreter::mtfsb0x(Instruction inst)
		{
			abort();
		}

		void Interpreter::mtfsb1x(Instruction inst)
		{
			abort();
		}

		void Interpreter::mtfsfix(Instruction inst)
		{
			abort();
		}

		void Interpreter::mtfsfx(Instruction inst)
		{
			abort();
		}

		void Interpreter::mtspr(Instruction inst)
		{
			abort();
		}
	}
}