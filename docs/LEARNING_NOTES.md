# FreeRTOS Reference — Washing Machine Simulator

**Nền tảng:** STM32F411E-DISCO (Cortex-M4F) · FreeRTOS kernel V11.1.0 · STM32CubeIDE

---

## §0 Về tài liệu này

### §0.1 Phạm vi và cách dùng

Tài liệu gồm ba phần, phục vụ ba mục đích khác nhau:

| Phần | Nội dung | Dùng khi |
| --- | --- | --- |
| **A — Cẩm nang khái niệm** | Kiến thức đầy đủ về từng primitive FreeRTOS: cơ chế bên trong, API, ràng buộc, pattern | Tra cứu khi thiết kế hoặc gỡ lỗi; áp dụng sang project khác |
| **B — Nhật ký xây dựng** | Ghi chép theo tiến trình Phase 1–8 của project này | Xem lại quyết định thiết kế đã đưa ra và lý do |
| **C — Phụ lục** | Bảng tra API, cấu hình kernel, thuật ngữ, lịch sử sửa đổi | Tra nhanh |

Phần A là nội dung chính. Phần B mang giá trị lịch sử: nó ghi lại *quá trình*
đi tới thiết kế hiện tại, kể cả những thiết kế đã bị thay thế.

Khi một khái niệm xuất hiện ở cả hai phần, **Phần A là bản có thẩm quyền**;
Phần B chỉ mô tả cách khái niệm đó được áp dụng tại thời điểm của phase tương
ứng.

### §0.2 Quy ước ký hiệu

| Ký hiệu | Nghĩa |
| --- | --- |
| `§x.y` | Tham chiếu mục trong tài liệu này |
| `file.c:123` | Vị trí đã đối chiếu trực tiếp với source trong repo |
| ✅ | Cách làm đúng |
| ❌ | Cách làm sai, không dùng |
| ⚠ | Hành vi dễ gây nhầm, cần đọc kỹ |
| **Trong project này** | Đoạn nói riêng về Washing Machine Simulator, không phải quy tắc chung của FreeRTOS |

Mã mục `§x.y` là **cố định**: khi bổ sung nội dung mới thì thêm mục mới ở
cuối chương, không đánh số lại các mục cũ, để mọi tham chiếu chéo luôn còn
hiệu lực.

### §0.3 Môi trường tham chiếu

Mọi con số, kích thước struct, và hành vi mô tả trong tài liệu đều ứng với
cấu hình dưới đây. Áp dụng sang môi trường khác phải kiểm tra lại.

| Hạng mục | Giá trị |
| --- | --- |
| Kernel | FreeRTOS V11.1.0 (`OS/FreeRTOS/include/task.h`, `tskKERNEL_VERSION_NUMBER`) |
| MCU | STM32F411VETx, Cortex-M4F, 512 KB Flash, 128 KB RAM |
| Port | `OS/FreeRTOS/portable/GCC/ARM_CM4F` |
| Nguồn clock | HSI nội 16 MHz, không PLL (xem §5.4) |
| Tick rate | `configTICK_RATE_HZ = 1000` → 1 tick = 1 ms |
| Heap | `heap_4.c` |
| Priority | `configMAX_PRIORITIES = 5` |

Bảng cấu hình kernel đầy đủ: §C.2.

### §0.4 Chuẩn kiểm chứng

Mọi khẳng định kỹ thuật trong Phần A được đối chiếu với source FreeRTOS thật
trong repo (`OS/FreeRTOS/`), không viết theo trí nhớ.

- Khẳng định có tham chiếu `file:dòng` là đã đối chiếu trực tiếp source.
- Khẳng định chưa kiểm chứng được phải ghi rõ **"chưa xác nhận"** — không
  trình bày giả thuyết như sự thật.
- Số liệu hiệu năng trích từ tài liệu chính thức phải kèm nguồn và ngữ cảnh
  gốc, không suy rộng sang ngữ cảnh khác.
- Chi tiết phụ thuộc phiên bản kernel (tên macro, điều kiện biên dịch, kích
  thước struct) được đánh dấu tại chỗ.

---

## Mục lục

### Phần A — Cẩm nang khái niệm

| Mục | Chương | Trạng thái |
| --- | --- | --- |
| §1 | Event Group | Đầy đủ |
| §2 | Guard Condition & thiết kế State Machine | Đầy đủ |
| §3 | Sticky / Stale Level Flag | Đầy đủ |
| §4 | Software Timer | Đầy đủ |
| §5 | Tick, SysTick & Context Switch | Đầy đủ |
| §6 | Task Notification | Đang biên soạn |

### Phần B — Nhật ký xây dựng

| Mục | Phase |
| --- | --- |
| §B.1 | Project Setup & FreeRTOS Tasks |
| §B.2 | Application Data Types & FreeRTOS Objects |
| §B.3 | UI Task & Button ISR |
| §B.4 | Sensor Task |
| §B.5 | WashingManager & State Machine |
| §B.6 | OLED Task & Driver Integration |
| §B.7 | Software Timers |
| §B.8 | SEGGER SystemView Analysis |

### Phần C — Phụ lục

| Mục | Nội dung |
| --- | --- |
| §C.1 | Bảng tra API theo primitive |
| §C.2 | Cấu hình kernel của project |
| §C.3 | Thuật ngữ |
| §C.4 | Lịch sử sửa đổi tài liệu |
| §C.5 | Lộ trình học và điều kiện tốt nghiệp |

### Phần D — Nhật ký Q&A

Hỏi–đáp theo Session, mới nhất ở trên (trước đây là file `QA_LOG.md`).

---

## Tiến độ Phần A

| # | Chủ đề | Trạng thái |
| --- | --- | --- |
| 1 | Task, Queue (cơ bản) | Đã có ở Phần B |
| 2 | Event Group | ✅ §1 |
| 3 | Software Timer + Timer Service Task | ✅ §4 |
| 4 | Task Notification | 🔶 §6 đang biên soạn |
| 5 | Semaphore & Mutex, priority inversion | ⬜ |
| 6 | Scheduler internals (priority, preemption, context switch, tick) | ✅ §5 |
| 7 | ISR design (NVIC, BASEPRI, FromISR, portYIELD_FROM_ISR) | ⬜ |
| 8 | Memory (heap_4, stack sizing, static vs dynamic, overflow) | ⬜ |
| 9 | Design patterns (state machine, layering, ownership) | 🔶 một phần ở §2 |
| 10 | Debug (SystemView, đọc trace, bug timing) | 🔶 một phần ở §B.8 |

Một chủ đề chỉ chuyển sang ✅ khi người học xác nhận đã đủ, không phải khi
nội dung đã viết dài.

---

# PHẦN A — Cẩm nang khái niệm

> Kiến thức đầy đủ về từng primitive FreeRTOS, viết để tra cứu và để áp dụng
> sang project khác — không giới hạn trong cách dùng của project này. Mỗi
> chương gồm: cơ chế bên trong, bảng API, hành vi runtime, ràng buộc, pattern
> thực chiến, lỗi thường gặp, và một mục riêng về cách áp dụng trong project
> hiện tại.
>
> Chuẩn kiểm chứng: §0.4. Môi trường tham chiếu: §0.3.

---

## §1 Event Group

**Tóm tắt:** tập hợp bit cờ dùng chung, cho phép **nhiều task cùng chờ** một
hoặc nhiều điều kiện, và một lần set bit có thể **đánh thức đồng thời** mọi
task đang chờ. Đây là primitive duy nhất trong FreeRTOS có ngữ nghĩa
broadcast.

**Dùng khi:** cần chờ tổ hợp nhiều điều kiện (AND/OR), hoặc cần nhiều
consumer phản ứng với cùng một sự kiện.

**Không dùng khi:** chỉ có một consumer (→ Task Notification, §6), hoặc cần
truyền dữ liệu có cấu trúc và giữ thứ tự (→ Queue).

### §1.1 Cấu trúc bên trong

Event Group thực chất chỉ là **một biến bitmask + một danh sách task đang chờ**:

Nguồn: `OS/FreeRTOS/event_groups.c:54-65`.

```c
typedef struct EventGroupDef_t {
    EventBits_t uxEventBits;              /* chính là các cờ, 1 biến duy nhất */
    List_t      xTasksWaitingForBits;     /* danh sách task đang Blocked      */

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t uxEventGroupNumber;   /* chỉ để trace tool nhận diện      */
    #endif
    #if ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
        uint8_t ucStaticallyAllocated;    /* để biết có được free hay không   */
    #endif
} EventGroup_t;
```

Mọi tính chất khác đều **suy ra được** từ cấu trúc này:

- Không có buffer → **không lưu lịch sử, không đếm được**.
- Set bit = phép `|=` → **idempotent**, set 2 lần = set 1 lần (`1 OR 1 = 1`).
- Rất nhẹ, và **kích thước không phụ thuộc** số bit hay số task chờ.

**Kích thước thực tế trên Cortex-M4** (tính từ `list.h`: `List_t` =
`uxNumberOfItems`(4) + `pxIndex`(4) + `MiniListItem_t`(12) = **20 bytes**):

| Thành phần | Bytes |
| --- | --- |
| `uxEventBits` | 4 |
| `xTasksWaitingForBits` (`List_t`) | 20 |
| `uxEventGroupNumber` (nếu `configUSE_TRACE_FACILITY == 1`) | 4 |
| `ucStaticallyAllocated` (nếu bật static alloc) | 1 → padding 4 |
| **Tổng — cấu hình của project này** (trace = 1) | **28** |

So với Queue: Queue còn tốn thêm `depth × item_size` cho vùng chứa data.

> `configUSE_EVENT_GROUPS` (mặc định `1` — `FreeRTOS.h:321-322`) phải bằng 1
> thì `xEventGroupCreate()` mới được biên dịch.

### §1.2 Số bit khả dụng

Số bit **không** phụ thuộc gì tới tick về mặt ý nghĩa, nhưng vì lý do cài đặt
nội bộ (event group tái dùng cơ chế list của kernel) nó bị quyết định bởi
`configTICK_TYPE_WIDTH_IN_BITS`. Nguồn:
`OS/FreeRTOS/include/event_groups.h:133-140`.

| `configTICK_TYPE_WIDTH_IN_BITS` | `EventBits_t` | Bit dùng được |
| --- | --- | --- |
| `TICK_TYPE_WIDTH_16_BITS` (0) | 16-bit | **8 bit** (bit 0 → 7) |
| `TICK_TYPE_WIDTH_32_BITS` (1) | 32-bit | **24 bit** (bit 0 → 23) |
| `TICK_TYPE_WIDTH_64_BITS` (2) | 64-bit | **56 bit** |

> ⚠️ **Tên macro đã đổi.** Từ FreeRTOS V11, tên chuẩn là
> `configTICK_TYPE_WIDTH_IN_BITS`. Tên cũ `configUSE_16_BIT_TICKS` **vẫn được
> hỗ trợ** và tự động ánh xạ sang tên mới (`FreeRTOS.h:66-78`), nhưng
> **không được định nghĩa cả hai** — kernel sẽ `#error`. Project này dùng tên
> cũ: `configUSE_16_BIT_TICKS = 0` → tương đương 32-bit tick → **24 bit dùng
> được**, đang xài 5/24.

**8 bit cao kernel giữ** làm cờ điều khiển nội bộ — với tick 32-bit
(`event_groups.h:47-51`):

```c
#define eventCLEAR_EVENTS_ON_EXIT_BIT    ( ( uint32_t ) 0x01000000U )
#define eventUNBLOCKED_DUE_TO_BIT_SET    ( ( uint32_t ) 0x02000000U )
#define eventWAIT_FOR_ALL_BITS           ( ( uint32_t ) 0x04000000U )
#define eventEVENT_BITS_CONTROL_BYTES    ( ( uint32_t ) 0xff000000U )
```

Lấn vào đó không phải "hành vi không xác định" một cách mơ hồ — kernel bắt
được bằng `configASSERT()` ngay đầu `WaitBits`/`SetBits`/`ClearBits`
(`event_groups.c:197, 328, 483`):

```c
configASSERT( ( uxBitsToWaitFor & eventEVENT_BITS_CONTROL_BYTES ) == 0 );
```

### §1.3 Bảng API

| API | Gọi từ ISR? | Ghi chú |
| --- | --- | --- |
| `xEventGroupCreate()` / `...Static()` | ❌ | Static = không dùng heap |
| `xEventGroupSetBits()` | ❌ | Giá trị trả về **không đáng tin** (§1.8) |
| `xEventGroupSetBitsFromISR()` | ✅ | **Deferred** — §1.5 |
| `xEventGroupClearBits()` | ❌ | Trả về giá trị **trước khi** xóa |
| `xEventGroupClearBitsFromISR()` | ✅ | Cũng deferred |
| `xEventGroupWaitBits()` | ❌ **không bao giờ** | Vì có blocking |
| `xEventGroupGetBits()` | ❌ | Đọc tức thì, không block |
| `xEventGroupGetBitsFromISR()` | ✅ | Đọc tức thì, an toàn |
| `xEventGroupSync()` | ❌ | **Rendezvous/barrier** — §1.6 |
| `vEventGroupDelete()` | ❌ | Task đang chờ bị unblock |

### §1.4 Ngữ nghĩa broadcast

```
Queue        : 1 item → CHỈ MỘT task nhận được (unicast, item bị tiêu thụ)
Event Group  : set 1 bit → TẤT CẢ task đang chờ bit đó đều thức (broadcast)
```

5 task cùng `WaitBits(EVT_WIFI_CONNECTED)` → chỉ cần **một** lần `SetBits` là
cả 5 cùng unblock. Với Queue phải gửi 5 lần.

→ Đây là lý do Event Group là công cụ chuẩn cho **"thông báo trạng thái hệ
thống cho nhiều bên quan tâm"**.

⚠️ Broadcast + `xClearOnExit = pdTRUE` là cặp nguy hiểm: task đến chờ **muộn
hơn một chút** sẽ mất sự kiện vĩnh viễn (lost wakeup). Quy tắc: **nhiều
waiter → dùng `pdFALSE`, và chỉ định MỘT chủ thể duy nhất chịu trách nhiệm
`ClearBits`**.

### §1.5 xEventGroupSetBitsFromISR — cơ chế trì hoãn

```c
xEventGroupSetBitsFromISR(xEvents, EVT_DATA_READY, &xHigherPriorityTaskWoken);
/* ← Bit CHƯA được set tại dòng này! */
```

Thao tác set bit + duyệt danh sách task chờ là **unbounded time**, không được
phép làm trong ISR. FreeRTOS đẩy sang **Timer Service Task** qua timer command
queue:

```
ISR → gửi lệnh vào timer command queue → (ISR kết thúc)
                                       ↓
                      Timer Service Task chạy → giờ mới thực sự set bit
```

**Bằng chứng từ source** (`event_groups.c:812-829`) — toàn bộ thân hàm chỉ
là một lời gọi chuyển tiếp:

```c
BaseType_t xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup,
                                      const EventBits_t uxBitsToSet,
                                      BaseType_t * pxHigherPriorityTaskWoken )
{
    BaseType_t xReturn;
    xReturn = xTimerPendFunctionCallFromISR( vEventGroupSetBitsCallback,
                                             ( void * ) xEventGroup,
                                             ( uint32_t ) uxBitsToSet,
                                             pxHigherPriorityTaskWoken );
    return xReturn;   /* pdPASS nếu xếp được lệnh vào timer queue, pdFAIL nếu không */
}
```

Hệ quả bắt buộc nhớ:

1. **Có độ trễ** — bit chỉ được set khi daemon task được lên lịch chạy.
2. **Có thể THẤT BẠI** — timer command queue đầy → trả `pdFAIL`. **Luôn phải
   kiểm tra return value**, khác với `xQueueSendFromISR`.
3. **Yêu cầu BA điều kiện biên dịch** (`event_groups.c:812`), không phải hai:

   ```c
   #if ( ( configUSE_TRACE_FACILITY == 1 ) && \
         ( INCLUDE_xTimerPendFunctionCall == 1 ) && \
         ( configUSE_TIMERS == 1 ) )
   ```

   Việc phải bật `configUSE_TRACE_FACILITY` là một điểm kỳ quặc thật của
   kernel, dễ khiến người dùng bối rối khi hàm "biến mất".

4. Nhiều codebase production **tránh hẳn** hàm này, thay bằng **Task
   Notification** — gọi trực tiếp từ ISR, không qua daemon, không thể fail
   vì lý do "queue đầy".

> 🔍 **Kiểm chứng cho chính project này:** `FreeRTOSConfig.h` có
> `configUSE_TRACE_FACILITY = 1` và `configUSE_TIMERS = 1`, nhưng **không hề
> định nghĩa** `INCLUDE_xTimerPendFunctionCall`, mà mặc định của nó là **0**
> (`FreeRTOS.h:283-285`). Nghĩa là **`xEventGroupSetBitsFromISR()` hiện KHÔNG
> được biên dịch trong project này** — gọi nó sẽ lỗi lúc link. Đây cũng là
> một lý do nữa cho thấy comment cũ trong `app_events.h` (nói timer callback
> dùng hàm này) là sai.

### §1.6 xEventGroupSync — rendezvous và barrier

Bài toán: N task phải **cùng đến một điểm** rồi mới được đi tiếp.

```c
#define TASK_A_READY  (1 << 0)
#define TASK_B_READY  (1 << 1)
#define TASK_C_READY  (1 << 2)
#define ALL_READY     (TASK_A_READY | TASK_B_READY | TASK_C_READY)

/* Trong task A: */
xEventGroupSync(xEvents,
                TASK_A_READY,   /* bit tôi set  — "tôi xong rồi"     */
                ALL_READY,      /* bit tôi chờ  — "chờ tất cả xong"  */
                portMAX_DELAY);
/* Tới đây: cả 3 task cùng chạy tiếp, bit tự động clear sạch */
```

`Sync` = `SetBits` + `WaitBits` **nguyên tử**. Không thể tự ghép 2 hàm rời để
đạt hiệu quả tương đương (sẽ có race condition).

### §1.7 WaitBits so với GetBits

| | `xEventGroupWaitBits()` | `xEventGroupGetBits()` |
| --- | --- | --- |
| Câu hỏi nó trả lời | *"Tôi **đợi cho tới khi** điều kiện đúng"* | *"**Ngay lúc này** đúng hay sai?"* |
| Blocking | Có — task rời CPU | Không — trả về tức thì |
| Dùng cho | Chờ sự kiện sẽ đến | **Guard / precondition check** |
| Task state | Running → Blocked → Ready | Vẫn Running suốt |

Dùng `WaitBits` làm guard là **anti-pattern**: task bị block sẽ không thể báo
cho user biết **tại sao** nó từ chối → hệ thống trông như bị treo. Xem §2.

### §1.8 Ngữ nghĩa giá trị trả về

**`WaitBits` — cách phát hiện timeout.** Không có mã lỗi riêng; phải tự kiểm
tra bit:

```c
EventBits_t bits = xEventGroupWaitBits(eg, EVT_WATER_FULL,
                                       pdTRUE, pdFALSE,
                                       pdMS_TO_TICKS(5000));
if ((bits & EVT_WATER_FULL) != 0) { /* thành công */ }
else                              { /* TIMEOUT   */ }
```

Hai điểm dễ sai:

- Với `xWaitForAllBits = pdTRUE` (AND), phải so `(bits & WANTED) == WANTED`,
  không phải `!= 0`.
- Giá trị trả về là bits **trước khi** clear-on-exit xóa → vẫn đọc được.

**Idiom cần thuộc:**

```c
(bits & mask) != 0      /* OR  — có ít nhất 1 bit  */
(bits & mask) == mask   /* AND — có đủ tất cả bit  */
```

**`SetBits` — giá trị trả về KHÔNG đáng tin.** Tài liệu chính thức
(`event_groups.h:485-493`) nêu **hai** lý do bit vừa set có thể đã biến mất
khỏi giá trị trả về:

1. Việc set bit làm một task đang chờ thoát Blocked, và task đó dùng
   `xClearOnExit = pdTRUE` → **bit bị xóa tự động**.
2. Bất kỳ task nào có **priority cao hơn** task gọi `SetBits` mà được unblock
   (hoặc đang Ready) sẽ chạy và **có thể đổi giá trị event group** trước khi
   `SetBits` kịp return.

→ Coi giá trị trả về của `SetBits` như `void`.

### §1.9 Nhiều task cùng chờ và clear-on-exit

Đây **không phải suy đoán** — đọc thẳng `xEventGroupSetBits()` trong
`event_groups.c` thấy rõ trình tự:

```c
vTaskSuspendAll();                              /* ① khoá scheduler        */
{
    pxEventBits->uxEventBits |= uxBitsToSet;    /* ② set bit TRƯỚC         */

    while( pxListItem != pxListEnd )            /* ③ duyệt HẾT task đang chờ */
    {
        ...
        if( điều kiện thỏa )
        {
            if( clear-on-exit )
                uxBitsToClear |= uxBitsWaitedFor;   /* chỉ GOM lại, chưa xóa */

            vTaskRemoveFromUnorderedEventList( ... );  /* unblock task này   */
        }
        pxListItem = pxNext;
    }

    pxEventBits->uxEventBits &= ~uxBitsToClear;  /* ④ giờ mới XÓA, sau vòng lặp */
}
( void ) xTaskResumeAll();                       /* ⑤ mở khoá → preempt ở đây  */
```

Hai kết luận quan trọng rút ra từ trình tự này:

- **Bước ③ chạy hết trước bước ④** → mọi task đang chờ **đều được unblock**,
  broadcast không bị mất dù có clear-on-exit. Nhưng task đến chờ **sau** bước
  ④ thì mất sự kiện vĩnh viễn (lost wakeup).
- **Toàn bộ nằm trong `vTaskSuspendAll()`/`xTaskResumeAll()`** → không task
  nào thực sự chạy ở giữa vòng lặp. Việc chuyển sang task priority cao hơn
  chỉ xảy ra tại `xTaskResumeAll()` (bước ⑤). Đây là lý do thao tác này an
  toàn dù đụng vào danh sách task đang chờ.

#### §1.9.1 Quyền clear bit khi có nhiều consumer

