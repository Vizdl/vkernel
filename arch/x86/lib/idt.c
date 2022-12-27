#include <asm/desc.h>

#define CALL_TAGE_TAG 12
#define INTR_TAGE_TAG 14
#define TRAP_TAGE_TAG 15

struct idt_desc idt_table[IDT_TABLE_SIZE] = { {0, 0}, };

void set_intr_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n, INTR_TAGE_TAG, 0,addr);
}


void set_trap_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n, TRAP_TAGE_TAG, 0,addr);
}


void set_system_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n, TRAP_TAGE_TAG, 3,addr);
}

void set_call_gate(void *a, void *addr)
{
	_set_gate(a, CALL_TAGE_TAG, 3,addr);
}