LINK.o = $(LINK.cc)
OBJDIR = ./obj
DEPDIR = ./.deps

SRCS	=	assembler.cc \
        s2650.cc opcode_2650.cc

FILENAMES	:= $(notdir $(basename $(SRCS)))
OBJS			:= $(addsuffix .o,$(addprefix $(OBJDIR)/,$(FILENAMES)))
DEPFILES  := $(addsuffix .d,$(addprefix $(DEPDIR)/,$(FILENAMES)))

COMPILE.c  = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

OUTPUT_OPTION = -o $(OBJDIR)/$*.o

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

%.o : %.cc
$(OBJDIR)/%.o : %.cc | $(DEPDIR) $(OBJDIR)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

asm8bit:  $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm asm2650

$(OBJDIR):
	@mkdir -p $@

$(DEPFILES): $(DEPDIR)

$(DEPDIR):
	@mkdir -p $@

include $(wildcard $(DEPFILES))
