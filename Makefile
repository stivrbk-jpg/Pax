# ==========================================================
# PAX OMNI-KERNEL BUILD SYSTEM (Makefile)
# ==========================================================

CC = gcc
CFLAGS = -O3 -Wall -Wextra

all:
	$(CC) $(CFLAGS) pax_v7.c -o pax
	@echo "=========================================================="
	@echo "[PAX] Omni-Kernel v7.0 successfully built. Run with: ./pax run <file>"
	@echo "=========================================================="

clean:
	rm -f pax pax_kernel.log
