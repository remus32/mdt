ifeq ($(RELEASE_BUILD)),1)
OPTFLAGS=-g0 -O2
BUILDDIR=build/release
else
OPTFLAGS=-ggdb -O0
BUILDDIR=build/debug
endif

all: $(BUILDDIR)/mdt

REMLIB_PATH=../platc/remlib
REMLIB_A=$(REMLIB_PATH)/remlib.a

$(REMLIB_A):
	make -C $(REMLIB_PATH) remlib.a

PREFIX=/usr/local
MDT_CFLAGS=-pipe -std=c11 -Iinc -I$(REMLIB_PATH)/inc -Wall -Wextra $(OPTFLAGS)
MDT_LDLIBS=-ljack

$(BUILDDIR)/%.o: src/%.c
	@mkdir -pv $(BUILDDIR)
	$(CC) $(MDT_CFLAGS) -MMD -c -o $@ $<

MDT_SRCS=main.c jack.c jack_inits.c jack_callbacks.c jack_r.crder.c wqueue.c ledbuffer.c jack_proccess.c p.ccess.c time_util.c write.c
MDT_OBJS=$(patsubst %.c,$(BUILDDIR)/%.o,$(MDT_SRCS))
MDT_DEPS=$(MDT_OBJS:%.o=%.d)

-include $(MDT_DEPS)

$(BUILDDIR)/mdt: $(MDT_OBJS) $(REMLIB_A)
	$(CC) $(MDT_CFLAGS) $(MDT_LDFLAGS) -o $@ $^ $(MDT_LDLIBS)

clean:
	-rm -rf build

run: $(BUILDDIR)/mdt
	./$<

format:
	clang-format -i src/*.c

.PHONY: clean all run format $(REMLIB_A)
