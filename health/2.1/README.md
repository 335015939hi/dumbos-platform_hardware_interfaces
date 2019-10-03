# Implementing Health 2.1 HAL

1. If you just want to use defaults for everything:

```
# Install default passthrough implementation to vendor.
PRODUCT_PACKAGES += android.hardware.health@2.1-impl

# Install default binderized implementation to vendor.
PRODUCT_PACKAGES += android.hardware.health@2.1-service

# Install default vendor charger
PRODUCT_PACKAGES += charger_vendor

# Install default passthrough implementation to recovery.
PRODUCT_PACKAGES += android.hardware.health@2.1-impl.recovery

# Install default recovery charger
PRODUCT_PACKAGES += charger_recovery
```

TODO
