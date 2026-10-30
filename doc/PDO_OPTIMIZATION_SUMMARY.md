# F28P65x SOES PDO optimization summary

## Result

For the fixed 42-octet RxPDO and 32-octet TxPDO used by `soes_test`, the
selected implementation runs the Sync0 path in **less than 30 us** and
preserves the correct `rx_pdo.ts` / `tx_pdo.rtt` behaviour.

| Sync0 time | PDO transfer and mapping method | Status |
| --- | --- | --- |
| ~200 us | Generic SOES mapping with `COE_pdoPack()` / `COE_pdoUnpack()` performing per-bit field handling | Functional, but too costly on C28x |
| ~70 us | Generic mapping with byte-aligned fast paths in CoE; ESC process data transferred through `ESC_read_octets()` / `ESC_write_octets()` | Functional |
| <30 us | F28-specific fixed mapping: explicit field encode/decode plus `ESC_read_octets()` / `ESC_write_octets()` | Functional and selected |

The fixed override is in `uc_test/f28p65x/soes_test/pdo_override.c`. It is
enabled only for `__TMS320C28XX__` by the test hook. Other targets retain the
generic mapper.

## Selected data flow

```text
SM2 -> ESC_read_octets(42) -> local_rx_wire -> fixed offsets -> rx_pdo
tx_pdo -> fixed offsets -> local_tx_wire -> ESC_write_octets(32) -> SM3
```

`F28P65X_RXPDO_WIRE_SIZE` and `F28P65X_TXPDO_WIRE_SIZE` define the packed
EtherCAT wire sizes. PDO field offsets are expressed in EtherCAT octets.

## Why the octet-wire flow is required on C28x

The C28x has 16-bit addressable memory: an EtherCAT 8-bit octet cannot be
represented by ordinary C pointer arithmetic or a packed C structure. The
SOES C28 port therefore represents a logical wire octet with `esc_octet_t`
and accesses it through `esc_octet.h` helpers.

`ESC_read_octets()` and `ESC_write_octets()` are the boundary between the
EtherCAT byte-addressed process image and C28 memory. They explicitly:

- map a pair of EtherCAT octets to/from one 16-bit ESC word;
- preserve low-octet then high-octet wire order;
- handle odd start/end addresses correctly; and
- refresh the AL-event state once after the full transfer.

The fixed override reuses `esc_get_le16()`, `esc_get_le32()`,
`esc_put_le16()`, and `esc_put_le32()` from `esc_octet.h`, so field decoding
has the same little-endian semantics as the generic CoE mapper.

## Why direct register-plus-offset scalar access was not selected

An experiment used `ESC_readWordISR()` / `ESC_readDWordISR()` and matching
write functions directly for every field. It was fast (~36 us), but the
returned `rx_pdo.ts` / `tx_pdo.rtt` data was incorrect compared with the
generic and octet-transfer paths.

Those primitives address the ESC as C28 16-/32-bit words (`address / 2`),
whereas PDO layout is defined in EtherCAT octets. Even for presently aligned
fields, that path bypasses the established wire-octet transfer contract and
does not exercise the same packing and event-refresh sequence. It is therefore
not used for process data.

Direct scalar PDI access remains suitable for native ESC registers when the
register width and semantics are explicitly known. For PDO process images on
the C28x, use the octet APIs.

## Constraint

The fixed override is tied to the current `0x1600` and `0x1A00` mappings. If
those PDO mappings change, update the offsets and wire-size constants, or
disable the override and use the generic mapper.
