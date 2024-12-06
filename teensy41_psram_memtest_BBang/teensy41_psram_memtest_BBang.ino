extern "C" uint8_t external_psram_size;

bool memory_ok = false;
uint32_t *memory_begin, *memory_end;

bool time_fixed_patterns();
bool check_fixed_pattern(uint32_t pattern);
bool check_lfsr_pattern(uint32_t seed);

static uint32_t lfsrPatt[] = { 2976674124ul, 1438200953ul, 3413783263ul, 1900517911ul, 1227909400ul, 276562754ul, 146878114ul, 615545407ul, 110497896ul, 74539250ul, 4197336575ul, 2280382233ul, 542894183ul, 3978544245ul, 2315909796ul, 3736286001ul, 2876690683ul, 215559886ul, 539179291ul, 537678650ul, 4001405270ul, 2169216599ul, 4036891097ul, 1535452389ul, 2959727213ul, 4219363395ul, 1036929753ul, 2125248865ul, 3177905864ul, 2399307098ul, 3847634607ul, 27467969ul, 520563506ul, 381313790ul, 4174769276ul, 3932189449ul, 4079717394ul, 868357076ul, 2474062993ul, 1502682190ul, 2471230478ul, 85016565ul, 1427530695ul, 1100533073ul };
const uint32_t lfsrCnt = sizeof(lfsrPatt) / sizeof(lfsrPatt[0]);

static uint32_t fixedPatt[] = { 0x5A698421, 0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF, 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF, 0x00000000 };
const uint32_t fixedCnt = sizeof(fixedPatt) / sizeof(fixedPatt[0]);


bool time_fixed_patterns() {
  elapsedMillis msec = 0;
  for (uint ii = 0; ii < fixedCnt; ii++) {
    digitalToggleFast(13);
    if (!check_fixed_pattern(fixedPatt[ii])) return false;
  }
  Serial.printf(" %d Fixed tests ran for %.2f seconds without error\n", fixedCnt, (float)msec / 1000.0f);
  return true;
}


void setup() {
  // Reset clock to 132 Mhz from 88 MHz startup : https://forum.pjrc.com/index.php?threads/teensy-4-1-psram-random-access-latency.62456/post-249444
  CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_OFF);
  CCM_CBCMR = (CCM_CBCMR & ~(CCM_CBCMR_FLEXSPI2_PODF_MASK | CCM_CBCMR_FLEXSPI2_CLK_SEL_MASK))
              | CCM_CBCMR_FLEXSPI2_PODF(4) | CCM_CBCMR_FLEXSPI2_CLK_SEL(2);  // 528/5 = 132 MHz
  CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_ON);

  while (!Serial)
    ;  // wait
  pinMode(13, OUTPUT);
  uint8_t size = external_psram_size;
  Serial.printf("EXTMEM Memory Test, %d Mbyte\n", size);
  if (size == 0) return;
  const float clocks[4] = { 396.0f, 720.0f, 664.62f, 528.0f };
  const float frequency = clocks[(CCM_CBCMR >> 8) & 3] / (float)(((CCM_CBCMR >> 29) & 7) + 1);
  Serial.printf(" CCM_CBCMR=%08X (%.1f MHz)\n", CCM_CBCMR, frequency);
  memory_begin = (uint32_t *)(0x70000000);
  memory_end = (uint32_t *)(0x70000000 + size * 1048576);
  elapsedMillis msec = 0;

  if (!check_fixed_pattern(fixedPatt[0])) return;

  for (uint ii = 0; ii < lfsrCnt; ii++) {
    digitalToggle(13);
//xx BUGBUG Skip this for faster DEV test runs  if (!check_lfsr_pattern(lfsrPatt[ii])) return;
  }

  for (uint ii = 1; ii < fixedCnt; ii++) {
    digitalToggle(13);
    if (!check_fixed_pattern(fixedPatt[ii])) return;
  }

  Serial.printf(" test ran for %.2f seconds\n", (float)msec / 1000.0f);
  Serial.printf("All memory tests passed :-) #Fixed: %d + # Patt: %d\n", fixedCnt, lfsrCnt);
  memory_ok = true;
  if (!time_fixed_patterns()) {
    Serial.printf("Failure in Fixed tests.\n");
    memory_ok = false;
  }
  setup2(); // run the BitBang testt version of time_fixed_patterns()  BUGBUG
  if (!time_fixed_patterns()) {
    Serial.printf("Failure in Fixed tests.\n");
    memory_ok = false;
  }
}  // setup()

