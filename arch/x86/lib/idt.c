#include <asm/desc.h>

struct idt_desc idt_table[IDT_TABLE_SIZE] = { {0, 0}, };

void set_intr_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n,14,0,addr);
}


void set_trap_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n,15,0,addr);
}


void set_system_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n,15,3,addr);
}

void set_call_gate(void *a, void *addr)
{
	_set_gate(a,12,3,addr);
}