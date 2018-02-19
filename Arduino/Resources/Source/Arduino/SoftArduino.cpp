#include "SoftArduino.h"
#include "ModelicaUtilities.h"


SoftArduino SoftArduino::instance;

#define INSTANCE SoftArduino::instance


static void assertPin(uint8_t pin) {
	if (pin >= NUM_DIGITAL_PINS) {
		ModelicaFormatError("Pin must be < NUM_DIGITAL_PINS = %d but was %d.", NUM_DIGITAL_PINS, pin);
	}
}

void SoftInterrupt::update(int potential) {

	bool executeISR = false;

	switch(mode) {
	case RISING:
		executeISR = prePotential == LOW && potential == HIGH;
		break;
	case FALLING:
		executeISR = prePotential == HIGH && potential == LOW;
		break;
	case CHANGE:
		executeISR = prePotential != potential;
		break;
	}

	prePotential = potential;

	if (executeISR) {
		// execute the service routine
		isr();
	}

}

void pinMode(uint8_t pin, uint8_t mode) {
	//ModelicaFormatMessage("pinMode(%d, %d)\n", pin, mode);

	assertPin(pin);

	switch (mode) {
	case INPUT:
	case INPUT_PULLUP:
		INSTANCE.portMode[pin] = SoftArduino::PORT_MODE_INPUT;
		break;
	case OUTPUT:
		INSTANCE.portMode[pin] = SoftArduino::PORT_MODE_DIGITAL;
		break;
	default:
		ModelicaFormatError("Error calling pinMode(). Mode must be one of INPUT = 0, OUTPUT = 1 or INPUT_PULLUP = 2 but was %d.", mode);
	}
}

void digitalWrite(uint8_t pin, uint8_t val) {

	assertPin(pin);

	INSTANCE.pulseWidth[pin] = (val == HIGH) ? SoftArduino::DEFAULT_PULSE_PERIOD : 0;
	//ModelicaFormatMessage("digitalWrite(%d, %d) -> %d\n", pin, val, INSTANCE.pulseWidth[pin]);
}

int digitalRead(uint8_t pin) {

	assertPin(pin);

	// ModelicaFormatMessage("digitalRead(%d) -> %f\n", pin, instance.digital[pin]);
	return INSTANCE.digital[pin] > 0 ? HIGH : LOW;
}

int analogRead(uint8_t pin) {

	assertPin(pin);

	if (pin < A0 || pin > A7) {
		ModelicaFormatError("Illegal analog pin: %d\n", pin);
	}

	// TODO: clip [0, 1023]?

	//ModelicaFormatMessage("analogRead(%d) -> %d\n", pin, val);
	
	return INSTANCE.analog[pin - A0];
}

void analogReference(uint8_t mode) {
	
	switch(mode) {
	case DEFAULT:
	//case INTERNAL:
	//case INTERNAL1V1:
	case EXTERNAL:
		INSTANCE.analogReferenceMode = mode;
		break;
	default:
		ModelicaFormatError("Illegal analog reference mode: %d\n", mode);
		break;
	}
}

void analogWrite(uint8_t pin, int val) {

	assertPin(pin);

	INSTANCE.portMode[pin] = SoftArduino::PORT_MODE_PWM;
	INSTANCE.pulseWidth[pin] = int((val / 255.) * SoftArduino::DEFAULT_PULSE_PERIOD);

	//ModelicaFormatMessage("analogWrite(%d, %d) -> %d\n", pin, val, INSTANCE.pulseWidth[pin]);
}

unsigned long millis() {
	return INSTANCE.time / 1000;
}

unsigned long micros() {
	return INSTANCE.time;
}

void delay(unsigned long ms) {

	//ModelicaFormatMessage("delay(%d) at %g s\n", ms, INSTANCE.time * 1e-6);
	
	const unsigned long endTime = INSTANCE.time + ms * 1000;

	while (INSTANCE.time < endTime) {
		// idle
	}
}

void delayMicroseconds(unsigned int us) {
	
	const unsigned long endTime = INSTANCE.time + us;
	
	while (INSTANCE.time < endTime) {
		// idle
	}
}

void attachInterrupt(uint8_t interrupt, void (*isr)(void), int mode) {

	// ModelicaFormatMessage("attachInterrupt(%d, %d, %d)\n", interrupt, isr, mode);

	if (interrupt != 0 && interrupt != 1) {
		ModelicaFormatError("Illegal interrupt: %d", interrupt);
	}

	int pin = interruptToDigitalPin(interrupt);
	int potential = INSTANCE.digital[pin];
	INSTANCE.interrupts[interrupt] = new SoftInterrupt(isr, mode, potential);
}

void detachInterrupt(uint8_t interrupt) {
	auto ir = INSTANCE.interrupts[interrupt];
	INSTANCE.interrupts[interrupt] = nullptr;
	delete ir;
}

void interrupts() { 
	INSTANCE.interruptsEnabled = true;
}

void noInterrupts() {
	INSTANCE.interruptsEnabled = false;
}

unsigned long pulseIn(const uint8_t pin, const uint8_t state, unsigned long timeout) {

	assertPin(pin);

	unsigned long currentTime = INSTANCE.time;
	const unsigned long endTime = currentTime + timeout;
	uint8_t preState = INSTANCE.digital[pin];
	long startTime = -1;

	// until the timeout is reached...
	while(currentTime < endTime) {

		const uint8_t currentState = INSTANCE.digital[pin];
		currentTime = INSTANCE.time;

		if (startTime < 0 && preState != state && currentState == state) {

				// start measurement
				startTime = currentTime;

		} else if (startTime >= 0 && preState == state && currentState != state) {

				// return the elapsed time
				return currentTime - startTime;
		}

		// remember the state
		preState = currentState;
	}

	// timed out
	return 0;
}

unsigned long pulseInLong(uint8_t pin, uint8_t state, unsigned long timeout) {
	return pulseIn(pin, state, timeout);
}

SoftArduino::SoftArduino() {

	// intialize pulsePeriod
	for (int i = 0; i < NUM_DIGITAL_PINS; i++) {
		pulsePeriod[i] = DEFAULT_PULSE_PERIOD;
	}

}
