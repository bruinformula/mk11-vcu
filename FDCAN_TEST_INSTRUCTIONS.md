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

### Step 2: Flash Both Boards

1. Connect ST-Link to mk11-vcu
2. Flash mk11-vcu: **Run > Debug** (or F11)
3. Stop debugging (but leave board powered)
4. Connect ST-Link to mk11-bms-mcu
5. Flash mk11-bms-mcu: **Run > Debug** (or F11)

---

## Testing Procedure

### Method 1: Debug Both Boards Simultaneously (Recommended)

If you have two ST-Link debuggers or a debug setup that supports multiple targets:

#### A. Start mk11-bms-mcu Debug Session

1. Open mk11-bms-mcu project in STM32CubeIDE
2. Click **Run > Debug** (F11)
3. When debugger stops at main(), click **Resume** (F8)
4. Open **Window > Show View > Expressions**
5. Add these watch expressions:
   - `fdcan_rx_count`
   - `fdcan_last_rx_id`
   - `fdcan_last_rx_data`
   - `fdcan_rx_error_count`
   - `RxData`
   - `RxHeader`

#### B. Start mk11-vcu Debug Session

1. Open mk11-vcu project in a second STM32CubeIDE instance (or same instance, different debug config)
2. Click **Run > Debug** (F11)
3. When debugger stops at main(), click **Resume** (F8)
4. Open **Window > Show View > Expressions**
5. Add these watch expressions:
   - `fdcan1_tx_count`
   - `txErrorCnt`
   - `ecr1`
   - `debug1_cb` (RX callback counter)
   - `RxData1` (for echo response)

### Method 2: Debug One Board at a Time

#### A. Flash and Run mk11-bms-mcu (Free Running)

1. Flash mk11-bms-mcu
2. Start debug session, then click **Resume**
3. **Disconnect** debugger but leave board powered
4. The board will continue running and waiting for CAN messages

#### B. Debug mk11-vcu

1. Connect debugger to mk11-vcu
2. Start debug session
3. Add watch expressions (see above)
4. Click **Resume** and observe variables

---

## Variables to Monitor

### mk11-vcu (Transmitter)

| Variable | Type | Description | Expected Value |
|----------|------|-------------|----------------|
| `fdcan1_tx_count` | uint32_t | Number of successful TX messages | Increments every ~1 second |
| `txErrorCnt` | uint8_t | TX error count from ECR register | Should be 0 |
| `ecr1` | uint32_t | Error Counter Register value | Should be 0x00000000 |
| `debug1_cb` | int | RX FIFO0 callback counter | Increments when echo received |
| `RxData1[8]` | uint8_t[] | Received echo data | Should match TxData1 |
| `TxData1[8]` | uint8_t[] | Transmitted data | `{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}` |

### mk11-bms-mcu (Receiver)

| Variable | Type | Description | Expected Value |
|----------|------|-------------|----------------|
| `fdcan_rx_count` | uint32_t | Number of received messages | Increments every ~1 second |
| `fdcan_last_rx_id` | uint32_t | Last received message ID | `0x11` |
| `fdcan_last_rx_data[8]` | uint8_t[] | Last received data bytes | `{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}` |
| `fdcan_rx_error_count` | uint32_t | RX/TX error counter | Should be 0 |
| `RxData[8]` | uint8_t[] | Copy of received data | Same as fdcan_last_rx_data |
| `RxHeader.Identifier` | uint32_t | Received message ID | `0x11` |

---

## Success Criteria

### Transmission Successful If:

1. **mk11-vcu side:**
   - `fdcan1_tx_count` increments every second
   - `txErrorCnt` remains 0
   - `debug1_cb` increments (echo received from BMS)
   - `RxData1` contains the echoed data

2. **mk11-bms-mcu side:**
   - `fdcan_rx_count` increments every second
   - `fdcan_last_rx_id` equals `0x11`
   - `fdcan_last_rx_data` contains `{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}`
   - `fdcan_rx_error_count` remains 0

### Full Round-Trip Successful If:

Both boards show incrementing counters and the VCU receives the echo response (ID `0x22`) from the BMS-MCU.

---

## Troubleshooting

### Problem: TX Count Increments but RX Count Stays at 0

**Possible Causes:**
1. CAN bus not properly connected (check CANH/CANL wiring)
2. Missing or incorrect termination resistors
3. Transceiver not powered or configured incorrectly
4. Bit timing mismatch between boards

**Solutions:**
- Verify physical connections
- Check transceiver power supply
- Use oscilloscope to verify CAN signals on CANH/CANL
- Verify both boards have matching bit timing (500 kbps)

### Problem: txErrorCnt Increases

**Possible Causes:**
1. No ACK received (no other node on bus)
2. Bus error (wiring issue)
3. Bit timing mismatch

**Solutions:**
- Ensure receiving board is powered and running
- Check for loose connections
- Verify termination resistors are present

### Problem: fdcan_rx_error_count Increases

**Possible Causes:**
1. HAL_FDCAN_GetRxMessage() failing
2. TX FIFO full when trying to echo

**Solutions:**
- Check if FDCAN is properly started
- Verify filter configuration
- Check for interrupt priority conflicts

### Problem: Code Stuck in Error_Handler()

**Possible Causes:**
1. FDCAN initialization failed
2. Filter configuration failed
3. FDCAN start failed

**Solutions:**
- Set breakpoint in Error_Handler()
- Check which function call failed
- Verify IOC configuration matches code
- Regenerate code from CubeMX if needed

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
