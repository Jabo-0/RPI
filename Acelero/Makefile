LIBS= -lpthread -lm  #Libraries used if needed

SRCS= mpu6000.c

BIN= appAce

CFLAGS+= -g -pthread -Wno-implicit-function-declaration -fpermissive

OBJS=$(subst .c,.o,$(SRCS))

all : $(BIN)

$(BIN): $(OBJS)
		@echo [link] $@
		$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
		
%.o: %.c
		@echo [Compile] $<
		$(CXX) -c $(CFLAGS) $< -o $@
		
clean:
		@rm -f $(OBJS) $(BIN)
