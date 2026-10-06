/* SPDX-License-Identifier: Apache-2.0 */
#include <stdint.h>
#include "cmsis_os2.h"
#include CMSIS_device_header

/* The linker orders these sections and asserts that the objects are adjacent. */
volatile int32_t arrWorker1[8]
    __attribute__((section(".bss.demo.arrWorker1"), aligned(4), used));
volatile int32_t varWorker2
    __attribute__((section(".bss.demo.varWorker2"), aligned(4), used));

_Static_assert(sizeof(arrWorker1) == 32, "Trace demo requires eight 32-bit entries");

static void worker1(void *argument)
{
    (void)argument;

    for (unsigned int cycle = 0; cycle < 2U; cycle++) {
        int cnt = 0;
        for (int i = 0; i <= 8; i++) {
            /* Deliberate bug: i == 8 writes 8 into worker2's variable.
             * This is undefined C behavior, retained for this -O0 demo.
             * The actual fix is i < 8.
             */
            arrWorker1[i] = cnt++;
            (void)osDelay(100U);
        }
    }

    /* Remain blocked until explicitly resumed by another thread. */
    (void)osThreadSuspend(osThreadGetId());
}

static void worker2(void *argument)
{
    (void)argument;

    for (unsigned int cycle = 0; cycle < 2U; cycle++) {
        int cnt = 0;
        for (int i = 0; i < 20; i++) {
            cnt += 10;
            varWorker2 = cnt;
            (void)osDelay(100U);
        }
    }

    /* Remain blocked until explicitly resumed by another thread. */
    (void)osThreadSuspend(osThreadGetId());
}

int main(void)
{
    static const osThreadAttr_t worker1_attr = {
        .name = "worker1",
        .stack_size = 512U,
        .priority = osPriorityNormal
    };
    static const osThreadAttr_t worker2_attr = {
        .name = "worker2",
        .stack_size = 512U,
        .priority = osPriorityNormal
    };

    /* Keep the vendor startup's 4 MHz MSI clock, matching the SWO profile. */
    SystemCoreClockUpdate();

    if (osKernelInitialize() == osOK) {
        osThreadId_t thread1 = osThreadNew(worker1, NULL, &worker1_attr);
        osThreadId_t thread2 = osThreadNew(worker2, NULL, &worker2_attr);
        if ((thread1 != NULL) && (thread2 != NULL)) {
            (void)osKernelStart();
        }
    }
    for (;;) {
        /* Kernel initialization, thread creation or scheduler start failed. */
    }
}
