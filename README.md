# Two-Pass Assembler

אסמבלר דו-מעבר עם מרחיב מאקרו לקורס "מעבדה במערכות".  
מתרגם קבצי assembly (.as) לקוד מכונה בפורמט base-4.

## Quick Start

```bash
# Build
make

# Run
./assembler filename
```

## Features

- ✅ **Pre-assembler** עם מרחיב מאקרו
- ✅ **16 פקודות assembly** (MOV, ADD, JMP, etc.)
- ✅ **4 מצבי מיען** (immediate, direct, matrix, register)
- ✅ **טיפול מקיף בשגיאות**
- ✅ **פלט base-4 מותאם** (.obj, .ext, .ent)

## Technical Specs

| מאפיין | ערך |
|---------|-----|
| זיכרון | 256 תאים × 10 ביט |
| כתובת טעינה | 100 |
| תקן C | ANSI C90 |
| פורמט מספרים | Base-4 (a=0,b=1,c=2,d=3) |

## Usage Example

```assembly
; example.as
MAIN: MOV #5, r1
      ADD r1, #3
      PRN r1
      STOP
```

```bash
./assembler example
# יוצר: example.obj (ואם צריך: example.ext, example.ent)
```

## Project Structure

```
├── assembler.c           # Main entry point
├── preassembler.c        # Macro expansion
├── first_pass.c          # Symbol table building
├── symbol_table.c        # Symbol management
├── instruction_*.c       # Instruction handling
├── line_*.c              # Line parsing
├── error_handling.c      # Error management
└── Makefile              # Build script
```

## Error Handling

הפרויקט מזהה ומדווח על:
- שגיאות מאקרו (הגדרה כפולה, שימוש לפני הגדרה)
- פקודות לא תקינות
- אופרנדים מחוץ לטווח
- תוויות לא מוגדרות
- חריגה מגבולות זיכרון

## Build Requirements

- GCC עם תמיכה ב-ANSI C90
- Make utility
- קימפול נקי עם `-Wall -ansi -pedantic`

---

**Course:** Systems Laboratory • **Year:** 2025 • **Author:** Rachely Glb