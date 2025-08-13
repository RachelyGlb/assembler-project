# ==== Unified Makefile for the Assembler project ====

# Compiler & Flags
CC      := gcc
CFLAGS  := -ansi -pedantic -Wall -g
# יצירת תלויות אוטומטיות לקבצי header (נתמך ב-gcc 5 שב-Ubuntu 16.04)
CFLAGS  += -MMD -MP

# Output binary name
TARGET  := assembler

# ---- Group your sources here ----
# הקובץ הראשי (יש בו main)
DRIVER      := assembler.c

# מודול ה-preassembler (אובייקטים בלבד)
SOURCES_PRE := preassembler.c file_utils.c line_parser.c \
               macro_and_label_func.c word_extractor.c error_handling.c

# מודול ה-first pass
SOURCES_FIRST := first_pass.c line_analysis.c symbol_table.c \
                 instruction_table.c instruction_validation.c

# אפשר להוסיף כאן מודולים נוספים בהמשך...

# ---- Combine them all ----
SOURCES := $(DRIVER) $(SOURCES_PRE) $(SOURCES_FIRST)
OBJECTS := $(SOURCES:.c=.o)
DEPS    := $(OBJECTS:.o=.d)

# ---- Default target ----
all: $(TARGET)

# ---- Link final binary ----
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)
	@echo "Build completed: ./$@"

# ---- Compile rule (with auto deps) ----
# אם יש לך headers ספציפיים שחייבים לכל קובץ, לא צריך לציין כאן – ה-MMD דואג לזה
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Convenience targets ----
# הרצה עם פרמטרים: make run ARGS="prog1 prog2"
run: $(TARGET)
	@./$(TARGET) $(ARGS)

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)
	@echo "Cleaned: objects, deps, binary"

rebuild: clean all

# אם תרצי לראות מה make עומד להריץ:
dry:
	@$(MAKE) -n all

# כללי עזר
.PHONY: all run clean rebuild dry

# כלול קבצי תלויות (אם קיימים) כדי ש-rebuild יקרה כש-headers משתנים
-include $(DEPS)
