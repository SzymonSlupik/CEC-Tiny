/* Turn an OUTPUT ON/OFF based on HDMI-CEC
 * A fork of the https://github.com/tsowell/avr-hdmi-cec-volume/blob/master/main.c
 * Original code Copyright by Thomas Sowell 
 * ATTINY25 and Arduino IDE adaptation by Szymon Słupik 
 */

#define ADDRESS 0x05  // Pretend to be an audio HDMI subsystem
#define CECPIN 3      // Input pin for the CEC signal
#define ONOFFPIN 0    // Output pin to drive the external relay
#define LEDPIN 4      // Output pin to drive the status LED
#define ACTIVE 1      // Set to 1 to active;y reply to CEC messages
                      // Set to 0 for a passive mode (when another active CEC sink is present

void send_ack(void) {
  unsigned long ticks_start;
  unsigned long ticks;
  
  ticks_start = micros();
  pinMode(CECPIN, OUTPUT); // Pull the CEC line low.
  for (;;) {
    ticks = micros();
    if ((ticks - ticks_start) >= 1500) {
      pinMode(CECPIN, INPUT); // Set the CEC line back to high-Z.
      break;
    }
  }
}

unsigned long wait_edge(bool e) {
  unsigned long ticks;
  unsigned long last, cec;

  last = cec = digitalRead(CECPIN);
  for (;;) {
    ticks = micros();
    last = cec;
    cec = digitalRead(CECPIN);
    if (e) { //rising edge
      if ((last == 0) && (cec == 1)) {
        return ticks;
      }
    }
    else {   //falling edge
      if ((last == 1) && (cec == 0)) {
        return ticks;
      }
    }
  }
}

unsigned long wait_falling_edge(void) {
  return wait_edge(0);
}

unsigned long wait_rising_edge(void) {
  return wait_edge(1);
}

byte recv_data_bit(void) {
  unsigned long ticks_start;
  unsigned long ticks;

  ticks_start = micros();
  for (;;) {
    ticks = micros();
    if ((ticks - ticks_start) >= 1050) {
      return digitalRead(CECPIN);
    }
  }
}

byte wait_start_bit(void) {
  unsigned long ticks_start;
  unsigned long ticks;

  for (;;) {
    ticks_start = wait_falling_edge();
    ticks = wait_rising_edge();
    if ((ticks - ticks_start) >= 3900) {
      continue; // Rising edge took longer than 3.9 ms
    }
    else if ((ticks - ticks_start) >= 3500) {
      ticks = wait_falling_edge();
      if ((ticks - ticks_start) >= 4700) {
        continue; // Falling edge took longer than 4.7 ms
      }
      else if ((ticks - ticks_start) >= 4300) {
        return 0;
      }
      else {
        continue; // The falling edge came too early
      }
    }
    else {
      continue; // The rising edge came sooner than 3.5 ms
    }
  }
}

byte recv_frame(byte *pld, byte address) {
  unsigned long ticks_start;
  unsigned long ticks;
  byte bit_count;
  byte pldcnt;
  byte eom;

  wait_start_bit();
  bit_count = 9;
  pldcnt = 0;
  pld[pldcnt] = 0;
  for (;;) {
    ticks_start = micros();
    if (bit_count > 1) {
      pld[pldcnt] <<= 1;
      pld[pldcnt] |= recv_data_bit();
    }
    else {
      eom = recv_data_bit();
    }
    bit_count--;
    ticks = wait_falling_edge();
    if ((ticks - ticks_start) < 2050) { //2.05 ms
      return -1;
    }
    ticks_start = ticks;
    if (bit_count == 0) {
      if (((pld[0] & 0x0f) == address) || !(pld[0] & 0x0f)) {
        send_ack();
      }
      if (eom) {
        return pldcnt + 1;
      }
      else {
        ticks = wait_falling_edge();
        if ((ticks - ticks_start) >= 2750) { //2.75 ms
          return -1;
        }
      }
      bit_count = 9;
      pldcnt++;
      pld[pldcnt] = 0;
    }
  }
}

void send_start_bit(void) {
  unsigned long ticks;
  unsigned long ticks_start;

  ticks_start = micros();
  pinMode(CECPIN, OUTPUT);  // Pull the CEC line low.
  for (;;) {
    ticks = micros();
    if ((ticks - ticks_start) >= 3700) { //3.7 ms
      break;
    }
  }
  pinMode(CECPIN, INPUT);  // Set the CEC line back to high-Z.
  for (;;) {
    ticks = micros();
    if ((ticks - ticks_start) >= 4500) { //4.5 ms
      break;
    }
  }
}

void send_data_bit(int8_t bit) {
  unsigned long ticks;
  unsigned long ticks_start;

  ticks_start = micros();
  pinMode(CECPIN, OUTPUT); // Pull the CEC line low.
  for (;;) {
    ticks = micros();
    if (bit) {
      if ((ticks - ticks_start) >= 600) {
        break;
      }
    }
    else {
      if ((ticks - ticks_start) >= 1500) {
        break;
      }
    }
  }
  pinMode(CECPIN, INPUT); // Set the CEC line back to high-Z.
  for (;;) {
    ticks = micros();
    if ((ticks - ticks_start) >= 2400) { //2.4 ms
      break;
    }
  }
}

void send_frame(byte pldcnt, byte *pld) {
  byte bit_count;
  byte i;

  delay(13);
  send_start_bit();
  for (i = 0; i < pldcnt; i++) {
    bit_count = 7;
    do {
      send_data_bit((pld[i] >> bit_count) & 0x01);
    } while (bit_count--);
    send_data_bit(i == (pldcnt - 1));
    send_data_bit(1);
  }
}

void set_system_audio_mode(byte initiator, byte destination, byte system_audio_mode) {
  byte pld[3];

  pld[0] = (initiator << 4) | destination;
  pld[1] = 0x72;
  pld[2] = system_audio_mode;
  send_frame(3, pld);
}

void report_power_status(byte initiator, byte destination, byte power_status) {
  byte pld[3];

  pld[0] = (initiator << 4) | destination;
  pld[1] = 0x90;
  pld[2] = power_status;
  send_frame(3, pld);
}

void setup() {
  pinMode(CECPIN, INPUT);
  pinMode(ONOFFPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  OSCCAL=65; // Tune this when using internal ocsilator, as CEC is timing-sensitive
}

void loop() {
  byte pld[16];
  byte pldcnt;
  byte initiator, destination;

  pldcnt = recv_frame(pld, ADDRESS);
  if (pldcnt < 0) return;
  initiator = (pld[0] & 0xf0) >> 4;
  destination = pld[0] & 0x0f;

  if ((pldcnt > 1)) {
    switch (pld[1]) {
      case 0x36: //OFF
        digitalWrite(ONOFFPIN, LOW);
        digitalWrite(LEDPIN, LOW);
        break;
      case 0x82: //ON
        digitalWrite(ONOFFPIN, HIGH);
        digitalWrite(LEDPIN, HIGH);
        break;
      case 0x70: //SYSTEM AUDIO MODE REQUEST, responding to this enables Vol+/Vol- on the CEC bus
        if (ACTIVE && (destination == ADDRESS)) set_system_audio_mode(ADDRESS, 0x0f, 1);
        break;
      case 0x8f:
        /*Hack forcing Chromecast to send CEC messages - reply as if we are a TV (0x0) */
        if (ACTIVE && (destination == 0x0)) report_power_status(0, initiator, 0x00);
        break;
      default:
        break;
    }
  }
}
