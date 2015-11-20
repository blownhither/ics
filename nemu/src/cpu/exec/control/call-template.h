#include "cpu/exec/template-start.h"

#define instr call

//push(ip or eip)
static void do_execute(){
	reg_l(4) += 4; //esp
	/*TODO warning:?! why +=
	//DATA_TYPE temp = reg_w(4);
	MEM_W(reg_l(4) , cpu.eip); //= swaddr_write	
	*/

	//TODO: currently only 0xe8 implemented
#if DATA_BYTE == 2
	cpu.eip = (cpu.eip + op_src->simm) & 0xffff; 
#elif DATA_BYTE == 4
	cpu.eip = cpu.eip + op_src->simm;		//decode_si saves result at simm 
#endif
	print_asm_template1(); 
	return; 
}

make_instr_helper(si); 
#include "cpu/exec/template-end.h"
