NAME=bladebomber
COMP=xz
RIVEMU_RUN=rivemu
RIVEMU_EXEC=rivemu -quiet -no-window -sdk -workspace -exec
ifneq (,$(wildcard /usr/sbin/riv-run))
	RIVEMU_RUN=riv-run
	RIVEMU_EXEC=
endif
LIBRIV_PATH=libriv
CFLAGS+=-Ilibriv -Llibriv
ifeq ($(CROSS),y)
	CFLAGS+=-Ilibriv -Llibriv -lriv
	CFLAGS+=-Wall -Wextra
	CFLAGS+=-march=rv64g -Og -g -rdynamic -fno-omit-frame-pointer -fno-strict-overflow -fno-strict-aliasing -std=c11
	CFLAGS+=-Wl,--build-id=none,--sort-common
	CC=riscv64-buildroot-linux-musl-gcc
else
	CC=$(RIVEMU_EXEC) gcc
	CFLAGS=$(shell $(RIVEMU_EXEC) riv-opt-flags -Ospeed)
endif

build: $(NAME).sqfs

run: $(NAME).sqfs
	$(RIVEMU_RUN) $<

lint: $(NAME).c
	clang-tidy $< -- -std=c11 -Ilibriv -D_CRT_SECURE_NO_WARNINGS
	gcc -std=c11 -Wall -Wextra -fanalyzer -fsyntax-only -Ilibriv $<

dev-run: $(NAME).elf
	$(RIVEMU_RUN) -no-loading -bench -workspace -exec ./$<

jit-run: $(NAME).c
	$(RIVEMU_RUN) -no-loading -bench -workspace -exec riv-jit-c ./$<

live-dev:
	luamon -e c,h,Makefile -l make 'CROSS=y lint dev-run -j2'

clean:
	rm -rf *.sqfs *.elf

distclean: clean
	rm -rf libriv

$(NAME).sqfs: $(NAME).elf *.png info.json
	$(RIVEMU_EXEC) riv-mksqfs $^ $@ -comp $(COMP)

$(NAME).elf: $(NAME).c *.h libriv
	$(CC) $< -o $@ $(CFLAGS)

libriv:
	mkdir -p libriv
	$(RIVEMU_EXEC) cp /usr/include/riv.h libriv/
	$(RIVEMU_EXEC) cp /usr/lib/libriv.so libriv/
	$(RIVEMU_EXEC) cp /lib/libc.musl-riscv64.so.1 libriv/
