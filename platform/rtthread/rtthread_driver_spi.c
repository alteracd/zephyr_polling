#include <stdio.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "rtthread_driver_serial.h"
#include "drivers/hci_driver.h"
#include "drivers/hci_h4.h"

#include "logging/bt_log_impl.h"

#include "common/bt_buf.h"

static rt_device_t h4_uart;

static int hci_driver_h4_open(void)
{
    RT_ASSERT(uart_config.name);

    printk("hci_driver_h4_open, uart_config.name: %s\n", uart_config.name);

    h4_uart = rt_device_find(uart_config.name);
//    printk("hci_driver_h4_open, h4_uart: 0x%x\n", h4_uart);
    RT_ASSERT(h4_uart);

    rt_err_t err;

    if ((err = rt_device_open(h4_uart, RT_DEVICE_FLAG_INT_RX))) {
        printk("Open h4_uart error\n");
        return -1;
    }
    if ((err = rt_device_control(h4_uart, RT_DEVICE_CTRL_CONFIG, &uart_config.rt_config))) {
        printk("Control h4_uart error\n");
        return -1;
    }

    return 0;
}