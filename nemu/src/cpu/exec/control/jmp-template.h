#include "cpu/exec/template-start.h"

//relative jump
#define instr jmpr	
static void do_execute(){
    cpu.eip += op_src->val;    
	print_asm("jmp 0x%x <offset = 0x%x\n>",cpu.eip+1+DATA_BYTE,op_src->val);
    return ;
}

make_instr_helper(si);
#undef instr

//direct jump
#define instr jmp	
static void do_execute(){
    cpu.eip = op_src->val -2;	//TODO: check 2    
	print_asm_template1();
    return ;
}
make_instr_helper(rm);
#undef instr

#include "cpu/exec/template-end.h"

