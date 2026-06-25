#include <kernel/types.h>
#include <kernel/trap.h>
#include <kernel/panic.h>
#include <csr.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq(u64 cause)
{
    switch (cause) {
        case 5: // Supervisor Timer Interrupt
            timer_irq();
            break;
        case 9: // Supervisor External Interrupt (UART/Keyboard later)
            // external_irq_handler();
            break;
        default:
            panic("Unhandled IRQ!");
            break;
    }
}

void handle_exception(u64 cause)
{
	/* not implemented */
	BUG();
}

void trap_setup()
{
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
    csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
    u64 old_sstatus = csr_read_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
    return (old_sstatus & CSR_SSTATUS_SIE) != 0;
}

void hart_irq_restore(u64 flags)
{
    if (flags) {
        hart_irq_enable();
    } else {
        hart_irq_disable();
    }
}

void hart_irq_disable()
{
    csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}