bool fail_message(volatile uint32_t *location, uint32_t actual, uint32_t expected) {
  Serial.printf(" Error at %08X, read %08X but expected %08X\n",
                (uint32_t)location, actual, expected);
  return false;
}

// fill the entire RAM with a fixed pattern, then check it
bool check_fixed_pattern(uint32_t pattern) {
  volatile uint32_t *p;
  Serial.printf("testing with fixed pattern %08X\n", pattern);
  for (p = memory_begin; p < memory_end; p++) {
    *p = pattern;
  }
  arm_dcache_flush_delete((void *)memory_begin,
                          (uint32_t)memory_end - (uint32_t)memory_begin);
  for (p = memory_begin; p < memory_end; p++) {
    uint32_t actual = *p;
    if (actual != pattern) return fail_message(p, actual, pattern);
  }
  return true;
}

// fill the entire RAM with a pseudo-random sequence, then check it
bool check_lfsr_pattern(uint32_t seed) {
  volatile uint32_t *p;
  uint32_t reg;

  Serial.printf("testing with pseudo-random sequence, seed=%u\n", seed);
  reg = seed;
  for (p = memory_begin; p < memory_end; p++) {
    *p = reg;
    for (int i = 0; i < 3; i++) {
      // https://en.wikipedia.org/wiki/Xorshift
      reg ^= reg << 13;
      reg ^= reg >> 17;
      reg ^= reg << 5;
    }
  }
  arm_dcache_flush_delete((void *)memory_begin,
                          (uint32_t)memory_end - (uint32_t)memory_begin);
  reg = seed;
  for (p = memory_begin; p < memory_end; p++) {
    uint32_t actual = *p;
    if (actual != reg) return fail_message(p, actual, reg);
    //Serial.printf(" reg=%08X\n", reg);
    for (int i = 0; i < 3; i++) {
      reg ^= reg << 13;
      reg ^= reg >> 17;
      reg ^= reg << 5;
    }
  }
  return true;
}

void loop() {
  digitalWrite(13, HIGH);
  delay(100);
  if (!memory_ok) digitalWrite(13, LOW);  // rapid blink if any test fails
  delay(100);
}

