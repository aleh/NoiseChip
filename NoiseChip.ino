//
// Turns an ATtiny85 into a "noise chip" suitable for an analogue drum machine such as 
// [LDB-1](http://mickeydelp.com/blog/anatomy-of-a-drum-machine).
//
// One of the pins outputs digital "white" noise using a linear feedback shift register, 
// 4 other pins output different square waves out of tune with each other.
//
// The noise is used by the snare drum, clap and hi-hat modules, while sqaure waves are 
// used to add a bit of metallic tembre for the hi-hat.
//
// Note that the frequencies of the square waves are not described in the article, 
// but there is a reference to a schematics of Boss DR-110 where each of these square 
// wave modules is made using 2 inverters with a 4.7nF capacitor being charged via 
// 82K, 120K, 330K, and 220K resistors. My crude calculation shows that the corresponding 
// frequencies are: 1049Hz, 717Hz, 261Hz, 392Hz. 
// 

#include <a21.hpp>

using namespace a21;

// This is to generate a single wave. The global timer calls our tick() function here every tick_period_us microseconds.
template<typename pin, uint16_t tick_period_us, uint16_t frequency>
class Wave {

  typedef Wave<pin, tick_period_us, frequency> Self;

  // How many ticks of the system timer are needed for a single period of our wave.
  static const uint8_t period_ticks = (1000000L / frequency) / tick_period_us;
  
  // When do we need to flip from HIGH to LOW. By varying this we could also adjust the pulse width.
  static const uint8_t half_period_ticks = period_ticks / 2;

  // The usual trick allowing to avoid to declare static members separately.
  static inline Self& getSelf() {
    static Self s = Self();
    return s;
  }

  // Number of ticks of the system timer passed so far. 
  // Varying the initial value here could allow to adjust the phase of the wave, though currenty does not seem to be needed.
  uint8_t counter;
  
public:

  static void begin() {
    pin::setOutput();
    tick();
  }
  
  static void tick() {
    
    Self& self = getSelf();

    // Note that we are not trying to save the number of operations here as it allows to get roughly the same execution time 
    // for tick() no matter the current state and thus decrease jitter of all the waves.

    if (self.counter <= period_ticks)
      self.counter++;
    else
      self.counter = 0;

    if (self.counter <= half_period_ticks)
      pin::setHigh();
    else
      pin::setLow();
  }
};

// A basic tick handler for our timer does nothing.
class NoHandler {
public:
  static void begin() {}
  static void tick() {}
};

// And this is our custom wrapper, setting up  timer 0 to CTC mode so every tick is exactly period_us microseconds for 8MHz F_CPU.
template<
  uint16_t period_us,
  typename Handler1, 
  typename Handler2 = NoHandler,
  typename Handler3 = NoHandler,
  typename Handler4 = NoHandler
>
class Timer {
  
public:

  static void inline handleCOMPA() {    
    Handler1::tick();
    Handler2::tick();
    Handler3::tick();
    Handler4::tick();
    TIFR |= _BV(OCF0A);
  }

  static void begin() {
    
    TCCR0A = 0;
    TCCR0B = 0;
    
    // Timer mode: Clear Timer on Compare Match (CTC)
    TCCR0A |= (1 << WGM01) | (0 << WGM00);
    TCCR0B |= (0 << WGM02);

    // Enable a compare match A interrupt.
    TIMSK = _BV(OCIE0A);

    // Want an interrupt this many microseconds (assuming single tick of the timer being exactly 1us).
    OCR0A = period_us;

    // Enable the timer with CLK/8 prescaler, so we get 1us per timer tick when running on 8MHz.
    TCCR0B |= (0 << CS02) | (1 << CS01) | (0 << CS00);

    Handler1::begin();
    Handler2::begin();
    Handler3::begin();
    Handler4::begin();
  }
};

// How often our timer should tick, microseconds. 
// Note that a very small value won't allow the interrupt handler too keep up.
const uint16_t timer_period_us = 25;

typedef Timer< 

  timer_period_us,
  
  // As mentioned above, the frequencies are calculated based on DR-110 schematics.
  // (The frequencies I tried before making the calculations were appoximately 685, 1087, 1613, 2500.)
  Wave<FastPin<0>, timer_period_us, 1049>,
  Wave<FastPin<1>, timer_period_us, 717>,
  Wave<FastPin<2>, timer_period_us, 261>,
  Wave<FastPin<3>, timer_period_us, 392>
  
> timer;

// Unfortunately an ISR cannot be assigned directly in the template, so have to do it here.
ISR(TIM0_COMPA_vect, ISR_BLOCK) {
  timer::handleCOMPA();
}

class LFSR {
private:

  uint16_t reg;
  
public:

  LFSR(uint16_t seed) : reg(seed) {}
  
  bool next() {
    uint8_t b = ((reg >> 0) ^ (reg >> 1) ^ (reg >> 3) ^ (reg >> 12)) & 1;
    reg = (reg >> 1) | (b << 15);
    return b;
  }
};

LFSR lsfr1(0xA1e);

typedef FastPin<4> NoisePin;

inline void setup() {
  NoisePin::setOutput();
  timer::begin();  
}

inline void loop() {
  NoisePin::write(lsfr1.next());
}

// Don't want Arduino timer routines since we are using the timer already. 
// Luckily we can override main().
int main(void) {

  setup();
  sei();
  
  while (true) {
    loop();
  }
}

