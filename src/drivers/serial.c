#include <kernel/serial.h>
#include <kernel/panic.h>
#include <kernel/types.h>
#include <arch/plic.h>
#include <arch/spinlock.h> 
#include "csr.h"

#define UART_BASE 0x10000000
#define UART_RBR 0 
#define UART_THR 0 
#define UART_IER 1 
#define UART_FCR 2 
#define UART_LSR 5 

#define IER_RX_ENABLE   (1 << 0)
#define FCR_FIFO_ENABLE (1 << 0)
#define FCR_CLEAR_RX    (1 << 1)
#define FCR_CLEAR_TX    (1 << 2)
#define LSR_DATA_READY  (1 << 0)
#define LSR_TX_EMPTY    (1 << 5)

#define REG(offset) ((volatile u8 *)(UART_BASE + (offset)))
#define UART_IRQ 10

#define SERIAL_BUF_SIZE 128

struct serial_buffer {
    struct spinlock lock;
    char data[SERIAL_BUF_SIZE];
    int head; 
    int tail; 
};

static struct serial_buffer uart_buf;


void serial_init()
{
    // Initialize our spinlock and buffer pointers
    spin_init(&uart_buf.lock);
    uart_buf.head = 0;
    uart_buf.tail = 0;

    // Initialize the hardware
    *REG(UART_FCR) = FCR_FIFO_ENABLE | FCR_CLEAR_RX | FCR_CLEAR_TX;
    serial_irq_disable();
}

void serial_irq_enable()
{
    *REG(UART_IER) |= IER_RX_ENABLE;
    plic_irq_set_priority(UART_IRQ, 1);
    plic_hart_enable_irq(0, UART_IRQ);
    plic_hart_set_threshold(0, 0);
    csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
    *REG(UART_IER) &= ~IER_RX_ENABLE;
}

size_t serial_read(char *buf)
{
    size_t count = 0;
    while (*REG(UART_LSR) & LSR_DATA_READY) {
        buf[count++] = *REG(UART_RBR);
    }
    return count;
}

void serial_putc(char c)
{
    while ((*REG(UART_LSR) & LSR_TX_EMPTY) == 0) {}
    *REG(UART_THR) = c;
}

void serial_puts(char *str)
{
    while (*str) {
        serial_putc(*str++);
    }
}

void serial_irq()
{
    char hw_buf[128]; 
    size_t bytes_read = serial_read(hw_buf);
    
    // Acquire the lock and disable interrupts to prevent deadlocks
    u64 flags = spin_lock_irqsave(&uart_buf.lock);
    
    for (size_t i = 0; i < bytes_read; i++) {
        int next_tail = (uart_buf.tail + 1) % SERIAL_BUF_SIZE;
        
        // If the buffer isn't full, add the character
        if (next_tail != uart_buf.head) {
            uart_buf.data[uart_buf.tail] = hw_buf[i];
            uart_buf.tail = next_tail;
        }
    }
    
    // Release the lock and restore interrupt state
    spin_unlock_irqrestore(&uart_buf.lock, flags);
}

/* * The Consumer: The shell in kmain calls this.
 * It waits until there is data in the buffer, locks it, reads from the head, and unlocks.
 */
char serial_getc()
{
    char c;
    while (1) {
        u64 flags = spin_lock_irqsave(&uart_buf.lock);
        
        // Check if the buffer has data (head != tail)
        if (uart_buf.head != uart_buf.tail) {
            c = uart_buf.data[uart_buf.head];
            uart_buf.head = (uart_buf.head + 1) % SERIAL_BUF_SIZE;
            
            spin_unlock_irqrestore(&uart_buf.lock, flags);
            return c; // We found a character, return it to the shell!
        }
        
        spin_unlock_irqrestore(&uart_buf.lock, flags);
        
        // If no data, the loop just spins and checks again.
    }
}