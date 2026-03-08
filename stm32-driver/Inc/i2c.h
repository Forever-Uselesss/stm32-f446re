#ifndef __I2C_H
#define __I2C_H
#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"
#include "stm32f4xx.h"
#include <stdbool.h>

// I2C Bus connection
typedef struct {
  FMPI2C_TypeDef *iface; // interface registers
  Pin_t pinSDA;       // SDA pin
  Pin_t pinSCL;       // SCL pin
} I2C_Bus_t;

extern I2C_Bus_t LeafyI2C;

// I2C Transfer records
typedef struct {
  I2C_Bus_t *bus; // I2C bus
  uint8_t addr;   // 7-bit address
  uint8_t *data;  // data buffer
  uint16_t size;  // data size
  bool stop;
  bool busy;
  struct I2C_Xfer_t *next; // next transfer in queue
} I2C_Xfer_t;

void I2C_Enable(I2C_Bus_t bus);
void I2C_Request(I2C_Xfer_t *p);

void serveiceI2CRequests(void); // called from main loop

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H */
