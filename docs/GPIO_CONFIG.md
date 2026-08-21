# GPIO Configuration — STM32CubeIDE / CubeMX

Tài liệu này mô tả cấu hình từng chân GPIO đầu vào trong project Washing Machine Simulator.

---

## Tổng quan — Bản đồ GPIO đầu vào

| Pin | Tên | Chức năng | Cấu hình trong .ioc? | Interface |
|-----|-----|-----------|----------------------|-----------|
| PA0 | B1 User Button | Navigate menu (short press) | ✅ Có sẵn | EXTI0, cả 2 cạnh |
| PE1 | Start Button | Confirm / Start cycle | ❌ Khởi tạo thủ công trong BSP | EXTI1, cạnh xuống |
| PD5 | Door Sensor Btn | Toggle cửa đóng/mở | ❌ Khởi tạo thủ công trong BSP | GPIO Input, polling |
| PD6 | Water Sensor Btn | Toggle nước đầy/trống | ❌ Khởi tạo thủ công trong BSP | GPIO Input, polling |

---

## 1. PA0 — B1 User Button (Navigate Menu)

> **Đã có trong file `.ioc` — CubeMX tự tạo.**

### Cài đặt trong STM32CubeIDE

Vào: **Pinout & Configuration → PA0**

| Tham số | Giá trị |
|---------|---------|
| GPIO mode | External Interrupt Mode with **Rising edge** trigger detection |
| GPIO Pull-up/Pull-down | No pull-up and no pull-down |
| User Label | `B1 [Blue PushButton]` |

> ⚠️ CubeMX tạo **Rising edge only**. `BSP_Button_Init()` sẽ tự đổi thành **Rising + Falling** trong code để đo được thời gian nhấn (debounce). Không cần sửa trong .ioc.

### NVIC Settings

Vào: **System Core → NVIC**

| Interrupt | Enabled | Preemption Priority | Sub Priority |
|-----------|---------|---------------------|--------------|
| EXTI line0 interrupt | ✅ | 0 | 0 |

> ⚠️ Priority `0` trong CubeMX — **BSP_Button_Init() ghi đè thành `5`** trong code để tuân thủ `configMAX_SYSCALL_INTERRUPT_PRIORITY` của FreeRTOS. Không sửa trong .ioc.

### Kết nối phần cứng

```
3.3V ─── [External Pull-down 10kΩ] ─── PA0 ─── [B1 Button] ─── 3.3V
```
> Board STM32F411E-DISCO có pull-down ngoại vi sẵn — không cần thêm linh kiện.

---

## 2. PE1 — Start / Confirm Button

> **KHÔNG có trong `.ioc`** — Pin được khởi tạo hoàn toàn trong `BSP_StartButton_Init()`.

### Lý do không dùng CubeMX

- Tránh CubeMX ghi đè khi tái tạo code từ `.ioc`.
- BSP module tự sở hữu và quản lý pin của nó.

### Cài đặt tương đương (nếu muốn thêm vào .ioc)

Vào: **Pinout & Configuration → PE1**

| Tham số | Giá trị |
|---------|---------|
| GPIO mode | External Interrupt Mode with **Falling edge** trigger detection |
| GPIO Pull-up/Pull-down | **Pull-up** |
| User Label | `START_BTN` |

Vào: **System Core → NVIC**

| Interrupt | Enabled | Preemption Priority | Sub Priority |
|-----------|---------|---------------------|--------------|
| EXTI line1 interrupt | ✅ | **5** | 0 |

> ⚠️ Nếu thêm vào `.ioc` và tái tạo code: xóa phần `__HAL_RCC_GPIOE_CLK_ENABLE()` và `HAL_GPIO_Init()` trong `BSP_StartButton_Init()` để tránh cấu hình 2 lần. Giữ nguyên phần `HAL_NVIC_SetPriority` và `HAL_NVIC_EnableIRQ`.

### Kết nối phần cứng

```
MCU PE1 ─── [Nút nhấn] ─── GND
            Pull-up nội 40kΩ bật sẵn trong BSP
            → không cần điện trở ngoài
```

| Trạng thái nút | Mức điện áp PE1 | EXTI trigger |
|---------------|-----------------|--------------|
| Không nhấn | HIGH (3.3V) | Không có |
| Nhấn (nối GND) | LOW (0V) | Falling edge ✅ → `BTN_EVENT_LONG_PRESS` |

