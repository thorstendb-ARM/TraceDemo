/* SPDX-License-Identifier: Apache-2.0 */
/* Memory selected from Keil::STM32L5xx_DFP@2.0.0, STM32L552ZETxQ.
 * Use one internal Flash region and one SRAM region for this baseline.
 * Additional banks and external memory are deliberately not linked yet.
 */
#ifndef TRACE_MEMORY_REGIONS_H
#define TRACE_MEMORY_REGIONS_H

#define __ROM0_BASE  0x08000000
#define __ROM0_SIZE  0x00080000
#define __RAM0_BASE  0x20000000
#define __RAM0_SIZE  0x00040000
#define __STACK_SIZE 0x00001000
#define __HEAP_SIZE  0x00000000

#endif
