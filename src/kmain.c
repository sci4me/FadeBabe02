#include "navi65.h"
#include "leds.h"
#include "switches.h"
#include "acia.h"
#include "io.h"
#include "fadebabe.h"


void acia_bufovfl(void) {
    acia_putln("Warning: ACIA recv buffer overflow");
    panic(0xFF);
}
  

u8 f = 0;

void kmain(void) {
    leds_init();
    acia_init(acia_bufovfl);

    leds_set_value(0x00);


    fadebabe_run();


    while(1) {
        leds_set_value(f++);

        delay_ms(250);
    }
}