
// I2C driver
#include "i2c.h"

#include <stddef.h>

#include "gpio.h"

// There is one I2C bus present on the lab platform:
I2C_Bus_t LeafyI2C = {
    I2C2,        // I2C controller 2
    {GPIOA, 0},  // SDA pin PF0
    {GPIOA, 1}   // SCL pin PF1
};
// Pointers to head and tail of the transfer queue
static I2C_Xfer_t *head = NULL;
static I2C_Xfer_t *tail = NULL;
static int n = 0;  // Current byte number

// Enable I2C controller and configure associated GPIO pins
void I2C_Enable(I2C_Bus_t bus) {
    if (bus.iface->CR1 & I2C_CR1_PE) return;  // Already enabled
    // Enable clock to selected I2C controller
    RCC->APB1ENR |= bus.iface == I2C1   ? RCC_APB1ENR_I2C1EN
                    : bus.iface == I2C2 ? RCC_APB1ENR_I2C2EN
                    : bus.iface == I2C3 ? RCC_APB1ENR_I2C3EN
                                        : 0;
    // Enable clocks to GPIO ports containing SDA and SCL pins

    GPIO_Enable(bus.pinSDA);
    GPIO_Enable(bus.pinSCL);

    // Configure for open drain (PMOS disabled)

    GPIO_Config(bus.pinSDA, OD, S0, NOPUPD);
    GPIO_Config(bus.pinSCL, OD, S0, NOPUPD);

    // Alternate function mode
    GPIO_Mode(bus.pinSDA, ALTFUNC);
    GPIO_Mode(bus.pinSCL, ALTFUNC);
    // Select alternate function as I2C
    GPIO_AltFunc(bus.pinSDA, 0x4);
    GPIO_AltFunc(bus.pinSCL, 0x4);
    // Configure I2C controller
    bus.iface->TIMINGR = 0xE14;
    bus.iface->CR1 = I2C_CR1_PE;
    head = NULL;
    tail = NULL;
}

// Add a transfer request to the queue
void I2C_Request(I2C_Xfer_t *p) {
    if (!(p->bus->iface->CR1 & I2C_CR1_PE)) return;  // I2C not enabled
    if (head == NULL)
        head = p;  // Add to empty queue
    else
        tail->next = p;  // Add to tail of non-empty queue
    tail = p;
    p->next = NULL;
    p->busy = true;
}

static void TransferDone(void) {
    I2C_Xfer_t *p = head;
    head = p->next;
    p->next = NULL;
    p->busy = 0;
}

static void ReadTransfer(void) {
    if (n == 0) {
        // Begin new transfer
        head->bus->iface->CR2 =
            (head->addr & 0xFE) | 1 << 10 | head->size << 16;
        head->bus->iface->CR2 |= head->stop ? FMPI2C_CR2_AUTOEND : 0;
        head->bus->iface->CR1 |= FMPI2C_CR1_TCIE;
        head->bus->iface->CR2 |= FMPI2C_CR2_START;
        n++;
    } else if (head->bus->iface->ISR & FMPI2C_ISR_RXNE) {
        // Copy receive data from hardware buffer to memory buffer
        head->data[n - 1] = head->bus->iface->RXDR;
        if (n < head->size)
            n++;
        else {
            head->bus->iface->CR1 &= ~FMPI2C_CR1_TCIE;
            TransferDone();
            n = 0;
        }
    }
}

static void WriteTransfer(void) {
    if (n == 0) {
        // Begin new transfer
        head->bus->iface->CR2 = (head->addr & 0xFE) | head->size << 16;
        head->bus->iface->CR2 |= head->stop ? FMPI2C_CR2_AUTOEND : 0;
        head->bus->iface->TXDR = head->data[n];
        head->bus->iface->CR1 |= FMPI2C_CR1_TCIE;
        head->bus->iface->CR2 |= FMPI2C_CR2_START;
        n++;
    } else if (n < head->size) {
        if (head->bus->iface->ISR & FMPI2C_ISR_TXIS)
            // Copy transmit data from memory buffer to hardware buffer
            head->bus->iface->TXDR = head->data[n++];
    } else {
        head->bus->iface->CR1 &= ~FMPI2C_CR1_TCIE;
        TransferDone();
        n = 0;
    }
}
void ServiceI2CRequests(void) {
    if (head == NULL) return;
    if (head->addr & 1)
        ReadTransfer();
    else
        WriteTransfer();
}