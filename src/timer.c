#include <arch/timer.h>
#include <kernel/panic.h>
#include "csr.h"

#define TIMER_FREQ 10000000

u64 timer_read()
{
	return csr_read(CSR_STIME);
}

void timer_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_STIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	u64 current_time = timer_read();
	u64 alarm_time = current_time + (secs * TIMER_FREQ);
	csr_write(CSR_STIMECMP, alarm_time);
}

void timer_irq()
{
	timer_irq_disable();
}
