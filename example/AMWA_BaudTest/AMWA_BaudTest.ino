/*
 * AMWA_BaudTest
 *
 * UART baud reliability sample for AMWA-01 and Arduino.
 *
 * This sketch checks how reliably the Arduino can receive AT responses from
 * AMWA-01 at several baud rates. It is useful when AutoUDP or other bursty
 * output looks unstable on boards such as Arduino UNO R4.
 *
 * What it does:
 *   1. Reset AMWA-01 and enter AT mode. If AutoUDP is enabled, it sends AT*
 *      during the boot escape window.
 *   2. Run a small "AT" response test and a larger "AT+SAUDP?" response test
 *      at each baud rate.
 *   3. Restore AMWA-01 UART baud to 115200 at the end.
 *
 * Notes:
 *   - AT+UART changes the runtime UART baud only. It is not saved to flash.
 *   - Increase SIMPLE_ITERATIONS / BURST_ITERATIONS for longer tests.
 *   - Set ECHO_AT_RESPONSES to 1 if you want the AMWA response lines echoed to
 *     Serial Monitor while testing. This adds USB CDC load and can make drops
 *     easier to reproduce.
 */

#include <AMWA_LIB.h>

// ---- Serial ports ----
#define AT_SERIAL          Serial1
#define INFO_SERIAL        Serial
#define INFO_SERIAL_BAUD   115200

// ---- Test settings ----
#define SIMPLE_ITERATIONS  100
#define BURST_ITERATIONS   50
#define RESPONSE_TIMEOUT   500
#define PROGRESS_INTERVAL  10

// 0: clean summary output / 1: echo AT responses to Serial Monitor.
#define ECHO_AT_RESPONSES  0

// Baud rates to test. Remove entries if you want a shorter run.
static const unsigned long TEST_BAUDS[] = {
  115200,
  57600,
  38400,
  19200,
  9600
};

AMWA wifihalow(ECHO_AT_RESPONSES != 0, &AT_SERIAL, &INFO_SERIAL);

struct BaudResult {
  unsigned long baud;
  int simpleFailures;
  int burstFailures;
};

static BaudResult results[sizeof(TEST_BAUDS) / sizeof(TEST_BAUDS[0])];

static void waitForSerialMonitor() {
  unsigned long start = millis();
  while (!INFO_SERIAL && (millis() - start) < 3000) {
    delay(10);
  }
}

static void drainAtSerial(unsigned long quiet_ms = 20) {
  unsigned long last_rx = millis();
  while ((millis() - last_rx) < quiet_ms) {
    while (AT_SERIAL.available() > 0) {
      AT_SERIAL.read();
      last_rx = millis();
    }
  }
}

static bool sendCommandWaitOK(const char *command, unsigned long timeout_ms) {
  AT_SERIAL.print(command);
  AT_SERIAL.print('\r');
  AT_SERIAL.flush();

  AMWA::WaitResult r = wifihalow.waitResponse("OK", timeout_ms);
  return r.result;
}

static bool switchBaud(unsigned long baud) {
  INFO_SERIAL.print("[BAUD] Switching AMWA UART to ");
  INFO_SERIAL.print(baud);
  INFO_SERIAL.print(" ... ");

  AT_SERIAL.print("AT+UART=");
  AT_SERIAL.print(baud);
  AT_SERIAL.print('\r');
  AT_SERIAL.flush();

  AMWA::WaitResult r = wifihalow.waitResponse("OK", 1000);
  if (!r.result) {
    INFO_SERIAL.println("FAILED");
    return false;
  }

  delay(200);
  AT_SERIAL.end();
  AT_SERIAL.begin(baud);
  delay(50);
  drainAtSerial();

  INFO_SERIAL.println("OK");
  return true;
}

static int runStress(const char *label, const char *command, int iterations) {
  int failures = 0;

  INFO_SERIAL.print("  ");
  INFO_SERIAL.print(label);
  INFO_SERIAL.print(" x");
  INFO_SERIAL.print(iterations);
  INFO_SERIAL.print(": ");

  for (int i = 0; i < iterations; i++) {
    if (!sendCommandWaitOK(command, RESPONSE_TIMEOUT)) {
      failures++;
      drainAtSerial();
    }

    if (((i + 1) % PROGRESS_INTERVAL) == 0) {
      INFO_SERIAL.print('.');
    }
  }

  INFO_SERIAL.print(" ");
  INFO_SERIAL.print(failures);
  INFO_SERIAL.print("/");
  INFO_SERIAL.println(iterations);

  return failures;
}

