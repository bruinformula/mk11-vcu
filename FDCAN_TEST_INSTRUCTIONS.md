# FDCAN Communication Test Instructions

## Overview

This document describes how to test FDCAN communication between the **mk11-vcu** (STM32H723ZGT6) and **mk11-bms-mcu** (STM32G474RE) boards.

**Test Configuration:**
- **mk11-vcu (Transmitter)**: Sends CAN message ID `0x11` every 1 second
- **mk11-bms-mcu (Receiver)**: Receives message, echoes back with ID `0x22`
- **Baud Rate**: 500 kbps (Classic CAN)

---

## Hardware Setup

### Required Equipment
1. mk11-vcu board (STM32H723ZGT6)
2. mk11-bms-mcu board (STM32G474RE Nucleo)
3. CAN transceiver modules (e.g., SN65HVD230, MCP2551, or TJA1050) - one per board
4. Two 120 ohm termination resistors
5. Jumper wires
6. Two ST-Link debuggers (or one, testing boards sequentially)
7. Oscilloscope (optional, for signal verification)

### Pin Connections

| Board | Function | Pin |
|-------|----------|-----|
| mk11-vcu | FDCAN1_TX | PD1 |
| mk11-vcu | FDCAN1_RX | PD0 |
| mk11-bms-mcu | FDCAN1_TX | PA12 |
| mk11-bms-mcu | FDCAN1_RX | PA11 |

### CAN Bus Wiring

```
mk11-vcu                                          mk11-bms-mcu
   |                                                   |
   |  PD1 (TX) -----> CAN Transceiver 1               |  PA12 (TX) -----> CAN Transceiver 2
   |  PD0 (RX) <----- CAN Transceiver 1               |  PA11 (RX) <----- CAN Transceiver 2
   |                       |                          |                        |
   |                      CANH ==================== CANH                       |
   |                      CANL ==================== CANL                       |
   |                       |                          |                        |
   |                    [120R]                     [120R]                      |
   |                       |                          |                        |
   |                      GND ==================== GND                         |
```

**Important:**
- Connect CANH to CANH and CANL to CANL between transceivers
- Place 120 ohm termination resistors at each end of the bus
- Ensure common ground between both boards
- Power transceivers appropriately (typically 3.3V or 5V depending on model)

---

## Software Setup

### Step 1: Build Both Projects

1. Open STM32CubeIDE
2. Import both projects:
   - `mk11-vcu`
   - `mk11-bms-mcu`
3. Build each project: **Project > Build Project** (or Ctrl+B)
4. Verify no build errors

### Step 2: Flash and Test (Single Laptop Setup)

This procedure is for when you only have one laptop available for debugging:

#### Phase 1: Flash mk11-vcu (Transmitter)

1. Connect laptop to mk11-vcu via ST-Link
2. Open mk11-vcu project in STM32CubeIDE
3. Click **Run > Debug** (F11) to flash the board
4. Once flashed, **disconnect the debugger/laptop**
5. Power mk11-vcu from an external power supply (ensure proper voltage)
6. The mk11-vcu will now run standalone, transmitting CAN message ID `0x11` every 1 second

#### Phase 2: Monitor mk11-bms-mcu (Receiver)

1. **Disconnect** laptop from mk11-vcu
2. Connect laptop to mk11-bms-mcu via ST-Link
3. Open mk11-bms-mcu project in STM32CubeIDE
4. Click **Run > Debug** (F11)
5. When debugger stops at main(), click **Resume** (F8)
6. Open **Window > Show View > Expressions**
7. Add these watch expressions:
   - `fdcan_rx_count`
   - `fdcan_last_rx_id`
   - `fdcan_last_rx_data`
   - `fdcan_rx_error_count`
   - `RxData`
   - `RxHeader`
8. Monitor the variables to verify CAN reception

---

## Testing Procedure

### What to Monitor on mk11-bms-mcu

With the mk11-vcu running standalone on power supply, you'll verify successful FDCAN communication by monitoring these variables on the mk11-bms-mcu:

#### Primary Success Indicators

| Variable | Type | Expected Behavior | What It Means |
|----------|------|-------------------|---------------|
| `fdcan_rx_count` | uint32_t | Increments every ~1 second | Successfully receiving messages from VCU |
| `fdcan_last_rx_id` | uint32_t | Shows `0x11` | Correct message ID received |
| `fdcan_last_rx_data[8]` | uint8_t[] | Shows `{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}` | Data payload is correct |
| `fdcan_rx_error_count` | uint32_t | Stays at 0 | No errors in reception or echo transmission |

#### Step-by-Step Monitoring

