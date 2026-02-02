//Aisosa Okunbor and Caleb Smith 
#ifndef IRRISYSTEM_H_
#define IRRISYSTEM_H_

// Bitband aliases
#define PIN_Z1 0
#define PIN_Z2 1
#define PIN_Z3 2

#define ZONE_ONE     (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + PIN_Z1*4)))
#define ZONE_TWO      (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + PIN_Z2*4)))
#define ZONE_THREE      (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + PIN_Z3*4)))

// PortF masks
#define ZONE_ONE_MASK (1<<PIN_Z1)
#define ZONE_TWO_MASK (1<<PIN_Z2)
#define ZONE_THREE_MASK (1<<PIN_Z3)

// Macros
#define MAX_CHARS       80
#define ZONE_COUNT      3
#define UNIT_SECONDS    1
#define UNIT_MINUTES    60
#define UNIT_HOURS      360

void irrigationTick();
void processCommand(char *topic, char *data);
void Timer4A_Handler(void);
#endif // IRRISYSTEM_H_

