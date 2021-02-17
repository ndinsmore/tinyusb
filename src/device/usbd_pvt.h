/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */
#ifndef USBD_PVT_H_
#define USBD_PVT_H_

#include "osal/osal.h"
#include "common/tusb_fifo.h"

#ifdef __cplusplus
 extern "C" {
#endif
//--------------------------------------------------------------------+
// Start of Frame (SOF) API
//--------------------------------------------------------------------+

#define TUSB_USBD_SOF_ERROR_BUFFER_SIZE 4

//  uint32_t sof_us ; //USB "Start of Frame (sof)"  this is a sythetic value to provide better availability
//   uint32_t eoa_us ; //This is the "End of availabilty"  which will not allow for reading of the sof after this period
//   uint16_t interval_us ; // Approximate micro seconds of the system clock per frame 
//   uint16_t avg_interval_us;
//   uint8_t  eoa_margin_us ; //Microseconds before next sof to end availability
typedef struct  {
    volatile uint8_t ind ;
    volatile uint16_t sof_direct[TUSB_USBD_SOF_ERROR_BUFFER_SIZE];
    volatile uint16_t sof_synthetic[TUSB_USBD_SOF_ERROR_BUFFER_SIZE];
    volatile int16_t sof_err[TUSB_USBD_SOF_ERROR_BUFFER_SIZE];
    volatile int64_t cum_err;
  } usbd_sof_err_t;
typedef struct
{
  volatile uint32_t sof_us ; 
  volatile uint32_t eoa_us ; 
  volatile uint16_t interval_us ; 
  volatile uint16_t avg_interval_us;
  volatile uint8_t  eoa_margin_us ;
  volatile enum  {
    USBD_SOF_LOCKED = 0,
    USBD_SOF_UNLOCKED
  } lock_state;
  usbd_sof_err_t err;
} usbd_sof_t;

//This two fuction are used by the direct and sythetic handlers to write their value of the sof timing for error calculation
static inline void  usbd_sof_direct_for_error(uint32_t sof);
static inline void  usbd_sof_synth_for_error(uint32_t sof);



uint32_t usbd_get_sof_us_32(usbd_sof_t * sof);
uint16_t usbd_get_sof_us_16(usbd_sof_t * sof);


//This really should only be used by initilization
bool usbd_set_sof(usbd_sof_t * sof, uint32_t sof_us, bool in_isr);

//This function incriments to the next sof and is intended to be call from an IRQ
// uint16_t usbd_incriment_sof(usbd_sof_t * sof, bool in_isr);

bool usbd_set_sof_interval(usbd_sof_t * sof, uint16_t interval_us, bool in_isr);

static void usbd_sof_set_synthetic_handler(void);
static void usbd_sof_mid_frame_handler(void);
//--------------------------------------------------------------------+
// Class Drivers
//--------------------------------------------------------------------+

typedef struct
{
  #if CFG_TUSB_DEBUG >= 2
  char const* name;
  #endif

  void     (* init             ) (void);
  void     (* reset            ) (uint8_t rhport);
  uint16_t (* open             ) (uint8_t rhport, tusb_desc_interface_t const * desc_intf, uint16_t max_len);
  bool     (* control_xfer_cb  ) (uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);
  bool     (* xfer_cb          ) (uint8_t rhport, uint8_t ep_addr, xfer_result_t event, uint32_t xferred_bytes);
  void     (* sof              ) (uint8_t rhport); /* optional */
} usbd_class_driver_t;

// Invoked when initializing device stack to get additional class drivers.
// Can optionally implemented by application to extend/overwrite class driver support.
// Note: The drivers array must be accessible at all time when stack is active
usbd_class_driver_t const* usbd_app_driver_get_cb(uint8_t* driver_count) TU_ATTR_WEAK;


typedef bool (*usbd_control_xfer_cb_t)(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);

//--------------------------------------------------------------------+
// USBD Endpoint API
//--------------------------------------------------------------------+

// Open an endpoint
bool usbd_edpt_open(uint8_t rhport, tusb_desc_endpoint_t const * desc_ep);

// Close an endpoint
void usbd_edpt_close(uint8_t rhport, uint8_t ep_addr);

// Submit a usb transfer
bool usbd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t * buffer, uint16_t total_bytes);

// Claim an endpoint before submitting a transfer.
// If caller does not make any transfer, it must release endpoint for others.
bool usbd_edpt_claim(uint8_t rhport, uint8_t ep_addr);

// Release an endpoint without submitting a transfer
bool usbd_edpt_release(uint8_t rhport, uint8_t ep_addr);

// Check if endpoint transferring is complete
bool usbd_edpt_busy(uint8_t rhport, uint8_t ep_addr);

// Stall endpoint
void usbd_edpt_stall(uint8_t rhport, uint8_t ep_addr);

// Clear stalled endpoint
void usbd_edpt_clear_stall(uint8_t rhport, uint8_t ep_addr);

// Check if endpoint is stalled
bool usbd_edpt_stalled(uint8_t rhport, uint8_t ep_addr);

static inline
bool usbd_edpt_ready(uint8_t rhport, uint8_t ep_addr)
{
  return !usbd_edpt_busy(rhport, ep_addr) && !usbd_edpt_stalled(rhport, ep_addr);
}

/*------------------------------------------------------------------*/
/* Helper
 *------------------------------------------------------------------*/

bool usbd_open_edpt_pair(uint8_t rhport, uint8_t const* p_desc, uint8_t ep_count, uint8_t xfer_type, uint8_t* ep_out, uint8_t* ep_in);
void usbd_defer_func( osal_task_func_t func, void* param, bool in_isr );


#ifdef __cplusplus
 }
#endif

#endif /* USBD_PVT_H_ */