1. **Initial State (Before VCU Transmission)**
   - All counters should be at 0
   - `fdcan_last_rx_id` will be 0
   - `fdcan_last_rx_data` will be all zeros

2. **After VCU Starts Transmitting**
   - Within 1 second, `fdcan_rx_count` should increment to 1
   - `fdcan_last_rx_id` should change to `0x11`
   - `fdcan_last_rx_data` should show the pattern `{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}`
   - Every second thereafter, `fdcan_rx_count` should increment by 1

3. **Continuous Operation**
   - Let it run for at least 30 seconds
   - `fdcan_rx_count` should reach approximately 30 or higher
   - Data pattern should remain consistent
   - `fdcan_rx_error_count` should never increment

---

## Variables to Monitor

### mk11-bms-mcu (Primary Focus)

| Variable | Type | Description | Expected Value |
|----------|------|-------------|----------------|
| `fdcan_rx_count` | uint32_t | Number of received messages | Increments every ~1 second |
| `fdcan_last_rx_id` | uint32_t | Last received message ID | `0x11` |
| `fdcan_last_rx_data[8]` | uint8_t[] | Last received data bytes | `{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}` |
| `fdcan_rx_error_count` | uint32_t | RX/TX error counter | Should be 0 |
| `RxData[8]` | uint8_t[] | Copy of received data | Same as fdcan_last_rx_data |
| `RxHeader.Identifier` | uint32_t | Received message ID | `0x11` |

### mk11-vcu Reference (Not Visible in This Test)

