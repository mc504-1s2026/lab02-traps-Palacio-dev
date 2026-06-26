#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];
void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	hart_irq_enable();

    char cmd_buf[128];
    int cmd_idx = 0;

	serial_puts("> ");

    while (1) {
        char c = serial_getc();

        if (c == '\r' || c == '\n') {
            serial_putc('\n');       
            cmd_buf[cmd_idx] = '\0'; 

            if (cmd_idx > 0) {
                if (strncmp(cmd_buf, "uptime", 6) == 0) {
                    u64 ticks = timer_read();
                    u64 seconds = ticks / 10000000ULL; 
                    printk(LOG_INFO, "%ds\n", (int)seconds);
                } 
                else if (strncmp(cmd_buf, "echo ", 5) == 0) {
                    printk(LOG_INFO, "%s\n", &cmd_buf[5]);
                } 
                else if (strncmp(cmd_buf, "alarm ", 6) == 0) {
                    u64 seconds = strtou64(&cmd_buf[6], 10);
                    if (seconds > 0) {
                        timer_set_alarm(seconds);
                        timer_irq_enable(); 
                    }
                } 
            }
            // Teste

            cmd_idx = 0;
            serial_puts("> ");
        } 
        else if (c == '\b' || c == 0x7F) {
            if (cmd_idx > 0) {
                cmd_idx--;
                serial_puts("\b \b"); 
            }
        } 
        else {
            if (cmd_idx < (int)sizeof(cmd_buf) - 1) {
                cmd_buf[cmd_idx++] = c;
                serial_putc(c); 
            }
        }
    }
}
