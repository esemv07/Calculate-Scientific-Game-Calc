# Calculate-Scientific-Game-Calc


## Project Progress Checklist
- [x] Electronic Design
  - [x] Schematic
  - [x] PCB
    - [x] Wiring
    - [x] Mounting Holes
- [ ] Case Design
  - [x] Top Lid
  - [x] Mounting
  - [x] Bottom Case
  - [x] Decoration
  - [x] Buttons
  - [ ] Button Labels
- [ ] Firmware
  - [x] Button Scanning
  - [x] Button Presses
  - [x] Evaluation
    - [x] Separating order of operations to tokens
    - [x] Ordering tokens as RPN
    - [x] Evaluating each operation in correct order
  - [x] Screen Display - to be tuned further once actual parts have arrived
  - [ ] Games - to be made with actual parts

## Images

### Schematic
<img width="860" height="587" alt="Screenshot 2026-07-08 at 12 59 47 PM" src="https://github.com/user-attachments/assets/982ef821-a502-4b9d-b307-f846f9461060" />

### PCB
<img width="413" height="622" alt="Screenshot 2026-07-08 at 1 00 05 PM" src="https://github.com/user-attachments/assets/0fab9c5a-99ee-43a8-b146-782d8d761b75" />

### Case
<img width="817" height="554" alt="Screenshot 2026-07-11 at 10 00 24 AM" src="https://github.com/user-attachments/assets/9750ca39-b7d5-411d-8ebe-b288d2969cd9" />
<img width="825" height="603" alt="Screenshot 2026-07-11 at 10 00 58 AM" src="https://github.com/user-attachments/assets/76956ce2-730b-4ddd-bf18-14d52024f660" />


## Firmware

### Overview
This is also in the top of the firmware file - it was just the flow of my thoughts and planning so is not perfectly polished
```
-- PROJECT FLOW --

setup:
- setup IO expander
- setup screen
- display initial screen

loop: 
- check for button press
- draw display - could probably leave this out of loop because it reloads on every input

check for button press:
- check whether shifted
- return row + column with shifted/normal key

HANDLE KEY PRESS
Types of key presses:
- Input keys
  - number (0-9)
  - in-built operation (+ - / * ^)
  - special operation (! sqrt otherroot x10^ %)
  - trig function (sin cos tan asin acos atan)
  - other function (log ln Abs nPr nCr)
  - constants (Pi Euler Ans)
  - other inputs ('(' ')' ',' '.')
- Control keys
  - other (Mode Game Shift)
  - deletion (AC DEL)
  - equals (= S<=>D Deg/Min/Sec)
  - direction (UP DOWN LEFT RIGHT)

Flow:
if input key : append to expression

if control key : check - 'can control be executed before evaluation'
  if yes : execute
  if no : continue


EVALUATION
- separate expression into order of operations
  1. brackets
  2. functions
  3. exponents
  4. operations (* /)
  5. operations (+ -)
- evaluate in separated parts
- return ans
- store ans as Ans
- draw display

Flow:
- convert expression to tokens
- turn into RPN
- evaluate RPN
- store Ans
- return ans to display
```

### Image
<img width="1440" height="827" alt="Screenshot 2026-07-10 at 6 27 17 PM" src="https://github.com/user-attachments/assets/f29ea4cc-cece-4484-aebb-00d59bd3d92f" />



## BOM

See [BOM.md](https://github.com/esemv07/Calculate-Scientific-Game-Calc/blob/main/BOM.md)

| Item | Price per Unit | Unit Size | No. of Units | Total Price | Link |
|------|----------------|-----------|--------------|-------------|------|
| Tactile Buttons (SHOU HAN TS4555CJ 250gf 009) | $0.43 | 50pcs | 1 | $0.43 | [https://www.lcsc.com/product-detail/C5359335.html](https://www.lcsc.com/product-detail/C5359335.html) |
| Arduino Pro Mini 3.3v 8MHz | $13.51 | 1pcs | 1 | $13.51 | [https://core-electronics.com.au/arduino-pro-mini-328-3-3v-8mhz.html](https://core-electronics.com.au/arduino-pro-mini-328-3-3v-8mhz.html) |
| 0.96" IPS LCD Display Module 80X160 | $3.25 | 1pcs | 1 | $3.25 | [https://www.aliexpress.com/item/1005001617019927.html](https://www.aliexpress.com/item/1005001617019927.html) |
| MT3608 | $1.00 | 1pcs | 1 | $1.00 | [https://www.aliexpress.com/item/1005010575920726.html](https://www.aliexpress.com/item/1005010575920726.html) |
| 2 AAA Battery Holder | $1.00 | 1pcs | 1 | $1.00 | [https://www.aliexpress.com/item/1005012118979890.html](https://www.aliexpress.com/item/1005012118979890.html) |
| IO Expander MICROCHIP MCP23017-E/SO | $1.96 | 1pcs | 1 | $1.96 | [https://www.lcsc.com/product-detail/C47023.html](https://www.lcsc.com/product-detail/C47023.html) |
| Sideways Slide Switch MSS-102568-14A-90-D | $1.03 | 1pcs | 1 | $1.03 | [https://www.digikey.com.au/en/products/detail/same-sky-formerly-cui-devices/MSS-102568-14A-90-D/13978973](https://www.digikey.com.au/en/products/detail/same-sky-formerly-cui-devices/MSS-102568-14A-90-D/13978973) |
| 1N4148 Diode | $1.00 | 100pcs | 1 | $1.00 | [https://www.aliexpress.com/item/1005008577093447.html](https://www.aliexpress.com/item/1005008577093447.html) |
| 4.7kΩ Resistor VO CR1/4W-4K7±5%-OT26 | $0.53 | 100pcs | 1 | $0.53 | [https://www.lcsc.com/product-detail/C2896831.html](https://www.lcsc.com/product-detail/C2896831.html) |
| M3 x 3 x 4 Heatset Inserts | $2.77 | 50pcs | 1 | $2.77 | [https://www.aliexpress.com/item/1005010664773135.html](https://www.aliexpress.com/item/1005010664773135.html) |

### Estimated Total Price ($USD) = $26.48 + PCB Cost ($12.60) = $39.08