static void printResultRow(const BaudResult &r) {
  INFO_SERIAL.print("  ");
  INFO_SERIAL.print(r.baud);
  if (r.baud < 100000) INFO_SERIAL.print(' ');
  if (r.baud < 10000)  INFO_SERIAL.print(' ');

  INFO_SERIAL.print("      ");
  INFO_SERIAL.print(r.simpleFailures);
  INFO_SERIAL.print("/");
  INFO_SERIAL.print(SIMPLE_ITERATIONS);

  INFO_SERIAL.print("          ");
  INFO_SERIAL.print(r.burstFailures);
  INFO_SERIAL.print("/");
  INFO_SERIAL.println(BURST_ITERATIONS);
}

static void halt(const char *message) {
  INFO_SERIAL.println(message);
  INFO_SERIAL.println("Halting.");
  while (true) {
    delay(1000);
  }
}

static void enterAtMode() {
  INFO_SERIAL.println("[BOOT] Resetting AMWA-01...");
  wifihalow.AMWA_init();
  delay(1000);
  drainAtSerial(100);

  INFO_SERIAL.println("[BOOT] Detecting boot state...");
  AMWA::BootState state = wifihalow.detect_boot_state(40000);

  if (state == AMWA::BOOT_AT_MODE) {
    INFO_SERIAL.println("[BOOT] AT mode.");
    return;
  }

  if (state == AMWA::BOOT_AUTOUDP) {
    INFO_SERIAL.println("[BOOT] AutoUDP detected. Escaping with AT*...");
    if (!wifihalow.auto_udp_escape(6000)) {
      halt("[BOOT] AutoUDP escape failed.");
    }
    INFO_SERIAL.println("[BOOT] AT mode.");
    return;
  }

  halt("[BOOT] No response from AMWA-01.");
}

void setup() {
  INFO_SERIAL.begin(INFO_SERIAL_BAUD);
  waitForSerialMonitor();

  AT_SERIAL.begin(115200);
  delay(500);

  INFO_SERIAL.println();
  INFO_SERIAL.println("=== AMWA UART Baud Test ===");
  INFO_SERIAL.println("Serial Monitor baud can be any value supported by USB CDC.");
  INFO_SERIAL.println();

  enterAtMode();

  const size_t count = sizeof(TEST_BAUDS) / sizeof(TEST_BAUDS[0]);

  for (size_t i = 0; i < count; i++) {
    unsigned long baud = TEST_BAUDS[i];

    INFO_SERIAL.println();
    INFO_SERIAL.print("=== Test @ ");
    INFO_SERIAL.print(baud);
    INFO_SERIAL.println(" baud ===");

    if (!switchBaud(baud)) {
      halt("[BAUD] Unable to switch baud.");
    }

    results[i].baud = baud;
    results[i].simpleFailures = runStress("AT", "AT", SIMPLE_ITERATIONS);
    results[i].burstFailures = runStress("AT+SAUDP?", "AT+SAUDP?", BURST_ITERATIONS);
  }

  INFO_SERIAL.println();
  INFO_SERIAL.println("[BAUD] Restoring AMWA UART to 115200...");
  if (!switchBaud(115200)) {
    INFO_SERIAL.println("[BAUD] Restore failed. Power-cycle AMWA-01 before normal use.");
  }

  INFO_SERIAL.println();
  INFO_SERIAL.println("=== Summary ===");
  INFO_SERIAL.println("  baud        AT failures   AT+SAUDP? failures");
  INFO_SERIAL.println("  ----------  ------------  ------------------");
  for (size_t i = 0; i < count; i++) {
    printResultRow(results[i]);
  }

  INFO_SERIAL.println();
  INFO_SERIAL.println("Done.");
}

void loop() {
  // This sample runs once in setup().
}