For your reference, here's what's happening on the mk11-vcu side (you won't see these):

| Variable | Type | Description | Expected Value |
|----------|------|-------------|----------------|
| `fdcan1_tx_count` | uint32_t | Number of successful TX messages | Increments every ~1 second |
| `txErrorCnt` | uint8_t | TX error count from ECR register | Should be 0 |
| `ecr1` | uint32_t | Error Counter Register value | Should be 0x00000000 |
| `debug1_cb` | int | RX FIFO0 callback counter | Increments when echo received |
| `RxData1[8]` | uint8_t[] | Received echo data | Should match TxData1 |
| `TxData1[8]` | uint8_t[] | Transmitted data | `{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}` |

---

## Success Criteria

### Test PASSES If (Single Laptop Setup):

Observing only the mk11-bms-mcu debug variables:

✅ **Communication Working:**
1. `fdcan_rx_count` increments steadily (approximately once per second)
2. `fdcan_last_rx_id` shows `0x11`
3. `fdcan_last_rx_data` shows `{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}`
4. `fdcan_rx_error_count` remains at 0

After 30 seconds of operation:
- `fdcan_rx_count` should be around 30 (±2 messages)
- Data pattern remains consistent throughout

### Test FAILS If:

❌ **No Communication:**
- `fdcan_rx_count` stays at 0 after 5+ seconds
- See troubleshooting section below

❌ **Intermittent Communication:**
- `fdcan_rx_count` increments irregularly or stops incrementing
- Check wiring and termination resistors

❌ **Errors Detected:**
- `fdcan_rx_error_count` increments
- Indicates issues with message reception or echo transmission

### What You CAN'T Verify in This Setup:

Since you're not monitoring the mk11-vcu:
- You won't see if the VCU is successfully transmitting (but if BMS receives, VCU is transmitting)
- You won't see if the VCU receives the echo response from BMS
- You won't see the VCU's error counters

**Note:** If you want to verify the full round-trip (echo response), you would need to reconnect the laptop to the mk11-vcu after this test and check if `debug1_cb` is incrementing.

---

## Troubleshooting

### Problem: BMS RX Count Stays at 0 (No Messages Received)

**Single Laptop Setup - What to Check:**

1. **Is mk11-vcu Actually Running?**
   - Verify external power supply is connected and correct voltage
   - Check if VCU board LED indicators show activity
   - Verify you flashed the latest code to VCU before disconnecting
   - Consider briefly reconnecting to VCU to verify `fdcan1_tx_count` is incrementing

2. **CAN Bus Wiring Issues**
   - Verify CANH connected to CANH, CANL to CANL
   - Check for loose wires or poor connections
   - Ensure both transceivers share common ground
   - Verify transceiver power supplies are active

3. **Termination Resistors**
   - Confirm 120Ω resistor at VCU end of bus
   - Confirm 120Ω resistor at BMS end of bus
   - Measure resistance between CANH and CANL: should be ~60Ω with both terminators

4. **CAN Transceiver Issues**
   - Check transceiver power (typically 3.3V or 5V)
   - Verify TX/RX pins connected correctly to MCU
   - Some transceivers have enable/standby pins - verify they're configured

5. **Bit Timing Mismatch**
   - Both boards should be configured for 500 kbps
   - VCU: 96 MHz / (12 × 16) = 500 kbps
   - BMS: 170 MHz / (20 × 17) = 500 kbps

### Problem: fdcan_rx_error_count Increases

**Possible Causes:**
1. `HAL_FDCAN_GetRxMessage()` failing in callback
2. TX FIFO full when BMS tries to echo response back to VCU
3. VCU not acknowledging the echo response

**Solutions (BMS Side):**
- Set breakpoint in `HAL_FDCAN_RxFifo0Callback()` to verify it's being called
- Check if the error occurs during message reception or during echo transmission
- Verify filter configuration is correct

**VCU Side (requires reconnecting laptop):**
- Check if VCU is receiving echo responses (may cause BMS echo failures)
- Verify VCU's RX filter is configured to accept ID `0x22`

### Problem: Intermittent Reception

**Symptoms:**
- `fdcan_rx_count` increments but not consistently every second
- Some messages are lost

**Possible Causes:**
1. Noise on CAN bus (poor shielding, long wires, no twisted pair)
2. Weak termination or impedance mismatch
3. Transceiver issues or marginal power supply

**Solutions:**
- Use twisted pair cable for CANH/CANL
- Keep cable length under 1 meter for initial testing
- Check power supply stability on both boards
- Add decoupling capacitors near transceivers if missing

### Problem: Code Stuck in Error_Handler()

**Possible Causes:**
1. FDCAN initialization failed (wrong clock configuration)
2. Filter configuration failed
3. FDCAN start failed

**Solutions:**
- Set breakpoint in `Error_Handler()` and check call stack
- Verify IOC file settings match code
- Check if FDCAN clock is enabled in RCC configuration
- Verify GPIO alternate functions are correctly set for FDCAN pins

### Debugging Tip: Use Oscilloscope

If you have an oscilloscope available:

1. **Probe CANH and CANL** on the bus between the two boards
2. **Trigger on CANH falling edge**
3. **Expected waveform:**
   - CANH idle: ~2.5V, dominant: ~3.5V
   - CANL idle: ~2.5V, dominant: ~1.5V
   - Differential voltage: ~0V idle, ~2V dominant state
4. **Bit time:** 2 microseconds (500 kbps)
5. **Frame rate:** Should see CAN frames every ~1 second

If you see frames on the scope but BMS isn't receiving:
- Problem is likely in BMS FDCAN configuration or interrupts
- Verify NVIC interrupt priority and enabled status
- Check if FDCAN1_IT0_IRQn is enabled

---

## Bit Timing Reference

### mk11-vcu (STM32H723, 96 MHz FDCAN Clock)

| Parameter | Value |
|-----------|-------|
| Prescaler | 12 |
| Time Seg 1 | 13 |
| Time Seg 2 | 2 |
| Sync Jump Width | 2 |
| Total Time Quanta | 16 |
| **Bit Rate** | 96 MHz / (12 * 16) = **500 kbps** |

### mk11-bms-mcu (STM32G474, 170 MHz FDCAN Clock)

| Parameter | Value |
|-----------|-------|
| Prescaler | 20 |
| Time Seg 1 | 13 |
| Time Seg 2 | 3 |
| Sync Jump Width | 3 |
| Total Time Quanta | 17 |
| **Bit Rate** | 170 MHz / (20 * 17) = **500 kbps** |

---

## Oscilloscope Verification (Optional)

If you have access to an oscilloscope:

1. Probe CANH and CANL lines
2. Set trigger on falling edge of CANH
3. Expected waveform:
   - CANH: Idle at ~2.5V, dominant at ~3.5V
   - CANL: Idle at ~2.5V, dominant at ~1.5V
   - Differential voltage: ~0V idle, ~2V dominant
4. Bit time should be 2 microseconds (500 kbps)
5. You should see CAN frames every 1 second

---

## Test Data Format

### Transmitted Frame (mk11-vcu -> mk11-bms-mcu)

| Field | Value |
|-------|-------|
| ID | 0x11 (Standard) |
| DLC | 8 |
| Data | 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88 |

### Echo Response Frame (mk11-bms-mcu -> mk11-vcu)

| Field | Value |
|-------|-------|
| ID | 0x22 (Standard) |
| DLC | 8 |
| Data | 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88 |

---

## Reverting to Original Code

After testing, to restore original functionality:

### mk11-vcu
- Change `FDCAN1.Mode` back to `FDCAN_MODE_EXTERNAL_LOOPBACK` in IOC if needed

### mk11-bms-mcu
1. Uncomment original BMS initialization code in main.c USER CODE BEGIN 2
2. Uncomment SPI test code in while loop
3. Restore original FDCAN bit timing if needed for your application