---

## 3. PD5 — Door Sensor Button

> **KHÔNG có trong `.ioc`** — Pin được khởi tạo trong `BSP_Sensor_Init()`.

### Lý do không dùng CubeMX

- PD12–PD15 đã có trong `.ioc` (GPIO Output). Thêm PD5/PD6 vào `.ioc` sẽ khiến CubeMX reset cấu hình toàn bộ GPIOD về mặc định khi tái tạo.
- Dùng **GPIO polling** (không cần EXTI) nên không cần NVIC — cấu hình đơn giản hơn.

### Cài đặt tương đương (nếu muốn thêm vào .ioc)

Vào: **Pinout & Configuration → PD5**

| Tham số | Giá trị |
|---------|---------|
| GPIO mode | **Input mode** |
| GPIO Pull-up/Pull-down | **Pull-up** |
| User Label | `DOOR_BTN` |

> Không cần NVIC — SensorTask đọc pin qua `HAL_GPIO_ReadPin()` mỗi 100ms.

### Kết nối phần cứng

```
MCU PD5 ─── [Nút nhấn] ─── GND
            Pull-up nội 40kΩ bật sẵn trong BSP
            → không cần điện trở ngoài
```

| Trạng thái nút | Mức điện áp PD5 | Hành vi |
|---------------|-----------------|---------|
| Không nhấn | HIGH | Không thay đổi trạng thái |
| Nhấn (falling edge phát hiện bởi poll) | LOW | Toggle `s_doorClosed` (0↔1) |

**Mặc định khi khởi động:** `s_doorClosed = 1` (cửa ĐÓNG)

---

## 4. PD6 — Water Sensor Button

> **KHÔNG có trong `.ioc`** — Pin được khởi tạo trong `BSP_Sensor_Init()` (cùng hàm với PD5).

### Cài đặt tương đương (nếu muốn thêm vào .ioc)

Vào: **Pinout & Configuration → PD6**

| Tham số | Giá trị |
|---------|---------|
| GPIO mode | **Input mode** |
| GPIO Pull-up/Pull-down | **Pull-up** |
| User Label | `WATER_BTN` |

> Không cần NVIC — polling 100ms trong SensorTask.

### Kết nối phần cứng

```
MCU PD6 ─── [Nút nhấn] ─── GND
            Pull-up nội 40kΩ bật sẵn trong BSP
            → không cần điện trở ngoài
```

| Trạng thái nút | Mức điện áp PD6 | Hành vi |
|---------------|-----------------|---------|
| Không nhấn | HIGH | Không thay đổi trạng thái |
| Nhấn (falling edge phát hiện bởi poll) | LOW | Toggle `s_waterFull` (0↔1) |

**Mặc định khi khởi động:** `s_waterFull = 0` (nước TRỐNG)

---

## Sơ đồ kết nối tổng hợp

```
STM32F411E-DISCO
┌─────────────────────────────────────────────────┐
│  PA0 ────────────────── [B1 on-board button]    │  (pull-down có sẵn trên board)
│  PE1 ──[40kΩ pull-up]── [Nút Start]  ── GND    │
│  PD5 ──[40kΩ pull-up]── [Nút Door]   ── GND    │
│  PD6 ──[40kΩ pull-up]── [Nút Water]  ── GND    │
│                                                  │
│  PD12 ── [Logic Analyzer CH0] (Motor)           │
│  PD13 ── [Logic Analyzer CH1] (Inlet Valve)     │
│  PD14 ── [Logic Analyzer CH2] (Drain Valve)     │
│  PD15 ── [Logic Analyzer CH3] (Status LED)      │
└─────────────────────────────────────────────────┘
```

---

## Lưu ý khi tái tạo code từ CubeMX (Generate Code)

| Pin | Ảnh hưởng khi Generate Code |
|-----|-----|
| PA0 | ✅ An toàn — code trong `USER CODE` sections được giữ nguyên |
| PE1 | ✅ An toàn — không có trong `.ioc`, CubeMX không động đến |
| PD5 | ✅ An toàn — không có trong `.ioc`, CubeMX không động đến |
| PD6 | ✅ An toàn — không có trong `.ioc`, CubeMX không động đến |

> **Không đặt code BSP ngoài vùng `USER CODE BEGIN / USER CODE END`** — CubeMX sẽ xóa khi Generate Code.