> ⚠️ Phần khung quyết định 4 bước dưới đây là **suy luận kỹ thuật của mình**,
> không phải một API hay rule được đặt tên chính thức trong tài liệu
> FreeRTOS. Đã đối chiếu với 3 nguồn để tránh nói sai phần cơ chế nền:
> `OS/FreeRTOS/event_groups.c` (source thật trong repo), trang API chính thức
> [`xEventGroupWaitBits()`](https://freertos.org/Documentation/02-Kernel/04-API-references/12-Event-groups-or-flags/04-xEventGroupWaitBits),
> và [`xEventGroupSync()`](https://freertos.org/Documentation/02-Kernel/04-API-references/12-Event-groups-or-flags/11-xEventGroupSync).

**Đính chính một điểm mình từng nói sai ở buổi trước:** dùng
`xClearOnExit = pdTRUE` trong chính lời gọi `WaitBits()` của một waiter
**không hề gây race**, kể cả khi nhiều waiter cùng khớp một lần `SetBits()`.
Tài liệu chính thức nói rõ (nguyên văn từ trang API):

> *"If xClearOnExit is set to pdTRUE then any bits set in the value passed as
> the uxBitsToWaitFor parameter will be cleared in the event group before
> xEventGroupWaitBits() returns..."*

Và đúng như đã trace ở mục C: việc "gom bit cần xóa rồi xóa một lần" xảy ra
**bên trong `vTaskSuspendAll()`/`xTaskResumeAll()`**, tức là **trước khi bất
kỳ waiter nào thực sự được chạy trở lại**. Không có waiter nào "chạy nhanh
hơn" waiter khác ở bước này — tất cả được xử lý xong bởi kernel trước khi ai
được resume. Nên **nếu nhiều task cùng gọi `WaitBits(..., pdTRUE, ...)` trên
cùng bit, đó là cách dùng an toàn và đúng chuẩn**, không cần "chọn 1 chủ thể".

**Vậy câu hỏi "ai chịu trách nhiệm clear" áp dụng cho trường hợp nào?** Chỉ
áp dụng khi có một lời gọi `xEventGroupClearBits()` **độc lập, tách rời**
khỏi bất kỳ `WaitBits()` nào — tức là một hành động "dọn dẹp vòng đời" do
application tự quyết định thực hiện lúc nào, không phải cơ chế tự động của
kernel. Đây chính xác là trường hợp `Manager_OnEnterIdle()`:

```c
xEventGroupClearBits(xSystemEvents, EVT_FINISHED | EVT_ERROR);
```

Lời gọi này **không nằm trong** một `WaitBits()` — nó là code ứng dụng chủ
động quyết định "giờ là lúc dọn sạch 2 bit này". Ở đây mới thực sự có câu
hỏi **"ai được phép làm việc dọn dẹp này"**, vì không có cơ chế kernel nào tự
động đảm bảo đúng lúc cho loại lời gọi này — và đây là chỗ khung quyết định
dưới đây áp dụng.

**Khung quyết định — 4 bước (cho việc gọi `ClearBits()` độc lập):**

1. **Bit là CONDITION (level) hay OCCURRENCE (broadcast một lần)?**
   Condition có transition ngược tự nhiên (`EVT_DOOR_CLOSED`); Occurrence thì
   không (`EVT_FINISHED`).

2. **Nếu CONDITION** → chủ thể là **producer**, clear đối xứng (quy tắc
   §3, không đổi gì khi có nhiều waiter).

3. **Nếu OCCURRENCE, kiểm tra có phải BARRIER thật** (mọi bên vừa set phần
   mình vừa chờ đủ N phần, đối xứng)?
   - Có → dùng **`xEventGroupSync()`**, kernel tự clear, câu hỏi "ai clear"
     biến mất.
   - Không (1 producer, N consumer thụ động) → bước 4.

4. **1 producer, N consumer thụ động** → chủ thể clear là **producer đó**,
   **với điều kiện producer là DUY NHẤT**. Nếu có ≥2 producer cho cùng một
   bit → không có lựa chọn an toàn bằng cách "chọn 1 trong số họ" — phải tách
   bit theo nguyên nhân, hoặc gán quyền sở hữu ở cấp kiến trúc (mở rộng
   nguyên tắc ownership sẵn có của project, xem dưới).

**Đối chứng bằng chính code project — `EVT_FINISHED` vs `EVT_ERROR`:**

Cả hai đều được clear tại cùng một chỗ, `Manager_OnEnterIdle()`:

```c
xEventGroupClearBits(xSystemEvents, EVT_FINISHED | EVT_ERROR);
```

Nhưng số phận khác nhau:

| Bit | Số producer | Kết quả |
| --- | --- | --- |
| `EVT_FINISHED` | **1** (chỉ `WashingManager`) | An toàn — đúng bước 4, producer duy nhất tự nhiên là chủ thể clear |
| `EVT_ERROR` | **2** (`SensorTask` + `WashingManager`) | **Bug #17** — không ai trong 2 bên dám tự tin clear mà không giẫm chân bên kia |

→ Khác biệt duy nhất giữa "đúng" và "Bug #17" là **số lượng producer trên
cùng một bit**. Đây là bằng chứng cụ thể nhất cho quy tắc ở bước 4.

**Bảng quyết định tổng hợp:**

| Tình huống | Chủ thể clear | Công cụ |
| --- | --- | --- |
| Bit là level, có true/false tự nhiên | Producer, đối xứng set/clear | `SetBits`/`ClearBits` thường |
| N bên vừa set vừa chờ nhau (barrier thật) | **Kernel** | `xEventGroupSync()` |
| 1 producer, N consumer thụ động | Producer đó | `ClearBits` khi khởi động lại vòng đời |
| N producer, không phải barrier | **Không có lựa chọn an toàn** bằng cách chọn 1 trong số họ | Tách bit theo nguyên nhân, hoặc gán ownership kiến trúc |
| N delivery độc lập, mỗi consumer cần bản riêng | *(không phải bài toán của Event Group)* | Task Notification × N, hoặc Queue riêng mỗi consumer |

### §1.10 Cấp phát tĩnh

```c
static StaticEventGroup_t xEventGroupBuffer;   /* nằm ở .bss, không dùng heap */
EventGroupHandle_t eg = xEventGroupCreateStatic(&xEventGroupBuffer);
```

Vì sao quan trọng: ô tô, y tế, hàng không (MISRA-C, DO-178C, IEC 61508)
thường **cấm cấp phát động hoàn toàn**. Mọi object FreeRTOS đều có bản Static
(`xQueueCreateStatic`, `xTaskCreateStatic`, `xTimerCreateStatic`...), bật bằng
`configSUPPORT_STATIC_ALLOCATION = 1`. Lợi ích phụ: biết chính xác RAM lúc
**compile-time**, không bao giờ fail lúc runtime.

### §1.11 Thay thế bằng Task Notification

Task Notification có thể hoạt động như **một event group 32-bit riêng của
từng task**:

```c
/* Bên gửi — thay cho xEventGroupSetBits() */
xTaskNotify(xManagerTaskHandle, EVT_WATER_FULL, eSetBits);

/* Bên nhận — thay cho xEventGroupWaitBits() */
uint32_t bits;
xTaskNotifyWait(0x00,        /* bit clear khi VÀO                        */
                ULONG_MAX,   /* bit clear khi RA (tương đương clear-on-exit) */
                &bits,       /* giá trị nhận được                        */
                portMAX_DELAY);
```

| | Event Group | Task Notification |
| --- | --- | --- |
| Số task nhận được | **Nhiều** (broadcast) | **Chỉ 1** (địa chỉ tới đúng 1 task) |
| Chờ tổ hợp AND | ✅ có sẵn (`xWaitForAllBits`) | ❌ phải tự viết vòng lặp |
| Chi phí RAM | 28 B cho object dùng chung | **Không cần object riêng** — nằm sẵn trong TCB của **mỗi** task |
| Gọi từ ISR | Deferred qua daemon, **có thể `pdFAIL`** | **Trực tiếp**, không qua daemon |
| Tốc độ | Chậm hơn | Nhanh hơn (xem ghi chú) |

**Chi phí RAM — con số chính xác.** Notification không phải "0 byte". Mỗi TCB
luôn mang sẵn (`tasks.c:419-420`):

```c
volatile uint32_t ulNotifiedValue[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
volatile uint8_t  ucNotifyState [ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
```

Với mặc định `configTASK_NOTIFICATION_ARRAY_ENTRIES = 1`
(`FreeRTOS.h:2782-2784`) → **5 bytes (padding lên 8) trong MỖI TCB**, dù task
đó có dùng notification hay không. Nghĩa là: notification rẻ hơn khi chỉ có 1
consumer, nhưng chi phí là **cố định theo số task**, không phải bằng 0.
`configUSE_TASK_NOTIFICATIONS` mặc định `1` (`FreeRTOS.h:2778-2780`); đặt `0`
sẽ bỏ 2 trường này khỏi TCB.

> ⚠️ **Về con số "nhanh hơn 45%" — đã xác minh với nguồn chính thức.** Trang
> [FreeRTOS Direct-to-task-notifications — As binary semaphore](https://freertos.org/Documentation/02-Kernel/02-Kernel-features/03-Direct-to-task-notifications/02-As-binary-semaphore)
> viết nguyên văn:
>
> *"Unblocking an RTOS task with a direct notification is 45% faster and
> uses less RAM than unblocking a task with a binary semaphore."*
>
> Đây là con số **có thật, chính thức**, nhưng so sánh với **BINARY
> SEMAPHORE**, không phải Event Group. FreeRTOS **không công bố** con số
> tương ứng khi so với Event Group, và con số này **không xuất hiện trong
> source kernel** (`event_groups.c`/`timers.c`) — nó tới từ đo đạc riêng của
> đội FreeRTOS, không tự suy ra được từ đọc code. Vì vậy khi so Event Group
> với Task Notification, chỉ nên nói "notification nhanh hơn vì đường đi
> ngắn hơn — không cần cấp phát object, không duyệt danh sách waiter, ISR
> không phải qua daemon task" — **đừng trích số 45% cho phép so sánh này.**

**Quy tắc chọn:**

- Chỉ **một** task quan tâm → **Task Notification** (nhanh, nhẹ, an toàn từ ISR).
- **Nhiều** task quan tâm, hoặc cần chờ AND → **Event Group**.

Áp vào project: `EVT_WATER_FULL`, `EVT_TIMEOUT`, `EVT_ERROR` chỉ có
WashingManager đọc → về kỹ thuật **có thể thay bằng Task Notification**. Dùng
Event Group ở đây vẫn hợp lý vì (a) `EVT_FINISHED` thiết kế cho nhiều
consumer tương lai, (b) đây là project **học FreeRTOS**, (c) chênh lệch hiệu
năng ở tần suất 10–20 giây/sự kiện là vô nghĩa.

### §1.12 Kỹ thuật debug

```c
EventBits_t now = xEventGroupGetBits(xSystemEvents);   /* đặt vào Live Expressions */
```

Mẹo thực dụng: khi hệ thống "đứng im không rõ lý do", việc **đầu tiên** nên
làm là pause debugger và đọc giá trị event group. Với Bug #17, làm vậy sẽ
thấy ngay `EVT_ERROR = 1` dù cửa đang đóng — chẩn đoán xong trong 10 giây
thay vì phải đọc code.

### §1.13 Use case điển hình ngoài project này

| Domain | Cách dùng |
| --- | --- |
| **Network stack** (lwIP, ESP-IDF) | `WIFI_CONNECTED_BIT`, `IP_GOT_BIT`, `WIFI_FAIL_BIT` — nhiều task chờ mạng sẵn sàng |
| **Power management** | `SLEEP_REQUEST`, `PERIPH_A_IDLE`, `PERIPH_B_IDLE` — dùng AND (`pdTRUE`) để chỉ ngủ khi **mọi** ngoại vi rảnh |
| **System init barrier** | `xEventGroupSync()` — mọi driver init xong mới cho application chạy |
| **Multi-sensor fusion** | Chờ **đủ cả** accel + gyro + mag có mẫu mới (AND) rồi mới tính |
| **Graceful shutdown** | Broadcast `SHUTDOWN_REQ`, rồi chờ AND của các bit `TASK_x_STOPPED` |

Pattern chung: **OR** (`pdFALSE`) cho *"cái nào tới trước cũng được"*, **AND**
(`pdTRUE`) cho *"phải đủ hết mới đi tiếp"*.

### §1.14 Bảng chọn primitive

| Nhu cầu | Công cụ đúng | Vì sao |
| --- | --- | --- |
| Truyền **dữ liệu** | **Queue** | Event Group chỉ mang true/false |
| Báo hiệu **trạng thái level** | **Event Group** | Idempotent, đọc bao nhiêu lần cũng được |
| **Đếm** số lần xảy ra | **Counting Semaphore** / counter + critical section | Event Group mất thông tin số lượng |
| Đánh thức **đúng 1 task**, cực nhanh | **Task Notification** | Nhanh nhất, ít RAM nhất |
| Đánh thức **nhiều task** cùng lúc | **Event Group** | Duy nhất hỗ trợ broadcast tự nhiên |
| Chờ **tổ hợp AND** | **Event Group** (`pdTRUE`) | Không primitive nào khác làm gọn được |
| Bảo vệ **tài nguyên dùng chung** | **Mutex** | Có priority inheritance |
| **Barrier** cho N task | **`xEventGroupSync()`** | Nguyên tử, không tự ghép được |

### §1.15 Lỗi thường gặp

1. Dùng Event Group để **truyền data** — nó chỉ mang true/false theo từng bit.
2. Dùng để **đếm** — không đếm được, mất thông tin số lượng.
3. Dùng `WaitBits` (blocking) làm **guard** — xem §2.
4. Tin vào **giá trị trả về của `SetBits`**.
5. Không kiểm tra return của **`SetBitsFromISR`** (có thể `pdFAIL`).
6. Cờ level duy trì **bất đối xứng** → sticky flag, xem §3.
7. Nhiều waiter + `clear-on-exit = pdTRUE` → lost wakeup.

### §1.16 Câu hỏi phỏng vấn

1. *"Event Group khác Semaphore ở điểm nào?"* → Semaphore đếm được và
   unicast (1 task lấy 1 token); Event Group là level-based, broadcast, hỗ
   trợ chờ tổ hợp AND/OR.
2. *"Tại sao `xEventGroupSetBitsFromISR` có thể fail còn `xQueueSendFromISR`
   thì không (khi queue chưa đầy)?"* → Vì nó là deferred operation qua timer
   command queue.
3. *"Có bao nhiêu bit dùng được và tại sao?"* → 24 (hoặc 8), do 8 bit cao
   dành cho kernel control flags.

---

## §2 Guard Condition và thiết kế State Machine

> Nguồn: **Guard condition** là thuật ngữ chính thức trong đặc tả UML State
> Machine (Harel statechart). Định nghĩa chuẩn: *"a condition that has to be
> met in order to enable the transition to which it belongs"*, được đánh giá
> đúng một lần tại thời điểm event xảy ra, và ký hiệu chuẩn trên diagram là
> `event [guard] / action`. Đây không phải thuật ngữ riêng của FreeRTOS hay
> project này — nó tồn tại độc lập với mọi RTOS, áp dụng cho bất kỳ state
> machine nào (kể cả `washing_sm.c` dù không dùng UML tooling).

### §2.1 Bốn thành phần của một FSM

Bảng transition của project hiện tại:

```
(current_state, event) → next_state
```

Dạng đầy đủ theo lý thuyết FSM / UML statechart:

```
(current_state, event, [guard]) → next_state / action
                        ↑
              điều kiện boolean phải ĐÚNG thì transition mới được phép nổ
```

Ký hiệu chuẩn UML cho cùng ý nghĩa: `event [guard] / action`.

**Guard** = "cổng gác". Event tới, nhưng guard sai → transition **không xảy
ra**, state giữ nguyên, event bị từ chối.

Ví dụ đời thường: thang máy nhận lệnh "đi tầng 5" (event), nhưng guard
`cửa_đã_đóng == true` sai → lệnh bị từ chối.

### §2.2 Vị trí đặt guard

**Guard phải đặt ở chỗ mọi đường đi đều bắt buộc chạy qua**, ngay tại điểm
quyết định transition, **trước khi bất kỳ action nào chạy**.

Ví dụ sai trong project: nếu đặt guard "cửa đóng" trong
`case WASH_STATE_FILL_WATER` thì:

- **Quá muộn** — `Manager_OnEnterFillWater()` đã chạy rồi, van đã mở, nước
  đang chảy.
- **Bỏ sót nhánh** — `IDLE → SPIN` không đi qua `FILL_WATER`, nên chế độ vắt
  hoàn toàn không bị kiểm tra. Lỗ hổng an toàn thật (hiện project vẫn đang
  có lỗ hổng này).

Chỗ đúng là `Manager_HandleCommand()` — nơi duy nhất **cả WASH lẫn SPIN** đều
bắt buộc đi qua:

```c
const EventBits_t required = EVT_DOOR_CLOSED | EVT_FILTER_OK;
EventBits_t bits = xEventGroupGetBits(xSystemEvents);   /* KHÔNG block */

if ((bits & required) != required) {
    Manager_PostDisplay(currentState, "CLOSE DOOR");
    return;                        /* từ chối, KHÔNG đổi state */
}
```

### §2.3 Guard không được block

Nếu dùng `WaitBits(..., portMAX_DELAY)` làm guard:

```
User bấm Start → Manager block vô hạn
                 → không xử lý được lệnh nào khác
                 → không gửi được message lên OLED
                 → user không biết TẠI SAO máy không chạy
                 → user tưởng máy hỏng
```

Nguyên tắc UX embedded: **từ chối ngay, và nói rõ lý do** — thay vì im lặng
chờ. Đây là lý do guard phải dùng `GetBits` (non-blocking), không phải
`WaitBits`.

### §2.4 Precondition, invariant, postcondition

| Loại | Định nghĩa | Kiểm tra khi nào | Ví dụ máy giặt |
| --- | --- | --- | --- |
| **Precondition (guard)** | Đúng **trước khi** cho phép bắt đầu | Tại điểm quyết định transition | Cửa đóng + nắp lọc OK trước khi chạy |
| **Invariant** | Đúng **suốt** thời gian ở state đó | Liên tục, song song (monitor) | Cửa luôn đóng trong lúc motor quay |
| **Postcondition** | Đúng **sau khi** hoàn thành | Khi thoát state | Nước đã xả hết trước khi vắt |

Project hiện tại xử lý cửa như **invariant** (mở van ngay, rồi giám sát song
song qua `EVT_ERROR` trong `WaitBits`) nhưng **thiếu guard**. Một hệ thống
tốt thường cần **cả hai**: guard chặn lúc bắt đầu, invariant canh chừng lúc
đang chạy.

> Đây cũng là hiểu nhầm phổ biến khi đọc code lần đầu: tưởng code kiểm tra
> điều kiện **tuần tự kiểu if-else** rồi mới hành động, trong khi thực tế nó
> **hành động ngay + canh chừng song song**. Hai mô hình khác bản chất.

### §2.5 Level flag so với edge event

| | Level | Edge |
| --- | --- | --- |
| Ý nghĩa | "Điều kiện **đang** đúng/sai" | "Sự kiện **vừa** xảy ra" |
| Được set/clear | Đối xứng, liên tục theo trạng thái vật lý | Set một lần khi xảy ra |
| Dùng làm **guard** | ✅ **Bắt buộc phải là level** | ❌ Không có giá trị "ngay lúc này" |
| Dùng để **phản ứng** | Được, nhưng dễ lặp | ✅ Tự nhiên |
| Ví dụ | `EVT_DOOR_CLOSED` | "nút vừa được nhấn" |

Quy tắc: **guard cần level, phản ứng cần edge.**

---

## §3 Sticky / Stale Level Flag

> Ca thực tế đầy đủ: xem **Bug #17** trong `HARDWARE_BUGS.md`. Dưới đây là
> phần nguyên lý tổng quát để nhận ra nó ở project khác.

### §3.1 Định nghĩa

Một cờ **level** (biểu diễn trạng thái hiện tại) nhưng được duy trì **bất đối
xứng**: có code set, không có code clear tương ứng. Kết quả: cờ mô tả **quá
khứ** thay vì hiện tại — **nó nói dối**.

```c
/* Level flag ĐÚNG (đối xứng)        Level flag SAI (sticky) */
if (cond) set(BIT);                  if (cond) set(BIT);
else      clear(BIT);                /* thiếu else → bit dính */
```

### §3.2 Vì sao khó phát hiện

1. **Không crash, không assert, không compiler warning** — hệ thống vẫn chạy,
   chỉ hành xử sai.
2. **Phụ thuộc lịch sử** — chỉ lộ ra sau một *chuỗi* thao tác cụ thể; test
   từng chức năng riêng lẻ không bao giờ thấy.
3. **Triệu chứng ở xa nguyên nhân** — bug ở module A, biểu hiện ở module B.
   → Phải trace theo **dữ liệu** (giá trị bit), không chỉ theo **luồng gọi hàm**.

### §3.3 Ba hướng khắc phục

| Cách | Nội dung | Đánh giá |
| --- | --- | --- |
| **A** | Cho producer clear ở nhánh ngược lại | Đơn giản, nhưng **sai nếu bit có nhiều producer** — sẽ xóa nhầm tín hiệu của module khác (vi phạm ownership) |
| **B** | **Tách cờ theo nguyên nhân**, mỗi bit một chủ sở hữu duy nhất | ✅ Đúng nhất về thiết kế |
| **C** | Consumer **dọn cờ trước khi "vũ trang"** (clear ngay đầu entry action) | Rẻ nhất, nhưng chỉ xử lý cờ *cũ* — **không thay thế được guard** |

Lời giải đầy đủ thường là **B (hoặc C) + guard** — hai thứ giải quyết hai vấn
đề khác nhau: cờ đối xứng đảm bảo thông tin không cũ, guard chặn khởi động
khi điều kiện chưa thỏa.

### §3.4 Quy tắc phòng tránh

1. Cờ level phải được duy trì **đối xứng ở mọi nhánh** của producer.
2. **Mỗi bit có đúng một chủ sở hữu** chịu trách nhiệm clear.
3. Không trộn ngữ nghĩa level và edge trong cùng một bit mà không ghi chú rõ.

---

## §4 Software Timer

### §4.1 Vấn đề được giải quyết

Cần "làm gì đó sau X ms" hoặc "làm định kỳ". Cách ngây thơ là tạo hẳn một
task cho mỗi việc — nhưng **mỗi task tốn một stack riêng** (project này: 1 KB
cho WashMgr, 512 B cho UITask). Bốn timer = bốn task = 2–4 KB RAM chỉ để đếm
giờ, cộng TCB (~100 B mỗi task) và gánh nặng cho scheduler.

Software Timer giải quyết bằng cách: **tất cả timer dùng chung MỘT task duy
nhất**. Thêm timer thứ 100 cũng **không tốn thêm stack nào**.

**Kích thước control block — tính từ source** (`timers.c`, struct
`tmrTimerControl`), trên Cortex-M4:

| Thành phần | Bytes |
| --- | --- |
| `pcTimerName` | 4 |
| `xTimerListItem` (`ListItem_t`) | 20 |
| `xTimerPeriodInTicks` | 4 |
| `pvTimerID` | 4 |
| `pxCallbackFunction` | 4 |
| `uxTimerNumber` (nếu `configUSE_TRACE_FACILITY == 1`) | 4 |
| `ucStatus` | 1 → padding 4 |
| **Tổng — cấu hình của project này** | **44** |

So sánh: 4 timer ở đây tốn **176 bytes** tổng cộng, thay vì 2–4 KB nếu làm
bằng 4 task riêng.

### §4.2 Kiến trúc bên trong

Software Timer **không phải phần cứng, không phải ngắt**:

```
┌──────────────┐   xTimerStart()    ┌──────────────────────┐
│ Task bất kỳ  │ ─────────────────► │ Timer Command Queue  │
│ hoặc ISR     │  (gửi LỆNH, không  │ configTIMER_QUEUE_   │
└──────────────┘   thực thi ngay!)  │ LENGTH slot          │
                                    └──────────┬───────────┘
                                               │ đọc lệnh
                                               ▼
                                    ┌──────────────────────┐
                                    │ Timer Service Task   │ ← MỘT TASK THẬT
                                    │ tên: "Tmr Svc"       │   có stack riêng
                                    │ - xử lý lệnh         │
                                    │ - quản 2 danh sách   │
                                    │   sắp theo hạn nổ    │
                                    │ - GỌI CALLBACK       │ ← callback chạy Ở ĐÂY
                                    └──────────────────────┘
```

Task này kernel **tự tạo** bên trong `vTaskStartScheduler()` khi
`configUSE_TIMERS = 1` — không thấy trong `main.c` nhưng nó có thật. Tên mặc
định `"Tmr Svc"`, đổi được qua `configTIMER_SERVICE_TASK_NAME`
(`timers.c:63-65`). Đây chính là tên bạn sẽ thấy trên timeline của SEGGER
SystemView.

Cấu hình của project này:

```c
#define configUSE_TIMERS              1
#define configTIMER_TASK_PRIORITY     ( 2 )     /* ngang UITask/SensorTask */
#define configTIMER_QUEUE_LENGTH      10
#define configTIMER_TASK_STACK_DEPTH  ( configMINIMAL_STACK_SIZE * 2 )
```

### §4.3 Tính bất đồng bộ của lệnh timer

`xTimerStart/Stop/Reset/ChangePeriod` **không thao tác lên timer**. Chúng chỉ
**gửi message** vào command queue rồi trả về ngay.

```c
xTimerStop(xFillTimeoutTimer, 0);
/* ← Tại dòng này timer VẪN ĐANG CHẠY.
     Nó chỉ dừng khi Timer Service Task được lên lịch và đọc lệnh. */
```

→ Đây là gốc rễ của rủi ro "timeout của state cũ bắn nhầm sang state mới":
không phải code viết ẩu, mà là **bản chất kiến trúc**.

Tham số thứ 2 **không phải** thời gian timer — nó là "chờ bao lâu nếu command
queue đang đầy":

```c
xTimerStart(xWashTimer, 0);      /* 0   = queue đầy thì trả pdFAIL NGAY  */
xTimerStart(xWashTimer, 100);    /* 100 = chờ tối đa 100 tick để có chỗ  */
```

**Chi tiết tinh tế cứu vãn độ chính xác — có bằng chứng source.** Khi gửi
lệnh START, FreeRTOS đính kèm **thời điểm phát lệnh** vào message. Bản thân
`xTimerStart` chỉ là một macro (`timers.h:507-508`):

```c
#define xTimerStart( xTimer, xTicksToWait ) \
    xTimerGenericCommand( ( xTimer ), tmrCOMMAND_START, ( xTaskGetTickCount() ), NULL, ( xTicksToWait ) )
/*                                                        ↑ tick ĐƯỢC CHỤP TẠI ĐÂY */
```

Và khi daemon xử lý lệnh, nó tính hạn nổ từ **con tem thời gian đó**, không
phải từ "bây giờ" (`timers.c:1006`):

```c
prvInsertTimerInActiveList( pxTimer,
                            xMessage.u.xTimerParameters.xMessageValue + pxTimer->xTimerPeriodInTicks,
                            xTimeNow,
                            xMessage.u.xTimerParameters.xMessageValue );
```

→ Dù daemon xử lý trễ, timer vẫn tính giờ từ lúc *gọi* `xTimerStart()`. Độ
trễ chỉ ảnh hưởng *thời điểm callback chạy*, **không làm sai chu kỳ**.

### §4.4 Ngữ cảnh chạy của callback

| Trong callback | Được phép? |
| --- | --- |
| API thường (`xEventGroupSetBits`, `xQueueSend`) | ✅ Có — và **phải** dùng bản thường |
| API `...FromISR()` | ❌ Sai — không ở trong ISR |
| `vTaskDelay()`, `xQueueReceive(..., portMAX_DELAY)` | ❌ **TUYỆT ĐỐI KHÔNG** |

### §4.5 Ràng buộc không block trong callback

Vì **tất cả** timer dùng chung một task: callback của timer A block 2 giây →
callback của B, C, D **đều trễ 2 giây theo**. Một timer hỏng làm hỏng cả hệ
thống timer.

```c
/* ❌ SAI NGHIÊM TRỌNG */
void BadCallback(TimerHandle_t t) {
    vTaskDelay(pdMS_TO_TICKS(100));            /* chặn daemon 100ms       */
    HAL_I2C_Mem_Write(..., HAL_MAX_DELAY);     /* có thể chặn VĨNH VIỄN   */
}
```

Nguyên tắc vàng: **callback phải ngắn, không block, chỉ "báo hiệu" rồi
thoát** — giống hệt nguyên tắc viết ISR.

### §4.6 Bảng API

| API | Từ ISR? | Ghi chú |
| --- | --- | --- |
| `xTimerCreate()` / `...Static()` | ❌ | Static = không dùng heap |
| `xTimerStart()` | `...FromISR()` | Nếu đang chạy thì reset |
| `xTimerStop()` | `...FromISR()` | Dừng, không xóa |
| `xTimerReset()` | `...FromISR()` | Đặt lại về 0 và chạy tiếp |
| `xTimerChangePeriod()` | `...FromISR()` | **Tự động start** nếu đang dừng |
| `xTimerDelete()` | ❌ | Handle thành vô hiệu |
| `xTimerIsTimerActive()` | ❌ | Đang chạy hay không |
| `pvTimerGetTimerID()` / `vTimerSetTimerID()` | ❌ | Gắn dữ liệu riêng — §4.8 |
| `xTimerGetPeriod()` / `xTimerGetExpiryTime()` | ❌ | Truy vấn |
| `pcTimerGetName()` | ❌ | Tên đặt lúc create |
| `xTimerPendFunctionCall()` | `...FromISR()` | Chạy hàm bất kỳ trong daemon — §4.9 |

**`xTimerReset()` — pattern hay dùng thực tế:** làm "watchdog mềm" hoặc phát
hiện end-of-frame. Ví dụ: mỗi ký tự UART đến thì `xTimerReset()`; nếu 50 ms
không có ký tự nào nữa → callback nổ → "gói tin đã kết thúc". Đây là cách
chuẩn cho giao thức không có ký tự kết thúc.

### §4.7 One-shot, auto-reload và drift

```c
xTimerCreate(name, period, uxAutoReload, id, callback);
/*                         ↑ pdFALSE = one-shot : nổ 1 lần rồi tự dừng
                             pdTRUE  = auto-reload: nổ lặp lại mãi        */
```

**Điểm quan trọng về độ chính xác — có bằng chứng source.** Auto-reload lên
lịch lần kế tiếp dựa trên **thời điểm hết hạn trước đó**, không phải lúc
callback chạy (`timers.c:699-707`):

```c
static void prvReloadTimer( Timer_t * const pxTimer,
                            TickType_t xExpiredTime,
                            const TickType_t xTimeNow )
{
    while( prvInsertTimerInActiveList( pxTimer,
                                       ( xExpiredTime + pxTimer->xTimerPeriodInTicks ),
                                       /*   ↑ cộng từ HẠN CŨ, không phải từ xTimeNow */
                                       xTimeNow, xExpiredTime ) != pdFALSE )
    { ... }
}
```

Thêm một chi tiết củng cố: trong `prvProcessExpiredTimer()`, việc **reload
được làm TRƯỚC khi gọi callback** (`timers.c:735` reload, `:742-743`
callback) → callback chạy chậm cũng không đẩy lịch lần sau.

```
Auto-reload (period 100):     nổ tại 100, 200, 300, 400...  ← chuẩn
Tự start lại trong callback:  nổ tại 100, 203, 307, 412...  ← DRIFT tích lũy
```

→ Cần định kỳ chính xác thì **luôn dùng auto-reload**, đừng tự `xTimerStart()`
lại trong callback.

### §4.8 Timer ID và callback dùng chung

Hai cách chuẩn để phân biệt "timer nào vừa nổ":

```c
/* Cách 1 — so sánh handle (đơn giản, cần biến global) */
static void Callback(TimerHandle_t xTimer) {
    if      (xTimer == xWashTimer)  { ... }
    else if (xTimer == xDrainTimer) { ... }
}

/* Cách 2 — dùng Timer ID (linh hoạt hơn, không cần global) */
xTimerCreate("WashTmr", ..., (void *)PHASE_WASH, Callback);

static void Callback(TimerHandle_t xTimer) {
    uint32_t phase = (uint32_t)pvTimerGetTimerID(xTimer);
    switch (phase) { ... }
}
```

Timer ID còn có thể trỏ tới một `struct` chứa dữ liệu riêng của từng timer →
cho phép viết **một callback tổng quát dùng lại được**, rất phổ biến trong
driver/library.

Project này dùng cách thứ ba: **không phân biệt gì cả** — set chung
`EVT_TIMEOUT` rồi để WashingManager nhìn state hiện tại mà suy ra (TD-10).
Gọn, nhưng chính là gốc rễ rủi ro ở §4.3.

### §4.9 xTimerPendFunctionCallFromISR — deferred interrupt processing

Cho phép ISR nói: *"chạy hàm này giùm tôi, nhưng trong ngữ cảnh task"*:

```c
/* Trong ISR — không làm việc nặng ở đây */
xTimerPendFunctionCallFromISR(MyHeavyWork, pvParam, ulParam, &xHigherPriorityTaskWoken);
/* → MyHeavyWork() chạy trong Timer Service Task, nơi được phép block */
```

Đây là pattern chuẩn để giữ ISR cực ngắn. Và đây cũng chính là cơ chế mà
`xEventGroupSetBitsFromISR()` dùng bên dưới — lý do nó "deferred và có thể
fail" (§1.5).

### §4.10 So sánh bốn cách hẹn thời gian

| Công cụ | Ngữ cảnh chạy | Độ chính xác | RAM thêm | Dùng khi |
| --- | --- | --- | --- | --- |
| **`vTaskDelay()`** | Task tự ngủ | Tick, **có drift** | 0 | Task tự nhịp, chấp nhận trôi |
| **`vTaskDelayUntil()`** | Task tự ngủ | Tick, **không drift** | 0 | Vòng lặp định kỳ chính xác |
| **Software Timer** | Timer Service Task | Tick, phụ thuộc priority daemon | ~44 B/timer | Nhiều sự kiện định giờ, không muốn tốn task |
| **Hardware Timer (TIM)** | ISR thật | **Rất cao** (µs–ns) | 0, tốn 1 peripheral | PWM, sampling, real-time ngặt |

Điểm mấu chốt: **Software Timer KHÔNG dùng cho real-time ngặt**. Cần chính
xác µs → phải dùng hardware timer.

### §4.11 Cấu hình priority trong project này

```
Priority 3 : WashingManager
Priority 2 : UITask, SensorTask, Timer Service Task   ← daemon ngang 2 task này
Priority 1 : OLEDTask
```

- Daemon **ngang priority** UITask/SensorTask → nếu 2 task đó đang chạy,
  daemon phải đợi. Với timer 10–20 giây thì trễ vài ms là vô nghĩa; nhưng
  project cần timer 10 ms thì đây sẽ là vấn đề thật.
- Daemon **thấp hơn** WashingManager (3) → khi callback set `EVT_TIMEOUT`,
  WashingManager lập tức được đánh thức và **preempt** daemon ngay tại dòng
  `xEventGroupSetBits()`. Phản hồi nhanh — thiết kế tốt.

**Khuyến nghị chung cho project khác:** đặt `configTIMER_TASK_PRIORITY`
**cao** (thường cao nhất hoặc gần cao nhất), vì callback ngắn và bạn muốn
timer đúng giờ. Đặt thấp là nguyên nhân phổ biến khiến "timer chạy không
chính xác".

### §4.12 Lỗi thường gặp

1. **Block trong callback** → treo toàn bộ hệ thống timer.
2. **Tưởng callback là ISR** → dùng nhầm `...FromISR()`.
3. **Tưởng `xTimerStop()` dừng ngay** → race condition.
4. **Không kiểm tra return value** của `xTimerStart/Stop`.
5. **Period = 0** → `configASSERT()` fail ngay lúc create.
6. **Tự start lại trong callback** thay vì auto-reload → drift tích lũy.
7. **`configTIMER_TASK_PRIORITY` quá thấp** → callback trễ, đổ lỗi nhầm cho FreeRTOS.
8. **Dùng handle sau `xTimerDelete()`** → dangling handle.
9. **Quên `configUSE_TIMERS = 1`** → daemon không tồn tại.

### §4.13 Câu hỏi phỏng vấn

1. *"Software timer callback chạy trong ngữ cảnh nào?"* → Timer Service Task,
   **không phải ISR**.
2. *"Vì sao không được block trong timer callback?"* → Daemon dùng chung cho
   mọi timer; block một cái làm trễ tất cả.
3. *"`xTimerStart()` có bắt đầu timer ngay không?"* → Không, nó gửi lệnh vào
   timer command queue; daemon xử lý sau.
4. *"Khác nhau giữa software timer và hardware timer?"* → Độ chính xác, ngữ
   cảnh chạy, chi phí tài nguyên, khả năng mở rộng số lượng.

### §4.14 Đề xuất cải tiến cho project này

`xEventGroupWaitBits()` có sẵn tham số timeout → hoàn toàn có thể **bỏ
`xFillTimeoutTimer`** và viết:

```c
bits = xEventGroupWaitBits(xSystemEvents, EVT_WATER_FULL | EVT_ERROR,
                           pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));
if      (bits & EVT_WATER_FULL) { /* nước đầy    */ }
else if (bits & EVT_ERROR)      { /* lỗi         */ }
else                            { /* TIMEOUT 10s */ }
```

→ Bỏ được 1 timer, bỏ luôn `EVT_TIMEOUT`, và **xóa sạch cả lớp bug
"timeout của state cũ rò rỉ sang state mới"** vì không còn bit timeout nào
tồn tại để mà rò rỉ. Đáng cân nhắc nghiêm túc khi quay lại sửa project.

---

### §4.15 Vòng đời timer và trạng thái Dormant

`timers.h:111` nói nguyên văn:

> *"Timers are created in the **dormant** state. The xTimerStart(),
> xTimerReset(), xTimerStartFromISR(), xTimerResetFromISR(),
> xTimerChangePeriod() and xTimerChangePeriodFromISR() API functions can all be
> used to transition a timer into the **active** state."*

Trạng thái nằm trong 3 bit của `ucStatus` (`timers.c:78-80`):

```c
#define tmrSTATUS_IS_ACTIVE                  ( 0x01U )
#define tmrSTATUS_IS_STATICALLY_ALLOCATED    ( 0x02U )
#define tmrSTATUS_IS_AUTORELOAD              ( 0x04U )
```

```text
        xTimerCreate()
              │
              ▼
      ┌───────────────┐   xTimerStart/Reset/ChangePeriod   ┌──────────────┐
      │   DORMANT     │ ────────────────────────────────►  │   ACTIVE     │
      │ (không đếm,   │ ◄────────────────────────────────  │ (nằm trong   │
      │  vẫn tồn tại) │           xTimerStop()             │  timer list) │
      └───────────────┘                                    └──────┬───────┘
              ▲                                                   │ hết hạn
              │  one-shot (pdFALSE) → tự về dormant               ▼
              └──────────────────────────────────────── gọi callback
                                                                  │
                        auto-reload (pdTRUE) → ở lại ACTIVE ◄──────┘
```

**Dormant ≠ bị xóa.** Timer vẫn chiếm 44 byte, handle vẫn hợp lệ,
`xTimerStart()` lại là chạy tiếp. Chỉ `xTimerDelete()` mới giải phóng.

Bằng chứng one-shot tự về dormant (`timers.c:733-739`):

```c
if( ( pxTimer->ucStatus & tmrSTATUS_IS_AUTORELOAD ) != 0U )
    prvReloadTimer( pxTimer, xNextExpireTime, xTimeNow );        /* ở lại active */
else
    pxTimer->ucStatus &= ( ( uint8_t ) ~tmrSTATUS_IS_ACTIVE );   /* → dormant    */
```

### §4.16 Hai danh sách timer và xử lý tick overflow

```c
PRIVILEGED_DATA static List_t * pxCurrentTimerList;    /* timers.c:145 */
PRIVILEGED_DATA static List_t * pxOverflowTimerList;   /* timers.c:146 */
```

**Vấn đề:** `TickType_t` 32-bit tràn về 0 sau ~49.7 ngày (@1 kHz). Giả sử tick
hiện tại `0xFFFFFF00`, start timer 1000 tick → hạn nổ `0xFFFFFF00 + 1000`
**tràn** thành `0x000002E7` — *nhỏ hơn* tick hiện tại. Nếu chỉ có một danh
sách sắp tăng dần, timer này bị coi là "đã quá hạn" và **nổ ngay** thay vì sau
1 giây.

**Giải pháp:** hạn nổ bị tràn → bỏ vào danh sách **overflow**
(`timers.c:916`); còn lại vào danh sách hiện tại (`:930`). Khi tick thật sự
tràn, kernel **hoán đổi hai con trỏ** (`prvSwitchTimerLists()`,
`timers.c:1086-1108`):

```c
/* The tick count has overflowed.  The timer lists must be switched.
 * If there are any timers still referenced from the current timer list
 * then they must have expired and should be processed before the lists
 * are switched. */
while( listLIST_IS_EMPTY( pxCurrentTimerList ) == pdFALSE )
{
    xNextExpireTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxCurrentTimerList );
    prvProcessExpiredTimer( xNextExpireTime, tmrMAX_TIME_BEFORE_OVERFLOW );
}

pxTemp = pxCurrentTimerList;
pxCurrentTimerList  = pxOverflowTimerList;   /* ← chỉ đổi CON TRỎ */
pxOverflowTimerList = pxTemp;
```

Đổi con trỏ là **O(1)** — không duyệt và sửa từng timer.

> **Kiến thức chuyển giao:** kỹ thuật kinh điển xử lý bộ đếm tràn, dùng y hệt ở
> `xDelayedTaskList` / `xOverflowDelayedTaskList` trong `tasks.c`. Hệ quả thực
> tế cho code của bạn: **đừng bao giờ viết `if (now > deadline)`** — luôn dùng
> `if ((now - start) >= period)`, vì phép trừ unsigned tự đúng khi tràn.

### §4.17 Vòng lặp của Timer Service Task

`timers.c:766-778`:

```c
for( ; configCONTROL_INFINITE_LOOP(); )
{
    xNextExpireTime = prvGetNextExpireTime( &xListWasEmpty );      /* ① hạn gần nhất */
    prvProcessTimerOrBlockTask( xNextExpireTime, xListWasEmpty );  /* ② ngủ hoặc nổ  */
    prvProcessReceivedCommands();                                  /* ③ dọn queue    */
}
```

Toàn bộ Software Timer gói gọn ở đây. Bước ② chính là chỗ gọi
`vQueueWaitForMessageRestricted()` đã phân tích ở §5.

Thứ tự ②→③ có ý nghĩa: **timer hết hạn được xử lý TRƯỚC lệnh trong queue**
trong cùng một vòng. Đây là cơ chế chính xác phía sau rủi ro ở §4.3 — một
`xTimerStop()` gửi "sát nút" vẫn có thể thua: callback đã nổ trước khi lệnh
stop được đọc.

### §4.18 Command queue đầy và lệnh gửi trước khi scheduler chạy

`timers.c:474-483`:

```c
if( xTaskGetSchedulerState() == taskSCHEDULER_RUNNING )
    xReturn = xQueueSendToBack( xTimerQueue, &xMessage, xTicksToWait );
else
    xReturn = xQueueSendToBack( xTimerQueue, &xMessage, tmrNO_DELAY );  /* ← ép 0 */
```

1. **Gọi `xTimerStart()` trước `vTaskStartScheduler()` là hợp lệ** — lệnh nằm
   chờ trong queue, daemon xử lý ngay khi scheduler chạy. Nhưng `xTicksToWait`
   **bị ép về 0** (chưa có scheduler thì không block được) → queue đầy lúc đó
   là `pdFAIL` im lặng.
2. **Queue đầy khi đang chạy** → tuỳ `xTicksToWait`.

**Trong project này:** cả 10 lời gọi đều dùng `0` và **không chỗ nào kiểm tra
giá trị trả về**:

```c
xTimerStart(xFillTimeoutTimer, 0);   /* washing_manager.c:76  — bỏ qua return */
xTimerStop(xWashTimer, 0);           /* washing_manager.c:306 — bỏ qua return */
/* ...8 chỗ nữa, tất cả đều bỏ qua */
```

`configTIMER_QUEUE_LENGTH = 10`, chỉ 4 timer, ít lệnh → thực tế chưa tràn.
Nhưng đây đúng là **lỗi thường gặp số 4** ở §4.12, đang tồn tại thật trong
code.

**Cách sizing đúng:** `configTIMER_QUEUE_LENGTH` ≥ số lệnh timer tối đa có thể
dồn lại **giữa hai lần daemon được chạy**. Daemon priority càng thấp thì con
số này càng lớn.

### §4.19 Cấp phát tĩnh

```c
TimerHandle_t xTimerCreateStatic( const char * pcTimerName,
                                  TickType_t xTimerPeriodInTicks,
                                  BaseType_t xAutoReload,
                                  void * pvTimerID,
                                  TimerCallbackFunction_t pxCallbackFunction,
                                  StaticTimer_t * pxTimerBuffer );  /* ← bạn cấp RAM */
```

`StaticTimer_t` (`FreeRTOS.h:3284-3295`) là struct "vỏ" cùng kích thước
`Timer_t` nhưng toàn field `Dummy` — chính sách **data hiding** của FreeRTOS:
bạn cấp đúng lượng RAM mà không nhìn thấy nội dung.

**Nhưng** `configSUPPORT_STATIC_ALLOCATION` không được khai trong
`FreeRTOSConfig.h` → mặc định `0` (`FreeRTOS.h:2804-2806`) →
**`xTimerCreateStatic()` không tồn tại trong build này.** 4 timer của project
đều cấp phát động từ heap_4.

Khi nào bắt buộc static: hệ thống an toàn (DO-178C, IEC 61508, automotive)
thường **cấm hoàn toàn dynamic allocation** vì không chứng minh được
worst-case. Static cũng loại bỏ khả năng `xTimerCreate()` trả `NULL` giữa
chừng.

### §4.20 Điều kiện biên dịch của xTimerPendFunctionCall

Buổi 1 dạy `xTimerPendFunctionCallFromISR()` như một API sẵn dùng. Kiểm tra
lại:

```c
#ifndef INCLUDE_xTimerPendFunctionCall
    #define INCLUDE_xTimerPendFunctionCall    0      /* FreeRTOS.h:283-284 */
#endif
```

Project không khai macro này → **mặc định 0 → hàm không được biên dịch**. Gọi
nó sẽ lỗi linker.

Đây là **cùng một điều kiện biên dịch** đã gặp ở §1.5 (`xEventGroupSetBitsFromISR`
cũng không được biên dịch vì chính lý do này). Kiến thức
về API vẫn đúng và vẫn đáng học cho project khác; muốn dùng thì phải thêm
`#define INCLUDE_xTimerPendFunctionCall 1`.

> **Bài học phương pháp:** "API tồn tại trong FreeRTOS" ≠ "API dùng được trong
> build này". Luôn kiểm tra macro `INCLUDE_*` / `configUSE_*` trước khi dạy
> hoặc trước khi viết code gọi nó.

### §4.21 API bổ sung

| API | Ghi chú |
| --- | --- |
| `vTimerSetReloadMode()` | **Đổi one-shot ↔ auto-reload lúc chạy** (`timers.c:1272`) |
| `xTimerGetReloadMode()` / `uxTimerGetReloadMode()` | Đọc chế độ hiện tại |
| `xTimerGetTimerDaemonTaskHandle()` | Lấy handle daemon — dùng đo stack (§4.26) |
| `xTimerGetStaticBuffer()` | Lấy lại buffer đã cấp cho timer static |
| `xTimerCreateTimerTask()` | Kernel tự gọi trong `vTaskStartScheduler()` — không gọi tay |
| `vApplicationDaemonTaskStartupHook()` | Hook chạy **một lần** khi daemon khởi động (`timers.c:762`), cần `configUSE_DAEMON_TASK_STARTUP_HOOK = 1` |

Cái cuối đáng chú ý: nó cho bạn một chỗ chạy code khởi tạo **sau khi scheduler
đã chạy** — hữu ích khi việc khởi tạo cần gọi API FreeRTOS blocking mà `main()`
không làm được.

### §4.22 Độ phức tạp và chi phí khi số timer lớn

Mục 1 khoe "thêm timer thứ 100 không tốn thêm stack nào". Đúng về **RAM**, sai
nếu suy ra là không tốn gì.

Timer được cắm vào danh sách **sắp xếp theo hạn nổ** bằng `vListInsert()`
(`list.c:192-196`):

```c
for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd );
     pxIterator->pxNext->xItemValue <= xValueOfInsertion;
     pxIterator = pxIterator->pxNext )
{
    /* There is nothing to do here, just iterating to the wanted
     * insertion position. */
}
```

→ **Chèn là O(n)**, n = số timer đang active. Với 100 timer active, mỗi
`xTimerStart()` duyệt trung bình 50 node — chạy **trong daemon**, làm trễ mọi
timer khác.

| Thao tác | Độ phức tạp | Ghi chú |
| --- | --- | --- |
| `xTimerStart/Reset/ChangePeriod` (chèn) | **O(n)** | Duyệt tìm vị trí sắp xếp |
| Lấy timer sắp nổ nhất | **O(1)** | Luôn là head |
| `xTimerStop` (gỡ) | **O(1)** | Doubly-linked list |
| Chuyển danh sách khi tick tràn | **O(1)** | Chỉ đổi con trỏ (§4.16) |

**Vì sao FreeRTOS chọn sorted list?** Tối ưu cho trường hợp phổ biến của hệ
nhúng nhỏ: ít timer (< 20), và thao tác nhiều nhất là "cái nào sắp nổ" — cái
đó O(1). Đánh đổi hợp lý; nhưng hàng trăm timer (server, network stack) thì
sorted list là điểm nghẽn.

**Cấu trúc khác dùng trong thực tế — kiến thức chuyển giao:**

| Cấu trúc | Chèn | Lấy min | Ai dùng |
| --- | --- | --- | --- |
| Sorted linked list | O(n) | O(1) | **FreeRTOS**, hệ nhỏ |
| Min-heap / priority queue | O(log n) | O(1) | Zephyr, nhiều RTOS khác |
| **Timer wheel** (hashed) | **O(1)** | O(1) amortized | Linux `timer_list`, Netty, Kafka |
| Red-black tree | O(log n) | O(1) | Linux `hrtimer` |

Timer wheel đáng biết: chia thời gian thành các "ô" như mặt đồng hồ, timer ném
vào ô tương ứng → chèn O(1) vì không cần sắp xếp. Trả giá bằng độ phân giải
theo ô và bộ nhớ cho mảng ô. Câu hỏi phỏng vấn hay ở vị trí system/backend.

### §4.23 Stack dùng cho callback

```c
#define configMINIMAL_STACK_SIZE       ( ( unsigned short ) 130 )   /* words */
#define configTIMER_TASK_STACK_DEPTH   ( configMINIMAL_STACK_SIZE * 2 )
```

→ 260 words × 4 = **1040 bytes**. Đó là **toàn bộ** stack mà mọi callback
timer dùng chung.

```c
/* ❌ Bom hẹn giờ */
void MyCallback(TimerHandle_t t) {
    char buffer[512];              /* nửa stack của daemon, cho MỘT biến */
    sprintf(buffer, "...");        /* sprintf tự nó cũng ngốn stack      */
}
```

Cái độc ác: **stack tràn ở một task bạn chưa từng viết** ("Tmr Svc"). Chưa bật
`configCHECK_FOR_STACK_OVERFLOW` thì triệu chứng là HardFault ngẫu nhiên hoặc
dữ liệu hỏng ở nơi hoàn toàn không liên quan — bạn sẽ tìm bug trong code của
mình mãi không ra.

**Quy tắc:**

- Callback chỉ dùng biến local nhỏ. Cần buffer lớn → `static` hoặc buffer cấp
  sẵn ở ngoài.
- Callback gọi `printf`/`sprintf`/hàm float → cộng thêm vài trăm byte stack →
  phải tăng `configTIMER_TASK_STACK_DEPTH`.
- Đo thật, đừng đoán — xem §4.26.

Callback của project chỉ có một dòng `xEventGroupSetBits()` → an toàn tuyệt
đối. Nhưng đây là lý do nguyên tắc "callback ngắn" không chỉ về *thời gian*,
mà còn về *stack*.

### §4.24 Reentrancy và chia sẻ dữ liệu

Vì **mọi** callback chạy trong **một** task duy nhất:

**✅ Hai callback KHÔNG BAO GIỜ chạy đồng thời** — tuần tự hoá tự nhiên.

```c
static int s_sharedCounter;      /* chia sẻ giữa callback A và callback B */

void CallbackA(TimerHandle_t t) { s_sharedCounter++; }   /* KHÔNG cần mutex */
void CallbackB(TimerHandle_t t) { s_sharedCounter--; }   /* KHÔNG cần mutex */
```

Lợi ích **miễn phí** rất ít người tận dụng: gom các thao tác trên một tài
nguyên vào các timer callback thì tự động loại bỏ nhu cầu đồng bộ giữa chúng —
cùng nguyên lý với "single-threaded event loop" của Node.js / Redis.

**❌ Nhưng callback KHÔNG được bảo vệ khỏi các TASK khác.** Daemon prio 2;
WashingManager prio 3 **preempt được** nó bất cứ lúc nào, kể cả giữa chừng
callback (chứng minh ở §5.8).

```c
static uint32_t s_sharedWithTask;   /* callback đọc/ghi, WashingManager cũng vậy */

void Callback(TimerHandle_t t) {
    s_sharedWithTask++;             /* ⚠ đọc-sửa-ghi KHÔNG nguyên tử         */
}                                   /*   WashingManager có thể chen vào giữa */
```

| Chia sẻ giữa | Cần đồng bộ? |
| --- | --- |
| Callback ↔ Callback | ❌ Không — cùng một task, tuần tự |
| Callback ↔ Task | ✅ Có |
| Callback ↔ ISR | ✅ Có (critical section / `...FromISR`) |

Tốt nhất: **đừng chia sẻ biến** — gửi message qua queue/event group, đúng như
project này đang làm.

### §4.25 Tám pattern thực chiến

**① Debounce bằng timer** (thay cho đếm tick trong ISR)

```c
void EXTI_Handler(void) {                       /* ISR chỉ reset timer */
    xTimerResetFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);
}
void DebounceCallback(TimerHandle_t t) {        /* one-shot 20ms */
    ButtonEvent_t e = BSP_Button_Read();
    xQueueSend(xButtonQueue, &e, 0);
}
```

Nhiễu bật-tắt liên tục → mỗi lần đều reset → callback chỉ nổ khi tín hiệu **đã
yên 20 ms**. Sạch hơn cách project đang dùng.

**② Watchdog mềm — phát hiện "cái gì đó NGỪNG xảy ra"**

```c
void OnHeartbeatReceived(void) { xTimerReset(xLinkWatchdog, 0); }
void LinkTimeoutCallback(TimerHandle_t t) { xEventGroupSetBits(ev, EVT_LINK_LOST); }
```

Pattern quan trọng nhất danh sách: **timer là công cụ duy nhất phát hiện được
sự VẮNG MẶT của sự kiện.** Queue/semaphore chỉ báo được cái *đã xảy ra*.

**③ Timeout guard cho phase** — chính là `xFillTimeoutTimer` của project.

**④ End-of-frame cho giao thức không có ký tự kết thúc** — mỗi byte UART đến
thì reset; im lặng 50 ms = hết gói. Cách chuẩn cho Modbus RTU.

**⑤ Retry với exponential backoff**

```c
void RetryCallback(TimerHandle_t t) {
    uint32_t attempt = (uint32_t)pvTimerGetTimerID(t);
    if (TrySend() != OK && attempt < 5) {
        vTimerSetTimerID(t, (void *)(attempt + 1));
        xTimerChangePeriod(t, pdMS_TO_TICKS(100 << attempt), 0);  /* tự start lại */
    }
}
```

`xTimerChangePeriod()` **tự động start** timer đang dormant — đúng thứ cần ở
đây, nhưng là bẫy nếu bạn chỉ định đổi period.

**⑥ Chuỗi one-shot — state machine chạy bằng timer.** Một callback, Timer ID
làm "bước hiện tại", mỗi lần nổ thì đặt period mới và start bước kế. Thay được
cả một task cho chuỗi hành động đơn giản.

**⑦ Rate limiting / throttle** — timer active nghĩa là "đang trong thời gian
khoá":

```c
if (xTimerIsTimerActive(xThrottleTimer) == pdFALSE) {
    DoExpensiveThing();
    xTimerStart(xThrottleTimer, 0);   /* khoá 1s */
}
```

**⑧ Blink pattern không chặn** — auto-reload timer đảo trạng thái LED. Thay
được toàn bộ các vòng `HAL_Delay()` nhấp nháy.

### §4.26 Kỹ thuật debug

FreeRTOS có 5 trace hook riêng cho timer (`FreeRTOS.h:860-877`):

| Hook | Bắn khi |
| --- | --- |
| `traceTIMER_CREATE` | Tạo thành công |
| `traceTIMER_CREATE_FAILED` | Hết heap → `xTimerCreate()` trả `NULL` |
| `traceTIMER_COMMAND_SEND` | Gửi lệnh vào queue — **thấy được lệnh bị `pdFAIL`** |
| `traceTIMER_COMMAND_RECEIVED` | Daemon đọc được lệnh — **đo được độ trễ queue** |
| `traceTIMER_EXPIRED` | Timer nổ, ngay trước khi gọi callback |

Cặp `COMMAND_SEND` → `COMMAND_RECEIVED` đo được **chính xác daemon trễ bao
lâu** — trả lời câu "priority daemon đặt thấp có sao không" bằng số liệu thay
vì suy đoán. Project đã có SEGGER SystemView (§B.8), chỉ cần nhìn
task `"Tmr Svc"` trên timeline.

**Đo stack thật của daemon:**

```c
TaskHandle_t hDaemon = xTimerGetTimerDaemonTaskHandle();
UBaseType_t freeWords = uxTaskGetStackHighWaterMark(hDaemon);
/* Trả về số WORD còn dư ít nhất từ trước tới giờ. Gần 0 = sắp tràn. */
```

Cách duy nhất trả lời "1040 byte có đủ không" mà không phải đoán.

### §4.27 Test logic phụ thuộc thời gian

**Vấn đề:** chu trình giặt của project là 10 s + 20 s + 15 s + 25 s = 70 s cho
MỘT test case. Test tự động chạy thời gian thật thì không ai chạy nó thường
xuyên → test chết yểu. Tệ hơn: timing thật trên máy build không ổn định →
test **flaky**, dạy người ta bỏ qua kết quả đỏ.

**Nguyên tắc cốt lõi — tách "quyết định" khỏi "chờ đợi":**

```text
❌ Trộn lẫn — không test được
   ┌─────────────────────────────────────────┐
   │ chờ 20s → rồi quyết định chuyển state    │
   └─────────────────────────────────────────┘

✅ Tách đôi
   ┌──────────────┐        ┌──────────────────────────┐
   │ Timer: chờ   │ ─────► │ Logic thuần: nhận sự kiện│
   │ (khó test)   │ sự kiện│ → quyết định (dễ test)   │
   └──────────────┘        └──────────────────────────┘
```

Phần "chờ" giao cho RTOS — không cần test, FreeRTOS đã test rồi. Phần "quyết
định" là code của bạn — test phần đó, và nó không cần thời gian trôi.

**`washing_sm.c` đã testable sẵn — kiểm chứng bằng include chain:**

```text
washing_sm.c → washing_sm.h → app_types.h → <stdint.h>
             → <stddef.h>
```

**Không có một dòng FreeRTOS nào** — đúng Design Rule #3 ("Drivers must not
depend on FreeRTOS") áp dụng cho tầng logic. File này biên dịch và chạy được
trên PC. Nó còn có sẵn dependency injection qua `WashingSM_Callbacks_t` (con
trỏ hàm, doc ghi *"May be NULL"* — test càng đơn giản).

**Test thật, chạy trên PC bằng gcc, mất vài mili giây:**

```c
/* test_washing_sm.c — biên dịch: gcc test_washing_sm.c washing_sm.c -o test */
#include "washing_sm.h"
#include <assert.h>

static int s_drainCalled;
static void FakeOnEnterDrain(void) { s_drainCalled++; }

static void test_wash_done_goes_to_drain(void) {
    WashingSM_Callbacks_t cb = { .onEnterDrain = FakeOnEnterDrain };
    WashingSM_Init(&cb);

    WashingSM_ProcessEvent(WASH_EVT_START_WASH);
    WashingSM_ProcessEvent(WASH_EVT_WATER_FULL);
    assert(WashingSM_GetState() == WASH_STATE_WASHING);

    WashingSM_ProcessEvent(WASH_EVT_WASH_DONE);      /* ← thay cho "chờ 20s" */
    assert(WashingSM_GetState() == WASH_STATE_DRAIN);
    assert(s_drainCalled == 1);
}
```

Dòng mấu chốt: `WASH_EVT_WASH_DONE` **thay thế 20 giây chờ**. Với state
machine, "thời gian trôi" chỉ là một sự kiện như mọi sự kiện khác — sự kiện
thì gửi được tức thì.

> **Kiến thức chuyển giao:** kiến trúc *table-driven state machine + callback
> injection* không chỉ sạch về thiết kế — nó **là điều kiện để test được**.
> Đây là lý do thật khiến kỹ sư giỏi tách tầng: tầng không phụ thuộc RTOS thì
> test được trên PC trong mili giây.

**Năm kỹ thuật, theo thứ tự nên ưu tiên:**

| # | Kỹ thuật | Test được gì | Chi phí |
| --- | --- | --- | --- |
| 1 | Tách logic thuần, test trên PC | Transition table, guard, thứ tự callback | Rẻ nhất nếu kiến trúc đúng |
| 2 | Rút ngắn period khi build test | Tích hợp thật, nhưng nhanh | Rẻ, không test được timing thật |
| 3 | Tiêm đồng hồ giả (fake clock) | Code gọi trực tiếp `xTaskGetTickCount()` | Trung bình |
| 4 | FreeRTOS simulator port (Win/POSIX) | Cả hệ RTOS, trên PC | Cao |
| 5 | Test trên board thật | Mọi thứ kể cả phần cứng | Đắt, chậm, khó tự động hoá |

Kỹ thuật 2:

```c
#ifdef UNIT_TEST
    #define WASH_CYCLE_DURATION_MS   20      /* 20ms thay vì 20000ms */
#else
    #define WASH_CYCLE_DURATION_MS   20000
#endif
```

Bẫy: rút ngắn quá thì **lộ ra race condition không có ở tốc độ thật**, hoặc
ngược lại **che mất** race chỉ xuất hiện khi chậm.

Kỹ thuật 3:

```c
/* ❌ Không test được — cứng nhắc gọi API kernel */
uint32_t elapsed = xTaskGetTickCount() - s_startTick;

/* ✅ Test được */
typedef uint32_t (*GetTickFn_t)(void);
static GetTickFn_t s_getTick = xTaskGetTickCount;   /* test đổi thành hàm giả */
uint32_t elapsed = s_getTick() - s_startTick;
```

Test "du hành thời gian" bằng hàm giả trả về giá trị bất kỳ — kể cả giá trị
sát điểm tràn `0xFFFFFF00`, thứ mà chờ thật phải mất 49.7 ngày mới gặp. Cách
duy nhất test được lớp bug overflow ở §4.16.

Kỹ thuật 4: FreeRTOS có port chạy trên PC (Windows MSVC/MinGW, POSIX/Linux) —
kernel thật chạy trên máy tính, test tương tác nhiều task không cần board.
Trả giá: timing trên PC không giống MCU, chỉ tin phần *logic*, đừng tin phần
*thời gian*.

**Liên quan §4.23:**

```c
#define configCHECK_FOR_STACK_OVERFLOW   0      /* FreeRTOSConfig.h:64 */
```

Project **đang tắt** kiểm tra tràn stack → bẫy "callback ngốn stack daemon" ở
§4.23 sẽ **không có cảnh báo nào**, chỉ HardFault hoặc dữ liệu hỏng ngẫu
nhiên. Khi viết callback phức tạp hơn, nên bật `2` (kiểm tra pattern, chính
xác hơn `1`). Tương tự `configGENERATE_RUN_TIME_STATS = 0` → chưa đo được %
CPU từng task.

### §4.28 So sánh với các hệ khác

> Phần này là kiến thức chung về các hệ khác, không trích từ source của
> project này — không gắn `file:dòng`.

**Bare-metal — 3 cách, và vì sao chúng không đủ:**

```c
/* ① Delay chặn — tệ nhất, CPU quay vòng vô ích */
HAL_Delay(20000);

/* ② Polling bộ đếm — không chặn, nhưng phình ra rất nhanh */
if (HAL_GetTick() - washStart >= 20000) { /* xong */ }

/* ③ Hardware timer + ISR — chính xác nhất, tốn 1 peripheral/việc */
void TIM2_IRQHandler(void) { flagWashDone = 1; }
```

Cách ② là cách bare-metal làm "software timer thủ công" — hoạt động, nhưng 10
việc định giờ thì `main()` biến thành khối `if` khổng lồ, mỗi cái một biến
`xxxStart` riêng, không quản lý nổi. **Software Timer của FreeRTOS chính là
cách ② được đóng gói tử tế**: một danh sách sắp xếp + một task duyệt nó. Chú
ý cách ② dùng phép **trừ** — đúng nguyên tắc chống tràn ở §4.16.

**Bảng so sánh quan trọng nhất — callback chạy ở ngữ cảnh nào:**

| Hệ | Cơ chế | Callback chạy trong | Được block? |
| --- | --- | --- | --- |
| **FreeRTOS** | `xTimerCreate` | **Task** ("Tmr Svc") | Không nên (làm trễ timer khác) |
| **Zephyr** | `k_timer` | **ISR** (ngắt hệ thống) | ❌ Tuyệt đối không |
| **Zephyr** | `k_work_delayable` | Thread (workqueue) | Có, tuỳ workqueue |
| **Linux kernel** | `timer_list` | **Softirq** | ❌ Không (không có ngữ cảnh process) |
| **Linux kernel** | `hrtimer` | Hardirq/softirq | ❌ Không |
| **Linux userspace** | `timerfd` | Thread của bạn (qua `poll`) | Có |
| **POSIX** | `timer_create` | Signal handler hoặc thread | Tuỳ `SIGEV_*` |

**Đây là cái bẫy chết người khi port code.** Callback timer viết cho FreeRTOS
(chạy trong task, được phép `xQueueSend`, thậm chí mutex) bê nguyên sang
Zephyr `k_timer` sẽ chạy **trong ISR** → gọi API blocking ở đó là crash hoặc
hỏng ngầm. Ngược lại, người quen Zephyr sang FreeRTOS dễ viết callback kiểu
ISR — dùng nhầm `...FromISR()` (lỗi thường gặp số 2, §4.12).

> **Câu hỏi đầu tiên phải hỏi khi dùng timer của BẤT KỲ hệ nào: "callback này
> chạy ở ngữ cảnh gì?"** Trả lời được thì tự suy ra mọi ràng buộc còn lại —
> được block không, gọi API nào, chia sẻ dữ liệu ra sao. Đây là câu hỏi làm
> trục xuyên suốt §4.

**Vì sao Linux cần tới HAI hệ thống timer** — đáng học vì cho thấy một đánh
đổi thiết kế thật:

| | `timer_list` | `hrtimer` |
| --- | --- | --- |
| Cấu trúc | Timer wheel | Red-black tree |
| Độ phân giải | Theo `jiffies` (~1–4 ms) | Nanosecond |
| Chèn | **O(1)** | O(log n) |
| Dùng cho | Timeout mạng, housekeeping — số lượng rất lớn, **đa số bị huỷ trước khi nổ** | Audio, `nanosleep`, POSIX timer — cần chính xác |

Ý tưởng cốt lõi: timeout thì nhiều nhưng hiếm khi nổ (gói tin thường đến
trước timeout, timer bị huỷ) → tối ưu cho *chèn và huỷ nhanh*, không cần
chính xác. Hẹn giờ để làm việc thì ít nhưng phải đúng giờ → chấp nhận O(log
n). FreeRTOS chỉ có một hệ, nghiêng về loại thứ hai, chấp nhận O(n) vì hệ
nhúng nhỏ ít timer.

### §4.29 Ranh giới của test và vùng mù

**Câu hỏi đặt ra:** unit test cho `washing_sm.c` chạy trên PC, đầy đủ đến đâu
cũng có một loại bug **không bao giờ bắt được**. Loại nào, và vì sao?

**Trả lời — chính là Bug #17 (`sensor_task.c:23-28`):**

```c
if (BSP_Sensor_IsDoorClosed()) {
    xEventGroupSetBits(xSystemEvents, EVT_DOOR_CLOSED);
} else {
    xEventGroupClearBits(xSystemEvents, EVT_DOOR_CLOSED);
    xEventGroupSetBits(xSystemEvents, EVT_ERROR);   /* set, KHÔNG BAO GIỜ clear */
}
```

Mở cửa → `EVT_ERROR` = 1 → đóng lại → bit vẫn = 1 → bấm Start → `WaitBits`
thấy bit sẵn → `WASH_EVT_ERROR` được gửi vào state machine.

**`washing_sm.c` xử lý sự kiện đó hoàn toàn đúng** — nhận `WASH_EVT_ERROR` ở
`FILL_WATER`, chuyển sang `ERROR`. Không transition nào sai. Test sẽ viết:

```c
WashingSM_ProcessEvent(WASH_EVT_ERROR);
assert(WashingSM_GetState() == WASH_STATE_ERROR);   /* ✅ PASS */
```

**Test xanh. Máy vẫn hỏng.** Vì bug không nằm ở "xử lý sự kiện ERROR thế
nào", mà ở "**tại sao sự kiện ERROR lại được sinh ra** khi cửa đang đóng" —
câu hỏi nằm hoàn toàn ngoài `washing_sm.c`, file đó không biết Event Group,
cảm biến, hay ai gọi nó tồn tại.

> **Nguyên tắc tổng quát: chỗ bạn tiêm phụ thuộc (seam) cũng chính là ranh
> giới vùng mù của test.** `washing_sm.c` tách khỏi FreeRTOS qua hai seam —
> đầu vào `WashEvent_t`, đầu ra callback. Test qua hai seam đó chỉ trả lời
> được *"cho đúng sự kiện, có chuyển đúng trạng thái không?"* — không trả lời
> được *"có sinh ra đúng sự kiện không?"*, nơi phần lớn bug nhúng thật sự sống.

| Loại bug | Nằm ở đâu | Test `washing_sm.c` bắt được? |
| --- | --- | --- |
| Transition sai, thiếu guard | `washing_sm.c` | ✅ Có |
| **Sự kiện thừa/sai/thiếu** (Bug #17) | `sensor_task.c` | ❌ Không |
| Race do lệnh timer bất đồng bộ (§4.3) | Giữa manager và daemon | ❌ Không |
| Chân GPIO đọc sai (Bug #14/#16) | Phần cứng | ❌ Không |
| Callback thật ghi nhầm chân | `washing_manager.c` | ❌ Không — test dùng callback giả |
| Priority sai, task đói CPU | Cấu hình RTOS | ❌ Không |
| Tràn stack, hết heap | Toàn hệ thống | ❌ Không |

**Cùng hình dạng lỗi còn một chỗ nữa** (`sensor_task.c:30-32`):

```c
if (BSP_Sensor_IsWaterFull()) {
    xEventGroupSetBits(xSystemEvents, EVT_WATER_FULL);
}
/* không có else → không bao giờ clear */
```

Cùng thuộc **Sticky/Stale Level Flag** (§3): nước đầy lúc IDLE → bit bật và
ở lại → bấm Start → bỏ qua giai đoạn cấp nước dù bể có thể vừa xả cạn. Xác
nhận nhận định của §3: khi một anti-pattern xuất hiện, tìm các thể hiện
khác của cùng hình dạng thay vì chỉ sửa chỗ đầu tiên. (Ghi nhận — chưa sửa,
theo quyết định gác bug lại của người dùng.)

**Tầng bị bỏ sót nên test bằng gì — kim tự tháp test cho hệ nhúng:**

| Tầng | Test cái gì | Chạy ở đâu | Bắt được Bug #17? |
| --- | --- | --- | --- |
| Unit | Logic thuần (`washing_sm.c`) | PC, mili giây | ❌ |
| **Component** | Tầng sinh sự kiện — cảm biến giả, kiểm tra **bit nào set/clear** | PC, cần tách BSP khỏi HAL | ✅ |
| Integration | Nhiều task chạy thật | Simulator / board | ✅ |
| Manual có kịch bản | Toàn hệ + phần cứng | Board | ✅ (cách đã tìm ra Bug #17 thật) |

Tầng Component đang thiếu và đáng đầu tư nhất:

```c
/* Cho cảm biến giả, kiểm tra CHÍNH XÁC bit nào được set/clear */
FakeSensor_SetDoorClosed(false);
SensorTask_PollOnce();                 /* tách vòng lặp khỏi thân xử lý */
assert(GetBits() & EVT_ERROR);

FakeSensor_SetDoorClosed(true);
SensorTask_PollOnce();
assert((GetBits() & EVT_ERROR) == 0);  /* ❌ FAIL → bắt được Bug #17 */
```

Cần một thay đổi kiến trúc nhỏ để viết được: **tách thân xử lý ra khỏi vòng
`while(1) + vTaskDelay()`**. Pattern chung: vòng lặp vô hạn và phần logic nên
là hai hàm khác nhau — vòng lặp không test được, logic thì có.

> **Bài học rút gọn:** test chứng minh code làm đúng điều nó được yêu cầu,
> không chứng minh nó được yêu cầu đúng điều. Test xanh 100% ở tầng state
> machine hoàn toàn tương thích với một cái máy giặt hỏng.

---

## §5 Tick, SysTick và Context Switch

> §4 mô tả tầng **logic** của Software Timer: Timer Service Task quản lý danh
> sách timer và gọi callback. Chương này mô tả tầng **vật lý** nằm dưới: cái
> gì tạo ra khái niệm "thời gian" cho toàn bộ FreeRTOS, và nguồn clock của nó
> đến từ đâu.

### §5.1 Hai tầng quản lý thời gian

| Tầng | Ai quản lý | Trách nhiệm |
| --- | --- | --- |
| **Thời gian** (vật lý) | **SysTick** — bộ đếm nằm trong lõi Cortex-M | Ngắt định kỳ, tạo "nhịp đập" cho hệ thống |
| **Logic** (§4) | **Timer Service Task** | Dựa vào nhịp đập, biết timer nào tới hạn → gọi callback |

Sai lầm phổ biến: nghĩ "Software Timer tự chạy". Không — nó **phụ thuộc hoàn
toàn vào SysTick**. Tắt SysTick thì mọi timer, mọi `vTaskDelay()`, mọi timeout
đứng im vĩnh viễn.

### §5.2 SysTick trong kiến trúc Cortex-M

SysTick **không** phải ngoại vi của STM32 (không phải TIM2/TIM3/…). Nó là bộ
đếm ngược **24-bit nằm trong chính lõi ARM Cortex-M**, thuộc chuẩn kiến trúc —
mọi chip Cortex-M của mọi hãng (ST, NXP, TI, Nordic…) đều có.

> **Vì sao điều này quan trọng:** đó là lý do FreeRTOS chỉ cần **một** port cho
> `ARM_CM4F` là chạy được trên mọi board Cortex-M4F, không cần biết hãng nào
> làm chip. Kernel không đụng tới ngoại vi riêng của hãng. Đây là ranh giới
> portability điển hình — đáng học để tự thiết kế lớp trừu tượng sau này.

### §5.3 Chuỗi từ dao động điện tử tới callback

```text
HSI — bộ dao động RC NỘI, 16 MHz, nằm trong chip
   │
   ▼
SystemCoreClock = 16 MHz            (không dùng PLL trong project này)
   │
   ▼
SysTick — bộ đếm ngược trong lõi Cortex-M4
   │  nạp = configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ - 1
   │      = 16 000 000 / 1000 - 1 = 15999
   │  → đếm 16000 xung CPU thì tràn → bắn ngắt
   ▼
Ngắt SysTick, đều đặn mỗi 1 ms
   │
   ▼
xPortSysTickHandler()               (chính là SysTick_Handler — xem §5)
   │
   ▼
xTaskIncrementTick()
   │  ① tăng biến đếm tick toàn cục
   │  ② quét danh sách task đang Blocked-with-timeout
   │  ③ task nào hết hạn → chuyển sang Ready
   ▼
Nếu Timer Service Task nằm trong số đó
   → được lên lịch chạy → gọi Timer_PhaseCallback()
```

### §5.4 Nguồn clock trong project này

Kiểm chứng bằng `SystemClock_Config()` trong `Core/Src/main.c`:

```c
RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;  /* ← chọn HSI  */
RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;            /* ← không PLL */
RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
```

→ **HSI = High Speed Internal**, dao động RC **nội** ~16 MHz, không cần thạch
anh ngoài. Board STM32F411E-DISCO *có* chân cho thạch anh ngoài (HSE — `.ioc`
vẫn khai `HSE_VALUE = 25000000` như hằng số dự phòng) nhưng project **không
chọn dùng**. Không PLL nên `SystemCoreClock = 16 MHz` thẳng từ HSI (khớp
`RCC.SYSCLKFreq_VALUE=16000000` trong `Freertos.ioc`).

**HSI vs HSE — bảng cần nhớ khi chọn cho project khác:**

| | HSI (nội) | HSE (thạch anh ngoài) |
| --- | --- | --- |
| Linh kiện ngoài | Không | Thạch anh + 2 tụ |
| Sai số điển hình | ~1 % (trôi theo nhiệt độ) | 20–50 ppm (~0.005 %) |
| Thời gian khởi động | Rất nhanh (µs) | Chậm hơn (ms) |
| Dùng khi | Học tập, timing không khắt khe | USB, CAN, Ethernet, RTC, đo thời gian dài |

Hệ quả cho project này: HSI trôi 1 % → chu trình giặt 20 s có thể lệch ±0.2 s.
Không quan trọng ở quy mô này. Nhưng nếu cần đếm giờ chính xác nhiều ngày, hoặc
cần USB (bắt buộc sai số < 0.25 %), **phải** chuyển sang HSE.

### §5.5 Ba điểm mấu chốt trong source

**(a) Cấu hình phần cứng SysTick** — `OS/FreeRTOS/portable/GCC/ARM_CM4F/port.c:826-828`:

```c
portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG
                            | portNVIC_SYSTICK_INT_BIT
                            | portNVIC_SYSTICK_ENABLE_BIT );
```

Với `configCPU_CLOCK_HZ = SystemCoreClock` và `configTICK_RATE_HZ = 1000`
(`FreeRTOSConfig.h:53-54`).

**(b) Nối vào vector ngắt thật** — `OS/FreeRTOS/include/FreeRTOSConfig.h:130`:

```c
#define xPortSysTickHandler   SysTick_Handler
```

→ Đây **chính là** pattern weak-symbol đã gặp ở Bug #4: hàm trong port layer
được đặt tên thành symbol mà bảng vector ARM đang trỏ tới. Nếu thiếu dòng
`#define` này, `SysTick_Handler` mặc định (weak, rỗng) của ST sẽ chạy →
**tick không bao giờ tăng → toàn hệ thống treo**. Một trong những lỗi cấu hình
FreeRTOS kinh điển nhất.

**(c) Timer Service Task KHÔNG polling** — `OS/FreeRTOS/timers.c:817`:

```c
vQueueWaitForMessageRestricted( xTimerQueue, ( xNextExpireTime - xTimeNow ), xListWasEmpty );
```

Nó tự tính "còn bao lâu tới hạn" rồi **ngủ đúng bấy nhiêu**, không hề hỏi liên
tục "đã tới giờ chưa". Cơ chế đánh thức là **cơ chế block-with-timeout chung
của kernel** — y hệt `xQueueReceive(..., xTicksToWait)` mà bạn đã biết.

> **Kết luận quan trọng nhất của chương này:** Timer Service Task **không có phần
> cứng riêng, không có đặc quyền lịch trình nào**. Nó chỉ là một task bình
> thường biết tự tính thời gian ngủ. Mọi "phép màu" đều nằm ở SysTick +
> `xTaskIncrementTick()`.

### §5.6 configUSE_TICKLESS_IDLE

`FreeRTOS.h:2702-2703` cho biết mặc định là `0` nếu không khai, và project này
không khai → **tickless idle KHÔNG bật** → SysTick bắn đều 1000 lần/giây kể cả
khi mọi task đều đang ngủ.

Ở project chạy pin, người ta bật `configUSE_TICKLESS_IDLE = 1`: kernel tính
xem task nào sẽ dậy sớm nhất, **tắt SysTick**, cho MCU vào chế độ ngủ sâu, đặt
một nguồn đánh thức khác (thường là RTC), rồi khi dậy **bù lại số tick đã bỏ
lỡ**. Tiết kiệm điện rất lớn. Trả giá: timing kém chính xác hơn, và debug khó
hơn nhiều.

### §5.7 Độ phân giải tick

Mọi thứ liên quan thời gian trong FreeRTOS đều lượng tử hoá theo tick:

- Tick = 1 ms → timer **không bao giờ** hết hạn giữa chừng một tick, chỉ có thể
  hết hạn tại mốc 1 ms, 2 ms, 3 ms…
- `vTaskDelay(pdMS_TO_TICKS(5))` thực tế ngủ **4–5 ms** (tick đầu tiên có thể
  đã trôi một phần).
- Cần chính xác hơn 1 ms (PWM, điều khiển động cơ, giao thức có timing chặt)
  → **phải dùng hardware timer riêng** (TIM2…TIM14), không dùng software timer.

Tăng `configTICK_RATE_HZ` để tăng độ phân giải? Được, nhưng mỗi tick là một
ngắt + quét danh sách → tick 10 kHz tốn ~10× overhead CPU. 1000 Hz là điểm cân
bằng phổ biến nhất.

### §5.8 Phân tích một chuỗi context switch

**Đề bài:** `xWashTimer` (20 s) vừa hết hạn. `WashingManager` (prio 3) đang
Blocked trong `xEventGroupWaitBits`. `UITask` (prio 2) đang Running. Daemon
(prio 2) đang Blocked trên timer queue. Hỏi thứ tự chạy và số lần context
switch tới khi `Manager_OnEnterDrain()` thực thi.

**Priority thật của project** (`app_config.h:61-70`, `FreeRTOSConfig.h:76`):

| Task | Priority |
| --- | --- |
| WashingManager | 3 |
| UITask | 2 |
| SensorTask | 2 |
| **Tmr Svc (daemon)** | **2** |
| OLEDTask | 1 |

#### §5.8.1 Bước 1 — vì sao UITask nhường CPU

Daemon **bằng** UITask (2 == 2), không cao hơn. `tasks.c:4787` trong
`xTaskIncrementTick()`:

```c
/* ...a context switch should only be performed if the unblocked task's
 * priority is HIGHER than the currently executing task. */
if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )   /* 2 > 2 → FALSE! */
    xSwitchRequired = pdTRUE;
```

Nhánh này **không** kích hoạt. Cái làm nó switch là khối ngay dưới,
`tasks.c:4810-4813`:

```c
#if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
    if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) ) > 1U )
        xSwitchRequired = pdTRUE;     /* ← 2 task cùng prio 2 → round-robin */
```

`configUSE_TIME_SLICING` không khai trong project → mặc định `1`
(`FreeRTOS.h:2734`).

> **Bài học chuyển giao:** nếu tắt `configUSE_TIME_SLICING` (một tối ưu phổ
> biến để giảm switch thừa), daemon **sẽ nằm Ready chờ tới khi UITask tự
> block** → callback trễ tuỳ ý. Muốn timer tất định, **đặt
> `configTIMER_TASK_PRIORITY` cao hơn mọi task dùng timer** — đây chính là
> khuyến nghị chính thức của FreeRTOS, và project này đang **không** tuân thủ
> (xem §4.11).

#### §5.8.2 Bước 2 — daemon bị preempt giữa callback

Callback (`App/Common/Src/freertos_objects.c:49-53`):

```c
static void Timer_PhaseCallback(TimerHandle_t xTimer) {
    xEventGroupSetBits(xSystemEvents, EVT_TIMEOUT);
}
```

`xEventGroupSetBits()` bọc trong `vTaskSuspendAll()` … `xTaskResumeAll()`
(`event_groups.c:567` và `:639`). Bên trong vùng suspend, WashingManager được
gỡ block nhưng **chưa switch được** — scheduler đang treo — nên chỉ đặt cờ
(`tasks.c:4632-4636`):

```c
if( pxTCB->uxPriority > pxCurrentTCB->uxPriority )   /* 3 > 2 → TRUE */
    xYieldPendings[ 0 ] = pdTRUE;    /* hoãn yield tới khi resume */
```

Rồi `xTaskResumeAll()` thấy cờ → `taskYIELD_WITHIN_API()` → **switch ngay tại
đó**.

→ WashingManager chiếm CPU **trước khi `Timer_PhaseCallback()` kịp return**.
Daemon bị bỏ dở giữa lời gọi hàm và **bắt buộc phải được chạy lại sau** để kết
thúc callback + dọn dẹp `prvProcessExpiredTimer()` + quay lại vòng lặp block.

#### §5.8.3 Kết quả

| # | Từ → Đến | Nguyên nhân |
| --- | --- | --- |
| 1 | UITask → daemon | SysTick tick, **time-slicing** (prio 2 == 2) |
| 2 | daemon → WashingManager | `xTaskResumeAll()` trong `xEventGroupSetBits()`, **giữa callback** |
| 3 | WashingManager → (một task prio 2) | `Manager_OnEnterDrain()` xong, `WaitBits` block lại |
| 4 | … → daemon (nếu #3 chưa phải daemon) | round-robin |

→ **Tối thiểu 4 lần**, không phải 2.

#### §5.8.4 Thứ tự giữa hai task cùng priority

Phụ thuộc con trỏ `pxIndex` của ready-list — `listGET_OWNER_OF_NEXT_ENTRY`
luân phiên qua danh sách (`tasks.c:189-191`):

```c
/* listGET_OWNER_OF_NEXT_ENTRY indexes through the list, so the tasks of
 * the same priority get an equal share of the processor time. */
listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );
```

> **Nguyên tắc:** thứ tự giữa các task **cùng priority là không tất định** theo
> góc nhìn thiết kế. Đừng bao giờ viết code phụ thuộc vào nó. Cần tất định →
> **tách priority**, không dựa vào round-robin.

Thêm một chi tiết: `Manager_OnEnterDrain()` còn gọi
`xQueueSend(xDisplayQueue, …)`, nhưng OLEDTask prio 1 **thấp hơn** nên không
cướp CPU → màn hình chỉ đổi sau khi cả prio 3 lẫn prio 2 rảnh hết. Đây là lý do
đặt task hiển thị ở priority thấp nhất (Part I, Phase 6).

### §5.9 Ranh giới giữa scheduler và logic ứng dụng

Câu hỏi thật đã đặt ra: *"Đang giặt rồi thì nhấn nút phải vô tác dụng chứ, sao
UITask vẫn chạy được?"* Đây là chỗ rất dễ trộn lẫn hai tầng:

| Tầng | Câu hỏi | Ai quyết định |
| --- | --- | --- |
| **Scheduler** | UITask có được cấp CPU không? | Priority + Blocked/Ready. **Không biết gì về "đang giặt"** |
| **Ứng dụng** | Lệnh đó có tác dụng không? | State machine của WashingManager |

Scheduler chỉ thấy: UITask prio 2, có message vào `xButtonQueue` → Ready → tới
lượt thì chạy. Nhấn nút lúc nào cũng vậy, không có ngoại lệ.

**Vậy "vô tác dụng" được cài đặt ở đâu?** Ba mảnh ghép trong code thật:

**(1)** UITask **vẫn chạy đủ**, vẫn gửi lệnh (`ui_task.c:49-68`):

```c
case BTN_EVENT_CONFIRM:
    cmd.command = CMD_CONFIRM;
    break;
...
xQueueSend(xCommandQueue, &cmd, 0);     /* ← vẫn gửi bình thường! */
```

**(2)** WashingManager ở state WASHING **không hề đọc `xCommandQueue`**
(`washing_manager.c:324`) — nó chỉ đọc ở IDLE / FINISH / ERROR
(`:282-288`). Lệnh **nằm im trong queue**, không ai lấy.

> *Cập nhật 2026-10-10:* đoạn mã ở (1) là bản **sau** khi UITask được đơn giản hoá
> thành bộ dịch không trạng thái (`CMD_NAVIGATE` / `CMD_CONFIRM`); trước đó nó gửi
> `CMD_START_WASH` / `CMD_START_SPIN` theo con trỏ menu mà UITask từng giữ. Lập
> luận của mục này không đổi — chỉ tên lệnh và vị trí con trỏ.

**(3)** Vì nằm im nên nó sẽ "nổ chậm" khi máy về IDLE (bấm nút lúc đang giặt →
giặt xong tự khởi động chu trình mới!). Code chặn đúng chỗ đó
(`washing_manager.c:71`, và tương tự trong `Manager_OnEnterFinish()` /
`Manager_OnEnterError()`):

```c
/* Clear any stale commands that queued up while the machine was running. */
(void)xQueueReset(xCommandQueue);
```

> **Pattern tổng quát (dùng được ở mọi project):** *lọc sự kiện ở phía
> consumer (state machine), không phải ở phía producer (task nhập liệu).*
> Producer cứ đơn giản và "ngu ngốc"; toàn bộ tri thức về ngữ cảnh nằm gọn một
> chỗ trong state machine. Đúng Design Rule #1 của project. Nếu làm ngược —
> cho UITask tự hỏi "máy đang giặt à?" — thì logic trạng thái bị rải ra 2 nơi,
> và sẽ có lúc hai nơi bất đồng.

**Hệ quả quan sát được trên board (đã đổi 2026-10-10):** nhấn nút NAVIGATE (PA0)
lúc đang giặt thì **con trỏ menu đứng im** — WashingManager không đọc
`xCommandQueue` trong lúc chạy chu trình, nên lệnh `CMD_NAVIGATE` nằm chờ rồi bị
`xQueueReset` xoá.

*Lịch sử:* trước đây UITask giữ con trỏ và gửi thẳng `xDisplayQueue` không qua
WashingManager, nên con trỏ **vẫn di chuyển** lúc đang giặt — kèm một lỗi: dòng
trạng thái bị đổi thành `IDLE`, và con trỏ nhảy ngược ở lần chuyển trạng thái kế
tiếp. Câu "feature hay bug là quyết định thiết kế, không phải tai nạn" ở bản cũ của
mục này hoá ra đúng theo nghĩa khác: đó là **bug**, do con trỏ có hai chủ. Phân
tích đầy đủ và cách sửa ở START_HERE Tầng 3 §3.6.

### §5.10 Lỗi thường gặp liên quan tick

| Lỗi | Hậu quả | Cách tránh |
| --- | --- | --- |
| Thiếu `#define xPortSysTickHandler SysTick_Handler` | Tick không tăng → treo toàn hệ thống ngay sau `vTaskStartScheduler()` | Kiểm tra `FreeRTOSConfig.h` khi port sang board mới |
| HAL cũng cài `SysTick_Handler` riêng | Hai bên tranh nhau vector | Cho HAL dùng TIM khác làm timebase (`HAL_InitTick`) |
| Dùng số tick thay vì `pdMS_TO_TICKS()` | Đổi `configTICK_RATE_HZ` là sai toàn bộ timing | Luôn dùng `pdMS_TO_TICKS()` |
| Kỳ vọng độ chính xác < 1 tick | Timing sai lệch không giải thích được | Hardware timer cho yêu cầu sub-tick |
| Tràn biến tick (`TickType_t` 32-bit ≈ 49.7 ngày @1 kHz) | So sánh thời gian sai sau ~7 tuần | Dùng phép trừ `(now - then)` chứ đừng so sánh `>` trực tiếp; kernel tự xử lý qua overflow list |

### §5.11 Câu hỏi phỏng vấn

1. *Điều gì xảy ra nếu quên nối `SysTick_Handler` vào FreeRTOS?* → Tick không
   tăng, không task nào rời trạng thái Blocked, hệ thống đứng im sau khi khởi
   động scheduler.
2. *Software timer có dùng phần cứng timer nào không?* → Không. Nó chỉ là danh
   sách sắp xếp trong RAM, được duyệt bởi một task thường, lấy nhịp từ SysTick.
   Một hệ thống có thể có hàng trăm software timer nhưng chỉ tốn một SysTick.
3. *Vì sao callback timer không được block?* → Nó chạy trong ngữ cảnh Timer
   Service Task; block ở đó làm đóng băng **mọi** timer khác (xem §4.5).
4. *Tick 1 kHz với tick 10 kHz khác nhau thế nào?* → Độ phân giải tốt hơn 10
   lần, overhead ngắt cao hơn ~10 lần. Cân bằng theo yêu cầu thật.
5. *Task A và B cùng priority, A đang chạy, B vừa Ready. Khi nào B chạy?* →
   Nếu `configUSE_TIME_SLICING = 1`: ở tick kế tiếp. Nếu `= 0`: chỉ khi A tự
   block hoặc gọi `taskYIELD()`.

---

# PHẦN B — Nhật ký xây dựng

> Ghi chép theo tiến trình xây dựng project, Phase 1–8. Phần này mô tả *cách
> khái niệm được áp dụng tại thời điểm của từng phase*; kiến thức đầy đủ và
> có thẩm quyền về mỗi khái niệm nằm ở Phần A.

---

## §B.1 Phase 1 — Project Setup & FreeRTOS Tasks

### Problem Being Solved

Bắt đầu từ một project STM32 trống. Cần thiết lập nền tảng: RTOS chạy được,
cấu trúc thư mục rõ ràng, và proof-of-life bằng 3 task blink chạy song song.

---

### FreeRTOS Concept: Task

#### ① Problem it solves

Trong lập trình truyền thống (bare-metal), chỉ có một luồng thực thi. Nếu muốn
hai việc xảy ra "cùng lúc" phải viết state machine thủ công hoặc dùng interrupt.
Task trong FreeRTOS cho phép chia chương trình thành nhiều luồng độc lập, mỗi
luồng tưởng như có CPU riêng.

#### ② Tại sao cần trong project này

Máy giặt cần xử lý đồng thời: đọc cảm biến, xử lý nút bấm, điều khiển motor,
cập nhật OLED. Nếu bare-metal phải tự quản lý từng việc trong một vòng lặp —
rất dễ bug và khó mở rộng.

#### ③ Tại sao alternatives bị loại

- **Super-loop (while 1):** Không phản ứng kịp thời, khó thêm feature mới
- **Interrupt-only:** ISR không nên chứa logic phức tạp, không thể block chờ

#### ④ Runtime behavior

```
vTaskStartScheduler() gọi → FreeRTOS chiếm quyền điều khiển
    → Tạo Idle Task (priority 0)
    → Bật SysTick interrupt (configTICK_RATE_HZ = 1000 → cứ 1ms một tick)
    → Chạy task có priority cao nhất đang Ready
```

#### ⑤ Task state transitions

```
xTaskCreate() → [Ready]
                   │ scheduler chọn
                   ▼
              [Running]  ←──────────────────┐
                   │ vTaskDelay()/block      │ preempt/yield
                   ▼                         │
              [Blocked]  ──timeout/signal──► [Ready]
                   │ vTaskSuspend()
                   ▼
             [Suspended] ──vTaskResume()──► [Ready]
```

Chỉ **một** task ở trạng thái Running tại một thời điểm (single-core).

#### ⑥ Context switching behavior

Khi SysTick fire (mỗi 1ms), FreeRTOS kiểm tra xem có task Ready nào có
priority cao hơn task đang chạy không. Nếu có → preempt ngay. Context switch
lưu toàn bộ registers (R0-R15, PSR, FPU registers trên Cortex-M4F) vào stack
của task bị preempt, nạp registers của task mới.

#### ⑦ Memory implications

```
xTaskCreate(func, name, stackDepth, param, priority, handle)
                         ^^^^^^^^^^^
                         ĐƠN VỊ: WORDS (4 bytes trên Cortex-M4)
                         stackDepth=256 → 256 × 4 = 1024 bytes RAM
```

Stack lưu: local variables, function call frames, saved registers khi
context switch, FPU registers (nếu dùng floating point).

Stack quá nhỏ → Stack Overflow → crash hoặc data corruption.

#### ⑧ Common mistakes

- Nhầm stack size: nghĩ 256 là 256 bytes, thực ra là 256 words = 1024 bytes
- Gọi code sau `vTaskStartScheduler()` — code đó không bao giờ chạy
- Task function `return` — undefined behavior, phải có `vTaskDelete(NULL)` hoặc vòng lặp vô tận
- Tạo task trước `App_FreeRTOSObjectsInit()` → task dùng queue NULL → assert

#### ⑨ Interview-style

> "FreeRTOS task là một luồng thực thi độc lập với stack riêng và priority riêng.
> Scheduler chạy task có priority cao nhất đang Ready. Mỗi task phải là vòng lặp
> vô tận — nếu function return, hành vi là undefined. Stack overflow trên Cortex-M4
> có thể kích hoạt HardFault hoặc làm corrupt dữ liệu của task khác tùy vị trí
> stack trong RAM."

---

### Design Decisions — Phase 1

**Tại sao `vTaskDelay()` thay vì `HAL_Delay()`:**

| | `HAL_Delay()` | `vTaskDelay()` |
|---|---|---|
| Cơ chế | Busy-wait (spin) | Block task, nhường CPU |
| CPU trong khi chờ | 100% | 0% (Idle task chạy) |
| Dùng trong RTOS | Sai | Đúng |

`HAL_Delay()` trong RTOS task = lãng phí CPU và có thể starve các task khác.

---

### What I Should Learn From Phase 1

1. Task là đơn vị thực thi cơ bản — mỗi task có stack, priority, state riêng.
2. Stack size đơn vị là **words**, không phải bytes — nhân 4 để ra bytes.
3. Code sau `vTaskStartScheduler()` không bao giờ chạy — scheduler không return.
4. `vTaskDelay()` là blocking (Blocked state), `HAL_Delay()` là busy-wait — luôn dùng `vTaskDelay` trong RTOS.

---

---

## §B.2 Phase 2 — Application Data Types & FreeRTOS Objects

### Problem Being Solved

Trước khi viết bất kỳ task logic nào, cần định nghĩa "ngôn ngữ" mà các tasks
dùng để giao tiếp: data types, message structs, event masks, và RTOS objects.
Làm điều này trước giúp tránh circular dependency và forced refactor sau.

---

### Design: Tại sao định nghĩa types trước khi viết tasks

Kinh nghiệm embedded: Data flows drive architecture. Nếu biết dữ liệu gì di
chuyển giữa tasks, tự nhiên biết cần queue/event group nào, ai là producer/consumer.

Ngược lại — viết task trước rồi mới nghĩ data flow → thường dẫn đến refactor lớn.

---

### FreeRTOS Concept: Queue

#### ① Problem it solves

Task A cần gửi dữ liệu cho Task B mà không cần biết Task B đang làm gì. Queue
là FIFO buffer thread-safe: A ghi vào, B đọc ra, FreeRTOS đảm bảo không có
race condition.

#### ② Tại sao cần trong project này

- `xButtonQueue`: ISR → UITask (button events)
- `xCommandQueue`: UITask → WashingManager (commands)
- `xDisplayQueue`: WashingManager → OLEDTask (display updates)

Queue cho phép producer và consumer chạy ở tốc độ khác nhau mà không mất dữ liệu.

#### ③ Tại sao alternatives bị loại

- **Global variable:** Race condition, không có synchronization
- **Task Notification:** Depth = 1 (mất event nếu chưa đọc kịp), chỉ mang 32-bit scalar

#### ④ Runtime behavior

```
xQueueSend(q, &item, timeout):
    Queue có chỗ trống  → copy item vào queue, return pdPASS ngay
    Queue đầy, timeout=0 → return errQUEUE_FULL ngay
    Queue đầy, timeout>0 → task vào Blocked state, chờ có chỗ trống

xQueueReceive(q, &item, portMAX_DELAY):
    Queue có item → copy ra, return pdPASS ngay
    Queue trống  → task vào Blocked state, chờ item mới
```

#### ⑤ Task state transitions

```
Producer (UITask):
  xQueueSend(..., 0) → queue đầy → return pdFAIL (không block, timeout=0)
  xQueueSend(..., portMAX_DELAY) → queue đầy → [Blocked] → có chỗ → [Ready]

Consumer (WashingManager):
  xQueueReceive(..., portMAX_DELAY) → queue trống → [Blocked]
  → producer gửi item → [Ready] → [Running] → đọc item
```

#### ⑥ Memory implications

```
xQueueCreate(length, itemSize):
    RAM = ~76 bytes (control block) + length × itemSize (storage buffer)

xButtonQueue  : ~76 + 5×4   = ~96  bytes
xCommandQueue : ~76 + 5×8   = ~116 bytes
xDisplayQueue : ~76 + 5×36  = ~256 bytes
```

FreeRTOS copy item **by value** vào queue storage. Không lưu pointer — an toàn với stack variables.

#### ⑦ Common mistakes

- Truyền pointer vào queue thay vì struct → pointer trỏ vào stack đã bị giải phóng
- Gọi `xQueueSend()` từ ISR thay vì `xQueueSendFromISR()` → crash
- Queue depth = 1 → producer block ngay khi consumer chưa đọc kịp

#### ⑧ Interview-style

> "FreeRTOS queue là FIFO buffer thread-safe. Nó copy data by value — producer
> không cần giữ data sống sau khi send. ISR phải dùng `xQueueSendFromISR()` và
> gọi `portYIELD_FROM_ISR()` sau để scheduler re-evaluate task priority."

---

### FreeRTOS Concept: Event Group

#### ① Problem it solves

Cho phép một task chờ **nhiều điều kiện cùng lúc** trong một lệnh blocking.
Queue chỉ chờ một item. Event Group cho chờ "bit A AND bit B" hoặc "bit A OR bit B".

#### ② Tại sao cần trong project này

WashingManager cần chờ `EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR` đồng thời.
Nếu dùng 3 queue riêng phải polling tuần tự — không thể block đúng nghĩa.

#### ③ Tại sao alternatives bị loại

- **Polling loop:** Tốn CPU, độ trễ phụ thuộc polling period
- **Binary semaphore:** Chỉ signal 1:1, không mang bit meaning
- **Queue:** Không thể wait OR/AND nhiều condition cùng lúc

#### ④ Runtime behavior

```
xEventGroupSetBits(group, EVT_TIMEOUT):
    Set bit trong EventBits_t (32-bit atomic)
    Kiểm tra xem task nào đang WaitBits trên các bit này
    Nếu điều kiện wait thỏa → unblock task đó

xEventGroupWaitBits(group, bits, clearOnExit, waitAll, timeout):
    Nếu bits đã set → return ngay
    Nếu chưa → task vào Blocked state
    clearOnExit=pdTRUE → xóa bits matched sau khi wake
    waitAll=pdFALSE    → wake khi BẤT KỲ bit nào trong mask được set
```

#### ⑤ Bits persistence — điểm quan trọng

Queue: item được consume → biến mất.
Event Group: bit được set → **tồn tại** cho đến khi xóa thủ công.

Nếu SensorTask set `EVT_DOOR_CLOSED` trước khi WashingManager đến lệnh
WaitBits, bit vẫn còn đó — WashingManager không bị miss event.

#### ⑥ Memory implications

`xEventGroupCreate()` = ~76 bytes trên heap_4.
`EventBits_t` = 32 bits, nhưng 8 bits cao reserved by kernel → 24 bits usable.

#### ⑦ Common mistakes

- Gọi `xEventGroupSetBits()` từ ISR → phải dùng `xEventGroupSetBitsFromISR()`
- Không clear bits sau khi xử lý → bit cũ ảnh hưởng wait tiếp theo
- Dùng bit 24-31 → reserved, behavior undefined

---

### Design: Self-Contained Message Structs

`DisplayMsg_t` chứa `char statusText[24]` — mảng inline, không phải pointer.

```c
// SAI: pointer vào stack
char buf[24] = "WASHING";
DisplayMsg_t msg;
msg.statusText = buf;        // buf có thể bị overwrite trước khi OLEDTask đọc
xQueueSend(xDisplayQueue, &msg, 0);

// ĐÚNG: copy by value
strncpy(msg.statusText, "WASHING", sizeof(msg.statusText) - 1);
xQueueSend(xDisplayQueue, &msg, 0);  // toàn bộ 24 bytes được copy vào queue
```

FreeRTOS copy `sizeof(DisplayMsg_t)` bytes khi send. Không có pointer, không có
heap string, không có lifetime issue.

---

### What I Should Learn From Phase 2

1. Queue copy by value — không dùng pointer trong message structs.
2. Event Group dành cho "chờ nhiều điều kiện" — Queue dành cho "truyền dữ liệu".
3. Event bits persistent — bits tồn tại đến khi xóa thủ công, không bị miss nếu producer nhanh hơn consumer.
4. Thiết kế data types trước task logic — data flow drives architecture.
5. ISR phải dùng `FromISR` variants của tất cả FreeRTOS API.

---

---

## §B.3 Phase 3 — UI Task & Button ISR

> **⚠ Đã cập nhật 2026-08-24** — bản gốc của Phase 3 mô tả kiến trúc **một
> nút PA0** phân biệt short/long press bằng thời lượng nhấn giữ. Kiến trúc
> đó đã bị thay bằng **hai nút vật lý riêng biệt** trong quá trình xử lý
> Bug #8 (PA0 dùng chung cho button và door sensor gây race condition khi
> thả tay dài — Phần D, Session 07) và hành trình đổi chân
> PE1→PD4→PD2 (`docs/HARDWARE_BUGS.md` Bug #14/#15/#16). Phần dưới đây mô tả
> đúng code hiện tại; đoạn cũ được giữ lại có đánh dấu để thấy sự thay đổi.
> Đây là ví dụ thật cho bài học đã ghi ở §C.4:
> *comment/tài liệu không được compiler kiểm tra nên trôi khỏi code âm thầm.*

### Problem Being Solved

Hai nút vật lý tạo EXTI interrupt độc lập (PA0 = navigate, PD2 = confirm).
Cần:
1. ISR ngắn, không chứa logic
2. Debounce PA0 để lọc bounce cơ học
3. Phân biệt navigate/confirm **bằng việc nút nào được nhấn**, không bằng
   thời lượng nhấn giữ (khác bản gốc)
4. Truyền event đến UITask một cách thread-safe
5. Giữ BSP **không phụ thuộc FreeRTOS** (Design Rule #3) dù vẫn cần gọi
   `xQueueSendFromISR()` ở đâu đó

---

### Design: ISR → Callback → Queue → Task Pattern

Sơ đồ gốc bỏ sót một tầng quan trọng: `bsp_button.c` **không hề gọi
`xQueueSendFromISR()`**. Nó gọi một **con trỏ hàm callback** do `UITask`
đăng ký — đây chính là cơ chế giữ Design Rule #3 (*"Drivers must not depend
on FreeRTOS"*) trong khi vẫn cần đưa dữ liệu vào FreeRTOS queue.

```
[Hardware interrupt PA0]              [Hardware interrupt PD2]
        │                                      │
        ▼                                      ▼
EXTI0_IRQHandler()                    EXTI2_IRQHandler()
        │                                      │
BSP_Button_IRQHandler()               BSP_StartButton_IRQHandler()
   debounce PA0 (so timestamp)           mọi falling edge = Confirm
        │                                      │
        └──────────────┬───────────────────────┘
                        ▼
            s_callback(event)          ← con trỏ hàm — KHÔNG PHẢI FreeRTOS API
            (đăng ký bởi UITask qua BSP_Button_RegisterCallback())
                        │
                        ▼
            Button_EventCallback()     ← nằm trong ui_task.c, KHÔNG PHẢI bsp_button.c
                        │
            xQueueSendFromISR()        ← copy ButtonEvent_t vào queue
                        │
            portYIELD_FROM_ISR()       ← cho scheduler biết có task high-priority ready
                        │
                        ▼
            UITask (Blocked → Ready → Running)
                        │
            xQueueReceive()            ← đọc event, xử lý logic
                        │
                        ▼
            xQueueSend(xCommandQueue)  ← gửi command nếu cần
```

> **Vì sao tầng callback quan trọng hơn nó trông có vẻ:** nếu `bsp_button.c`
> gọi thẳng `xQueueSendFromISR()`, thì BSP — tầng lẽ ra tái dùng được ở bất
> kỳ project bare-metal nào khác — sẽ bị khoá cứng vào FreeRTOS. Con trỏ hàm
> là điểm "cắt dây" duy nhất giữ được ranh giới đó. Đây là ví dụ cụ thể của
> nguyên tắc *dependency inversion* trong C nhúng: module cấp thấp không biết
> gì về module cấp cao gọi nó, chỉ biết một chữ ký hàm.

ISR chỉ làm một việc: capture event và gọi callback. Mọi logic FreeRTOS nằm
ở tầng `ui_task.c`, không phải BSP.

---

### Tại sao `portYIELD_FROM_ISR()` bắt buộc

Đoạn này nằm trong `Button_EventCallback()` ở `ui_task.c`, **không phải**
trong `bsp_button.c` như sơ đồ gốc ngụ ý:

```c
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xQueueSendFromISR(xButtonQueue, &event, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

`xQueueSendFromISR()` set `xHigherPriorityTaskWoken = pdTRUE` nếu việc gửi
item vừa unblock một task có priority cao hơn task đang chạy.

Nếu không gọi `portYIELD_FROM_ISR()`: ISR return, CPU tiếp tục chạy task
cũ (priority thấp hơn), phải đợi đến tick tiếp theo mới switch sang UITask.
Latency tăng lên đến 1ms.

Nếu có `portYIELD_FROM_ISR(pdTRUE)`: ISR return → PendSV được trigger ngay →
scheduler chạy UITask ngay lập tức. Latency chỉ vài microseconds.

---

### Design: Debounce

> ⚠ **Mục này đã được viết lại (2026-08-29).** Cơ chế cũ — đo *thời lượng nhấn*
> rồi bắn sự kiện lúc **thả tay** — là tàn dư của thiết kế short/long press và
> đã bị gỡ bỏ. Phần mô tả cũ giữ ở cuối mục để đối chiếu lịch sử.

**Cơ chế hiện tại — lọc theo khoảng lặng (*quiet window*)**, dùng chung cho cả
hai nút (`bsp_button.c`):

```c
uint32_t now   = HAL_GetTick();
uint8_t  quiet = ((now - s_lastEdgeMs) >= BUTTON_DEBOUNCE_MS) ? 1u : 0u;

s_lastEdgeMs = now;        /* MỌI cạnh đều nạp lại — kể cả cạnh bị từ chối */

if (pressed && quiet && (s_callback != NULL)) {
    s_callback(BTN_EVENT_NAVIGATE);
}
```

Bốn tính chất, mỗi cái đều có lý do:

| Tính chất | Vì sao |
| --- | --- |
| Bắn ở **cạnh bấm**, không phải cạnh nhả | Không còn phải đo thời lượng, nên không còn lý do đợi tới lúc thả. UI phản hồi ngay khi ngón tay còn đang đè |
| Nạp lại timestamp **ở mọi cạnh**, kể cả cạnh bị từ chối | Đây là điểm mấu chốt. Nếu so với *sự kiện đã chấp nhận* gần nhất, chùm dội lúc nhả tay (xảy ra vài giây sau) sẽ vượt qua bộ lọc và sinh sự kiện giả |
| Mỗi nút một biến timestamp riêng | Dùng chung thì bấm nút này sẽ khoá nút kia trong 50 ms |
| Trừ trên `uint32_t` | `HAL_GetTick()` tràn sau ~49 ngày; phép trừ unsigned vẫn cho khoảng cách đúng |

**Giới hạn đã biết trên PD2:** `.ioc` cấu hình EXTI2 **chỉ cạnh xuống**, nên ISR
không nhìn thấy cạnh nhả. Dội trong một lần bấm bị lọc sạch, nhưng cạnh đầu tiên
của chùm dội lúc *nhả* vẫn có thể lọt thành một CONFIRM thừa. Muốn triệt để phải
bật cả hai cạnh cho PD2 trong CubeMX.

**Một phương án đã cân nhắc rồi loại:** thêm bước đọc mức chân cho PD2 để xác
nhận nút đang bị đè. Lúc dội khi bấm, ISR có thể đọc trúng mức nhả và **từ chối
cả lần bấm thật** — đổi một lỗi hiếm lấy một lỗi thường xuyên.

Tại sao không dùng `vTaskDelay()` trong ISR để debounce:

- ISR không thể block
- `vTaskDelay()` trong ISR → crash

<details>
<summary>Cơ chế cũ (đã gỡ) — giữ để đối chiếu lịch sử</summary>

```c
/* Rising edge — ghi lại thời điểm bắt đầu nhấn */
s_pressStartMs = HAL_GetTick();
s_pressed = 1u;

/* Falling edge — chỉ chấp nhận nếu đủ lâu để không phải nhiễu cơ học */
uint32_t duration = HAL_GetTick() - s_pressStartMs;
if (duration >= BUTTON_DEBOUNCE_MS) {
    s_callback(BTN_EVENT_NAVIGATE);
}
```

Hai vấn đề khiến nó bị thay: (1) sự kiện chỉ bắn khi **thả tay** — ràng buộc bắt
buộc của việc đo thời lượng, nhưng vô nghĩa sau khi long-press bị bỏ; (2) PD2
lúc đó **hoàn toàn không có bộ lọc** — mọi cạnh xuống đều thành CONFIRM, nên một
lần bấm dội có thể sinh 3-4 sự kiện. Triệu chứng bị che đi bởi các lệnh
`xQueueReset(xCommandQueue)` ở hạ nguồn trong `washing_manager.c`.

</details>

---

### Design: Navigate vs Confirm — ĐÃ ĐỔI từ short/long press sang 2 nút riêng

**Thiết kế gốc (đã bỏ):** một nút PA0, phân biệt bằng thời lượng nhấn giữ:

```c
/* ❌ KHÔNG CÒN TỒN TẠI trong code — chỉ còn giá trị lịch sử */
ButtonEvent_t event = (duration >= BUTTON_LONG_PRESS_MS)
                    ? BTN_EVENT_CONFIRM
                    : BTN_EVENT_NAVIGATE;
```

**Thiết kế hiện tại:** hai nút vật lý, phân biệt bằng **nút nào được nhấn**,
không còn đo thời lượng:

```c
/* bsp_button.c:56  — PA0, sau khi debounce qua */
s_callback(BTN_EVENT_NAVIGATE);

/* bsp_button.c:76  — PD2, mọi falling edge */
s_callback(BTN_EVENT_CONFIRM);
```

**Hệ quả của thay đổi — đáng so sánh:**

| | Thiết kế cũ (1 nút, đo thời lượng) | Thiết kế hiện tại (2 nút) |
| --- | --- | --- |
| Độ trễ phản hồi Confirm | Phải đợi đủ `BUTTON_LONG_PRESS_MS` (1500 ms) mới biết là Confirm | Tức thì — falling edge là confirm ngay |
| Phần cứng cần | 1 chân GPIO | 2 chân GPIO |
| Rủi ro | Người dùng không biết đang giữ bao lâu là đủ; UX mơ hồ | Rõ ràng, mỗi nút một chức năng cố định |
| Bug đã gặp | Bug #8 — PA0 dùng chung với door sensor, thả tay dài → đọc nhầm cửa mở | Không còn — PD2 tách biệt khỏi door sensor (PD5) |

**Dấu vết còn sót lại — ✅ ĐÃ DỌN (2026-08-29).** Macro `BUTTON_LONG_PRESS_MS`
(`app_config.h:35`, giá trị `1500U`) từng còn nằm lại dù không nơi nào trong
code dùng tới. Cùng đợt dọn đó còn gỡ nốt hai biến `s_pressStartMs` /
`s_pressed` và toàn bộ phép đo thời lượng trong `BSP_Button_IRQHandler()` —
xem mục "Design: Debounce" ở trên.

Bài học từ vụ này đáng giữ lại: **macro chết chỉ là phần dễ thấy.** Phần nguy
hiểm hơn là *cấu trúc điều khiển* mà thiết kế cũ để lại — ở đây là việc sự kiện
bắn ở cạnh **nhả tay**. Macro không dùng thì vô hại, nhưng cấu trúc thừa vẫn
đang **quy định hành vi thật của hệ thống**, và không ai chất vấn nó vì "code
đang chạy được". Khi bỏ một tính năng, phải truy ngược xem tính năng đó đã áp
đặt những ràng buộc nào lên phần còn lại.

---

### What I Should Learn From Phase 3

1. ISR phải ngắn nhất có thể — chỉ capture và gọi callback, không xử lý logic.
2. `portYIELD_FROM_ISR()` giảm latency từ 1ms xuống vài microseconds.
3. Debounce bằng timestamp comparison — không dùng delay trong ISR.
4. `xQueueSendFromISR()` không block — nếu queue đầy thì drop event.
5. Pattern: ISR → Callback → Queue → Task là nền tảng của mọi
   interrupt-driven system trong FreeRTOS — **tầng callback ở giữa** là thứ
   giữ được ranh giới BSP/FreeRTOS, không phải chi tiết phụ.
6. **Tài liệu về kiến trúc phải được kiểm tra lại khi phần cứng/thiết kế đổi**
   — đoạn short/long-press từng đúng lúc viết, rồi sai âm thầm suốt nhiều
   commit sau khi Bug #8 buộc phải đổi sang 2 nút. Không có gì tự động cảnh
   báo việc này; chỉ phát hiện được khi chủ động đối chiếu lại với code thật.

---

---

## §B.4 Phase 4 — Sensor Task

### Problem Being Solved

Cần poll trạng thái door sensor và water level sensor định kỳ, rồi cập nhật
event group để WashingManager có thể react.

---

### Design: Periodic Task Pattern

```c
void SensorTask(void *pvParameters)
{
    for (;;)
    {
        // đọc sensor, set/clear bits
        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));  // 100ms
    }
}
```

Đây là "tick-driven polling" — không phải event-driven. Chấp nhận được cho
sensor polling vì sensor hardware không tạo interrupt.

**Tại sao 100ms:**
- Đủ nhanh để phát hiện door open trong < 100ms
- Không quá nhanh để tốn CPU

---

### Design: Event Group Producer

```c
if (BSP_Sensor_IsDoorClosed())
    xEventGroupSetBits(xSystemEvents, EVT_DOOR_CLOSED);
else {
    xEventGroupClearBits(xSystemEvents, EVT_DOOR_CLOSED);
    xEventGroupSetBits(xSystemEvents, EVT_ERROR);
}
```

SensorTask là producer — chỉ set/clear bits, không biết consumer là ai.
WashingManager là consumer — chờ bits, không biết producer là ai.
Đây là loose coupling qua event group.

---

### `vTaskDelay` vs `vTaskDelayUntil`

| | `vTaskDelay(100)` | `vTaskDelayUntil(&last, 100)` |
|---|---|---|
| Ý nghĩa | Ngủ 100ms kể từ **bây giờ** | Ngủ đến khi **last + 100ms** |
| Nếu task bị preempt | Period bị kéo dài | Period vẫn đúng 100ms |
| Dùng khi nào | Delay đơn giản | Cần period chính xác (control loop) |

SensorTask dùng `vTaskDelay` là đủ — không cần period chính xác đến microsecond.

---

### What I Should Learn From Phase 4

1. Sensor polling là acceptable khi sensor không có interrupt output.
2. `vTaskDelay` vs `vTaskDelayUntil` — chọn dựa trên yêu cầu timing.
3. Event group producer/consumer loose coupling — producer không biết ai đang chờ.
4. Clear bits chủ động khi condition không còn đúng — bit persistent nên phải clear thủ công.

---

---

## §B.5 Phase 5 — WashingManager & State Machine

### Problem Being Solved

Cần một module duy nhất điều phối toàn bộ logic máy giặt: nhận command, chạy
state machine, điều khiển actuators, cập nhật display. Nếu logic phân tán
trong nhiều task → race condition và khó debug.

---

### Design: Table-Driven State Machine

```c
static const SmTransition_t kTransitions[] = {
    { WASH_STATE_IDLE,       WASH_EVT_START_WASH,  WASH_STATE_FILL_WATER },
    { WASH_STATE_FILL_WATER, WASH_EVT_WATER_FULL,  WASH_STATE_WASHING    },
    // ...
};
```

**Tại sao table-driven thay vì switch-case lồng nhau:**

```c
// BAD: switch-case lồng nhau
switch (state) {
    case IDLE:
        switch (event) {
            case START_WASH: ...   // thêm event mới → sửa switch
        }
}

// GOOD: table-driven
// Thêm transition → chỉ thêm 1 row vào table
// Dispatcher loop không bao giờ thay đổi
```

Table-driven SM: O(n) tìm kiếm nhưng n nhỏ (< 20 transitions). Đổi lại
được code cực kỳ dễ đọc và maintain.

---

### Design: Entry-Action Callbacks

SM core không biết FreeRTOS hay HAL tồn tại. Hardware actions được inject
qua callback pointers:

```c
typedef struct {
    void (*onEnterWashing)(void);
    void (*onEnterDrain)(void);
    // ...
} WashingSM_Callbacks_t;
```

**Lợi ích:**
- SM có thể unit test mà không cần hardware
- WashingManager quyết định hardware action, SM quyết định state logic
- Separation of concerns rõ ràng

---

### Design: State-Dependent Blocking

WashingManager block trên primitive khác nhau tùy state:

```
IDLE/FINISH/ERROR → xQueueReceive(xCommandQueue)   ← chờ user input
FILL_WATER        → xEventGroupWaitBits(EVT_WATER_FULL|EVT_TIMEOUT|EVT_ERROR)
WASHING/DRAIN/SPIN→ xEventGroupWaitBits(EVT_TIMEOUT|EVT_ERROR)
```

**Tại sao không dùng một vòng lặp unified với timeout ngắn:**

```c
// BAD: polling approach
while (1) {
    checkQueue();
    checkEvents();
    vTaskDelay(10);  // 10ms polling → tốn CPU, không event-driven
}

// GOOD: state-dependent blocking
// Task block 100% cho đến khi đúng signal đến
// Zero CPU consumption khi chờ
```

---

### Design: BSP Layer

```
Application (washing_manager.c)
        │
        │ BSP_Motor_Start()
        ▼
BSP (bsp_motor.c)
        │
        │ HAL_GPIO_WritePin(...)
        ▼
HAL / Hardware
```

**Tại sao cần BSP:**
- Application code không phụ thuộc HAL → dễ port sang board khác
- Nếu chân GPIO thay đổi: chỉ sửa BSP, không sửa application
- Tên hàm có ý nghĩa domain: `BSP_Motor_Start()` rõ hơn `HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET)`

**Một BSP file per peripheral:**
`bsp_motor.c`, `bsp_valve.c`, `bsp_led.c` riêng biệt — không gộp vào
`bsp_actuators.c`. Lý do: file nhỏ, trách nhiệm rõ ràng, dễ tìm code.

---

### What I Should Learn From Phase 5

1. Table-driven SM: thêm transition = thêm 1 row, dispatcher không đổi.
2. Entry-action callbacks tách SM logic khỏi hardware — SM không import FreeRTOS hay HAL.
3. State-dependent blocking: mỗi state block trên đúng signal nó cần — không polling.
4. BSP layer là ranh giới giữa application và hardware — chỉ BSP biết HAL tồn tại.
5. WashingManager là single owner của state machine — không task nào khác gọi WashingSM_ProcessEvent().

---

---

## §B.6 Phase 6 — OLED Task & Driver Integration

### Problem Being Solved

Cần render trạng thái máy giặt lên OLED SSD1306 qua I2C mà không làm
WashingManager phải chờ I2C transaction hoàn tất.

---

### Design: Pure Renderer Pattern

OLEDTask là "pure renderer" — không có state riêng, không quyết định gì,
chỉ render những gì được gửi đến:

```c
void OLEDTask(void *pvParameters)
{
    BSP_OLED_Init();
    DisplayMsg_t msg;
    for (;;) {
        xQueueReceive(xDisplayQueue, &msg, portMAX_DELAY);
        OLED_Render(&msg);  // render từ msg, không từ global state
    }
}
```

**Tại sao không để WashingManager gọi BSP_OLED trực tiếp:**
- I2C transaction có thể mất 5-10ms (128×64 pixels, 100kHz I2C)
- WashingManager block 10ms mỗi lần update display → bỏ lỡ events
- OLEDTask priority thấp nhất → bị preempt bất cứ lúc nào, không ảnh hưởng hệ thống

**Tại sao `DisplayMsg_t` self-contained:**
```c
typedef struct {
    WashState_t state;
    WashMode_t  selectedMode;
    uint8_t     menuIndex;
    char        statusText[24];  // mảng inline, không phải pointer
} DisplayMsg_t;
```

OLEDTask có đủ mọi thứ cần render chỉ từ một message — không cần đọc
global state, không cần lock/mutex.

---

### Design: Driver Integration Rule

```
Drivers/SSD1306/ssd1306.c   ← driver thuần, không biết FreeRTOS
        │
BSP/bsp_oled.c              ← wraps driver, expose simple API
        │
OLEDTask/oled_task.c        ← gọi BSP, không bao giờ gọi driver trực tiếp
```

Driver không được include FreeRTOS headers. Driver là pure C library.
BSP là adapter giữa driver và application.

---

### FreeRTOS: OLEDTask Priority = Lowest (1)

OLEDTask có priority 1 (thấp nhất trong application). Lý do:
- Display update không time-critical — trễ 10ms không ai nhận ra
- Nếu hệ thống bận, OLEDTask bị preempt → chấp nhận được
- WashingManager (priority 3) luôn preempt OLEDTask nếu cần

---

### What I Should Learn From Phase 6

1. Queue decouples producer (WashingManager) khỏi consumer (OLEDTask) về thời gian.
2. Pure renderer không có state — mọi thứ cần render phải nằm trong message.
3. Self-contained message structs tránh dangling pointer và race condition.
4. Driver không được depend on FreeRTOS — chỉ BSP layer mới wrap driver.
5. Priority thấp cho renderer task — bị preempt là acceptable, không block hệ thống.

---

---

## §B.7 Phase 7 — Software Timers

### Problem Being Solved

`vTaskDelay(20000ms)` trong WASHING state block WashingManager cứng — không
thể phản ứng với EVT_ERROR (cửa mở) trong 20 giây. Cần cơ chế timeout
event-driven: timer đếm độc lập, báo signal khi hết giờ.

---

### Design Overview

```
WashingManager (task)          Timer Service Task
       │                              │
       │  xTimerStart(xWashTimer)     │
       │─────────────────────────────>│
       │                              │  đếm 20s
       │  xEventGroupWaitBits(        │
       │    EVT_TIMEOUT|EVT_ERROR)    │
       │◄── blocked ─────────────────│
       │                              │  20s hết → callback
       │                              │  xEventGroupSetBits(EVT_TIMEOUT)
       │◄── unblocked ───────────────│
       │  xử lý event                │
```

---

### FreeRTOS Concept: Software Timer

#### ① Problem it solves

Thực thi code sau một khoảng thời gian mà không cần tạo task riêng. Thay
polling `while(tick < deadline)` bằng event-driven callback.

#### ② Tại sao cần trong project này

Mỗi phase máy giặt có deadline cố định. Cần báo "hết giờ" mà không block
WashingManager bằng `vTaskDelay`.

#### ③ Tại sao alternatives bị loại

| Alternative | Vấn đề |
|---|---|
| `vTaskDelay` | Block cứng, không thể interrupt bằng EVT_ERROR |
| HAL TIM peripheral | Tốn hardware resource, không portable |
| Polling trong task | Tốn CPU, không event-driven |
| Dedicated timeout task | Lãng phí RAM/stack cho task chỉ đếm giờ |

#### ④ Runtime behavior

```
xTimerCreate() → tạo timer control block trên heap_4 (stopped state)
xTimerStart()  → gửi lệnh vào Timer Command Queue
                 Timer Service Task đọc lệnh, bắt đầu đếm tick
Khi hết period → Timer Service Task gọi callback trực tiếp
                 (trong context của Timer Service Task)
```

#### ⑤ Task state transitions

```
WashingManager: Running → Blocked (xEventGroupWaitBits)
Timer Service : Blocked → Running (khi timer expire)
                Running → gọi callback → xEventGroupSetBits(EVT_TIMEOUT)
WashingManager: Blocked → Ready → Running (preempt vì priority cao hơn)
```

#### ⑥ Context switching

`xEventGroupSetBits()` trong callback unblock WashingManager (priority 3).
Timer Service Task có priority 2. Scheduler preempt Timer Service Task ngay
sau khi callback return — WashingManager chạy ngay, không đợi tick tiếp theo.

#### ⑦ Memory implications

```
xTimerCreate() = ~60 bytes/timer trên heap_4
4 timers       = ~240 bytes

Timer Service Task stack:
configTIMER_TASK_STACK_DEPTH = configMINIMAL_STACK_SIZE × 2 = 260 words = 1040 bytes
```

#### ⑧ Common mistakes

- Gọi blocking API trong callback → treo Timer Service Task, mọi timer trong hệ thống bị ảnh hưởng
- Quên `xTimerStop()` khi signal khác đến trước → zombie timer fire vào state sai
- Dùng `uxAutoReload = pdTRUE` cho timeout → fire liên tục thay vì một lần
- `xTimerStart()` gửi command vào Timer Command Queue — nếu queue đầy trả về pdFAIL, không block

#### ⑨ Interview-style

> "FreeRTOS software timer không dùng hardware timer — nó chạy trong Timer
> Service Task, đếm tick thông qua SysTick. Callback chạy trong context của
> Timer Service Task, không phải task của bạn. Không được block trong callback.
> One-shot timer dùng cho deadline/timeout. Auto-reload dùng cho periodic action."

---

### Design: One Shared Callback

Tất cả 4 timers dùng chung `Timer_PhaseCallback` vì tất cả đều làm cùng việc:
set `EVT_TIMEOUT`. WashingManager đọc SM state để biết timer nào fire — không
cần phân biệt timer ID.

```c
static void Timer_PhaseCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    xEventGroupSetBits(xSystemEvents, EVT_TIMEOUT);
}
```

**Nguyên tắc:** Callback chứa càng ít code càng tốt. Logic nằm trong task.

---

### Design: xTimerStop() — Khi nào cần

```
FILL_WATER: chờ EVT_WATER_FULL | EVT_TIMEOUT
    → EVT_WATER_FULL đến trước → xTimerStop(xFillTimeoutTimer)  ← cần
    → EVT_TIMEOUT đến trước   → timer đã tự dừng (one-shot)    ← không cần

WASHING: chờ EVT_TIMEOUT | EVT_ERROR
    → EVT_TIMEOUT đến trước → timer đã tự dừng                  ← không cần
    → EVT_ERROR đến trước   → xTimerStop(xWashTimer)            ← cần
```

Rule: gọi `xTimerStop()` khi và chỉ khi một signal khác kết thúc phase trước
khi timer tự fire.

---

### What I Should Learn From Phase 7

1. `vTaskDelay` là "ngủ mù" — không thể interrupt. Software timer + `xEventGroupWaitBits` là "ngủ thức tỉnh" — phản ứng được với bất kỳ event nào.
2. Timer callback là shared resource — mọi callback trong hệ thống chạy tuần tự trong Timer Service Task. Một callback block = toàn bộ timer system treo.
3. Callback chứa ít code nhất có thể: chỉ set bits, không logic.
4. One-shot cho timeout/deadline. Auto-reload cho periodic.
5. Luôn stop timer thủ công khi signal khác kết thúc phase trước timer fire.
6. `xTimerStart()` gửi command vào queue, không block WashingManager — timer bắt đầu chạy bất đồng bộ.

---

---

## §B.8 Phase 8 — SEGGER SystemView Analysis

### Problem Being Solved

Hệ thống có 4 tasks, 3 queues, 4 timers chạy đồng thời. Không có tool thì
chỉ đoán: task nào chiếm CPU? Queue send/receive đúng thứ tự không? Timer
fire đúng lúc không? SystemView cho nhìn thấy toàn bộ trên timeline thời gian thực.

---

### How SystemView Works — SEGGER RTT

```
MCU (STM32)                        PC (SEGGER SystemView app)
    │                                        │
    │  RTT buffer (ring buffer trong RAM)    │
    │  ← FreeRTOS hooks ghi event vào đây   │
    │                                        │
    │◄────── ST-Link đọc trực tiếp RAM ─────│
    │        qua SWD — CPU không tham gia   │
```

**RTT vs UART:**

| | UART printf | SEGGER RTT |
|---|---|---|
| CPU overhead | Cao — chờ TX xong | ~0 — chỉ ghi RAM |
| Ảnh hưởng timing | Làm trễ task | Không đáng kể |
| Cần thêm pin | UART TX/RX | Không — dùng SWD sẵn có |

Một `printf` UART 115200 baud mất ~1ms cho 10 ký tự. RTT ghi RAM mất vài chục ns.

---

### Key APIs

**`SEGGER_SYSVIEW_Conf()`** — khởi tạo RTT buffer, đăng ký tên interrupt, setup MCU info. Gọi một lần trước `vTaskStartScheduler()`.

**`SEGGER_SYSVIEW_Start()`** — bắt đầu record events. Phải gọi trước scheduler để capture từ t=0.

**`configQUEUE_REGISTRY_SIZE`** — số queue/semaphore/mutex tối đa có thể đăng ký tên. Project dùng 3 queues → cần ≥ 3, set = 8.

**`vQueueAddToRegistry(handle, name)`** — gán tên cho queue handle. Nếu không gọi, SystemView hiển thị địa chỉ RAM thô (`0x20003A40`) thay vì `"ButtonQ"`.

---

### Expected Timeline

```
WashMgr  ████░░░░░░████░░░░░░░░░░░░░░  (blocked chờ command/event)
UITask   ░░░░████░░░░░░░░░░░░░░░░░░░░  (blocked chờ button)
Sensor   ░░░░░░░░░████░░░░░░░░░░████░  (100ms periodic)
OLED     ░░░░░░░░░░░░░████░░░░░░░░░░░  (blocked chờ display msg)
Tmr Svc  ░░░░░░░░░░░░░░░░░░░░░░░░░███  (rất ngắn khi timer fire)
IDLE     ████░░░░░░░░░░░░░░░░░░░░░░░░  (chạy khi không ai cần CPU)
```

---

### What I Should Learn From Phase 8

1. RTT tận dụng SWD debug interface — không cần UART, không làm chậm hệ thống.
2. `SEGGER_SYSVIEW_Start()` phải gọi trước `vTaskStartScheduler()` — code sau scheduler không chạy được.
3. `vQueueAddToRegistry()` không ảnh hưởng hoạt động queue — chỉ thêm tên cho debug tool.
4. Timer Service Task (`Tmr Svc`) hiện trên timeline — verify timer callback chạy đúng lúc.
5. IDLE task chạy khi tất cả tasks đều blocked — thời gian IDLE = CPU free time.

---

# PHẦN C — Phụ lục

---

## §C.1 Bảng tra API theo primitive

Tra nhanh API nào gọi được từ ISR và mục nào mô tả chi tiết.

### Event Group

| API | Từ ISR | Chi tiết |
| --- | --- | --- |
| `xEventGroupCreate()` / `...Static()` | ❌ | §1.10 |
| `xEventGroupWaitBits()` | ❌ | §1.7, §1.9 |
| `xEventGroupGetBits()` | `...FromISR()` | §1.7 |
| `xEventGroupSetBits()` | `...FromISR()` ⚠ | §1.4, §1.5 |
| `xEventGroupClearBits()` | `...FromISR()` | §1.8, §1.9.1 |
| `xEventGroupSync()` | ❌ | §1.6 |

### Software Timer

| API | Từ ISR | Chi tiết |
| --- | --- | --- |
| `xTimerCreate()` / `...Static()` | ❌ | §4.19 |
| `xTimerStart()` / `xTimerStop()` | `...FromISR()` | §4.3, §4.18 |
| `xTimerReset()` | `...FromISR()` | §4.6, §4.25 |
| `xTimerChangePeriod()` | `...FromISR()` | §4.6 ⚠ tự động start |
| `xTimerDelete()` | ❌ | §4.15 |
| `xTimerIsTimerActive()` | ❌ | §4.25 |
| `pvTimerGetTimerID()` / `vTimerSetTimerID()` | ❌ | §4.8 |
| `vTimerSetReloadMode()` / `uxTimerGetReloadMode()` | ❌ | §4.21 |
| `xTimerGetTimerDaemonTaskHandle()` | ❌ | §4.21, §4.26 |
| `xTimerPendFunctionCall()` | `...FromISR()` | §4.9, §4.20 ⚠ |

### Task Notification

| API | Từ ISR | Chi tiết |
| --- | --- | --- |
| `xTaskNotify()` | `...FromISR()` | §6 |
| `xTaskNotifyGive()` | `vTaskNotifyGiveFromISR()` | §6 |
| `ulTaskNotifyTake()` | ❌ | §6 |
| `xTaskNotifyWait()` | ❌ | §6 |
| `xTaskNotifyStateClear()` / `ulTaskNotifyValueClear()` | ❌ | §6 |

⚠ = có điều kiện biên dịch hoặc hành vi bất ngờ, đọc mục tương ứng trước khi
dùng.

---

## §C.2 Cấu hình kernel của project

Trích từ `OS/FreeRTOS/include/FreeRTOSConfig.h`. Giá trị **mặc định ẩn** là
những macro không được khai trong file cấu hình, kernel tự lấy default từ
`FreeRTOS.h` — đây là nguồn của nhiều hiểu lầm (§4.20).

### Khai báo tường minh

| Macro | Giá trị | Ảnh hưởng |
| --- | --- | --- |
| `configUSE_PREEMPTION` | 1 | §5.8 |
| `configCPU_CLOCK_HZ` | `SystemCoreClock` (16 MHz) | §5.3, §5.4 |
| `configTICK_RATE_HZ` | 1000 | §5.3, §5.7 |
| `configMAX_PRIORITIES` | 5 | §4.11 |
| `configMINIMAL_STACK_SIZE` | 130 words | §4.23 |
| `configUSE_TIMERS` | 1 | §4.2 |
| `configTIMER_TASK_PRIORITY` | 2 | §4.11, §5.8 |
| `configTIMER_QUEUE_LENGTH` | 10 | §4.18 |
| `configTIMER_TASK_STACK_DEPTH` | 260 words (1040 B) | §4.23 |
| `configUSE_TRACE_FACILITY` | 1 | §4.1 |
| `configUSE_IDLE_HOOK` | 0 | — |
| `configUSE_TICK_HOOK` | 0 | — |
| `configCHECK_FOR_STACK_OVERFLOW` | **0** | §4.23, §4.27 ⚠ |
| `configGENERATE_RUN_TIME_STATS` | 0 | §4.27 |

### Mặc định ẩn (không khai — dùng giá trị default của kernel)

| Macro | Default | Hệ quả |
| --- | --- | --- |
| `configUSE_TIME_SLICING` | 1 | Round-robin giữa task cùng priority — §5.8.1 |
| `configUSE_TICKLESS_IDLE` | 0 | SysTick chạy liên tục 1 kHz — §5.6 |
| `configUSE_TASK_NOTIFICATIONS` | 1 | Mọi task đều có sẵn hộp thư — §6 |
| `configTASK_NOTIFICATION_ARRAY_ENTRIES` | 1 | Một slot notification mỗi task — §6 |
| `configSUPPORT_STATIC_ALLOCATION` | **0** | `xTimerCreateStatic()` không tồn tại — §4.19 |
| `INCLUDE_xTimerPendFunctionCall` | **0** | `xTimerPendFunctionCall()` không tồn tại — §4.20 |

### Priority các task

| Task | Priority | Ghi chú |
| --- | --- | --- |
| WashingManager | 3 | Cao nhất trong ứng dụng |
| UITask | 2 | |
| SensorTask | 2 | |
| Tmr Svc (daemon) | 2 | Ngang UITask/SensorTask — §4.11 |
| OLEDTask | 1 | Thấp nhất, thuần render |
| IDLE | 0 | Kernel tự tạo |

---

## §C.3 Thuật ngữ

| Thuật ngữ | Nghĩa |
| --- | --- |
| **Daemon task** | Task do kernel tự tạo, không xuất hiện trong `main.c`. Ở đây chỉ Timer Service Task ("Tmr Svc"). |
| **Dormant** | Trạng thái timer đã tạo nhưng không đếm giờ. Khác "đã xoá" — §4.15. |
| **Drift** | Sai lệch tích luỹ khi mỗi chu kỳ tính từ "bây giờ" thay vì từ hạn cũ — §4.7. |
| **Guard** | Điều kiện boolean phải đúng thì transition mới xảy ra — §2. |
| **Level flag** | Cờ mô tả *trạng thái đang tồn tại* (cửa đang mở), đối lập với edge event mô tả *khoảnh khắc chuyển* — §2.5. |
| **Preempt** | Task priority cao hơn chiếm CPU của task đang chạy, không cần task đó nhường — §5.8. |
| **Seam** | Điểm tách phụ thuộc cho phép thay bằng bản giả khi test; cũng là ranh giới vùng mù của test — §4.29. |
| **Sticky flag** | Cờ level chỉ có đường set mà không có đường clear, mô tả quá khứ thay vì hiện tại — §3. |
| **Tick** | Đơn vị thời gian nhỏ nhất của kernel. Ở đây 1 tick = 1 ms — §5.7. |
| **Time slicing** | Chia đều CPU giữa các task cùng priority theo vòng — §5.8.1. |
| **Timer wheel** | Cấu trúc dữ liệu chèn O(1) cho số lượng timer rất lớn; Linux dùng, FreeRTOS không — §4.22. |

---

## §C.4 Lịch sử sửa đổi tài liệu

Nơi ghi các đính chính và thay đổi lớn. Thân bài luôn giữ nội dung **đúng
nhất hiện tại**; lịch sử sai/đúng nằm ở đây.

| Ngày | Thay đổi |
| --- | --- |
| 2026-10-11 | Gộp `docs/QA_LOG.md` vào đây thành **Phần D — Nhật ký Q&A**; nội dung và thứ tự Session giữ nguyên, file `QA_LOG.md` bị xoá. Tham chiếu cũ "QA_LOG Session NN" đọc là "Phần D, Session NN". |
| 2026-08-24 | Tái cấu trúc toàn tài liệu theo dạng reference manual: đánh mã mục cố định `§x.y`, gộp §4 (trước đây bị tách làm 3 mảnh DD-04 / DD-04 tiếp / DD-04 khép), chuẩn hoá tiêu đề sang giọng tra cứu, tách metadata buổi học khỏi thân bài, bổ sung §0 và Phần C. |
| 2026-08-24 | §B.3: sửa mô tả kiến trúc nút bấm đã lỗi thời — bản cũ mô tả một nút PA0 phân biệt short/long press; code thật dùng hai nút riêng (PA0 navigate, PD2 confirm) từ khi xử lý Bug #8. Bổ sung tầng callback bị thiếu trong sơ đồ ISR. |
| 2026-08-24 | §4.27–§4.29: bổ sung test logic phụ thuộc thời gian, so sánh với Linux/Zephyr/bare-metal, và ranh giới vùng mù của test. |
| 2026-08-23 | §4.20: đính chính — `xTimerPendFunctionCall()` được mô tả ở §4.9 như API sẵn dùng, thực tế `INCLUDE_xTimerPendFunctionCall` mặc định 0 nên không được biên dịch trong project này. |
| 2026-08-23 | §1.9.1: đính chính — bản trước khẳng định nhiều waiter cùng dùng `xClearOnExit = pdTRUE` sẽ tranh chấp khi clear. Sai: kernel xử lý nguyên tử trong `vTaskSuspendAll()`/`xTaskResumeAll()`. Vấn đề quyền sở hữu chỉ áp dụng cho lời gọi `xEventGroupClearBits()` độc lập. |
| 2026-08-23 | §1.2: sửa tên macro — từ V11 `configUSE_16_BIT_TICKS` đổi thành `configTICK_TYPE_WIDTH_IN_BITS`. |
| 2026-08-23 | §1.11: sửa số liệu — Task Notification **không** tốn 0 byte; mỗi TCB luôn mang ~8 byte cho `ulNotifiedValue[]`/`ucNotifyState[]`. Con số "45% nhanh hơn" của FreeRTOS là so với **binary semaphore**, không phải Event Group. |

### Bài học phương pháp

Rút ra từ chính các lần sai ở trên, áp dụng được cho mọi project:

1. **Đọc code → dự đoán hành vi → kiểm chứng trên phần cứng.** Kết quả khác
   dự đoán nghĩa là hoặc hiểu sai code, hoặc có yếu tố thứ ba — cả hai đều
   đáng tìm hiểu.
2. **Comment và tài liệu không được compiler kiểm tra** nên trôi khỏi code âm
   thầm. Project này đã có 4 lần comment mô tả sai hành vi thật, cộng thêm một
   mục tài liệu mô tả kiến trúc đã bị thay thế (§B.3). Luôn xác minh bằng code
   thật trước khi tin.
3. **Trace theo dữ liệu, không chỉ theo luồng gọi hàm.** Bug #17 chỉ lộ ra khi
   lần theo *giá trị của một bit* qua từng bước thao tác người dùng.
4. **Pattern lặp lại quan trọng hơn từng điểm dữ liệu đơn lẻ.** Hai chân GPIO
   khác hẳn nhau cùng đọc LOW → nghi ngờ yếu tố *không đổi* giữa hai lần đo,
   đừng đổi tiếp biến số thứ ba.
5. **Kết luận suy ra gián tiếp phải được gắn nhãn "chưa xác nhận"** — không
   trình bày giả thuyết như sự thật đã kiểm chứng.
6. **"API tồn tại trong FreeRTOS" khác "API dùng được trong build này".** Luôn
   kiểm tra macro `INCLUDE_*` / `configUSE_*` trước khi dùng hoặc trước khi
   ghi vào tài liệu — xem §C.2, bảng mặc định ẩn.

---

## §C.5 Lộ trình học và điều kiện tốt nghiệp

Bảng tiến độ chi tiết nằm ở đầu tài liệu ("Tiến độ Phần A").

**Điều kiện tốt nghiệp** không phải học hết bảng đó, mà là qua được **bài
kiểm tra thiết kế**: nhận một đề bài mới hoàn toàn (thang máy, máy pha cà phê,
hệ tưới cây…) và tự thiết kế được — bao nhiêu task, priority nào, dùng
primitive gì ở đâu, vẽ state machine — rồi được review như một junior engineer
thật.

---

## §C.6 Khung viết cho chương mới của Phần A

Mỗi chương Phần A viết theo thứ tự cố định để tra cứu được nhất quán:

```text
## §N Tên primitive

**Tóm tắt:** một đoạn — primitive này là gì, đặc tính phân biệt nó với
những primitive khác.
**Dùng khi:** …
**Không dùng khi:** … (kèm trỏ sang primitive phù hợp hơn)

### §N.1  Cấu trúc bên trong        — struct thật, kích thước tính từ source
### §N.2  Bảng API                  — kèm cột "gọi được từ ISR?"
### §N.3  Hành vi runtime           — trạng thái, chuyển trạng thái, context switch
### §N.4  Ràng buộc và điều kiện biên dịch
### §N.5  Pattern thực chiến        — dùng được ở project khác
### §N.6  Lỗi thường gặp
### §N.7  So sánh với primitive khác — bảng quyết định
### §N.8  Trong project này          — cách áp dụng cụ thể, đánh giá thiết kế
### §N.9  Câu hỏi phỏng vấn
```

Quy tắc đánh số: **không bao giờ đánh số lại mục cũ.** Nội dung bổ sung được
thêm vào cuối chương với số tiếp theo, để mọi tham chiếu chéo đã viết vẫn
đúng.

---

# PHẦN D — Nhật ký Q&A

Hỏi–đáp theo thứ tự thời gian, **mới nhất ở trên**. Phần A trả lời "cái này là gì, hoạt động ra sao" theo chủ đề; phần D giữ lại *câu hỏi thật* đã dẫn tới hiểu biết đó, kể cả những chỗ hiểu sai và được chỉnh lại. Cập nhật sau mỗi phiên Q&A, trước khi viết code.

Mục được đánh số theo Session, không đánh số lại. Tham chiếu chéo trong tài liệu dùng dạng "Phần D, Session NN".

---


## Session 19 — 2026-10-11 — `strncpy`, `strcpy`, `memset` trong `Manager_PostDisplay`

**Chủ đề:** Đọc dòng `strncpy(msg.statusText, ...)` ở `washing_manager.c:181` và hỏi vai trò
của các hàm `<string.h>` đi kèm. Không có thay đổi code.

---

### Q1: `memset` và `(void)strncpy` có vai trò gì?

**Lưu ý:** code hiện tại **không có `memset`**, chỉ có `strncpy` (dòng 181) và dòng ghi
`'\0'` (dòng 182).

**`(void)strncpy(msg.statusText, Manager_StatusText(state), sizeof(msg.statusText) - 1u);`**

- Copy chuỗi trạng thái vào mảng `char statusText[24]` nằm **bên trong** `DisplayMsg_t`
  (`app_msg.h:70`).
- Phải copy vào mảng chứ không gửi con trỏ: `xQueueSend` copy cả struct theo giá trị, nên
  chuỗi đi cùng message và OLEDTask không phụ thuộc lifetime của chuỗi nguồn.
- Dùng `strncpy` chứ không `strcpy` vì có giới hạn `n = 23`, tránh tràn buffer.
- `(void)` báo rõ là cố ý bỏ qua giá trị trả về (con trỏ `dest`).

**Dòng 182 `msg.statusText[sizeof(msg.statusText) - 1u] = '\0';`** là phần bắt buộc đi kèm,
vì `strncpy` không tự thêm `'\0'` khi nguồn dài ≥ `n` (xem Q2).

**`memset(&msg, 0, sizeof(msg))`** (nếu dùng): tô 0 cả struct. Ở đây không cần vì struct
27 byte không có padding (`freertos_objects.c:130`), 3 field còn lại đã gán tường minh, và
`strncpy` đã tự đệm `'\0'` khi nguồn ngắn. `memset` chỉ đáng dùng khi struct có padding
hoặc field chưa gán, vì queue copy cả rác đó.

| Lỗi hay gặp | Hậu quả |
| --- | --- |
| `strncpy(dst, src, sizeof(dst))` rồi quên ghi `'\0'` | Chuỗi không kết thúc, đọc tràn |
| `sizeof(ptr)` thay vì `sizeof(array)` | Chỉ copy 4 byte trên Cortex-M |
| Gán `msg.statusText = "..."` | Lỗi biên dịch, mảng không gán được |
| Đưa con trỏ chuỗi qua queue thay vì copy | Dangling pointer nếu chuỗi nằm trên stack |

---

### Q2: Chuỗi nguồn 30 ký tự thì `statusText` chứa gì? Bỏ dòng 182 thì sao?

**Bạn trả lời:**

> chứa 23 kí tự đầu của chuỗi 30 kí tự, kí tự cuối là `\0`, còn nếu ko có dòng 182 thì sẽ
> lấy 24 kí tự đầu … nhưng cuối sẽ ko hợp lệ và BSP_OLED_DrawText có thể đọc lan ra ngoài mảng.

**Phần đúng:**

- Có dòng 182: `statusText[0..22]` là 23 ký tự đầu, `statusText[23] = '\0'`. Chuỗi bị cắt
  cụt nhưng hợp lệ.
- Không có dòng 182: chuỗi không hợp lệ và `BSP_OLED_DrawText` có thể đọc tràn.

**Phần sai: "lấy 24 ký tự đầu".** Dòng 181 truyền `sizeof(...) - 1u` = **23**, nên
`strncpy` chỉ copy 23 ký tự. Byte 23 **không được đụng tới**, nó là **rác chưa khởi tạo**
của biến local `msg` trên stack (không có `memset`).

| Rác ở byte 23 | Kết quả |
| --- | --- |
| `0x00` (tình cờ) | Chạy đúng, bug ẩn đi |
| Khác `0x00` | `DrawText` đọc tiếp sang vùng lân cận cho đến khi gặp `0x00` |

Bug **không ổn định** (phụ thuộc trạng thái stack) nên rất khó debug.

**Điểm tinh tế:** với chuỗi **ngắn** (`"WASHING"`, 7 ký tự), `strncpy` đệm `'\0'` cho byte
7..22, nhưng ranh giới đệm là `n = 23`, nên byte 23 vẫn **không** được đệm. Dòng 182 cần
thiết cả với chuỗi ngắn, không chỉ chuỗi dài.

**Cách sửa thay thế:** `DisplayMsg_t msg = {0};` trước khi copy, khi đó dòng 182 thừa, đổi
lại tốn thêm 27 byte ghi mỗi lần gọi. Code hiện tại chọn ghi một byte vì rẻ và rõ ý đồ hơn.

---

### Q3: Các parameter của `strncpy` và `strcpy` là gì?

**`char *strcpy(char *dest, const char *src);`**

| Parameter | Ý nghĩa |
| --- | --- |
| `dest` | Buffer đích |
| `src` | Chuỗi nguồn kết thúc `'\0'`, chỉ đọc nên là `const` |

Copy đến và **bao gồm** `'\0'`. Không có tham số độ dài nên nếu `src` dài hơn `dest` thì
**tràn buffer**. Trả về `dest`.

**`char *strncpy(char *dest, const char *src, size_t n);`**

| Parameter | Ý nghĩa |
| --- | --- |
| `dest` | Buffer đích |
| `src` | Chuỗi nguồn |
| `n` | **Số byte tối đa ghi vào `dest`** (kích thước đích, không phải nguồn) |

| Điều kiện | Kết quả |
| --- | --- |
| `strlen(src) < n` | Copy cả `src` kèm `'\0'`, rồi **đệm `'\0'` cho đủ `n` byte** |
| `strlen(src) == n` | Copy `n` ký tự, **không có `'\0'`** |
| `strlen(src) > n` | Copy `n` ký tự đầu, **không có `'\0'`** |

Hai dòng cuối là lý do dòng 182 phải ghi `'\0'` thủ công. Cả hai hàm trong `<string.h>`;
`size_t` trên Cortex-M4 là `unsigned int` (4 byte). Hai vùng `dest`/`src` chồng lấn nhau là
undefined behavior, cần dùng `memmove`.

---

### Q4: Sau `xTimerStart` và khi timer fire, có phải nó tự set `EVT_TIMEOUT` không?

**Bạn hiểu:** đúng, nhưng chính xác hơn: timer **không tự** set event. Timer Service Task
gọi **callback** `Timer_PhaseCallback` (`freertos_objects.c:49`) do project đăng ký, và
callback đó gọi `xEventGroupSetBits(xSystemEvents, EVT_TIMEOUT)`. Cả 4 timer dùng chung
một callback, một bit, nên bit không cho biết timer nào nổ; chỉ state hiện tại phân biệt.

**Câu kiểm tra, bạn trả lời đúng:** quên `xTimerStop(xFillTimeoutTimer)` thì timer vẫn
đếm tiếp và `WASHING` sẽ thấy `EVT_TIMEOUT` sai, cắt ngắn chu kỳ giặt, rồi lan sang
các state sau (cascade).

---

### Q5: Tham số của `xTimerStart` / `xTimerStop`; queue mà lệnh được đẩy vào là gì?

- Cả hai là **macro** bọc `xTimerGenericCommand` (`timers.h:507, 550`):
  `(TimerHandle_t xTimer, TickType_t xTicksToWait)`, trả `pdPASS`/`pdFAIL`.
- `xTicksToWait` là thời gian **chờ gửi lệnh vào queue** khi đầy, không phải chu kỳ timer.
- Start gắn `xTaskGetTickCount()` lúc gọi vào lệnh; Stop gửi `0`.
- Queue là `xTimerQueue` (`timers.c:149`), queue FreeRTOS thường, `static` riêng của
  `timers.c`; dài `configTIMER_QUEUE_LENGTH = 10`, phần tử `DaemonTaskMessage_t`
  (~12 byte, chưa đo `sizeof`); chỉ **Timer Service Task** nhận. Tạo lười lúc
  `xTimerCreate` đầu tiên (`timers.c:1136`).
- Timer Service Task (Prio 2) < WashingManager (Prio 3): gọi `xTimerStart` **không**
  preempt; lệnh nằm trong queue đến khi manager block.
- **Lần gọi thứ 11** liên tiếp với `xTicksToWait = 0`: queue đầy → `pdFAIL`, lệnh mất, task
  chỉ biết nếu kiểm tra giá trị trả về (project hiện **không** kiểm tra).

---

### Q6: `xTimerStop` rồi `xTimerStart` xử lý theo thứ tự nào? Timing của project có đủ an toàn không?

**Bạn trả lời:** Stop rồi Start (đúng, FIFO); và timing đủ dài thì ổn.

**Chỉnh lại:** FIFO chỉ bảo đảm thứ tự giữa hai **lệnh**. Vòng lặp `prvTimerTask`
(`timers.c:774-777`) xử lý **timer đã hết hạn trước**, rồi mới đọc command queue. Timer
hết hạn khi lệnh STOP còn nằm trong queue sẽ vẫn chạy callback và set `EVT_TIMEOUT`.
Độ dài timer (5-20 s) chỉ làm xác suất nhỏ (~cửa sổ 1 ms / 20 000 ms), không làm nó bằng 0.

**Hệ quả nếu trúng:** bit `EVT_TIMEOUT` mồ côi, lần Start sau FILL_WATER báo lỗi ngay.

---

### Q7: `xClearOnExit = pdTRUE` xóa bit nào?

**Bạn đoán:** xóa hết event. **Đúng về cơ chế**, chính xác hơn: xóa **cả mask
`uxBitsToWaitFor`** (`event_groups.c:353` và `614/637`), không phải chỉ bit khớp và
không phải cả group.

**Hệ quả:** (1) hai bit cùng set thì chỉ branch `if` đầu chạy, bit kia **mất**;
(2) trong đường block, bit bị xóa ngay lúc `SetBits` đánh thức, nên bit đến sau đó
(trước khi task chạy lại) nằm lại trong group, không có trong `bits` trả về.

---

### Q8: Sửa theo hướng (a) đảo thứ tự hay (b) xử lý cả hai bit?

**Bạn chọn (a), đúng:** lỗi phải thắng event thường; event thường vô nghĩa khi đã vào ERROR.
(b) cũng chạy được nhưng dựa vào việc bảng `kTransitions[]` "tình cờ" không có dòng khớp.

**Phát hiện thêm:** FILL_WATER trước đó có thứ tự `WATER_FULL → TIMEOUT → ERROR`,
ERROR ở **cuối** ở cả 4 state.

**Đã sửa** (`washing_manager.c`): ERROR kiểm tra đầu tiên ở cả 4 state, và thêm
`Manager_StartPhaseTimer()` clear `EVT_TIMEOUT` trước `xTimerStart`. Build OK, chưa nạp
board. Chi tiết và phần còn tồn tại xem `PROJECT_PROGRESS.md` mục 2026-10-11.

**Đính chính của mình:** trước đó mình nói `ClearBits` ở entry-action làm cho việc này
"chắc chắn". Chưa đúng: nó chỉ dọn bit đến **trước** entry. Bit do timer cũ hết hạn trong
lúc lệnh STOP còn trong queue vẫn có thể set **sau** lần clear. Muốn kín hẳn cần mỗi timer
một bit riêng.

---

### Câu hỏi còn mở (chưa trả lời)

1. Sau khi `Manager_PostDisplay` return, `msg` trên stack bị hủy. Vì sao OLEDTask vẫn đọc
   đúng `statusText`? Queue đã làm gì ở bước `xQueueSend`?
2. `strncpy(buf, "HI", 8)` với `char buf[8]` chưa khởi tạo: 8 byte của `buf` có giá trị gì?
   Và `strcpy(buf, "HELLO WORLD")` thì chuyện gì xảy ra?
3. Có nên tách `EVT_TIMEOUT` thành một bit riêng cho mỗi timer để đóng hẳn cửa sổ còn lại?
   Và `xTimerStart` trả `pdFAIL` thì máy nên làm gì?

---

## Session 18 — 2026-10-11 — Ba câu hỏi về scheduler, timer và state machine

**Chủ đề:** Ba thắc mắc khi đọc sơ đồ tuần tự và `washing_manager.c`: OLEDTask hiển thị
thế nào khi chưa được chạy, timer làm gì sau khi hết giờ, và `WashingSM_ProcessEvent`
bên trong làm những gì.

---

### Q1: Sau `xQueueSend(DisplayMsg "FILL WATER")` vẫn đang ở WashingManager, OLEDTask chưa chạy — sao OLED hiển thị được?

**Câu hỏi của bạn (từ sơ đồ §3 kịch bản A):** đoạn này vẫn ở manager task; gửi queue
xong thì OLEDTask chưa chạy, vậy sao OLED hiển thị được?

**Bạn hiểu đúng:** ngay sau `xQueueSend`, OLEDTask **chưa** chạy.

**Giải thích:**

1. `xQueueSend` bỏ message vào `xDisplayQueue`. OLEDTask đang `Blocked` chờ queue
   (`xQueueReceive`, `portMAX_DELAY`) nên chuyển sang `Ready`.
2. OLEDTask có Prio 1, WashingManager Prio 3. Scheduler luôn chọn task `Ready` có
   priority cao nhất ⇒ **không preempt**, WashingManager chạy tiếp.
3. WashingManager chạy tới `xEventGroupWaitBits(...)` (hoặc `xQueueReceive`) và tự
   chuyển sang `Blocked`.
4. Lúc đó task `Ready` cao nhất là OLEDTask ⇒ nó chạy, lấy message, vẽ qua I2C.

Độ trễ giữa bước 1 và 4 cỡ vài chục µs nên mắt thấy như tức thời. OLEDTask để Prio thấp
vì vẽ qua I2C mất vài chục ms, không được chen vào lúc manager điều khiển motor/van.

**Sơ đồ cũ gây hiểu lầm:** vẽ "Render" nằm trước dòng block của manager. Đã sửa
`SYSTEM_DIAGRAMS.md` §3–§5: "Render" nằm **sau** dòng block, kèm ghi chú
"Ready nhưng CHƯA chạy (Prio 1 < 3)".

**Ý chính:** gửi vào queue không có nghĩa là bên nhận chạy ngay. Nhận chạy khi nó là task
`Ready` có priority cao nhất — tức khi bên gửi block hoặc bị preempt.

---

### Q2: Sau `xTimerStart`, khi timer hết giờ thì nó reset hay đếm tiếp?

**Trả lời: không reset, không đếm tiếp — dừng hẳn.**

Cả 4 timer tạo với `uxAutoReload = pdFALSE` (one-shot,
`freertos_objects.c:162-183`). Khi hết giờ, Timer Service Task chạy
`prvProcessExpiredTimer` (`timers.c:718-745`):

1. gỡ timer khỏi danh sách đang chạy;
2. vì không phải auto-reload: xoá cờ `tmrSTATUS_IS_ACTIVE` (dòng 739) → timer **dormant**
   (còn handle và chu kỳ, nhưng không đếm);
3. gọi callback (dòng 744) → `Timer_PhaseCallback()` set `EVT_TIMEOUT`.

Auto-reload (`pdTRUE`) thì bước 2 tính lần hết hạn kế tiếp và nạp lại, tự lặp.

**Chạy lại:** entry-action của state gọi `xTimerStart` lần nữa
(`washing_manager.c:80,88,96,104`); đếm lại từ **đầu chu kỳ đầy đủ**, không nhớ lần trước.

| Tình huống | Hành vi |
| --- | --- |
| Muốn dừng sớm (nước đầy trước 10 s) | `xTimerStop` — nếu không, timer vẫn nổ và set `EVT_TIMEOUT` oan |
| `xTimerStart` trên timer đang chạy | **Reset** về đầy chu kỳ (như `xTimerReset`) |

---

### Q3: `WashingSM_ProcessEvent(WASH_EVT_SPIN_DONE)` làm những gì?

Gọi khi manager ở `SPIN` và `xSpinTimer` hết giờ. Theo `washing_sm.c:138-149`:

1. **Tra bảng** `kTransitions[]`: tìm dòng có `fromState == s_currentState` và
   `event == SPIN_DONE`.
2. **Khớp dòng 51** `{SPIN, SPIN_DONE, FINISH}` → `s_currentState = FINISH`.
3. **Chạy entry-action** `SM_ExecuteEntryAction(FINISH)` → `Manager_OnEnterFinish()`
   (`washing_manager.c:108`): `BSP_Motor_Stop()`, `BSP_Valve_Close(DRAIN)`, `BSP_LED_Off()`,
   set `EVT_FINISHED`, `xQueueReset(xCommandQueue)`, `Manager_PostDisplay(FINISH)`.
4. **`break`** — chỉ dòng khớp đầu tiên chạy; trả về state mới.

**Hai điểm cần nhớ:**

- SM **không biết timer/motor/OLED**; nó chỉ tra bảng và gọi callback do WashingManager
  đăng ký lúc init ⇒ `washing_sm.c` không phụ thuộc FreeRTOS hay phần cứng.
- Không có dòng khớp (ví dụ `SPIN_DONE` khi ở `IDLE`) ⇒ **âm thầm bỏ qua**, state giữ nguyên.

Sau khi trả về, vòng lặp manager quay lại `switch (state)`; state là `FINISH` nên nó chặn ở
`xQueueReceive(xCommandQueue)` chờ người dùng bấm PD2.

---

## Session 17 — 2026-10-10 — Van cấp nước dừng theo cảm biến, không theo giờ

**Yêu cầu:** "van xả nước vào lồng giặt lúc bấm giặt … xả đến lúc cảm biến báo đầy thì dừng
chứ không xả theo thời gian nữa".

**Hiểu nhầm lần đầu:** tôi tưởng là van xả (`DRAIN`) và đã sửa `DRAIN` kết thúc bằng bit
`EVT_WATER_EMPTY`. Sai đối tượng — đã hoàn tác toàn bộ. Người dùng nói về **van cấp nước**.

**Thực tế code:** `FILL_WATER` đã chờ `EVT_WATER_FULL | EVT_TIMEOUT | EVT_ERROR`; van cấp
đóng khi cảm biến báo đầy. 10 s chỉ là timeout an toàn (hết giờ → `ERROR`). Chính tài liệu
ghi "Bơm nước: 10 giây" khiến nó giống chạy theo giờ → đã sửa chữ trong START_HERE, SPEC.

**Lỗi thật tìm thấy:** `EVT_WATER_FULL` không bao giờ bị xoá → bit cũ làm `FILL_WATER`
ở chu trình sau kết thúc oan. Nay SensorTask xoá bit khi cảm biến báo cạn.

---

## Session 16 — 2026-10-10 — Một biến, một chủ: sửa lỗi `s_statusCache`

**Chủ đề:** Khi viết Tầng 3 phát hiện UITask hiển thị trạng thái `IDLE` cũ và giữ
một con trỏ menu riêng lệch với WashingManager. Sửa bằng cách dồn quyền sở hữu về
WashingManager, rồi đồng bộ toàn bộ tài liệu.

---

### Q1: Sửa theo hướng nào?

UITask có `s_statusCache` không bao giờ được ghi (luôn `IDLE`), và con trỏ menu
tồn tại ở hai nơi. Ba phương án; chọn **hướng 2**: WashingManager giữ con trỏ và là
bên duy nhất dựng `DisplayMsg_t`; UITask thành bộ dịch không trạng thái
(`BTN_EVENT_NAVIGATE → CMD_NAVIGATE`, `CONFIRM → CMD_CONFIRM`). Hệ quả có chủ ý: nút
bị bỏ qua lúc đang chạy chu trình.

### Q2: `sizeof` của message là bao nhiêu?

Tôi từng "sửa" comment thành 12/36 byte theo giả định enum = 4 byte — **sai**.
Đo bằng probe `char (*p)[sizeof(T)] = 1;` cho ra 1/1/27 vì arm-none-eabi-gcc dùng
`-fshort-enums`. Bài học: đo, đừng đoán.

### Q3: Vì sao file có byte NUL?

Một lớp xử lý chuỗi nuốt một nửa dấu `\` trong tham số Bash/node nên `\0` thành NUL.
Sửa bằng mã ký tự; về sau dùng Edit/Write cho văn bản có backslash.

### Q4: Kiểm chứng thế nào khi chưa nạp được board?

Biên dịch sạch 0 cảnh báo; chạy mã thật của manager + SM trên host (37/37) và thử
"đột biến" — đưa lại từng lỗi cũ, bộ test đều bắt được. **Chưa** kiểm chứng trên phần cứng.

### Q5: Có nên bỏ UITask?

Sau bản sửa nó chỉ đổi tên enum, và phản đối "manager phải chờ nhiều nguồn" gần như
hết (nút không dùng lúc đang chạy). Về kỹ thuật bỏ được; chưa quyết định.

### Ghi chú tài liệu

SYSTEM_DIAGRAMS §1 tách thành 1a/1b/1c (mỗi sơ đồ ≤ ~800 px). Sơ đồ tuần tự §5 gãy vì
dấu `;` trong `Note` (Mermaid coi là ngăn lệnh) — đã đổi thành dấu phẩy.

---

## Session 15 — 2026-10-09 — Thiết kế lại HTML: sơ đồ zoom được

**Chủ đề:** Sơ đồ kiến trúc trong `SYSTEM_DIAGRAMS.html` bị co cho vừa chiều rộng
trang nên chữ không đọc được và không phóng to được. Dựng lại toàn bộ bộ HTML.

---

### Q1: Vì sao không zoom được, và nên sửa ở đâu?

**Chẩn đoán:** Mermaid mặc định đặt `useMaxWidth: true` — tự co SVG cho vừa khung
chứa nó. Sơ đồ kiến trúc tổng thể thực chất rộng **2757 × 2157 px**; nhét vào cột
960 px thì còn ~35 %, chữ ~5 px. Trình duyệt không zoom riêng từng sơ đồ được,
và Ctrl + cuộn zoom cả trang (làm vỡ bố cục chứ không giúp đọc sơ đồ).

**Hai phát hiện kèm theo khi khảo sát 3 file HTML:**

| File | Vấn đề |
| --- | --- |
| `PROJECT_SPEC.html` | Nạp `mermaid.min.js` và `katex.css` từ `file:///c:\Users\Dell\.vscode\extensions\...` — chỉ mở được trên đúng máy này, và gãy khi extension đổi phiên bản |
| `SYSTEM_DIAGRAMS.html` | Viết tay, ghi chú "regenerate when diagrams change" nhưng không có công cụ nào để regenerate → sẽ lệch khỏi `.md` |
| `START_HERE.html` | Xuất từ Markdown Preview Enhanced; cũ hơn `.md` 16 phút ở phiên trước |

Cả ba do ba cách khác nhau tạo ra. Đây lại là vấn đề "một sự thật nằm ở hai chỗ".

**Quyết định:** một script sinh cả ba từ `.md`, chạy lại là đồng bộ.

---

### Thiết kế

**Công cụ** (`tools/`): `build_docs.mjs` + `template.html` + `viewer.css` +
`viewer.js`. Phụ thuộc: `markdown-it` (render), `highlight.js` (tô màu C/bash).
Mermaid vẫn nạp từ CDN, **ghim phiên bản 10.9.8** (trước đó `@10` trôi theo bản mới).
Không dùng timestamp trong output → build lại khi `.md` không đổi thì **không sinh
git diff** (đã kiểm chứng bằng sha1).

**Bộ điều khiển sơ đồ:**

| Thao tác | Cách làm |
| --- | --- |
| Di chuyển | Kéo chuột |
| Zoom tại con trỏ | **Ctrl + cuộn** (hoặc pinch trackpad); trong toàn màn hình thì cuộn thường |
| Phóng nhanh | Nhấp đúp (Shift + nhấp đúp để thu) |
| Nút | `−`  `+`  `1:1`  `Vừa rộng`  `Vừa khung`  `⤢` |
| Phím (khi khung có focus) | `+ −` zoom · `0` vừa khung · `1` 100 % · `W` vừa rộng · `F` toàn màn hình · mũi tên di chuyển · `Esc` thoát |

**Quyết định thiết kế đáng ghi lại:**

1. **Inline chỉ zoom bằng Ctrl + cuộn, không phải cuộn thường.** Nếu cuộn thường
   cũng zoom thì sơ đồ "nuốt" thao tác cuộn trang — người đọc rê chuột qua sơ đồ
   là bị kẹt. Chỉ trong toàn màn hình (không còn trang để cuộn) mới cho cuộn thường
   = zoom. Đã kiểm chứng: cuộn thường trên sơ đồ làm `scrollY` tăng, zoom đứng yên.
2. **Mặc định "vừa chiều rộng", không phải "vừa khung".** Sơ đồ cao như sequence
   diagram mà ép vừa khung sẽ co rất nhỏ; vừa chiều rộng đọc được ngay và kéo dọc
   là xem tiếp. "Vừa khung" vẫn có nút riêng cho ai muốn nhìn toàn cảnh.
3. **Giữ kích thước gốc của SVG** (`useMaxWidth: false`) và scale bằng CSS
   `transform` → chữ vector, zoom 800 % vẫn sắc nét, không bị vỡ như ảnh bitmap.
4. **Cảm ứng:** trong trang, cảm ứng vẫn cuộn trang bình thường
   (`touch-action: pan-x pan-y`); pinch/kéo chỉ bật trong toàn màn hình. Nếu bật
   luôn thì ngón tay chạm vào sơ đồ là không cuộn trang được.
5. **Mất mạng không vỡ trang:** nếu không tải được Mermaid, mỗi khung hiện thông
   báo và **mã nguồn sơ đồ dạng text** thay vì để trống.
6. Đổi giao diện sáng/tối **vẽ lại** sơ đồ với theme tương ứng (Mermaid cố định
   màu lúc vẽ, đổi CSS thôi là không đủ).

**Thêm cho cả 3 trang:** mục lục bên trái (tự mở chương đang đọc), tô màu cú pháp C,
bảng rộng cuộn ngang, neo `#` ở mỗi tiêu đề, link `.md` → `.html` khi đích cũng
được build, thanh chuyển giữa các tài liệu.

---

### Kiểm chứng: điều khiển Chrome thật, không chỉ "build xong"

Dùng `puppeteer-core` + Chrome (cài ở thư mục tạm, **không** thêm vào project).
**25/25 kiểm tra đạt**: 9/9 sơ đồ vẽ ra SVG · giữ kích thước gốc (2757 × 2157) ·
nút `+` / `1:1` · Ctrl + cuộn zoom · cuộn thường **không** zoom · kéo chuột dịch
đúng 120 × 60 px · nhấp đúp · toàn màn hình phủ đủ 1400 × 900 · cuộn zoom trong
toàn màn hình · Esc thoát · đổi sang tối vẽ lại đủ 9 sơ đồ · mục lục tô đúng chương
· không lỗi JS · `PROJECT_SPEC.html` không còn đường dẫn extension.
Tĩnh: **0** liên kết `#` gãy (215 liên kết), **0** placeholder sót.

**Bài học từ chính quá trình test:** lần chạy đầu có 2 FAIL, và **cả hai đều do
bài test sai, không phải trang sai** — nhưng không thể biết điều đó nếu không tách
ra điều tra:

- *Nhấp đúp:* Puppeteer luôn gửi `clickCount: 1` nên trình duyệt không sinh
  `dblclick`. Gửi sự kiện giả `dblclick` → zoom đúng ×1.8 (0.367 → 0.661); gửi
  chuỗi `clickCount` 1 rồi 2 như chuột thật → cũng đúng. Kết luận: trang đúng.
- *Mục lục:* `scroll-behavior: smooth` khiến test đọc trạng thái giữa chừng lúc cuộn.

Quy tắc rút ra: **test fail chưa phải bug — phải cô lập xem lỗi ở đối tượng được
test hay ở công cụ test, trước khi sửa bất cứ thứ gì.**

---

### Còn tồn tại: bản thân sơ đồ kiến trúc quá lớn

Zoom giải quyết việc đọc, nhưng **sơ đồ tổng thể rộng 2757 × 2157 px là dấu hiệu
sơ đồ làm quá nhiều việc**: 4 task + 3 queue + 1 event group + 4 timer + 4 GPIO +
2 ISR trong một hình, với rất nhiều đường chéo nhau. Ngay cả toàn màn hình vẫn chỉ
vừa ở ~38 %. Hướng cải thiện (chưa làm, chờ quyết định): tách thành 3 sơ đồ —
luồng nút bấm / luồng cảm biến + event group / luồng timer — mỗi cái dưới ~1200 px.

---

### Q2: Minh hoạ mục "7. RTOS Architecture" của PROJECT_SPEC bằng sơ đồ dễ nhìn

**Hiện trạng:** mục 7 là chuỗi chữ xuống dòng với mũi tên `↓ ↑` ở giữa — 12 đoạn
văn rời, người đọc phải tự ghép lại. Nó còn **thiếu Software Timer** dù mục 9 của
chính đặc tả khai timer là nguồn của `EVT_TIMEOUT`.

**Đã làm:** thay bằng một sơ đồ Mermaid trong `PROJECT_SPEC.md` (nguồn), kèm bảng
chú giải ký hiệu và 3 dòng "Cách đọc". HTML tự cập nhật qua `npm run build`.

**Quyết định thiết kế — vẽ qua 4 vòng lặp, mỗi vòng sửa đúng một lỗi nhìn thấy:**

| Vòng | Bố cục | Lỗi nhìn thấy | Sửa |
| --- | --- | --- | --- |
| 1 | Ngang (`LR`) | Rộng **1668 px** → bị co còn 61 %, chữ nhỏ; nền vàng mặc định lệch tông | Đổi sang dọc (`TB`) → 807 px, đọc được ở 100 %; tô nền khung bằng màu xám **có độ trong suốt** để dùng được cả giao diện sáng lẫn tối |
| 2 | Dọc | Mũi tên đi **xuyên qua tiêu đề** "③ Luồng hiển thị"; cao 990 px bị cắt | Nối `WM --> DSP` (vào **khung**, không vào node bên trong) → mũi tên dừng ở viền; đồng thời Mermaid tôn trọng `direction LR` bên trong khung ③ nên DQ → OLED nằm ngang, bớt một tầng |
| 3 | + nén khoảng cách | Tham số `padding: 10` làm node **đè lên tiêu đề** ① ② | Bỏ `padding`; dùng `subGraphTitleMargin` chừa riêng chỗ cho tiêu đề |
| 4 | Hoàn chỉnh | Cao 840 px, màn 900 px chỉ hiện ~740 → cắt đáy khung ③ | Sửa **bộ xem** (xem dưới) |

**Bài học Mermaid đáng nhớ:** khi một khung con (`subgraph`) có node bị nối ra
ngoài, `direction` bên trong bị **bỏ qua** và kế thừa hướng toàn cục. Nối vào
**chính khung** (`WM --> DSP`) thay vì node bên trong thì hướng riêng được giữ.
Mẹo này vừa tránh va chạm mũi tên–tiêu đề, vừa làm sơ đồ gọn hơn.

**Cải tiến bộ xem rút ra từ sơ đồ này:** sơ đồ chỉ cao hơn khung một chút thì
đọc trọn vẹn dễ hơn là phải cuộn. `viewer.js` giờ tự thu nhỏ **tối đa 30 %** để
sơ đồ vừa khung; sơ đồ cao hơn nhiều (sequence diagram) vẫn giữ "vừa chiều rộng"
rồi kéo dọc. Trên màn 900 px, sơ đồ này hiện trọn ở 85 %.

**Kiểm chứng:** chụp ảnh Chrome thật ở sáng + tối sau mỗi vòng; chạy lại bộ test
**25/25 đạt**. Một kiểm tra lúc đầu FAIL (`PROJECT_SPEC.html` kỳ vọng 1 sơ đồ,
giờ có 2) — đây là FAIL **đúng**, vì nội dung đã thay đổi chủ ý; cập nhật con số
kỳ vọng trong test chứ không "sửa cho test qua".

**Khớp với code thật:** các bit event (`DOOR_CLOSED · WATER_FULL · TIMEOUT ·
FINISHED · ERROR`), priority (UI 2, Sensor 2, Manager 3, OLED 1), chu kỳ Sensor
100 ms đối chiếu với `app_events.h` / `app_config.h`. Sơ đồ vẫn theo **đặc tả**
(có UI Task) — nếu sau này gộp UI Task vào Manager thì phải sửa cả mục 8, 9 chứ
không riêng sơ đồ.

---

## Session 14 — 2026-08-31 — Viết Tầng 3 & phát hiện bug `s_statusCache`

**Chủ đề:** Viết Tầng 3 (Queue) của `START_HERE.md`. Trong lúc đọc `ui_task.c`
để trích code, phát hiện một lỗi thiết kế kênh liên lạc có triệu chứng nhìn thấy
được trên màn hình.

---

### Q1: "ổn rồi viết tiếp tầng 2" — nhưng Tầng 2 đã viết xong

**Nguyên nhân hiểu nhầm:** bạn đang đọc `docs/START_HERE.html` — bản xuất bằng
Markdown Preview Enhanced lúc **22:03**, trong khi Tầng 2 được ghi vào `.md` lúc
**22:19**. HTML cũ hơn 16 phút nên vẫn hiện "Tầng 2–6 — 🚧 Chưa viết".

**Rút ra cho quy trình:** `START_HERE.html` là **file dẫn xuất**, không tự cập
nhật. Mỗi lần sửa `.md` phải xuất lại (chuột phải → Markdown Preview Enhanced →
Export → HTML), nếu không sẽ đọc nhầm bản cũ. Đây đúng là vấn đề "một sự thật
nằm ở hai chỗ" mà §0.1 của chính tài liệu đó cảnh báo — lần này áp vào chính bộ
tài liệu.

---

### Q2: Bug phát hiện khi viết Tầng 3 — `s_statusCache` không bao giờ được ghi

**Triệu chứng:** đang giặt, bấm PA0 để di chuyển con trỏ menu → dòng trạng thái
OLED đổi từ `WASHING` thành `IDLE`, dù máy vẫn đang giặt (đèn LED vẫn đúng).

**Truy nguyên:**

```c
/* ui_task.c:44-46 */
/* Cached status text from last WashingManager display update.
 * UITask piggybacks this when it sends its own cursor-update messages. */
static char s_statusCache[24] = "IDLE";
```

Grep cả file: `s_statusCache` xuất hiện **đúng hai lần** — dòng khai báo và dòng
đọc (`ui_task.c:65`). **Không có dòng nào ghi vào nó.** Comment mô tả một cơ chế
cache không tồn tại, và cũng **không thể tồn tại**: `xDisplayQueue` là kênh một
chiều `WashingManager → OLEDTask`, UITask không đọc nó nên không có đường nào
biết được statusText hiện tại.

**Chẩn đoán — đây là lỗi thiết kế kênh, không phải lỗi đánh máy:**

`DisplayMsg_t` chở **cả cụm** (menuIndex + statusText + state + selectedMode),
nhưng có **hai producer** cùng gửi vào `xDisplayQueue`, và mỗi bên chỉ biết một
phần:

| Producer | Biết | Phải bịa |
| --- | --- | --- |
| WashingManager | state, statusText, selectedMode | `menuIndex` (giữ bản sao riêng `s_menuIndex`) |
| UITask | menuIndex | `statusText` (bịa bằng hằng `"IDLE"`) |

Vi phạm đúng tiêu chí ownership đã dạy ở Tầng 2: **nội dung màn hình đang có hai
chủ.**

**Chứng cứ phụ củng cố chẩn đoán:** `ui_task.c:63` khai `DisplayMsg_t disp;` rồi
chỉ gán `menuIndex` và `statusText`, **để nguyên `state` và `selectedMode` chưa
khởi tạo** — queue vẫn trung thành copy đủ 36 byte kể cả hai trường rác. Chưa
gây triệu chứng vì `OLED_Render()` không dùng tới chúng, nhưng nó cho thấy chính
người viết cũng cảm nhận được UITask không đủ thông tin để dựng một `DisplayMsg_t`
hoàn chỉnh.

**Ba hướng sửa đã nêu trong tài liệu (§3.6), chưa áp dụng — chờ quyết định:**

| Hướng | Cách làm | Đánh đổi |
| --- | --- | --- |
| 1. OLEDTask nhớ trạng thái | Thêm `updateMask` vào `DisplayMsg_t`; OLEDTask giữ khung hình gần nhất, chỉ ghi đè phần được đánh dấu | Sửa ít nhất; OLEDTask không còn là pure renderer |
| 2. UITask không gửi thẳng OLED | UITask báo "cursor moved" cho WashingManager; bên duy nhất biết đủ mọi thứ dựng `DisplayMsg_t` | Đúng nguyên tắc một chủ nhất; thêm một chặng |
| 3. Tách hai queue | Một cho trạng thái, một cho con trỏ; OLEDTask ghép | Ranh giới sạch nhất; tốn RAM và một chỗ chờ |

Nghiêng về hướng 2 vì hợp nguyên tắc "WashingManager là chủ duy nhất của trạng
thái", nhưng cả ba đều hợp lệ.

**Đã sửa ngay trong tài liệu:** Thí nghiệm A của Tầng 1 bảo người học bấm PA0 lúc
đang giặt, nhưng không nhắc gì tới việc dòng trạng thái nhảy về `IDLE`. Đã thêm
ghi chú cảnh báo đây là lỗi thật, kèm câu hỏi gợi mở dẫn sang Tầng 3 ("đèn vẫn
đúng nhưng chữ thì sai — hai thứ đó đi bằng hai đường khác nhau, đường nào?").

---

### Nội dung Tầng 3

**Cách ngây thơ được chọn để bác bỏ:** biến toàn cục + `volatile`. Bốn lỗ hổng
xếp theo độ khó phát hiện tăng dần: mất sự kiện → không block được → race trên
đọc-sửa-ghi (`LDR`/`ADD`/`STR`) → **struct bị đọc nửa vời**. Nhấn mạnh
`volatile` **không** cho tính nguyên tử — hiểu nhầm phổ biến nhất về từ khoá này.

**Bốn tính chất của Queue**, mỗi tính chất khớp một lỗ hổng: có chiều sâu · chặn
được hai chiều · thao tác nguyên tử · **sao chép theo giá trị**.

**Thí nghiệm A (quyết định):** thêm `disp.menuIndex = 99u;` **ngay sau**
`xQueueSend()` trong `ui_task.c:68`. Con trỏ vẫn nhảy đúng → chứng minh queue đã
chụp ảnh 36 byte trong lúc hàm còn chạy, không giữ con trỏ. Lập luận sắc hơn đi
kèm: `disp` nằm trên stack UITask và khung stack đó bị thu hồi rất lâu trước khi
OLEDTask (prio 1) được chạy — nếu queue giữ con trỏ thì đây là dangling pointer
100 % lần nào cũng dính.

**Thí nghiệm B:** bấm PD2 giữa lúc đang giặt → lệnh nằm đệm trong `xCommandQueue`
→ tới `FINISH` bị `xQueueReset()` dọn → máy đứng yên. Bỏ dòng reset đó đi chính
là **Bug #10**.

**Điểm thiết kế được nhấn mạnh:** vì sao **mọi** chỗ gửi đều dùng timeout `0`.
Nếu `Manager_PostDisplay()` dùng `portMAX_DELAY`, WashingManager (prio 3) sẽ bị
OLEDTask (prio 1) chặn — đảo ưu tiên thủ công. Quy tắc mang đi được: *bên gửi ở
priority cao không bao giờ được block chờ bên nhận ở priority thấp.*

Câu 2 mục Tự kiểm tra mở rộng thêm một bước suy luận đáng tập thành phản xạ:
chặn UITask không chỉ làm chậm UITask — UITask là bên tiêu thụ duy nhất của
`xButtonQueue`, nó dừng thì queue nút bấm đầy dần. **Chặn một task luôn kéo theo
hậu quả lan ngược lên thượng nguồn của nó.**

**Giữ kỷ luật một tầng một khái niệm:** `xQueueSendFromISR` và
`portYIELD_FROM_ISR` được trích và giải thích ở mức "chuyện gì xảy ra theo trình
tự", nhưng PendSV/BASEPRI/NVIC để nguyên cho Tầng 6.

---

### Sửa kèm trong code

`freertos_objects.c` — comment ghi `sizeof(CommandMsg_t) = 4 + 4 = 8 bytes`, sai
từ khi trường `menuIndex` được thêm vào struct. Thực tế là `4 + 4 + 1 + 3 padding
= 12`. Đã sửa cả dòng đó lẫn bảng "Approximate RAM cost" (`~116` → `~136`,
tổng `~544` → `~564`). Build lại: sạch.

Lần thứ ba trong project phát hiện **comment mô tả sai code thật** (trước đó:
`app_events.h` ở Session 09, `bsp_button.h` ở Session 13). Củng cố nguyên tắc đã
ghi ở "Bài học phương pháp": comment không được compiler kiểm tra, chỉ phát hiện
sai khi chủ động đối chiếu.

---

## Session 13 — 2026-08-29 — Dọn tàn dư short/long press trong bsp_button.c

**Chủ đề:** Người dùng phát hiện `BSP_Button_IRQHandler()` vẫn còn bộ máy đo
thời lượng nhấn, dù thiết kế short/long press đã bỏ từ lâu. Truy vết, thiết kế
lại bộ lọc nhiễu, áp dụng cho cả hai nút.

---

### Q1: "code chỗ này chưa sửa thì phải làm gì còn long short press gì nữa"

**Quan sát của bạn:** đúng. Kiểm tra lại tìm ra ba tàn dư:

| # | Tàn dư | Vị trí |
| --- | --- | --- |
| 1 | Macro chết `BUTTON_LONG_PRESS_MS` — grep toàn bộ `App/` + `Core/Src/`: không `.c` nào dùng | `app_config.h:35` |
| 2 | Bộ máy đo thời lượng: `s_pressStartMs`, `s_pressed`, biến `duration` | `bsp_button.c:28-29, 44-52` |
| 3 | Comment tự mâu thuẫn: "check duration for debounce, then fire short press" rồi ngay dưới phải đính chính "Long-press is no longer handled" | `bsp_button.c:47, 53-54` |

**Hệ quả thật, không chỉ là thẩm mỹ:** cấu trúc `if (rising) ghi giờ / else
(falling) bắn sự kiện` khiến sự kiện **chỉ bắn khi THẢ TAY**. Đó là ràng buộc
bắt buộc của thiết kế cũ (phải đợi buông mới đo được thời lượng), nhưng vô nghĩa
sau khi long-press bị bỏ. Giữ nút PA0 3 giây thì con trỏ menu đứng im tới lúc
buông. Nút PD2 ngay bên dưới thì bắn ngay lúc bấm — **hai nút cùng project, hai
triết lý khác nhau, và khác biệt đó không do yêu cầu nào cả.**

Đây cũng chính là điều kiện kích hoạt **Bug #8** ngày trước.

---

### Q2: Phát hiện thêm — PD2 hoàn toàn không có bộ lọc nhiễu

`BSP_StartButton_IRQHandler()` bắn `BTN_EVENT_CONFIRM` trên **mọi** cạnh xuống.
Một lần bấm dội 3-4 cạnh = 3-4 sự kiện CONFIRM.

Triệu chứng đang bị che đi chứ không phải không có: `Manager_OnEnterIdle/Finish/
Error` đều gọi `xQueueReset(xCommandQueue)` nên lệnh thừa bị dọn ở hạ nguồn.
**Hạ nguồn dọn rác cho thượng nguồn** — thượng nguồn vẫn đang sinh sự kiện giả.

**Bạn quyết:** cải tiến cả PD2.

---

### Thiết kế đã chọn: lọc theo khoảng lặng (*quiet window*)

```c
uint32_t now   = HAL_GetTick();
uint8_t  quiet = ((now - s_lastEdgeMs) >= BUTTON_DEBOUNCE_MS) ? 1u : 0u;

s_lastEdgeMs = now;        /* MỌI cạnh đều nạp lại — kể cả cạnh bị từ chối */

if (pressed && quiet && (s_callback != NULL)) { s_callback(...); }
```

Điểm mấu chốt là `s_lastEdgeMs = now;` nằm **ngoài** mọi điều kiện.

**Phương án ngây thơ đã loại:** "bỏ qua cạnh cách *sự kiện đã chấp nhận* gần nhất
dưới 50 ms". Hỏng khi giữ nút lâu: chùm dội lúc nhả cách sự kiện trước 2 giây →
lọt thành sự kiện giả. Phải so với **cạnh gần nhất**, không phải **sự kiện gần
nhất**.

**Phương án thứ hai đã loại:** đọc thêm mức chân cho PD2 để xác nhận nút đang bị
đè. Lúc dội khi bấm, ISR có thể đọc trúng mức nhả và **từ chối cả lần bấm thật**
— đổi một lỗi hiếm lấy một lỗi thường xuyên. Không đáng.

**Giới hạn đã biết, ghi thẳng vào comment thay vì lờ đi:** `.ioc` cấu hình EXTI2
chỉ cạnh xuống, nên ISR của PD2 không nhìn thấy cạnh nhả. Dội trong một lần bấm
bị lọc sạch, nhưng cạnh đầu của chùm dội lúc *nhả* vẫn có thể lọt thành một
CONFIRM thừa. Muốn triệt để phải bật cả hai cạnh cho PD2 trong CubeMX — đã thống
nhất **không đụng `.ioc`** để tránh rủi ro CubeMX reset GPIOD (tiền lệ ghi trong
`GPIO_CONFIG.md`).

---

### Files đã sửa

| File | Sửa gì |
| --- | --- |
| `App/BSP/Src/bsp_button.c` | Viết lại cả hai ISR theo quiet-window; xóa `s_pressStartMs`/`s_pressed`; mỗi nút một biến timestamp riêng (dùng chung sẽ khiến bấm nút này khoá nút kia 50 ms) |
| `App/BSP/Inc/bsp_button.h` | Sửa "on release" → "on press"; **khôi phục dòng tiêu đề "Button 2 — Start / Confirm (PD2)"** vốn bị mất, khiến khối comment đọc như thể vẫn đang mô tả PA0 |
| `App/Common/Inc/app_config.h` | Xóa `BUTTON_LONG_PRESS_MS`; viết lại comment của `BUTTON_DEBOUNCE_MS` cho khớp cơ chế mới |
| `App/Common/Inc/app_types.h` | Comment "the BSP measures duration" — sai từ khi bỏ long press |
| `docs/LEARNING_NOTES.md` | Viết lại mục "Design: Debounce"; đánh dấu mục "macro chết" là đã dọn |

**Build:** `make all` thành công, `bsp_button.c` **không một warning**.
`text 37880 / data 44 / bss 85812`.

---

### Bài học phương pháp

**Macro chết chỉ là phần dễ thấy của một refactor bỏ dở.** Phần nguy hiểm hơn là
*cấu trúc điều khiển* mà tính năng cũ để lại — ở đây là việc sự kiện bắn ở cạnh
nhả tay. Macro không dùng thì vô hại; cấu trúc thừa thì vẫn đang **quy định hành
vi thật của hệ thống**, và không ai chất vấn nó vì "code đang chạy được".

Khi bỏ một tính năng, phải truy ngược: **tính năng đó đã áp đặt những ràng buộc
nào lên phần còn lại?** Rồi gỡ luôn các ràng buộc đó.

---

### Q3: Dọn nốt warning phát sinh lúc build — ✅ đã xong

Build cảnh báo `implicit declaration of function 'BSP_Sensor_Poll'`
(`sensor_task.c:21`). Nguyên nhân: `bsp_sensor.h` **thiếu prototype** cho cả
`BSP_Sensor_Init()` lẫn `BSP_Sensor_Poll()` — chỉ khai báo hai hàm `Is*()`.

**Vì sao nguy hiểm dù code vẫn chạy:** thiếu prototype thì trình biên dịch tự
giả định `int f()`. Ở đây đúng được là **nhờ may** — hàm `void f(void)` trên ARM
EABI tình cờ khớp cách truyền tham số/giá trị trả về. Nếu hàm có tham số, hoặc
trả về kiểu khác `int`, chương trình sẽ hỏng theo kiểu rất khó truy. Từ C23 đây
là **lỗi cứng**, không còn là cảnh báo.

**Đã sửa, ba chỗ:**

| Chỗ | Sửa gì |
| --- | --- |
| `bsp_sensor.h` | Thêm prototype `BSP_Sensor_Init()` và `BSP_Sensor_Poll()` kèm doc |
| `main.c` | Thêm `#include "bsp_sensor.h"` (trong `USER CODE BEGIN Includes`) — header có prototype nhưng `main.c` chưa hề include, nên `BSP_Sensor_Init()` ở dòng 110 vẫn implicit |
| `oled_task.c` | Xóa biến `char line[22]` không dùng tới (`-Wunused-variable`) |

Bài học nhỏ: thêm prototype vào header **chưa đủ** — phải kiểm tra mọi nơi gọi
đã include header đó chưa. Warning thứ nhất biến mất ở `sensor_task.c` nhưng
vẫn còn nguyên ở `main.c`, chỉ lộ ra khi **build sạch** (`make clean`) chứ
build tăng dần không biên dịch lại `main.c`.

**Kết quả:** `make clean && make all` → **0 warning, 0 error** trên toàn project.
`text 37880 / data 44 / bss 85812`.

---

## Session 07 — 2026-07-10 — Test thủ công WashingManager & Bug #8/#9

**Chủ đề:** Soạn kịch bản test tay cho state machine, phát hiện và fix race
condition PA0 (button vs door sensor), giải thích LED/OLED expected behavior.

---

### Q1: Kịch bản test WashingManager nên chạy theo cách nào — unit test PC hay test tay trên board?

**Trả lời của bạn:** Test thủ công trên board thật.

**Kết quả:** Soạn bảng test case bám theo `kTransitions[]` trong `washing_sm.c`,
map Logic Analyzer CH0-CH3 vào PD12-PD15, dùng debugger Live Expressions cho
`s_waterFull`. Phát hiện trước khi test: `bsp_sensor.c` đọc door qua PA0 —
trùng chân với `bsp_button.c` — dự đoán sẽ gây lỗi ngay khi long-press.

---

### Q2: Test thật trên board — bấm Wash, LED cam+xanh dương sáng, SM vào FILL_WATER rồi lập tức ERROR khi thả tay

**Đánh giá:** Đúng như dự đoán ở Q1 — xác nhận bug PA0 dùng chung cho
button (cần thả tay để xác nhận long-press) và door sensor (thả tay = đọc
thành "cửa mở"). Ghi lại thành **Bug #8** trong `HARDWARE_BUGS.md`.

**Quyết định fix:** Phương án A — biến `s_doorClosed` giả lập qua debugger
(giống `s_waterFull`), bỏ hẳn việc đọc GPIO PA0 trong `bsp_sensor.c`. Build
thành công, không lỗi. Xem TD-11 trong `PROJECT_PROGRESS.md`.

---

### Q3: LED hiển thị đúng như thế nào khi chọn Wash?

**Trả lời:** Bảng trạng thái LD3/LD4/LD5/LD6 theo từng state, dựa trên các
entry-action callback trong `washing_manager.c`. LD6 (status) sáng liên tục
từ `FILL_WATER` đến hết `SPIN`, chỉ tắt ở `FINISH`/`ERROR`/`IDLE` — không
phải đèn theo pha mà là đèn "máy đang hoạt động".

---

### Q4: OLED hiển thị đúng như thế nào khi chọn Wash?

**Trả lời:** 4 dòng cố định (tiêu đề, Wash, Spin, statusText), cursor giữ
nguyên vị trí đã chọn suốt chu trình vì `s_menuIndex` chỉ đổi khi có lệnh
mới. Phát hiện thêm **Bug #9** (chưa fix): short-press giữa chu trình làm
OLED tạm hiện sai `"IDLE"` do `s_statusCache` trong `ui_task.c` không bao
giờ được cập nhật sau lần khởi tạo.

---

### Q5: Sau khi vào FILL_WATER, vài giây sau ERROR — có phải bug không?

**Trả lời:** Không — đây là hành vi đúng thiết kế. `WASH_FILL_DURATION_MS`
= 10000ms; vì `s_waterFull` mặc định = 0 và không ai set qua debugger,
`xFillTimeoutTimer` tự bắn `EVT_TIMEOUT` sau đúng 10s → SM chuyển
`FILL_WATER → ERROR` theo transition table. Xác nhận bug PA0 (Bug #8) đã
được fix thành công — đường lỗi còn lại là do chưa mô phỏng nước đầy, không
phải do cửa.

---

## Session 08 — 2026-07-10 — Bug #10: Không thoát được FINISH/ERROR

**Chủ đề:** Test tiếp sau khi fix Bug #8 — chạy hết chu trình Wash bằng cách
set `s_waterFull = 1u`, phát hiện máy đứng yên ở `FINISHED` không phản hồi
nút bấm nữa.

---

### Q1: Set `s_waterFull = 1u`, chạy đến FINISHED, nhưng sau đó chọn Wash/Spin thì hệ thống không hoạt động nữa — vì sao?

**Trả lời:** `UITask` không bao giờ gửi `CMD_RESET` — long-press handler chỉ
build `CMD_START_WASH`/`CMD_START_SPIN` ([ui_task.c:73-90](../App/UITask/Src/ui_task.c#L73-L90)).
Trong khi đó, transition table chỉ cho phép thoát `FINISH`/`ERROR` bằng
`WASH_EVT_RESET` — mọi event khác bị âm thầm bỏ qua theo đúng spec của SM.
Ghi lại thành **Bug #10** trong `HARDWARE_BUGS.md`.

**Hai phương án đã thảo luận:**
- Phương án A (khuyến nghị): `Manager_HandleCommand()` tự kiểm tra
  `WashingSM_GetState()` — nếu đang FINISH/ERROR, mọi command đến đều hiểu
  là RESET. Không cần IPC mới, giữ đúng nguyên tắc UITask không cần biết
  state máy.
- Phương án B: thêm kênh phản hồi ngược để UITask tự biết state — phức tạp
  hơn, phá vỡ tách biệt kiến trúc sẵn có.

**Trạng thái:** Đã ghi lại phân tích đầy đủ vào `HARDWARE_BUGS.md` (Bug #10)
và `PROJECT_PROGRESS.md`. Chưa implement — đang chờ xác nhận chọn phương án
trước khi sửa code, theo đúng quy trình review của CLAUDE.md.

---

## Session 09 — 2026-08-23 — Mentoring: Event Group, Guard Condition, Level vs Edge

**Chủ đề:** Phiên dạy có hệ thống theo yêu cầu "làm chủ project". Đánh giá
kiến thức hiện tại qua câu hỏi, dạy sâu Event Group (cả kiến thức tổng quát
ngoài project), phát hiện lỗ hổng thiết kế guard condition.

**Bối cảnh:** Bạn cho biết mới chỉ nắm chắc Task và Queue. Đã thống nhất lộ
trình 10 module, kết thúc bằng bài kiểm tra tự thiết kế một project mới.
Bạn cũng yêu cầu: khi dạy khái niệm nào thì phải mở rộng ra kiến thức tổng
quát của khái niệm đó, không bó hẹp trong cách project này dùng.

---

### Q1: Kể lại từng bước từ lúc nhấn PD2 đến khi motor bật

**Trả lời của bạn:** ISR tạo data queue → đánh thức UITask → UITask gửi lệnh
qua queue sang Manager → Manager kiểm tra cửa và mức nước, nếu cửa đóng thì
mở van, đợi đầy thì bật motor, đồng thời gửi data sang OLED.

**Đánh giá:** Phần Task + Queue **đúng** — nắm chắc cơ chế queue vừa truyền
data vừa đánh thức task Blocked.

Hai chỗ sai:

1. Nhầm tên van: nạp nước dùng `BSP_VALVE_INLET` (PD13), không phải van xả
   `BSP_VALVE_DRAIN` (PD14) — van xả chỉ dùng ở state `DRAIN`.
2. **Sai kiến trúc (quan trọng):** hiểu thành "kiểm tra cửa **rồi mới** mở
   van" (if-else tuần tự). Thực tế `Manager_OnEnterFillWater()` mở van **vô
   điều kiện**, còn cửa được giám sát **song song** qua `EVT_ERROR` trong
   `xEventGroupWaitBits`. Đây là mô hình *"cứ hành động, đồng thời canh
   chừng sự cố"*, khác bản chất với if-else.

---

### Q2: `xEventGroupWaitBits` khác `xQueueReceive` thế nào? Dùng Queue để chờ 3 điều kiện thì bất tiện gì?

**Trả lời của bạn:** Một event thì dùng queue được nhưng tốn tài nguyên hơn;
nhiều event thì không dùng queue được.

**Đánh giá:** Trực giác đúng hướng, nhưng lý do chưa trúng. Queue **vẫn dùng
được** cho nhiều loại sự kiện. Khác biệt thật nằm ở bản chất:

- Queue = dòng sự kiện **rời rạc, có thứ tự, đọc 1 lần là mất** (hộp thư).
- Event Group = **trạng thái true/false hiện tại** của nhiều cờ độc lập
  (bảng đèn báo trên taplo).

Điểm chốt: SensorTask poll mỗi 100 ms và set lại bit liên tục — với Event
Group là **idempotent** (`1 OR 1 = 1`, vô hại), với Queue sẽ **chồng item**
gây tràn hàng đợi.

---

### Q3: Set cùng một bit 2 lần liên tiếp vs `xQueueSend` 2 lần — khác gì?

**Trả lời của bạn:** Event group thì action đã trigger ở lệnh đầu; queue
depth ≥ 2 thì lưu 2 lần và consumer xử lý cả 2. Event group "có thể gây lỗi
hoặc không phản ứng với event thứ 2".

**Đánh giá:** Ý chính đúng. Cần làm rõ: **không có lỗi nào cả** — "không
phản ứng với lần thứ 2" chính là **thiết kế đúng**, không phải tác dụng phụ.
Event Group chỉ biểu diễn **level** (đang đúng/sai), **không đếm được**
(không có edge/count). Muốn đếm phải dùng counting semaphore hoặc biến
counter + critical section.

Bổ sung: Queue giữ đủ 2 item **không tự động an toàn** — ở project này an
toàn là nhờ state machine bỏ qua im lặng cặp `(state, event)` không khớp,
tức nhờ thiết kế phòng thủ của FSM chứ không nhờ bản thân Queue.

---

### Q4 (bạn tự đặt): `EVT_TIMEOUT` của state cũ bắn nhầm sang state sau thì có gây lỗi không?

**Đánh giá:** Câu hỏi rất tốt — bạn tự phát hiện một rủi ro thật trong code.
`EVT_TIMEOUT` là **một bit dùng chung cho cả 4 timer** (TD-10), nhưng ý
nghĩa phụ thuộc state đang đọc: ở `FILL_WATER` nghĩa là *lỗi hết giờ*, ở
`WASHING` nghĩa là *giặt xong bình thường*.

Code đã có phòng thủ bằng `xTimerStop()` gọi **trước** khi đổi state. Nhưng
`xTimerStop()` không tức thời — nó chỉ gửi lệnh vào timer command queue, xử
lý bất đồng bộ. Vậy vẫn còn khe hở cực hẹp, **giảm thiểu chứ không loại bỏ**.

**Giải pháp bạn tự đề xuất:** tách thành nhiều bit timeout riêng. Đúng
hướng. Tinh chỉnh: đặt tên theo ngữ nghĩa phase thay vì `TIMEOUT0/1/2`:

```c
#define EVT_FILL_TIMEOUT   (1 << 5)   // FILL_WATER: lỗi
#define EVT_WASH_DONE      (1 << 6)   // WASHING: xong bình thường
#define EVT_DRAIN_DONE     (1 << 7)
#define EVT_SPIN_DONE      (1 << 8)
```

Còn một điểm tinh vi: bit "rác" của state cũ vẫn nằm lại (vì `pdTRUE` chỉ
xóa bit *được chờ và khớp*) → cần `xEventGroupClearBits()` khi vào state
mới, theo đúng pattern `Manager_OnEnterIdle()` đang làm.

**Trạng thái:** Chưa implement — mới là thiết kế trên giấy.

---

### Q5: Thêm điều kiện "cửa đóng VÀ nắp lọc OK" mới cho khởi động — đặt ở đâu, `xWaitForAllBits` là gì?

**Trả lời của bạn:** `pdTRUE`, đặt trong `case WASH_STATE_FILL_WATER` vì đó
là case bắt đầu chu trình giặt.

**Đánh giá:** `pdTRUE` (AND) **đúng**. Vị trí **sai**, 3 lý do:

1. **Quá muộn:** khi vào được `case FILL_WATER` thì
   `Manager_OnEnterFillWater()` đã chạy — van đã mở, nước đang chảy.
2. **Bỏ sót nhánh SPIN:** `IDLE → SPIN` không đi qua `FILL_WATER`, nên chế
   độ vắt sẽ **hoàn toàn không bị kiểm tra**. Lỗ hổng an toàn thật.
   → Nguyên tắc: guard phải đặt ở chỗ **mọi đường đi đều bắt buộc qua**.
3. **Sai công cụ:** dùng `WaitBits` (blocking) làm guard.

**Khái niệm mới — Guard Condition (kiến thức FSM tổng quát):**

Bảng transition hiện tại chỉ có `(state, event) → next_state`. Dạng đầy đủ
theo UML statechart có 4 thành phần:

```
(state, event, [guard]) → next_state / action
```

Guard sai → transition **không nổ**, state giữ nguyên, event bị từ chối.

**Phân biệt sống còn — `WaitBits` vs `GetBits`:**

| | `xEventGroupWaitBits()` | `xEventGroupGetBits()` |
| --- | --- | --- |
| Ý nghĩa | "Tôi **đợi cho tới khi** đúng" | "**Ngay lúc này** đúng hay sai?" |
| Blocking | Có | Không |
| Dùng cho | Chờ sự kiện tương lai | **Guard / precondition** |

Nếu block trong Manager: máy không hiển thị được lý do từ chối, user tưởng
máy hỏng. Nguyên tắc UX embedded: **fail fast with feedback**.

Vị trí đúng là `Manager_HandleCommand()`, dùng `GetBits`:

```c
const EventBits_t required = EVT_DOOR_CLOSED | EVT_FILTER_OK;
EventBits_t bits = xEventGroupGetBits(xSystemEvents);
if ((bits & required) != required) {
    Manager_PostDisplay(currentState, "CLOSE DOOR");
    return;                       // từ chối, KHÔNG đổi state
}
```

Idiom cần nhớ: `(bits & mask) != 0` là **OR**, `(bits & mask) != mask` là **AND**.

**Ba loại điều kiện — đừng nhầm:**

| Loại | Đúng khi nào | Ví dụ |
| --- | --- | --- |
| Precondition (guard) | **Trước khi** bắt đầu | Cửa đóng + nắp lọc OK trước khi chạy |
| Invariant | **Suốt** thời gian ở state | Cửa luôn đóng khi motor quay |
| Postcondition | **Sau khi** hoàn thành | Xả hết nước trước khi vắt |

→ Chính chỗ này giải thích hiểu nhầm ở Q1: bạn nghĩ cửa là *precondition
tuần tự*, thực tế code đang xử lý nó như *invariant*. Hệ thống tốt cần **cả
hai** — project hiện đang có invariant nhưng **thiếu guard**.

**Level flag vs Edge event:** chỉ **level flag** mới làm guard được.
`EVT_DOOR_CLOSED` được SensorTask set/clear liên tục theo trạng thái vật lý
→ là level → hợp lệ. `EVT_FILTER_OK` mới cũng phải thiết kế theo kiểu này.

**Trạng thái:** Guard chưa implement — mới là thiết kế. Chỉ mới sửa 3 lỗi
tài liệu phát sinh khi rà `app_events.h` (xem dưới).

---

### Phát sinh: 3 lỗi tài liệu trong `app_events.h` (đã sửa)

Phát hiện khi đọc file để kiểm chứng trước lúc dạy:

1. **Comment `EVT_TIMEOUT`** ghi timer callback set bit *"from ISR context
   via `xEventGroupSetBitsFromISR()`"* — sai. `Timer_PhaseCallback()` chạy
   trong **Timer Service Task** (task thật, priority `configTIMER_TASK_PRIORITY`)
   và dùng `xEventGroupSetBits()` bản thường. Cùng loại lỗi đã sửa trong
   `SYSTEM_DIAGRAMS.md` trước đây nhưng file này còn sót.
2. **"Usage example"** gợi ý dùng `WaitBits(EVT_DOOR_CLOSED, portMAX_DELAY)`
   làm guard — chính là thiết kế blocking-guard vừa phân tích là sai. Đã
   thay bằng ví dụ phân biệt rõ 2 cách đọc (WaitBits vs GetBits). Ví dụ cũ
   còn ghi sai tên handle (`hSystemEvents` thay vì `xSystemEvents`).
3. **Comment `EVT_ERROR`** ghi *"clears only on CMD_RESET"* — sai kép:
   `case CMD_RESET` đã bị xóa khỏi `Manager_HandleCommand()` (dead code), và
   bit này thực tế được clear ở `Manager_OnEnterIdle()` cùng tự động bởi
   `WaitBits(..., pdTRUE, ...)`.

**Bài học chung:** comment không được compiler kiểm tra — nó **trôi khỏi
code** một cách âm thầm. Đã 4 lần trong project này comment mô tả sai hành
vi thật (Bug #12, và 3 chỗ vừa rồi). Khi đọc code lạ, luôn xác minh comment
bằng code thật trước khi tin.

---

### Q6: Trace kịch bản "mở cửa bỏ đồ → đóng cửa → bấm Start" — chuyện gì xảy ra?

**Trả lời của bạn:** Manager nhận command wash, nếu không có lỗi gì thì máy
hoạt động bình thường, nếu có `EVT_ERROR` thì dừng.

**Đánh giá:** Mới mô tả logic chung, chưa trace giá trị bit thật — mà đó
chính là mấu chốt. Đáp án: máy **KHÔNG** chạy bình thường. Tại bước bấm
Start, `EVT_ERROR` **vẫn đang bằng 1**: cửa đã đóng lại nhưng cái bit thì
không tự đóng theo, vì nhánh `if` trong `sensor_task.c` chỉ set
`EVT_DOOR_CLOSED` mà **không clear `EVT_ERROR`**.

Hệ quả: `xEventGroupWaitBits()` trong `case FILL_WATER` thấy bit đã set sẵn
→ trả về ngay, không block → `FILL_WATER → ERROR` trong vài chục micro giây.

Đã ghi lại đầy đủ thành **Bug #17** trong `HARDWARE_BUGS.md`, kèm 3 phương
án fix (A/B/C) và phân tích ownership.

**Khái niệm tổng quát rút ra — Sticky / Stale Level Flag:**

Cờ level chỉ có đường lên mà không có đường xuống sẽ mô tả **quá khứ** thay
vì hiện tại. Đặc điểm khiến nó khó bắt: không crash, không warning, phụ
thuộc *lịch sử thao tác*, và triệu chứng xuất hiện ở module cách xa nguyên
nhân. Nguyên tắc phòng tránh: **cờ level phải được duy trì đối xứng ở mọi
nhánh của producer**, và **mỗi bit nên có đúng một chủ sở hữu** chịu trách
nhiệm clear.

**Trạng thái:** Chưa fix — người dùng quyết định ưu tiên học để làm chủ
project trước, sửa sau. Người dùng cũng cho biết hệ thống hiện còn một số
lỗi khác chưa liệt kê; sẽ xử lý ở phiên riêng.

---

## Session 10 — 2026-08-23 — Mentoring buổi 2: Tick/SysTick & Context Switch

**Chủ đề:** Nguồn thời gian vật lý của FreeRTOS (SysTick, HSI vs HSE), cơ chế
đánh thức Timer Service Task, phân tích đầy đủ một chuỗi context switch thật,
và tách bạch tầng scheduler với tầng logic ứng dụng.

**Kết quả:** Ghi thành **DD-05** trong `LEARNING_NOTES.md` Part II. Không có
thay đổi code nào trong buổi này.

---

### Q1: Software Timer — ai quản lý nó, nguồn của nó từ đâu (clock nội hay ngoại)?

**Bối cảnh:** DD-04 (buổi 1) mới trả lời tầng **logic** — Timer Service Task
quản lý danh sách timer. Câu hỏi này chỉ đúng vào lỗ hổng còn lại: tầng **vật
lý**, cái gì thực sự tạo ra khái niệm "thời gian".

**Trả lời:**

Có **hai tầng quản lý** khác nhau, không được gộp:

| Tầng | Ai quản lý |
| --- | --- |
| Thời gian (vật lý) | **SysTick** — bộ đếm 24-bit nằm trong lõi Cortex-M, thuộc chuẩn ARM chứ không phải ngoại vi của ST |
| Logic | **Timer Service Task** — dựa vào nhịp SysTick để biết timer nào tới hạn |

Chuỗi đầy đủ: `HSI 16 MHz → SystemCoreClock → SysTick (nạp 15999, tràn mỗi
1 ms) → xPortSysTickHandler() → xTaskIncrementTick() → daemon chuyển Ready →
Timer_PhaseCallback()`.

**Nguồn clock của project này = NỘI.** Kiểm chứng bằng `SystemClock_Config()`
trong `main.c`: `OscillatorType = RCC_OSCILLATORTYPE_HSI`, `PLL.PLLState =
RCC_PLL_NONE`, `SYSCLKSource = RCC_SYSCLKSOURCE_HSI` → HSI (RC nội ~16 MHz),
không dùng thạch anh ngoài (HSE), không nhân tần bằng PLL.

**Ba bằng chứng source đã trích:**

- `port.c:826-828` — công thức nạp thanh ghi SysTick.
- `FreeRTOSConfig.h:130` — `#define xPortSysTickHandler SysTick_Handler`
  (chính là pattern weak-symbol của Bug #4; thiếu dòng này → tick không tăng →
  treo toàn hệ thống).
- `timers.c:817` — `vQueueWaitForMessageRestricted(...)`: daemon **không
  polling**, nó tự tính thời gian ngủ và dùng đúng cơ chế block-with-timeout
  chung của kernel.

**Kết luận quan trọng nhất:** Timer Service Task **không có phần cứng riêng,
không có đặc quyền lịch trình nào** — chỉ là một task bình thường biết tự tính
thời gian ngủ.

---

### Q2: Bài toán context switch — `xWashTimer` hết hạn khi UITask đang chạy

**Đề bài:** `xWashTimer` (20 s) vừa hết hạn. WashingManager (prio 3) Blocked
trong `WaitBits`. UITask (prio 2) Running. Daemon (prio 2) Blocked. Hỏi thứ tự
chạy và số lần context switch tới khi `Manager_OnEnterDrain()` thực thi.

**Trả lời của bạn:** UITask dừng để Timer Service Task chạy → callback set
event → WashingManager chạy → sau khi WashingManager block lại thì các task
prio 2 chạy tiếp. **2 lần context switch.**

**Đánh giá:** Chuỗi logic **đúng**. Con số **thiếu** — thực tế tối thiểu **4**.

**Hai điểm bổ sung:**

**(a) UITask nhường CPU KHÔNG phải vì daemon ưu tiên cao hơn.** Daemon = prio
2, bằng UITask. `tasks.c:4787` yêu cầu `pxTCB->uxPriority > pxCurrentTCB->
uxPriority` → `2 > 2` là FALSE, nhánh này không kích hoạt. Cái làm nó switch là
khối **time-slicing** ở `tasks.c:4810-4813` (`configUSE_TIME_SLICING` mặc định
`1`). → Nếu ai tắt time slicing, daemon sẽ nằm Ready chờ tới khi UITask tự
block, callback trễ tuỳ ý. Muốn tất định phải đặt `configTIMER_TASK_PRIORITY`
cao hơn mọi task dùng timer.

**(b) Daemon bị cướp CPU GIỮA CHỪNG callback** — đây là chỗ bị bỏ sót.
`xEventGroupSetBits()` bọc trong `vTaskSuspendAll()`/`xTaskResumeAll()`
(`event_groups.c:567,639`). WashingManager được gỡ block bên trong vùng suspend
nên chỉ đặt cờ `xYieldPendings[0] = pdTRUE` (`tasks.c:4632-4636`), rồi
`xTaskResumeAll()` thực hiện yield → switch xảy ra **trước khi
`Timer_PhaseCallback()` kịp return**. Daemon vì thế **bắt buộc phải được chạy
lại sau** để hoàn tất callback và dọn dẹp.

| # | Từ → Đến | Nguyên nhân |
| --- | --- | --- |
| 1 | UITask → daemon | tick + time-slicing (2 == 2) |
| 2 | daemon → WashingManager | `xTaskResumeAll()` giữa callback |
| 3 | WashingManager → task prio 2 | `WaitBits` block lại |
| 4 | … → daemon | round-robin |

Thứ tự giữa UITask và daemon ở bước 3 phụ thuộc con trỏ `pxIndex` của
ready-list (`tasks.c:189-191`) → **không tất định theo góc nhìn thiết kế, đừng
bao giờ phụ thuộc vào nó**.

---

### Q3: Sao UITask lại chạy được khi máy đang giặt? Nhấn nút lúc đó phải vô tác dụng chứ?

**Đánh giá:** Câu hỏi hay nhất buổi — nó lộ ra chỗ đang **trộn lẫn hai tầng**.

| Tầng | Câu hỏi | Ai quyết định |
| --- | --- | --- |
| Scheduler | UITask có được cấp CPU không? | Priority + Blocked/Ready. **Không biết gì về "đang giặt"** |
| Ứng dụng | Lệnh đó có tác dụng không? | State machine của WashingManager |

Scheduler hoàn toàn không có khái niệm "máy đang giặt". Nó chỉ thấy UITask prio
2 vừa có message → Ready → tới lượt thì chạy.

**"Vô tác dụng" thực ra được cài đặt bằng 3 mảnh trong code:**

1. UITask **vẫn chạy đủ**, vẫn `xQueueSend(xCommandQueue, ...)`
   (`ui_task.c:69-81`).
2. WashingManager ở state WASHING **không đọc `xCommandQueue`**
   (`washing_manager.c:291-297`) — chỉ đọc ở IDLE/FINISH/ERROR (`:249-254`).
   Lệnh nằm im trong queue.
3. Vì nằm im nên sẽ "nổ chậm" khi về IDLE → code chặn bằng
   `(void)xQueueReset(xCommandQueue)` trong `Manager_OnEnterIdle()` /
   `OnEnterFinish()` / `OnEnterError()` (`washing_manager.c:66-67`).

**Pattern tổng quát rút ra:** *lọc sự kiện ở phía consumer (state machine),
không phải ở phía producer.* Producer cứ đơn giản; toàn bộ tri thức về ngữ cảnh
nằm gọn một chỗ — đúng Design Rule #1. Làm ngược lại (cho UITask tự hỏi "máy
đang giặt à?") thì logic trạng thái bị rải ra 2 nơi và sẽ có lúc bất đồng.

**Hệ quả kiểm chứng được trên board:** nhấn NAVIGATE (PA0) lúc đang giặt thì
con trỏ menu trên OLED **vẫn di chuyển**, vì UITask gửi thẳng `xDisplayQueue`
không qua WashingManager (`ui_task.c:60-67`). Feature hay bug là quyết định
thiết kế, không phải tai nạn.

---

### Q4: "Software Timer chưa xong mà" — phản hồi về nhịp dạy

**Bối cảnh:** Sau khi viết DD-05 và đánh dấu ✅ cho Software Timer trong bảng
lộ trình, người dùng phản hồi hai lần liên tiếp:

1. *"Software Timer chưa xong mà"*
2. *"ý mình là kiến thức về Software Timer còn nhiều mà, sao chuyển sang bài
   học khác sớm thế"*

**Đánh giá — phản hồi đúng.** Rà lại DD-04 (mục 1–14) thì thấy mới dạy phần
**dùng API**, thiếu hẳn phần **nội tại** và **thực chiến**. Đã bổ sung mục
15–26, gồm cả **một đính chính**.

**Nội dung đã bổ sung:**

| Mục | Nội dung |
| --- | --- |
| 15 | Vòng đời timer — trạng thái **Dormant/Active** (thuật ngữ chính thức, `timers.h:111`), 3 bit `ucStatus` |
| 16 | **Vì sao có HAI danh sách timer** — xử lý tick overflow, `prvSwitchTimerLists()` đổi con trỏ O(1) |
| 17 | Vòng lặp thật của daemon chỉ 3 dòng; thứ tự "xử lý timer hết hạn TRƯỚC lệnh trong queue" |
| 18 | Command queue đầy; gọi `xTimerStart()` trước scheduler thì `xTicksToWait` **bị ép về 0** |
| 19 | Static allocation — `configSUPPORT_STATIC_ALLOCATION` mặc định 0 → project này **không dùng được** |
| 20 | ⚠ **Đính chính mục 9** |
| 21 | 6 API còn thiếu ở bảng mục 6 |
| 22 | **Độ phức tạp O(n)** khi chèn; so sánh sorted list / min-heap / timer wheel / RB-tree |
| 23 | Callback chạy trên **stack của daemon** (1040 B) — bug tràn stack ở task mình chưa từng viết |
| 24 | **Reentrancy** — callback↔callback không cần mutex, callback↔task thì cần |
| 25 | 8 pattern thực chiến (debounce, watchdog mềm, end-of-frame, backoff, throttle...) |
| 26 | 5 trace hook timer; đo độ trễ daemon bằng cặp `COMMAND_SEND`→`COMMAND_RECEIVED` |

**⚠ Đính chính quan trọng (mục 20):** buổi 1 dạy
`xTimerPendFunctionCallFromISR()` như API sẵn dùng, nhưng
`INCLUDE_xTimerPendFunctionCall` mặc định `0` (`FreeRTOS.h:283-284`) và project
không khai → **hàm không được biên dịch**, gọi sẽ lỗi linker. Đây là **cùng
một lỗi** đã mắc ở DD-01 mục 5 với `xEventGroupSetBitsFromISR` mà vẫn lặp lại.
→ Nguyên tắc: *"API tồn tại trong FreeRTOS" ≠ "API dùng được trong build
này"* — luôn kiểm tra macro `INCLUDE_*` / `configUSE_*` trước khi dạy.

**Phát hiện thêm về code project:** cả **10 lời gọi**
`xTimerStart()`/`xTimerStop()` trong `washing_manager.c` đều bỏ qua giá trị trả
về — đúng lỗi kinh điển số 4 trong checklist DD-04 mục 12. Chưa gây hậu quả
(`configTIMER_QUEUE_LENGTH = 10`, chỉ 4 timer) nhưng đã ghi nhận.

**Bài học phương pháp (đã lưu vào memory):** không tự đánh dấu một chủ đề là
✅ hoàn thành rồi chuyển sang chủ đề khác — **liệt kê thẳng những gì còn thiếu
cho người học thấy và để họ quyết định khi nào chủ đề xong**. Trạng thái trong
bảng lộ trình mặc định để 🔶.

**Còn lại của DD-04 lúc đó:** (F) test logic phụ thuộc thời gian, (G) so sánh
với timer của Linux / Zephyr / bare-metal. Đã dạy tiếp ở Q5, Q6 dưới đây —
DD-04 khép lại trong buổi này.

---

### Q5: Tại sao mình đọc source FreeRTOS nhiều vậy?

**Trả lời:** Hai lý do. (1) Chuẩn "sách giáo khoa, không bịa" mà người dùng
đặt ra — bắt nguồn từ 7 lỗi thật đã mắc khi viết theo trí nhớ (sai tên macro,
thiếu điều kiện biên dịch, trích sai số liệu). (2) Sâu hơn: ba loại thông tin
chỉ đọc được, không suy luận được — cấu hình riêng của project (macro
`INCLUDE_*`/`configUSE_*` không khai thì lấy default ẩn, ví dụ
`INCLUDE_xTimerPendFunctionCall=0` khiến một API "không tồn tại" trong build
này), chi tiết phụ thuộc phiên bản kernel (V11.1.0), và số liệu chính xác
(44 byte `Timer_t`, 1040 byte stack daemon...) mà tài liệu sẽ dùng để tính
toán thật về sau.

Thừa nhận có phần dư: không phải lần tra cứu nào cũng cần — chỉ bắt buộc cho
cấu hình/API compile được và cơ chế nội tại; khái niệm chung, pattern, câu
hỏi phỏng vấn thì không cần đọc source mỗi lần.

---

### Q6: Test unit cho `washing_sm.c` — có loại bug nào không bắt được?

**Trả lời của bạn:** "không biết" — hỏi ngược lại thay vì đoán.

**Đáp án:** **Bug #17** (sticky `EVT_ERROR` trong `sensor_task.c:23-28`).
`washing_sm.c` xử lý `WASH_EVT_ERROR` hoàn toàn đúng khi nhận được nó — test
sẽ xanh. Nhưng bug nằm ở chỗ **sự kiện đó bị sinh ra sai** (bit không được
clear khi đóng cửa lại), và việc sinh sự kiện nằm ở `sensor_task.c`, ngoài
seam mà `washing_sm.c` test qua (đầu vào `WashEvent_t`, đầu ra callback).

**Nguyên tắc rút ra:** seam dùng để tiêm phụ thuộc test cũng chính là ranh
giới vùng mù của test — trả lời được "xử lý đúng sự kiện chưa" nhưng không
trả lời được "có sinh đúng sự kiện không". Phát hiện thêm cùng hình dạng lỗi
ở `EVT_WATER_FULL` (`sensor_task.c:30-32`, cũng thiếu nhánh `else` để clear).
Đã ghi thành DD-04 mục 29 kèm bảng phân loại bug theo tầng và đề xuất tầng
test "Component" (cảm biến giả) để bắt được loại này.

---

## Session 11 — 2026-08-24 — Task Notification & phát hiện tài liệu lỗi thời

**Chủ đề:** Bắt đầu module Task Notification (mentoring roadmap #4); trên
đường học phát hiện và sửa một phần Phase 3 của `LEARNING_NOTES.md` đã lỗi
thời so với code thật.

---

### Q1: Đường dẫn ISR PA0 → `xButtonQueue` → UITask trong project — có nên thay bằng Task Notification không?

**Trả lời của bạn:** Không nên, vì "button có thể gửi nhiều tín hiệu mà task
notify chỉ bắt được 1 cái, sẽ gây ra miss command".

**Đánh giá:** Kết luận đúng, cơ chế nêu ra chưa chính xác. Task Notification
không "rớt" sự kiện thứ 2 như queue tràn — `eSetBits`/`eIncrement` đều
**tích luỹ** (`ulNotifiedValue |= ulValue` hoặc `++`). Vấn đề thật là **mất
danh tính và thứ tự**: nếu `NAVIGATE=bit0`, `CONFIRM=bit1` và cả hai đến
trước khi task đọc, giá trị cuối chỉ còn `0b11` — không biết đã bấm NAVIGATE
mấy lần, không biết thứ tự. Ngược lại, nếu chỉ có **một loại tín hiệu duy
nhất lặp lại** thì `eIncrement` (`xTaskNotifyGive`/`ulTaskNotifyTake`) không
mất gì cả — đây chính là mẫu "counting semaphore nhanh hơn" chính thức của
FreeRTOS. Quy tắc chọn: nhiều loại cần phân biệt + giữ thứ tự → Queue; một
loại lặp lại, chỉ cần đếm → Task Notification.

---

### Q2: Đường dẫn UITask → `xCommandQueue` → WashingManager — ứng viên tốt hơn?

**Đánh giá câu hỏi mở — chưa có câu trả lời trực tiếp từ người dùng, phân
tích được đưa ra kèm câu hỏi tiếp theo:** Đây là ứng viên **tốt hơn** case
Q1 vì WashingManager chỉ đọc queue lúc IDLE/FINISH/ERROR, xử lý đúng 1 lệnh
rồi rời state ngay, và thiết kế hiện tại **đã tự vứt bỏ backlog** qua
`xQueueReset()` ở `OnEnterIdle/OnEnterFinish/OnEnterError` — tức là chỉ quan
tâm lệnh mới nhất, đúng ngữ nghĩa `eSetValueWithOverwrite`. Có thể đóng gói
`command|mode|menuIndex` vào một `uint32_t` và bỏ hẳn 3 lời gọi
`xQueueReset()` rải rác. Đánh đổi: Task Notification cần UITask biết handle
cụ thể của WashingManager (khớp nối chặt hơn queue trung lập).

---

### Q3: "Từ giờ không cần trace lại source FreeRTOS sau mỗi câu hỏi, dựa vào kiến thức trong các file này"

**Ghi nhận:** Đã lưu thành memory `feedback_reuse_verified_knowledge` — chỉ
tra lại `OS/FreeRTOS/` khi gặp khẳng định thật sự mới hoặc nghi ngờ điều đã
ghi trước đó sai; kiến thức đã trích dẫn trong `LEARNING_NOTES.md` (Phần A–D)
thì dùng lại trực tiếp. Đọc code của project (`App/`, `Core/`) không thuộc
phạm vi hạn chế này — vẫn đọc bình thường mỗi khi cần.

---

### Q4: "Sao file learning vẫn còn phần này" (ảnh chụp `Design: Short Press vs Long Press`)

**Phát hiện:** Đối chiếu `bsp_button.c` thật thì mục "Design: Short Press vs
Long Press" ở Phase 3 (`LEARNING_NOTES.md`) mô tả kiến trúc **đã không còn
tồn tại** — một nút PA0 duy nhất phân biệt short/long press bằng thời lượng
nhấn giữ (`duration >= BUTTON_LONG_PRESS_MS`). Code thật hiện dùng **hai nút
vật lý riêng biệt** (PA0 = navigate sau debounce, PD2 = confirm ngay khi có
falling edge), không còn đo thời lượng. Đây là hệ quả của Bug #8 (PA0 dùng
chung với door sensor gây race condition) và hành trình đổi chân
PE1→PD4→PD2 (Bug #14/#15/#16) — Phase 3 được viết **trước** các thay đổi đó
và không ai quay lại đối chiếu.

**Phát hiện thêm:** sơ đồ gốc "ISR → Queue → Task" cũng bỏ sót một tầng thật
— `bsp_button.c` không hề gọi `xQueueSendFromISR()` trực tiếp; nó gọi qua
con trỏ hàm callback (đăng ký bởi `UITask`), và `xQueueSendFromISR()` +
`portYIELD_FROM_ISR()` thực chất nằm trong `Button_EventCallback()` ở
`ui_task.c`. Đây chính là cơ chế giữ Design Rule #3 ("Drivers must not
depend on FreeRTOS") — bị thiếu hoàn toàn trong tài liệu gốc.

**Phát hiện phụ:** `BUTTON_LONG_PRESS_MS` (`app_config.h:35`, giá trị
`1500U`) vẫn còn định nghĩa nhưng **không còn nơi nào trong code sử dụng** —
dead code sau refactor.

**Đã sửa:** Viết lại toàn bộ mục "Design: ISR → Queue → Task Pattern" (đổi
tên thành "ISR → Callback → Queue → Task Pattern"), "Design: Short Press vs
Long Press" (đổi tên thành "Navigate vs Confirm", có bảng so sánh thiết kế
cũ/mới), và "What I Should Learn" trong `LEARNING_NOTES.md` Phase 3, kèm ghi
chú lịch sử trỏ về Bug #8 và #14/#15/#16 để không mất giá trị tham chiếu.

**Bài học:** đây là lần thứ hai phát hiện Part I (ghi theo phase xây dựng)
lỗi thời so với code thật — lần trước là 3 comment trong `app_events.h`
(Session 09). Xác nhận lại nguyên tắc đã ghi ở "Bài học phương pháp" mục 2:
tài liệu không được compiler kiểm tra, chỉ phát hiện được khi chủ động đối
chiếu lại với code.

---

## Session 12 — 2026-08-29 — Thiết kế tài liệu nhập môn START_HERE.md

**Chủ đề:** Không phải kiến thức FreeRTOS mới, mà là **thiết kế tài liệu học**:
viết file dẫn đường thế nào để người chưa từng dùng RTOS hiểu được project.

---

### Q1: Người đọc mục tiêu của file learning là ai?

**Ba phương án đưa ra:** (1) chính bạn — ôn lại, (2) người mới hoàn toàn, (3)
người phỏng vấn / đồng nghiệp cần nắm kiến trúc trong 30 phút.

**Bạn chọn:** (2) — người mới hoàn toàn: biết C, biết HAL cơ bản, chưa từng
dùng RTOS.

**Ràng buộc kéo theo:**

| Ràng buộc | Hệ quả lên cách viết |
| --- | --- |
| Chưa từng dùng RTOS | Không dùng *block / preempt / context switch / yield / ISR-safe* trước khi định nghĩa. Mỗi thuật ngữ có **một** điểm định nghĩa duy nhất |
| Biết HAL, quen superloop | HAL là điểm tựa bắc cầu: mỗi khái niệm mới mở đầu bằng "trong superloop bạn làm thế này… nó hỏng ở đây" |
| Đọc để hiểu project này | Không ví dụ bịa — mọi đoạn code trích thật từ `App/`, kèm `file:dòng` |

---

### Q2: Người học có board STM32F411E-DISCO trong tay khi đọc không?

**Bạn chọn:** có board, nạp được firmware.

**Hệ quả:** mục "Thí nghiệm" của mỗi tầng viết dạng **mệnh lệnh** (nạp code,
bấm nút này, quan sát LED kia), không cần kèm sẵn số liệu đo trước. File mỏng
hơn và học sâu hơn, đổi lại người không có board sẽ đọc bị hụt.

---

### Chẩn đoán trước khi viết

Đối chiếu bộ tài liệu hiện có: `LEARNING_NOTES.md` (3352 dòng) là **reference**
— sắp theo primitive (Phần A) và theo thứ tự lịch sử xây dựng (Phần B). Cả hai
thứ tự đều là thứ tự của người đã biết. Người mới mở ra sẽ gặp
`struct EventGroupDef_t` ngay §1.1 — kiến thức trang 200 đặt ở trang 1.

Thiếu sót thật: **không có cửa vào**. 6 file trong `docs/` + 2 file gốc, không
file nào nói "bắt đầu ở đây, đọc theo thứ tự này".

**Quyết định:** không sửa `LEARNING_NOTES.md`. Viết thêm một file mỏng làm
xương sống dẫn đường (`docs/START_HERE.md`), **link** sang các file kia thay vì
chép lại. Quy tắc chống trùng lặp: *một sự thật chỉ nằm ở một chỗ.*

---

### Bảy nguyên tắc đã chốt cho START_HERE.md

1. **Vấn đề trước, primitive sau** — mở chương bằng nhu cầu cảm nhận được, không
   bằng định nghĩa API.
2. **Cho xem cách làm sai trước** — mỗi primitive sinh ra để thay một cách ngây
   thơ; phải thấy nỗi đau mới nhớ được giá trị của công cụ.
3. **Neo vào code thật của repo**, kèm `file:dòng` — không pseudo-code.
4. **Mỗi tầng phải quan sát được bằng mắt** — chỉ bằng LED, OLED và nút bấm.
5. **Dự đoán trước, đo sau** — chỗ dự đoán sai là chỗ mô hình tư duy sai.
6. **Một tầng = một khái niệm mới**, không dùng thuật ngữ chưa định nghĩa.
7. **Nông ở đây, sâu ở LEARNING_NOTES** — cắt độ sâu ở mức đủ hiểu code project.

---

### Khung 7 tầng đã chốt

| Tầng | Tên | Câu hỏi trung tâm |
| --- | --- | --- |
| 0 | Sờ vào hệ thống | Cái máy này làm gì? |
| 1 | Vì sao cần RTOS | Ba việc cùng lúc — superloop hỏng ở đâu? |
| 2 | Bốn task của project | Ai làm gì, ai quan trọng hơn ai? |
| 3 | Task nói chuyện với nhau | Dữ liệu đi từ task này sang task kia bằng gì? |
| 4 | Chờ nhiều điều kiện | Chờ "cửa đóng VÀ nước đầy" kiểu gì? |
| 5 | Trái tim: State Machine | Máy giặt "đang ở đâu" lưu ở chỗ nào? |
| 6 | Từ chân MCU đến task | Nút bấm đi qua những bước nào? |

Khuôn cố định 7 mục cho mọi tầng: Hiện tượng → Cách ngây thơ → Ý tưởng → Code
thật → Thí nghiệm → Tự kiểm tra → Đào sâu.

---

### Q3: Có đưa phần công cụ debug (SystemView / RTT / logic analyzer) vào lộ trình không?

**Bạn quyết:** chưa cần, để sau.

**Đã sửa:** bỏ tầng "Nhìn thấy hệ thống chạy" khỏi lộ trình (còn 7 tầng: 0–6),
thêm ghi chú "Để sau" ở `START_HERE.md` §0.3, và đổi comment hai dòng
`SEGGER_SYSVIEW_*` trong trích đoạn `main.c` thành "chưa dùng tới → bỏ qua".

**Hệ quả về mặt sư phạm — đáng ghi lại:** ràng buộc này *tốt cho tài liệu*.
Toàn bộ 7 tầng giờ chỉ cần board + LED + nút bấm, không phụ thuộc công cụ nào
ngoài STM32CubeIDE. Người học không bị chặn ở Tầng 0 vì chưa cài được
SystemView. Nguyên tắc số 4 ("mỗi tầng phải quan sát được bằng mắt") vẫn giữ
nguyên — chỉ đổi phương tiện quan sát từ trace sang đèn LED, mà đèn LED thì
trực quan hơn với người mới.

Khi nào bổ sung tầng debug thì đánh số tiếp là Tầng 7 — kiến thức nền đã có sẵn
ở `LEARNING_NOTES.md` §B.8 và Session 06 của file này.

---

### Đã viết: Tầng 0 và Tầng 1

Viết dần từng tầng theo quy tắc "không sinh cả project một lượt" trong
`CLAUDE.md` — chỉnh khuôn ở 2 tầng đầu rẻ hơn chỉnh ở tầng thứ 8.

**Thí nghiệm chủ lực của Tầng 1** (đóng góp mới của phiên này): đổi
`vTaskDelay()` → `HAL_Delay()` trong `sensor_task.c:34`, dự đoán rồi đo. Suy
luận từ bảng priority (WashMgr 3, UITask 2, Sensor 2, OLED 1) và
`configUSE_TIME_SLICING = 1`:

- WashMgr (prio 3) — vẫn chạy, priority cao hơn nên chen vào được.
- UITask (prio 2) — vẫn chạy, **bằng** priority nên được chia CPU theo vòng.
- OLEDTask (prio 1) — **starvation**, không bao giờ được chọn → màn hình đứng.

Giá trị sư phạm: chương trình **không crash**. Đèn vẫn chạy, nút vẫn ăn, chỉ
màn hình chết — đúng loại bug mà người mới sẽ đi tìm nhầm trong driver OLED.
Cho thấy `HAL_Delay()` trong task không phải lỗi chức năng mà là lỗi thiết kế,
và lỗi thiết kế thì im lặng.

Ghi chú: project chuyển HAL timebase sang TIM riêng
(`Core/Src/stm32f4xx_hal_timebase_tim.c`) nên `HAL_Delay()` **vẫn chạy đúng
chức năng** — chính điều đó làm bug khó nhận ra hơn.

---
### Q4: Duyệt Tầng 0–1 — nhịp độ đạt, nhưng cách trình bày đáp án phải sửa

**Phản hồi của bạn:** "nhịp độ tầng 0 1 ổn rồi, tuy nhiên các câu hỏi cần có
trả lời và giải thích rõ ngay ở bên dưới".

**Chẩn đoán:** bản đầu để đáp án trong khối `<details><summary>` gấp lại ở cuối
mỗi tầng. Hai vấn đề:

1. **Mất mạch đọc** — người học phải cuộn xuống cuối tầng tìm đáp án rồi cuộn
   ngược lại.
2. **`<details>` không phải markdown chuẩn** — nó là HTML nhúng. GitHub render
   được, nhưng bản preview của STM32CubeIDE và một số editor thì không; khi đó
   đáp án **biến mất hoàn toàn** hoặc hiện ra dạng thẻ HTML thô.

**Đã sửa:**

- Bỏ toàn bộ khối `<details>` trong `START_HERE.md`.
- Cấu trúc mới cho mỗi câu: `**Câu N. <câu hỏi>**` → `**Trả lời:** ...` →
  giải thích dày → `---` ngăn cách.
- **Viết lại giải thích dày hơn hẳn bản cũ**, không chỉ chuyển vị trí:

| Câu | Bản cũ | Bản mới |
| --- | --- | --- |
| 0.6 Q1 (LED thay motor) | 3 dòng | 3 lợi ích cụ thể, trong đó lợi ích thứ 3 dẫn thẳng sang lý do tồn tại của `App/BSP/` (bắc cầu sang Tầng 2) |
| 0.6 Q2 (ai điều khiển) | 2 dòng | Bảng so sánh bare-metal vs RTOS + giải thích `vTaskStartScheduler()` chỉ return khi thiếu heap tạo Idle task |
| 0.6 Q3 (cái gì đếm 10 s) | 3 dòng | Nêu rõ **mâu thuẫn** timer giải quyết: không thể vừa đếm giờ vừa chờ sự kiện trên một luồng |
| 1.6 Q1 (priority ≠ CPU) | 5 dòng | Bảng "priority cao nghĩa là / không nghĩa là" + quy tắc gán priority thực tế (theo độ gấp, không theo độ quan trọng) |
| 1.6 Q2 (`vTaskDelay` 100 ms) | 4 dòng | Sơ đồ 3 mốc `Blocked → Ready → Running`, phân biệt điều RTOS đảm bảo và không đảm bảo, nối sang drift và `vTaskDelayUntil` |
| 1.6 Q3 (stack 512 bytes) | 6 dòng | Thêm: vì sao crash lệch chỗ, vì sao compiler bó tay (`sprintf` ngốn stack), bảng 2 công cụ phòng ngừa, và cách làm thực tế: cấp dư → đo high-water-mark → cắt |

- Kết quả Thí nghiệm B cũng bỏ `<details>`, thay bằng tiêu đề
  **"⛔ Dừng lại — viết dự đoán ra giấy xong mới đọc tiếp"** rồi trình bày công
  khai. Giữ được kỷ luật dự-đoán-trước mà không phụ thuộc HTML.

**Nguyên tắc bổ sung vào bộ quy tắc viết tài liệu (thành nguyên tắc số 8):**
đáp án và giải thích luôn nằm ngay dưới câu hỏi. Tài liệu học không được dùng
cơ chế "giấu đi rồi mở ra" — nó đánh đổi tính chắc chắn của việc đọc lấy một
chút gọn gàng hình thức, và đó là đánh đổi sai.

---

### Q5: Viết Tầng 2 — "Bốn task của project"

**Yêu cầu:** viết tiếp theo khuôn đã duyệt.

**Câu hỏi trung tâm được chọn cho tầng này:** không phải "bốn task này làm gì"
(cái đó tra `SYSTEM_DIAGRAMS.md` là ra), mà **"chia hệ thống thành bao nhiêu
task, theo tiêu chí gì?"** — đây mới là chỗ người mới thực sự mắc kẹt, và là
kiến thức mang được sang project khác.

**Cách ngây thơ được chọn để bác bỏ:** chia task **theo đồ vật**
(`MotorTask`, `ValveTask`, `LedTask`...). Đây là sai lầm tự nhiên ngay sau khi
học xong Tầng 1 — biết RTOS có nhiều task thì lao sang thái cực chia quá nhỏ.
Ba lý do bác bỏ, xếp tăng dần: tốn RAM → task rỗng không có việc → **trạng thái
hệ thống bị xé nát, không ai sở hữu, buộc phải dùng biến toàn cục**.

**Bốn tiêu chí chia task (nội dung chính của tầng):**

| # | Tiêu chí | Câu hỏi |
| --- | --- | --- |
| 1 | Nguồn chờ (blocking source) | Việc này nằm chờ cái gì? |
| 2 | Nhịp (rate) | Định kỳ hay theo sự kiện? |
| 3 | Độ gấp (latency) | Chậm 100 ms có sao không? |
| 4 | Quyền sở hữu (ownership) | Tài nguyên chung này ai được đụng? |

Tiêu chí 1 đặt lên đầu vì nó giải thích *chính xác* vì sao project này có 4 task:
bốn task nằm chờ bốn nguồn khác nhau (`xButtonQueue` / `xDisplayQueue` /
`xCommandQueue` / `vTaskDelay`).

**Hai thí nghiệm:**

- **A (không sửa code)** — bấm PD6 ở `IDLE` (không có gì xảy ra) rồi bấm PD6 ở
  `FILL_WATER` (chuyển trạng thái ngay). Cùng một nút, cùng đoạn code trong
  `sensor_task.c`, phản ứng khác nhau vì **WashingManager đổi nguồn chờ theo
  trạng thái**. Dạy được ý cốt lõi: *lọc sự kiện bằng lựa chọn nguồn chờ, không
  bằng chuỗi `if`.*
- **B (sửa 1 dòng)** — đổi `MOTOR_PIN` từ `GPIO_PIN_12` sang `GPIO_PIN_13` trong
  `bsp_motor.c:40`. Kết quả kiểm chứng được bằng mắt: đèn xanh lá **không sáng
  lần nào** trong cả chu trình, đèn cam gánh vai trò motor. Điểm dạy: **0 dòng
  phải sửa trong `washing_manager.c`** — giá trị lớp BSP được người học tự tay
  chứng minh, không phải nghe kể.

  Thí nghiệm B còn có tác dụng phụ đắt giá: PD13 giờ có **hai chủ** (van + motor),
  và kết quả chỉ đúng vì thứ tự lệnh trong callback tình cờ hợp lý
  (`Valve_Close()` chạy trước `Motor_Start()`). Đây là tiêu chí 4 (ownership)
  áp dụng ở mức chân MCU thay vì mức task — nối liền hai ý mà không cần thêm
  khái niệm mới.

**Câu 3 của mục Tự kiểm tra nối vào lịch sử thật của project:** "vì sao
WashingManager không tự đọc PD5?" → dẫn tới **Bug #8** (`bsp_sensor.c` từng đọc
cửa qua PA0, trùng chân nút menu). Tài liệu học dùng được bug thật của chính
project làm ví dụ là thứ sách vở không có.

**Giữ kỷ luật "một tầng một khái niệm":** code trích ở §2.4 có
`xQueueReceive()` và `xEventGroupWaitBits()`, nhưng tầng này **không giải thích
cơ chế** của chúng — chỉ dùng chúng làm bằng chứng cho "mỗi task chờ một nguồn
khác nhau", kèm ghi chú trỏ sang Tầng 3 và Tầng 4.

---


## Session 06 — 2026-06-26 — Phase 8: SEGGER SystemView

**Chủ đề:** RTT transport, queue registry, vQueueAddToRegistry, SEGGER_SYSVIEW_Conf/Start.

*Lưu ý: Session này tự hỏi và tự trả lời vì người học chưa có kiến thức nền về SystemView.*

---

### Q1: SEGGER SystemView truyền data từ MCU về PC qua cơ chế gì? Tại sao không dùng UART?

**Trả lời:** SystemView dùng SEGGER RTT (Real-Time Transfer). RTT ghi event vào ring buffer trong RAM. ST-Link đọc trực tiếp RAM qua SWD — CPU không tham gia. UART mất ~1ms cho 10 ký tự (làm chậm task), RTT ghi RAM chỉ mất vài chục ns (không ảnh hưởng timing).

---

### Q2: `vQueueAddToRegistry(xButtonQueue, "ButtonQ")` làm gì? Nếu không gọi thì sao?

**Trả lời:** Gán tên cho queue handle để SystemView hiển thị. Nếu không gọi, SystemView chỉ thấy địa chỉ RAM thô như `0x20003A40` thay vì `"ButtonQ"` — rất khó đọc khi debug. Hàm này không ảnh hưởng hoạt động queue, chỉ phục vụ debug tool.

---

### Q3: `configQUEUE_REGISTRY_SIZE` là gì và cần set bao nhiêu?

**Trả lời:** Số lượng queue/semaphore/mutex tối đa có thể đăng ký tên. Project có 3 queues cần đăng ký → cần ≥ 3. Hiện tại set = 8 trong FreeRTOSConfig.h — đủ dùng.

---

### Q4: Tại sao `SEGGER_SYSVIEW_Start()` phải gọi trước `vTaskStartScheduler()`?

**Trả lời:** Để capture event từ t=0 (scheduler start, task creation, first context switch). Nếu gọi sau `vTaskStartScheduler()`, code đó không bao giờ chạy vì scheduler không return. Nếu gọi từ trong task thì miss toàn bộ events từ đầu.

---

### Q5: Timer Service Task có hiện trên SystemView không? Dùng để làm gì?

**Trả lời:** Có. Hiện tên `"Tmr Svc"`. Trên timeline sẽ thấy nó chạy rất ngắn mỗi khi timer fire — đây là cách verify timer callback hoạt động đúng thời điểm và không chiếm CPU quá lâu.

---

### Q6: `SEGGER_SYSVIEW_Conf()` khác gì `SEGGER_SYSVIEW_Start()`?

**Trả lời:** `Conf()` khởi tạo một lần — setup RTT buffer, đăng ký interrupt names, thông tin MCU. `Start()` bắt đầu record events — có thể gọi/stop nhiều lần để capture khoảng thời gian cụ thể.

---

## Session 05 — 2026-06-26 — Phase 7: Software Timers

**Chủ đề:** vTaskDelay vs event-driven, Timer Service Task, thiết kế timer, one-shot vs auto-reload.

---

### Q1: Vấn đề gì xảy ra khi WashingManager đang `vTaskDelay(20000ms)` mà cửa máy giặt bị mở?

**Trả lời của bạn:** EVT_ERROR được ưu tiên hơn nên WashingManager sẽ thoát khỏi block.

**Đánh giá:** Sai.

**Giải thích:** `vTaskDelay()` không quan tâm đến event group — chỉ ngủ đủ ms rồi thức. WashingManager không hay biết EVT_ERROR trong suốt 20 giây, motor tiếp tục quay dù cửa đã mở. Đây là lỗi safety nghiêm trọng. Fix: dùng `xEventGroupWaitBits(EVT_TIMEOUT | EVT_ERROR)` — thức ngay khi có bất kỳ signal nào đến trước.

---

### Q2: Software timer callback chạy trong context của task nào? Tại sao không được dùng blocking API?

**Trả lời của bạn:** Chạy trong SensorTask.

**Đánh giá:** Sai.

**Giải thích:** Callback chạy trong **Timer Service Task** — task đặc biệt do FreeRTOS tự tạo khi enable software timers. Task này phục vụ tất cả timers trong hệ thống. Nếu một callback block (timeout > 0), Timer Service Task bị treo — mọi timer khác đều không được gọi callback. Phải dùng `FromISR` variants: `xEventGroupSetBitsFromISR()`, `xQueueSendFromISR()`.

---

### Q3: Nên tạo 4 timer riêng hay 1 timer dùng lại?

**Trả lời của bạn:** 4 timer riêng, nhưng tốn tài nguyên hơn.

**Đánh giá:** Đúng lựa chọn, nhận xét đúng về trade-off.

**Giải thích:** 4 timer tốn thêm ~150 bytes RAM nhưng code rõ ràng, dễ debug trên SystemView, không có risk nhầm timer. Với 88KB RAM còn trống, 150 bytes không đáng kể. Nguyên tắc: chỉ tối ưu khi đo được vấn đề.

---

### Q4: `xTimerStart()` cho wash timer nên được gọi ở đâu và bởi ai?

**Trả lời của bạn:** Gọi khi bắt đầu quá trình washing bởi WashingManager task.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích:** Gọi trong `Manager_OnEnterWashing()` — entry action callback của SM. Ngay khi vào state WASHING, motor bật và timer bắt đầu đếm cùng lúc. Fill-timeout timer cần được stop thủ công nếu nước đầy trước khi timer fire.

---

### Q5: Làm sao WashingManager biết `EVT_TIMEOUT` đến từ timer nào?

**Trả lời của bạn:** Kiểm tra xem SM đang ở state nào.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích:** Mỗi state chỉ có đúng một timer đang chạy — không cần phân biệt timer ID. EVT_TIMEOUT trong WASHING chỉ có thể từ wash timer, trong DRAIN chỉ từ drain timer. SM context thay thế hoàn toàn cho việc phân biệt.

---

### Q6: `uxAutoReload` nên là `pdTRUE` hay `pdFALSE`?

**Trả lời của bạn:** Chỉ fire 1 lần cho mỗi lần giặt.

**Đánh giá:** Đúng — `pdFALSE` (one-shot).

**Giải thích:** Mỗi giai đoạn chỉ cần một tín hiệu "hết giờ" duy nhất. Auto-reload sẽ fire liên tục, gây EVT_TIMEOUT thừa ở các state sau. One-shot là lựa chọn tự nhiên cho mọi deadline hay timeout.

---

## Case Study 01 — 2026-06-25 — Multiple Definition: SVC_Handler / PendSV_Handler / SysTick_Handler

---

### Triệu chứng

Build thất bại với lỗi linker:

```
multiple definition of `SVC_Handler'
multiple definition of `PendSV_Handler'
multiple definition of `SysTick_Handler'
```

---

### Nguyên nhân gốc rễ

Dự án tích hợp FreeRTOS thủ công (không qua CubeMX RTOS middleware).
Khi CubeMX tái tạo code (sau khi thêm I2C1), nó sinh lại `stm32f4xx_it.c`
với đầy đủ các interrupt handler tiêu chuẩn, bao gồm:

- `SVC_Handler`
- `PendSV_Handler`
- `SysTick_Handler`

Đồng thời, `FreeRTOSConfig.h` đang có các macro:

```c
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```

Các macro này khiến `port.c` của FreeRTOS compile ra các hàm trùng tên.
Kết quả: linker thấy hai định nghĩa cho cùng một symbol → lỗi.

---

### Tại sao xảy ra mỗi lần CubeMX tái tạo?

CubeMX không biết FreeRTOS đã được tích hợp thủ công. Nó luôn sinh ra
`SVC_Handler`, `PendSV_Handler`, `SysTick_Handler` trong `stm32f4xx_it.c`
vì đây là các handler tiêu chuẩn của Cortex-M4. Đây là conflict cấu trúc,
không phải lỗi code.

---

### Giải pháp (permanent — sống sót qua CubeMX tái tạo)

**Bước 1 — Đổi tên FreeRTOS handlers trong `FreeRTOSConfig.h`:**

```c
// Trước
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

// Sau
#define vPortSVCHandler     vFreeRTOS_SVC_Handler
#define xPortPendSVHandler  vFreeRTOS_PendSV_Handler
#define xPortSysTickHandler vFreeRTOS_SysTick_Handler
```

`port.c` giờ compile ra các hàm có tên khác — không còn conflict.

**Bước 2 — Forward calls trong `stm32f4xx_it.c` (trong USER CODE sections):**

```c
/* USER CODE BEGIN EV */
extern void vFreeRTOS_SVC_Handler(void);
extern void vFreeRTOS_PendSV_Handler(void);
extern void vFreeRTOS_SysTick_Handler(void);
/* USER CODE END EV */
```

```c
void SVC_Handler(void) {
  /* USER CODE BEGIN SVCall_IRQn 0 */
  vFreeRTOS_SVC_Handler();
  /* USER CODE END SVCall_IRQn 0 */
}

void PendSV_Handler(void) {
  /* USER CODE BEGIN PendSV_IRQn 0 */
  vFreeRTOS_PendSV_Handler();
  /* USER CODE END PendSV_IRQn 0 */
}

void SysTick_Handler(void) {
  /* USER CODE BEGIN SysTick_IRQn 0 */
  vFreeRTOS_SysTick_Handler();
  /* USER CODE END SysTick_IRQn 0 */
}
```

---

### Tại sao giải pháp này là permanent?

CubeMX tái tạo `stm32f4xx_it.c` nhưng **chỉ xóa code ngoài USER CODE sections**.
Các lệnh `extern` và lời gọi `vFreeRTOS_*` nằm trong USER CODE sections →
được giữ nguyên sau mỗi lần regenerate.

---

### Phân tích luồng sau khi fix

```
Cortex-M4 hardware trigger
        │
        ▼
SVC_Handler()          ← định nghĩa bởi stm32f4xx_it.c (CubeMX)
        │
        ▼
vFreeRTOS_SVC_Handler()  ← định nghĩa bởi port.c (FreeRTOS)
        │
        ▼
FreeRTOS scheduler / context switch logic
```

---

### Bài học

- Khi tích hợp FreeRTOS thủ công với CubeMX, luôn phải giải quyết conflict
  interrupt handler này.
- Giải pháp đúng không phải là xóa handler trong `stm32f4xx_it.c` (sẽ mất
  sau mỗi lần regenerate), mà là đổi tên FreeRTOS symbols và forward từ
  USER CODE sections.
- USER CODE sections là ranh giới an toàn duy nhất giữa code của bạn và
  code do CubeMX quản lý.

---

## Session 04 — 2026-06-25 — Phase 6: OLED Integration

**Chủ đề:** OLEDTask priority, display queue timeout, Blocked vs Idle, preemption, self-contained message, BSP layer.

---

### Q1: Tại sao OLEDTask có priority thấp nhất (1)?

**Trả lời của bạn:** Màn hình chỉ cần update từng thời điểm, không yêu cầu nghiêm ngặt về mặt thời gian đến mức ms hay us.

**Đánh giá:** Đúng.

**Giải thích thêm:** OLEDTask không được phép block WashingManager. Nếu OLEDTask có priority cao hơn, WashingManager có thể bị preempt giữa chừng khi đang điều khiển motor và valve — nguy hiểm. Priority thấp đảm bảo OLEDTask chỉ chạy khi mọi task quan trọng hơn đã Blocked.

---

### Q2: Tại sao WashingManager gửi display queue với timeout 0, còn UITask gửi command queue không dùng timeout 0?

**Trả lời của bạn:** Không biết.

**Giải thích:**
- **WashingManager → display queue (timeout 0):** WashingManager là task priority cao nhất. Nếu block chờ OLEDTask, toàn hệ thống đứng lại. Display message bị drop không phải thảm họa — lần transition tiếp theo gửi message mới.
- **UITask → command queue (có timeout):** Command mang ý định người dùng. Nếu bị drop, người dùng nhấn nút nhưng máy không phản hồi — lỗi chức năng nghiêm trọng.

Nguyên tắc: producer priority cao hơn consumer → timeout 0. Producer priority thấp hơn consumer → có thể block ngắn.

---

### Q3: Khi queue rỗng và OLEDTask đang chờ, nó ở trạng thái nào? Có tiêu thụ CPU không?

**Trả lời của bạn:** Trạng thái idle, không tiêu thụ CPU.

**Đánh giá:** Đúng về CPU, sai về tên trạng thái.

**Giải thích:** OLEDTask ở trạng thái **Blocked**, không phải Idle. Idle là trạng thái của Idle Task — task đặc biệt FreeRTOS tạo ra, chạy khi không có task nào khác Ready. Task thông thường khi chờ tín hiệu → Blocked. Blocked không tiêu thụ CPU.

---

### Q4: Trong khi OLEDTask đang ghi I2C (mất vài ms), nó có thể bị preempt không?

**Trả lời của bạn:** Bị preempt.

**Đánh giá:** Đúng.

**Giải thích:** FreeRTOS dùng preemptive scheduling — SysTick mỗi 1ms kiểm tra có task priority cao hơn đang Ready không. Nếu có, OLEDTask bị preempt ngay giữa I2C write. Đây là lý do OLEDTask phải là task duy nhất truy cập OLED — tránh corrupt data.

---

### Q5: Tại sao `DisplayMsg_t` chứa toàn bộ thông tin (self-contained) thay vì chỉ gửi phần thay đổi (partial update)?

**Trả lời của bạn:** Struct đầy đủ thì display dễ hơn.

**Đánh giá:** Đúng hướng.

**Giải thích đầy đủ:** Nếu dùng partial update, OLEDTask phải tự lưu state — giữ bản copy màn hình hiện tại, merge từng field. OLEDTask trở thành stateful, vi phạm nguyên tắc "pure renderer". Với self-contained message: nhận → render ngay → không cần nhớ gì. `DisplayMsg_t` chỉ 32 bytes — chi phí không đáng kể trên 128KB RAM.

Nguyên tắc: mỗi message phải tự đủ nghĩa — người nhận không cần context bên ngoài.

---

## Session 03 — 2026-06-25 — Phase 4: SensorTask

**Chủ đề:** vTaskDelay vs HAL_Delay, priority preemption, polling vs interrupt.

---

### Q1: Tại sao dùng `vTaskDelay()` thay vì `HAL_Delay()` cho chu kỳ 100ms?

**Trả lời của bạn:** HAL_Delay do CPU kiểm soát nên tốn tài nguyên CPU; vTaskDelay do RTOS kiểm soát.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích thêm:**
- `HAL_Delay()` là busy-wait — CPU chạy vòng lặp rỗng, không nhường thời gian cho task khác.
- `vTaskDelay()` chuyển task sang trạng thái Blocked — CPU rảnh để chạy task khác. Sau đúng 100ms scheduler đưa task về Ready. Đây là cách đúng trong RTOS.

---

### Q2: Sau khi SensorTask set `EVT_WATER_FULL`, WashingManager có chạy ngay không?

**Trả lời của bạn:** Phải chờ cả event của door nữa.

**Đánh giá:** Chưa chính xác.

**Giải thích:**
Trong FILL_WATER state, WashingManager chỉ chờ `EVT_WATER_FULL | EVT_TIMEOUT` — không chờ door.

Điều quan trọng hơn là **priority preemption**:
- SensorTask: priority 2
- WashingManager: priority 3 (cao hơn)

Khi SensorTask gọi `xEventGroupSetBits(EVT_WATER_FULL)`, WashingManager unblock. Vì priority cao hơn, scheduler preempt SensorTask **ngay bên trong** lời gọi `xEventGroupSetBits()` — trước khi SensorTask kịp chạy thêm dòng nào. Priority quyết định ai chạy, không phải ai set bit trước.

---

### Q3: Tại sao SensorTask dùng polling thay vì interrupt?

**Trả lời của bạn:** Trong thực tế nên dùng ngắt để notify; đây là mô phỏng và không có cảm biến thật nên dùng tạm poll.

**Đánh giá:** Đúng hoàn toàn.

**Tổng kết:**
- **Interrupt:** tín hiệu thay đổi bất kỳ lúc nào, cần phản ứng ngay (vài micro giây).
- **Polling:** không có phần cứng thật, tín hiệu thay đổi chậm, latency 100ms chấp nhận được.

SensorTask trong dự án này là stub. Khi có phần cứng thật, thay bằng ISR + `xEventGroupSetBitsFromISR()` — phần còn lại không đổi.

---

## Session 02 — 2026-06-25 — Phase 3: BSP Button + UITask

**Chủ đề:** Tại sao tách ISR khỏi UITask, tại sao tách BSP khỏi application layer, debounce.

---

### Q1: Tại sao ISR không xử lý menu logic trực tiếp mà phải post event vào `xButtonQueue`?

**Trả lời của bạn:** Vì khi người dùng bấm nhiều lần context rất dễ bị miss nếu xử lý menu trực tiếp.

**Đánh giá:** Đúng — đây là lý do chính.

**Giải thích đầy đủ:**
1. **Queue có depth — buffering nhiều lần nhấn:** Queue tích lũy event, UITask xử lý khi được schedule. Không mất event dù nhấn nhanh.
2. **ISR phải chạy cực ngắn:** ISR chạy ở elevated priority, block interrupt khác. Menu logic trong ISR làm tăng latency toàn hệ thống.
3. **FreeRTOS API trong ISR bị giới hạn:** Không được gọi `xQueueSend()` từ ISR — phải dùng `xQueueSendFromISR()`. Không được gọi blocking API (vTaskDelay, xQueueReceive). Tách ISR và UITask giúp ranh giới này rõ ràng.

---

### Q2: Tại sao không để UITask đọc GPIO trực tiếp thay vì đi qua BSP?

**Trả lời của bạn:** Tách layer giúp làm việc rõ ràng hơn; nhiều task có thể dùng BSP nên phải tách ra.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích thêm:**
- **Portability:** Nếu đổi board, GPIO pin thay đổi, chỉ sửa `bsp_button.c` — UITask không đụng đến.
- **Rule trong dự án:** *"BSP hides HAL from application code"* — UITask không được biết PA0 là pin nào, chỉ biết "có button event".

---

### Q3: Debounce là gì? Tại sao cần debounce cho nút nhấn cơ học?

**Trả lời của bạn:** Thuật ngữ mới, đoán liên quan đến nhiễu.

**Đánh giá:** Đúng hướng.

**Giải thích:**
Nút nhấn cơ học có tiếp điểm kim loại — khi nhấn, tiếp điểm **nảy (bounce)** nhiều lần trong ~50ms trước khi ổn định. MCU chạy ở MHz, thấy mỗi lần nảy là một lần nhấn riêng. Người dùng nhấn 1 lần nhưng EXTI có thể trigger 5–10 lần.

```
PA0:  ____    _ __________
          |  | |
__________|  |_|
          ^bounce^ (~50ms)
```

**Giải pháp:** Sau EXTI đầu tiên, bỏ qua mọi trigger trong vòng `BUTTON_DEBOUNCE_MS` (50ms). Chỉ lần trigger đầu tiên được post vào `xButtonQueue`.

---

## Session 01 — 2026-06-25 — Phase 5: Wiring main.c

**Chủ đề:** Thứ tự khởi tạo FreeRTOS, stack size, stack overflow, test không có UITask.

---

### Q1: Tại sao `App_FreeRTOSObjectsInit()` phải gọi trước `vTaskStartScheduler()`? Điều gì xảy ra nếu task được tạo trước khi queue tồn tại?

**Trả lời của bạn:** Không biết.

**Giải thích:**
Khi `WashingManagerTask` chạy, nó gọi `xQueueReceive(xCommandQueue, ...)`.
Handle `xCommandQueue` là một con trỏ tới queue object trong RAM.
Nếu `App_FreeRTOSObjectsInit()` chưa chạy, handle đó vẫn là `NULL`.
Truyền `NULL` vào bất kỳ FreeRTOS API nào là undefined behavior — crash hoặc corrupt memory.

**Quy tắc đúng:**
```
Tạo RTOS objects → Tạo tasks → vTaskStartScheduler()
```
Task được tạo ở trạng thái suspended — chúng không chạy cho đến khi scheduler bắt đầu.
Vì vậy, chỉ cần tạo queue/event group trước `vTaskStartScheduler()` là đủ.

---

### Q2: Tại sao không thể gọi `App_FreeRTOSObjectsInit()` sau `vTaskStartScheduler()`?

**Trả lời của bạn:** Vì `vTaskStartScheduler()` không bao giờ trả về.

**Đánh giá:** Đúng hoàn toàn.

**Giải thích thêm:**
Khi `vTaskStartScheduler()` được gọi, FreeRTOS:
1. Tạo Idle Task và Timer Task nội bộ.
2. Cấu hình SysTick interrupt.
3. Chuyển quyền điều khiển sang task có priority cao nhất ở trạng thái Ready.

CPU không bao giờ quay lại dòng code sau `vTaskStartScheduler()`.
Trường hợp duy nhất nó trả về là khi không đủ RAM để tạo Idle Task — và lúc đó hệ thống crash ngay.

---

### Q3: Stack size 256 words trên STM32F411 = bao nhiêu bytes?

**Trả lời của bạn:** 4 × 256 = 1024 bytes.

**Đánh giá:** Đúng.

**Giải thích thêm:**
FreeRTOS dùng đơn vị "words" để portable:
- Kiến trúc 8-bit: 1 word = 1 byte
- Kiến trúc 32-bit (STM32): 1 word = 4 bytes

Cùng con số `256` nhưng ý nghĩa thực tế khác nhau tùy nền tảng.
`xTaskCreate(..., 256, ...)` trên STM32 = cấp 1 KB stack cho task đó.

---

### Q4: Tại sao stack overflow gây hard fault thay vì lỗi rõ ràng hơn? Điều gì xảy ra ở mức phần cứng?

**Trả lời của bạn:**
- Stack lưu các execution frame của task (đúng).
- Nếu stack không đủ thì đã fail ở bước compile (sai).
- Khi chạy gây stack overflow thì crash ngay (gần đúng).

**Điểm cần sửa — compile-time detection là sai:**
Compiler không thể biết call stack sẽ sâu bao nhiêu lúc runtime.
`xTaskCreate(..., 256, ...)` chỉ là con số nguyên — compiler không phân tích được
toàn bộ call chain sẽ dùng bao nhiêu stack.
Đây là vấn đề **runtime**, không phải compile-time.

**Điều thực sự xảy ra lúc runtime:**
Khi stack tràn, task ghi đè vào vùng RAM của task khác hoặc kernel.
Không có cơ chế bảo vệ phần cứng nào ngăn điều này (trừ khi bật MPU).
Crash xảy ra **muộn hơn** và **ở chỗ khác** — rất khó debug.

FreeRTOS có `configCHECK_FOR_STACK_OVERFLOW` để phát hiện sớm hơn,
nhưng vẫn là runtime check, không phải compile-time.

---

### Q5: Khi chưa có UITask, làm sao test `WashingManagerTask`?

**Trả lời của bạn:** Truyền trực tiếp command vào queue.

**Đánh giá:** Đúng — đây chính xác là cách embedded engineer kinh nghiệm làm.

**Hai cách phổ biến:**

**Cách 1 — Hardcode tạm trong `main.c`** (đơn giản hơn):
```c
CommandMsg_t cmd = { .command = CMD_START_WASH };
xQueueSend(xCommandQueue, &cmd, 0);
// Đặt sau App_FreeRTOSObjectsInit(), trước vTaskStartScheduler()
```

**Cách 2 — Debugger Expression** (không cần recompile):
Pause debugger, gọi trong Expressions window:
```c
xQueueSend(xCommandQueue, &cmd, 0)
```
