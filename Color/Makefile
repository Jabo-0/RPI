LIBS= -lpthread -lm  #Libraries used if needed

SRCS= TCS34725.c

BIN= appColor

CFLAGS+= -g -pthread -O0 -Wno-implicit-function-declaration -fpermissive

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
