LIBS= -lpthread -lm  #Libraries used if needed

SRCS= client.c

BIN= appClient

CFLAGS+= -g -O0 -Wno-implicit-function-declaration -fpermissive

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
