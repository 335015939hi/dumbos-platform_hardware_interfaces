# HCI Proxy

This app directly accesses IBluetoothHci HIDL or AIDL HAL, receiving packets from HAL/client and forward them to each other.

## Build
```
m HciProxy
```

## Usage
```
# forward TCP to host
adb forward tcp:9100 tcp:9100

# root
adb root

# disable selinux(because access HAL requires sepolicy but we don't want to add it)
adb shell setenforce 0

# install
adb install -r -g out/target/product/<target>/testcases/HciProxy/<arch>/HciProxy.apk

# run
adb shell 'am instrument --no-hidden-api-checks -w android.hardware.bluetooth.hci_proxy/.Main'
```

## Example Usage

A common tool for HciProxy is [Bumble](https://github.com/google/bumble), which is a Python scriptable Bluetooth stack. Its TCP client transport can directly access the HciProxy.

Sometimes, we want to inspect some behavior related to Bluetooth controllers instead of Android Bluetooth stacks, but the chip vendor may not provide us other interface to access it. Then, we can use Bumble to send specific HCI commands without vendor-specific tools and long winded setup procedure.

Another case is we want to use some features not common supported by USB dongles or devboards(ex. LE Audio & Dual Mode), then we can turn our Android devices into a Bluetooth interface.
