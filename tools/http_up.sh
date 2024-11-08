#!/bin/bash

curl -X "POST" http://192.168.12.101/fw_upgrade -F "file=@build/data_collect/zephyr/zephyr.signed.confirmed.bin"
