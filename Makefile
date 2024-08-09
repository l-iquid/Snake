COMPILER = gcc
FILE_EXTENSION = .c
OBJS = main.o buffer.o head.o lex.o log.o parse.o
EXEC_NAME = pya
CFLAGS =

.PHONY:
all: $(EXEC_NAME)

.PHONY:
clean:
	@echo CLEANING *.o
	@rm *.o $(EXEC_NAME)


$(EXEC_NAME): $(OBJS)
	@echo TARGET '$(EXEC_NAME)' CREATING BINARIES FROM
	@echo '$(OBJS)'...
	@echo FLAGS [$(CFLAGS)]
	@$(COMPILER) $(CFLAGS) $(OBJS) -o $(EXEC_NAME)

%.o: %$(FILE_EXTENSION)
	@$(COMPILER) $(CFLAGS) -c $< -o $@
