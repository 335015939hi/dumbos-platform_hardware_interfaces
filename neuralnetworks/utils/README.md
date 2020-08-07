# NNAPI Conversions

`convertible` indicates that the canonical type is _representable_ in this
version type. Specifically, it indicates that the type resulting from
convert is valid. For example, let's say that an enumeration in the
current version has fewer possible values than the "same" canonical
enumeration, such as `OperationType`. The new value of `HARD_SWISH` does
not map to any valid existing value in OperationType, but an older value
of ADD is valid:

```cpp
// Unsuccessful conversion
const auto canonical = nn::OperationType::HARD_SWISH
const auto maybeVersioned = V1_0::utils::convert(canonical);
EXPECT_FALSE(maybeVersioned.has_value());
```
```cpp
// Successful conversion
const auto canonical = nn::OperationType::ADD
const auto maybeVersioned = V1_0::utils::convert(canonical);
ASSERT_TRUE(maybeVersioned.has_value());
const auto versioned = maybeVersioned.value();
EXPECT_TRUE(V1_0::utils::valid(versioned));
```

`V1_X::utils::convert` does not guarantee that all information is preserved.
For example, In the case of `nn::ErrorStatus`, the new value of
`MISSED_DEADLINE_TRANSIENT` can be represented by the existing value of
`V1_0::GENERAL_FAILURE`:

```cpp
// Lossy Canonical -> HAL -> Canonical conversion
const auto canonicalBefore = nn::ErrorStatus::MISSED_DEADLINE_TRANSIENT;
const auto versioned = V1_0::utils::convert(canonicalBefore).value();
const auto canonicalAfter = nn::convert(versioned).value();
EXPECT_NE(canonicalBefore, canonicalAfter);
```

However, `nn::convert` is guaranteed to preserve all information:

```cpp
// Lossless HAL -> Canonical -> HAL conversion
const auto versionedBefore = V1_0::ErrorStatus::GENERAL_FAILURE;
const auto canonical = nn::convert(versionedBefore).value();
const auto versionedAfter = V1_0::utils::convert(canonical);
EXPECT_EQ(versionedBefore, versionedAfter);
```

The `convert` functions operate on types that are either:
1. used in a HIDL method call directly (i.e., not as a nested class)
2. used in a subsequent version of the NN HAL
