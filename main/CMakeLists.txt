idf_component_register(SRC_DIRS "examples/general_test"
                       INCLUDE_DIRS "examples/general_test"
                                    "../libraries/private_library"
                       REQUIRES driver
                                esp_driver_gpio
                                esp_driver_uart
                                esp_event
                                esp_http_server
                                esp_netif
                                esp_timer
                                esp_wifi
                                led_strip
                                nvs_flash)
