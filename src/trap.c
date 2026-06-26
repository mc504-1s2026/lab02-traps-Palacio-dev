#include <kernel/types.h>
#include <kernel/trap.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <arch/timer.h>
#include <arch/plic.h>
#include <kernel/serial.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq(u64 cause)
{
    switch (cause) {
        case 5: // Supervisor Timer Interrupt
            timer_irq();
            break;
            
        case 9: { // Supervisor External Interrupt
            // 1. CLAIM: Ask the PLIC which external device interrupted us.
            // We assume we are running on Hart 0.
            u32 irq = plic_hart_claim_irq(0);
            
            if (irq != 0) {
                // 2. HANDLE: Route the specific IRQ to its driver
                if (irq == 10) {
                    serial_irq(); // Read from the UART and echo it
                } else {
                    // If you add a virtio disk or network card later, 
                    // you would add their IRQ numbers here!
                    // printk("Unknown external IRQ: %d\n", irq);
                }
                
                // 3. COMPLETE: Tell the PLIC we are done, so it can send more.
                plic_hart_complete_irq(0, irq);
            }
            break;
        }
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