#define PIN_PSRAM_D3 54
#define PIN_PSRAM_D2 50
#define PIN_PSRAM_D1 49
#define PIN_PSRAM_D0 52
#define PIN_PSRAM_CLK 53
#define PIN_PSRAM_CS_n 48
#define PSRAM_RESET_VALUE 0x01000000
#define PSRAM_CLK_HIGH 0x02000000
static void PSRAM_Write_Clk_Cycle(uint8_t d) {
  GPIO9_DR = (d & 0xF0) << 22;
  GPIO9_DR_SET = PSRAM_CLK_HIGH;
  GPIO9_DR = (d & 0x0F) << 26;
  GPIO9_DR_SET = PSRAM_CLK_HIGH;
}
static uint8_t PSRAM_Read_Clk_Cycle() {
  GPIO9_DR_CLEAR = PSRAM_CLK_HIGH;
  GPIO9_DR_SET = PSRAM_CLK_HIGH;
  uint8_t d = (GPIO9_DR >> 22) & 0xF0;
  GPIO9_DR_CLEAR = PSRAM_CLK_HIGH;
  GPIO9_DR_SET = PSRAM_CLK_HIGH;
  d |= (GPIO9_DR >> 26) & 0x0F;
  return d;
}
static void PSRAM_Configure() {
  delayMicroseconds(200);
  pinMode(PIN_PSRAM_CLK, OUTPUT);
  pinMode(PIN_PSRAM_CS_n, OUTPUT);
  pinMode(PIN_PSRAM_D3, INPUT);
  pinMode(PIN_PSRAM_D2, INPUT);
  pinMode(PIN_PSRAM_D1, INPUT);
  pinMode(PIN_PSRAM_D0, OUTPUT);
  GPIO9_DR = PSRAM_RESET_VALUE;
  delayMicroseconds(1);

  // enter QPI mode - send SPI cmd 0x35
  PSRAM_Write_Clk_Cycle(0x00);
  PSRAM_Write_Clk_Cycle(0x11);
  PSRAM_Write_Clk_Cycle(0x01);
  PSRAM_Write_Clk_Cycle(0x01);
  GPIO9_DR = PSRAM_RESET_VALUE;
  GPIO9_GDIR = 0x3F000000;
}
static uint8_t PSRAM_Read8(const uint8_t *c) {
  // Send Command: Quad Read (slow) 0x0B
  PSRAM_Write_Clk_Cycle(0x0B);
  // Send 24-bit address
  uint32_t offset = (uint32_t)c;
  PSRAM_Write_Clk_Cycle(offset >> 16);
  PSRAM_Write_Clk_Cycle(offset >> 8);
  PSRAM_Write_Clk_Cycle(offset);
  // Four clocks of hi-Z
  GPIO9_GDIR = 0x03000000;
  PSRAM_Read_Clk_Cycle();
  PSRAM_Read_Clk_Cycle();
  uint8_t d = PSRAM_Read_Clk_Cycle();
  // prepare to send next command
  GPIO9_DR = PSRAM_RESET_VALUE;
  GPIO9_GDIR = 0x3F000000;
  return d;
}
static EXTMEM uint8_t extmem_data[0x10000];
//static uint32_t cycles_to_ns(uint32_t cycles) {  return (uint32_t)(1000000000llu * cycles / F_CPU_ACTUAL); }
uint32_t rand_test(const uint8_t *src, int num_iters) {
  uint8_t result[10];
  uint32_t cycles = 0;
  uint32_t *idxes = (uint32_t *)alloca(num_iters * sizeof(uint32_t));
  // fill in random indices to access
  for (int i = 0; i < num_iters; i++) {
    idxes[i] = random(sizeof(extmem_data));
  }
  for (int i = 0; i < num_iters; i++) {
    cli()
      cycles = ARM_DWT_CYCCNT;
    result[i % 10] = PSRAM_Read8(src + idxes[i]);
    uint32_t res = ARM_DWT_CYCCNT - cycles;
    sei();
    Serial.printf("%d(%08X):\t%d cycles", i, idxes[i], res);
    if (result[i % 10] != ((uint8_t)idxes[i] ^ 0x55)) {
      digitalWriteFast(LED_BUILTIN, HIGH);
      Serial.print(" !");
    }
    Serial.println();
  }
  Serial.println("Done!");
  return result[5];
}
uint32_t seq_test(const uint8_t *src, int num_iters) {
  uint8_t result[10];
  uint32_t cycles = 0;
  for (int i = 0; i < num_iters; i++) {
    cli()
      cycles = ARM_DWT_CYCCNT;
    result[i % 10] = PSRAM_Read8(src + i);
    uint32_t res = ARM_DWT_CYCCNT - cycles;
    sei();
    Serial.printf("%d: %d cycles", i, res);
    if (result[i % 10] != ((uint8_t)i ^ 0x55)) {
      digitalWriteFast(LED_BUILTIN, HIGH);
      Serial.print(" !");
    }
    Serial.println();
  }
  Serial.println("Done!");
  return result[5];
}
void setup2() {
  size_t ii;
  for (ii = 0; ii < sizeof(extmem_data); ii++) {
    extmem_data[ii] = ii ^ 0x55;
  }
  arm_dcache_flush_delete((void *)extmem_data, ii);
  PSRAM_Configure();
  digitalWriteFast(LED_BUILTIN, LOW);

  Serial.println("Seq Test");
  seq_test(extmem_data, 0x10);
  Serial.println("Rand Test");
  rand_test(extmem_data, 0x10);
}
