# HCI Proxy

This app directly accesses IBluetoothHci HIDL or AIDL HAL, receiving packets from HAL/client and forward them to each other.

## Build

```
m hci_proxy
```

## Usage
```
# forward TCP to host
adb forward tcp:9100 tcp:9100

# root
adb root

# push
adb push out/target/product/<target>/testcases/hci_proxy/<arch>/hci_proxy /data/local/tmp/
adb shell chmod +x /data/local/tmp/hci_proxy

# run
adb shell /data/local/tmp/hci_proxy <port(optional, default=9100)>
```

If port 0 is specified, a random port will be allocated and reported in Logcat.

## Example Usage

A common tool for HciProxy is [Bumble](https://github.com/google/bumble), which is a Python scriptable Bluetooth stack. Its TCP client transport can directly access the HciProxy.

Sometimes, we want to inspect some behavior related to Bluetooth controllers instead of Android Bluetooth stacks, but the chip vendor may not provide us other interface to access it. Then, we can use Bumble to send specific HCI commands without vendor-specific tools and long winded setup procedure.

Another case is we want to use some features not commonly supported by USB dongles or devboards(ex. LE Audio & Dual Mode), then we can turn our Android devices into a Bluetooth interface.
