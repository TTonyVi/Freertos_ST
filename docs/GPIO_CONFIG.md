# GPIO Configuration — STM32CubeIDE / CubeMX

Tài liệu này mô tả cấu hình từng chân GPIO đầu vào trong project Washing Machine Simulator.

---

## Tổng quan — Bản đồ GPIO đầu vào

| Pin | Tên | Chức năng | Cấu hình trong .ioc? | Interface |
|-----|-----|-----------|----------------------|-----------|
| PA0 | B1 User Button | Navigate menu (short press) | ✅ Có sẵn — CubeMX generated | EXTI0, cả 2 cạnh |
| PD2 | Start Button | Confirm / Start cycle | ✅ Có sẵn — CubeMX generated | EXTI2, cạnh xuống |
| PD5 | Door Sensor Btn | Toggle cửa đóng/mở | ❌ Khởi tạo thủ công trong BSP | GPIO Input, polling |
| PD6 | Water Sensor Btn | Toggle nước đầy/trống | ❌ Khởi tạo thủ công trong BSP | GPIO Input, polling |

> **Cập nhật:** PA0 và nút Start/Confirm trước đây được BSP tự khởi tạo/ghi đè
> bằng tay (`BSP_Button_Init()`, `BSP_StartButton_Init()`). Sau đợt refactor
> gần nhất, cả hai đã chuyển hoàn toàn sang CubeMX-generated (`MX_GPIO_Init()`
> trong `main.c`, khai báo trong `Freertos.ioc`) — BSP giờ chỉ còn giữ vai trò
> xử lý logic IRQ (`BSP_Button_IRQHandler`, `BSP_StartButton_IRQHandler`) và
> đăng ký callback (`BSP_Button_RegisterCallback`), không còn hàm `_Init()`
> nào cho 2 chân này.
>
> Quá trình migrate này để lại 2 lỗi (thiếu gọi IRQ handler, PA0 thiếu cạnh
> falling) — xem [HARDWARE_BUGS.md § Bug #11](HARDWARE_BUGS.md#bug-11--pe1-startconfirm-chết-hoàn-toàn-quên-gọi-irq-handler)
> và [§ Bug #12](HARDWARE_BUGS.md#bug-12--pa0-menu-navigate-chết-thiếu-cạnh-falling-trong-cubemx).
>
> **Chân Start/Confirm đã đổi 2 lần** kể từ đó:
> `PE1` (xung đột MEMS_INT2 của cảm biến gia tốc tích hợp — [Bug #14](HARDWARE_BUGS.md#bug-14--pe1-xung-đột-với-mems_int2-của-cảm-biến-gia-tốc-tích-hợp))
> → `PD4` (cũng đo được LOW cố định, **nguyên nhân chưa xác nhận** —
> [Bug #16](HARDWARE_BUGS.md#bug-16--pd4-cũng-đọc-low-cố-định-nguyên-nhân-chưa-xác-nhận))
> → **`PD2` (hiện tại)**. Xem chi tiết ở mục 2 bên dưới.

---

## 1. PA0 — B1 User Button (Navigate Menu)

> **Có trong file `.ioc` — CubeMX tự tạo toàn bộ, kể cả cạnh trigger và NVIC priority.**

### Cài đặt trong STM32CubeIDE

Vào: **Pinout & Configuration → PA0**

| Tham số | Giá trị |
|---------|---------|
| GPIO mode | External Interrupt Mode with **Rising + Falling edge** trigger detection |
| GPIO Pull-up/Pull-down | No pull-up and no pull-down |
| User Label | `B1 [Blue PushButton]` |

Trong `.ioc`, ứng với 2 dòng:

```
PA0-WKUP.GPIOParameters=GPIO_ModeDefaultEXTI
PA0-WKUP.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING
```

> ⚠️ **Lịch sử:** chân "WKUP" mặc định CubeMX chỉ tạo **Rising edge only**
> (vai trò gốc là wake-from-standby, chỉ cần 1 cạnh). Bản refactor gần nhất
> quên thêm override này vào `.ioc`, khiến `BSP_Button_IRQHandler()` — vốn
> cần cả 2 cạnh để đo duration debounce — không bao giờ nhận được cạnh
> falling → nút Navigate chết hoàn toàn. Đã fix bằng cách thêm
> `GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING` như trên. Chi tiết:
> [HARDWARE_BUGS.md § Bug #12](HARDWARE_BUGS.md#bug-12--pa0-menu-navigate-chết-thiếu-cạnh-falling-trong-cubemx).
>
> ⚠️ **Tái phát (Bug #15):** dòng override này từng bị mất lần thứ hai — khi
> đổi chân Start/Confirm từ PE1 sang PD4 trong STM32CubeIDE GUI, CubeMX
> generate lại toàn bộ `MX_GPIO_Init()` **và lặng lẽ reset luôn PA0 về mặc
> định** (`GPIO_MODE_IT_RISING`, priority `0`), dù người dùng không hề đụng
> vào PA0. Nguyên nhân: `.ioc` trên đĩa tại thời điểm đó chưa có 2 dòng
> override ở trên (bị mất từ trước, không ai để ý vì firmware vẫn build).
> Bài học: **mọi lần Generate Code đều phải re-check toàn bộ GPIO liên quan
> đến FreeRTOS ISR, không chỉ chân vừa sửa.** Xem
> [HARDWARE_BUGS.md § Bug #15](HARDWARE_BUGS.md#bug-15--generate-code-cho-pd4-làm-hồi-sinh-bug-12-13-trên-pa0).

### NVIC Settings

Vào: **System Core → NVIC**

| Interrupt | Enabled | Preemption Priority | Sub Priority |
|-----------|---------|---------------------|--------------|
| EXTI line0 interrupt | ✅ | **5** | 0 |

Trong `.ioc`:

```
NVIC.EXTI0_IRQn=true:5:0:false:false:true:true:true:true
```

> Priority **5** (không phải 0 — mặc định CubeMX) vì `Button_EventCallback()`
> gọi `xQueueSendFromISR()`, phải tuân thủ
> `configMAX_SYSCALL_INTERRUPT_PRIORITY` của FreeRTOS. Giá trị này nằm ngay
> trong `.ioc` nên **sống sót qua Generate Code** — không cần override thủ
> công trong `USER CODE` nữa (khác với thiết kế cũ). Nếu bạn từng thấy `.ioc`
> ghi priority `0`: đó là bug config-drift đã fix, xem
> [HARDWARE_BUGS.md § Bug #13](HARDWARE_BUGS.md#bug-13--ioc-và-mainc-lệch-nhau-về-nvic-priority-bom-nổ-chậm-khi-regenerate)
> và [§ Bug #15](HARDWARE_BUGS.md#bug-15--generate-code-cho-pd4-làm-hồi-sinh-bug-12-13-trên-pa0).

### Kết nối phần cứng

```
3.3V ─── [External Pull-down 10kΩ] ─── PA0 ─── [B1 Button] ─── 3.3V
```

> Board STM32F411E-DISCO có pull-down ngoại vi sẵn — không cần thêm linh kiện.

---

## 2. PD2 — Start / Confirm Button

> **Có trong file `.ioc` — CubeMX tự tạo**, giống PA0. Đây là **chân thứ 3**
> được thử cho nút này:
>
> 1. **PE1** — xung đột phần cứng với `MEMS_INT2` của cảm biến gia tốc
>    LIS3DSH tích hợp trên board, xác nhận qua schematic + đo logic analyzer
>    (luôn LOW, đổi pull-up/pull-down không đổi). Xem
>    [Bug #14](HARDWARE_BUGS.md#bug-14--pe1-xung-đột-với-mems_int2-của-cảm-biến-gia-tốc-tích-hợp).
> 2. **PD4** — chọn vì tin là output điều khiển reset của audio DAC CS43L22
>    (dựa theo board STM32F4-Discovery đời cũ, chưa xác nhận trực tiếp với
>    schematic của đúng board 411E). Đo thực tế **cũng luôn LOW**. Nguyên
>    nhân chưa được xác nhận — người dùng chọn bỏ qua bước chẩn đoán cô lập
>    (đo PD4 khi tháo rời dây/nút, kiểm tra continuity của nút, kiểm tra
>    tham chiếu GND của thiết bị đo) để đổi chân ngay. Xem
>    [Bug #16](HARDWARE_BUGS.md#bug-16--pd4-cũng-đọc-low-cố-định-nguyên-nhân-chưa-xác-nhận).
> 3. **PD2 (hiện tại)** — cũng **chưa được xác nhận bằng schematic gốc**
>    (việc tra cứu PDF schematic chính thức của board bị timeout nhiều lần).
>    Nếu PD2 vẫn gặp hiện tượng tương tự, bước tiếp theo **nên là** chạy phép
>    thử cô lập (xem Bug #16) trước khi đổi sang chân thứ 4, thay vì tiếp tục
>    đoán.

### Cài đặt trong STM32CubeIDE

Vào: **Pinout & Configuration → PD2**

| Tham số | Giá trị |
|---------|---------|
| GPIO mode | External Interrupt Mode with **Falling edge** trigger detection |
| GPIO Pull-up/Pull-down | **Pull-up** |
| User Label | `START_BTN` |

Trong `.ioc`, ứng với:

```
PD2.GPIOParameters=GPIO_PuPd,GPIO_ModeDefaultEXTI
PD2.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_FALLING
PD2.GPIO_PuPd=GPIO_PULLUP
```

### NVIC Settings

Vào: **System Core → NVIC**

| Interrupt | Enabled | Preemption Priority | Sub Priority |
|-----------|---------|---------------------|--------------|
| EXTI line2 interrupt | ✅ | **5** | 0 |

Trong `.ioc`:

```
NVIC.EXTI2_IRQn=true:5:0:false:false:true:true:true:true
```

> ⚠️ **Lịch sử:** khi migrate sang CubeMX-generated (lúc còn ở PE1),
> `EXTI1_IRQHandler()` trong `stm32f4xx_it.c` bị bỏ trống — quên gọi
> `BSP_StartButton_IRQHandler()` — nên nút Start/Confirm chết hoàn toàn dù
> GPIO/NVIC đã cấu hình đúng (Bug #11). Mỗi lần đổi pin (PE1→PD4→PD2), lỗi
> này **tái phát y hệt**: CubeMX sinh khung `EXTIx_IRQHandler()` mới toanh,
> và vì tên khối `USER CODE BEGIN EXTIy_IRQn 0` cũ không còn tồn tại (tên
> khối đổi theo tên IRQ), nội dung cũ **không được copy sang** khối mới —
> phải tự thêm lại lời gọi bằng tay mỗi lần. Bài học: **đổi số chân EXTI
> luôn kéo theo đổi tên khối `USER CODE`, code trong khối cũ sẽ bị bỏ rơi
> chứ không tự chuyển.** Chi tiết:
> [HARDWARE_BUGS.md § Bug #11](HARDWARE_BUGS.md#bug-11--pe1-startconfirm-chết-hoàn-toàn-quên-gọi-irq-handler),
> [§ Bug #14](HARDWARE_BUGS.md#bug-14--pe1-xung-đột-với-mems_int2-của-cảm-biến-gia-tốc-tích-hợp),
> [§ Bug #16](HARDWARE_BUGS.md#bug-16--pd4-cũng-đọc-low-cố-định-nguyên-nhân-chưa-xác-nhận).

### Kết nối phần cứng

```
MCU PD2 ─── [Nút nhấn] ─── GND
            Pull-up nội 40kΩ bật sẵn trong BSP
            → không cần điện trở ngoài
```

| Trạng thái nút | Mức điện áp PD2 | EXTI trigger |
|---------------|-----------------|--------------|
| Không nhấn | HIGH (3.3V) | Không có |
| Nhấn (nối GND) | LOW (0V) | Falling edge ✅ → `BTN_EVENT_CONFIRM` |

> Thiết kế hiện tại **không còn long-press** — bất kỳ lần nhấn nào (falling
> edge) cũng xác nhận lựa chọn ngay lập tức (`BSP_StartButton_IRQHandler()`
> gọi callback vô điều kiện, không đo duration). Long-press chỉ còn ý nghĩa
> lịch sử ở PA0 trước khi bị loại bỏ — xem comment trong
> [bsp_button.h](../App/BSP/Inc/bsp_button.h).
>
> ⚠️ **Nếu PD2 vẫn đọc LOW cố định:** trước khi đổi sang chân khác, hãy chạy
> phép thử cô lập 3 bước mô tả trong
> [HARDWARE_BUGS.md § Bug #16](HARDWARE_BUGS.md#bug-16--pd4-cũng-đọc-low-cố-định-nguyên-nhân-chưa-xác-nhận) —
> đo chân khi không nối gì, kiểm tra continuity của nút, kiểm tra tham chiếu
> đo. Hai lần liên tiếp gặp cùng triệu chứng trên 2 chân khác hẳn nhau về
> mặt chức năng (PE1, PD4) gợi ý nguyên nhân có thể nằm ở dây/nút/thiết bị
> đo chứ không chỉ ở bản thân từng chân MCU.

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

> ⚠️ **Lưu ý phần cứng:** PD5 trên STM32F411E-DISCO còn được đấu chung với
> tín hiệu `OTG_FS_OverCurrent` — khi nhấn nút này, LED **LD8** (đỏ, báo quá
> dòng USB OTG) trên board sẽ sáng lên đồng thời. Đây là hành vi phần cứng
> vô hại, không phải lỗi firmware (đã xác nhận qua schematic chính thức của
> board).

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
│  PD2 ──[40kΩ pull-up]── [Nút Start]  ── GND    │
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
| PA0 | ⚠️ Do CubeMX generate **toàn bộ** GPIO mode + NVIC priority từ `.ioc` — regenerate sẽ ghi đè `MX_GPIO_Init()`. An toàn **chỉ khi** `.ioc` đang khai báo đúng (xem bảng bên dưới); code xử lý logic (`BSP_Button_IRQHandler`) vẫn nằm trong `USER CODE` nên không mất. **Đã từng bị reset về mặc định 2 lần** dù không ai chủ ý sửa PA0 (Bug #12, tái phát ở Bug #15) — luôn kiểm tra lại sau mỗi lần Generate Code, kể cả khi bạn chỉ sửa chân khác |
| PD2 | ⚠️ Tương tự PA0 — GPIO mode + NVIC priority do `.ioc` quyết định hoàn toàn. Lời gọi `BSP_StartButton_IRQHandler()` trong `EXTI2_IRQHandler` nằm trong `USER CODE` nên an toàn qua regenerate, nhưng **phải tự kiểm tra nó có mặt** sau mỗi lần regenerate mới — đặc biệt sau khi đổi số chân EXTI, vì tên khối `USER CODE` đổi theo và nội dung cũ không tự chuyển (xem Bug #11, #14, #16). Ngoài ra: chân này **chưa được xác nhận tự do bằng schematic gốc** — nếu vẫn LOW cố định, xem Bug #16 |
| PD5 | ✅ An toàn — không có trong `.ioc`, CubeMX không động đến |
| PD6 | ✅ An toàn — không có trong `.ioc`, CubeMX không động đến |

> **Không đặt code BSP ngoài vùng `USER CODE BEGIN / USER CODE END`** — CubeMX sẽ xóa khi Generate Code.
>
> **Checklist trước khi tin tưởng PA0/PD2 sau một lần Generate Code (bất kỳ chân nào vừa đổi):**
>
> 1. `Freertos.ioc` có đúng `PA0-WKUP.GPIO_ModeDefaultEXTI=GPIO_MODE_IT_RISING_FALLING` không?
> 2. `Freertos.ioc` có đúng `NVIC.EXTI0_IRQn`/`NVIC.EXTI2_IRQn` preempt priority = `5` không?
> 3. `EXTI0_IRQHandler`/`EXTI2_IRQHandler` trong `stm32f4xx_it.c` có gọi đúng
>    `BSP_Button_IRQHandler()`/`BSP_StartButton_IRQHandler()` trong `USER CODE` không?
>
> Cả 3 mục này từng sai cùng lúc (Bug #11, #12, #13), rồi **tái phát lại sau
> khi đổi chân PE1 → PD4** (Bug #14, #15), và **PD4 → PD2 lặp lại đúng quy
> trình sửa tương tự** — mà firmware vẫn build thành công mọi lần. Compiler
> không phát hiện được lớp lỗi này; chỉ review thủ công hoặc test phần cứng
> mới thấy.
