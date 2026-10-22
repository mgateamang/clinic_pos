# AIDANS Pharmacy Point-of-Sale (POS) Client (Qt6 / QML)

## Executive Summary (For Engineering Leaders & Hiring Managers)

**`clinic_pos`** is an operational desktop Point-of-Sale and dispensary client developed in **C++20 / Qt6 / QML**. It is designed specifically for pharmacy cashiers, clinic receptionists, and dispensing pharmacists.

---

## Architectural Highlights

1. **Digital Prescription Integration:**
   - Cashiers can enter or scan a physician's e-Prescription code (e.g. `RX-XXXXXX-XXXX`).
   - The application automatically calls `GET /api/v1/prescriptions/:code`, resolves all prescribed medicines and dosages, checks available stock, and populates the checkout cart in a single click.
2. **Direct Over-The-Counter (OTC) Sales:**
   - Staff can search the live clinic inventory catalog, select items, specify quantities, and add them to the cart with instant subtotal, tax, and discount computations.
3. **Atomic Checkout & Dispensation:**
   - On checkout confirmation, a unified payload is posted to `POST /api/v1/pos/checkout`.
   - The backend atomically deducts stock from the shelves, records audited receipt records, and marks the linked prescription as `DISPENSED`, preventing double-dispensing fraud.
4. **Offline Resilience & Runtime Configuration:**
   - Incorporates automated health status indicators and an in-app Settings modal allowing on-the-fly backend URL changes and connection tests.

---

## Build and Run

```bash
# Configure with Meson
meson setup build

# Compile
meson compile -C build

# Launch application
./build/clinic_pos
```
