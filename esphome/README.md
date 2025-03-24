ESPHome configuration for LILYGO T-CAN485
=========================================

The YAML files in this directory are [ESPHome](https://esphome.io) packages that
make it easy to use configure the T-CAN485 for its main purposes.

Usage example for configuring [a UART
component](https://esphome.io/components/uart.html) to communicate over RS-485:

```yaml
packages:
  lilygo:
    url: https://github.com/Xinyuan-LilyGO/T-CAN485
    file: esphome/rs485.yaml

substitutions:
  # For the `lilygo` package
  rs485_uart_id: my_uart_id

uart:
  id: my_uart_id
  baud_rate: ...
```

Then add ESPHome components like `modbus` that use the `uart` component.
