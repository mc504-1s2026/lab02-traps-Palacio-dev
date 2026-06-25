#include <kernel/types.h>
#include <kernel/trap.h>
#include <kernel/panic.h>
#include <csr.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq()
{
	/* not implemented */
	BUG();
}

void handle_exception()
{
	/* not implemented */
	BUG();
}

void trap_setup()
{
    /* * Write the virtual address of our assembly trap handler into stvec.
     * By default, writing the raw address sets it to "Direct Mode" 
     * (all traps jump to this exact address).
     */
    csr_write(CSR_STVEC, (u64)trap_entry);
}

void handle_trap()
{
    u64 scause = csr_read(CSR_SCAUSE);
    u64 stval  = csr_read(CSR_STVAL);
    
    /* bit 63 indica se foi uma exceção ou interrupção */
    int is_interrupt = (scause & (1ULL << 63)) != 0;
    
    u64 exception_code = scause & ~(1ULL << 63);

    if (is_interrupt) {
        handle_irq(exception_code);
    } else {
        handle_exception(exception_code);
    }
}

void hart_irq_enable()
{
	/* not implemented */
	BUG();
}

u64 hart_irq_save()
{
	/* not implemented */
	BUG();
}

void hart_irq_restore(u64 flags)
{
	/* not implemented */
	BUG();
}

void hart_irq_disable()
{
	/* not implemented */
	BUG();
}
