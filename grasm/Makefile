LINK.o = $(LINK.cc)
OBJDIR = ./obj
DEPDIR = ./.deps

SRCS	=	main.cc \
				../common/cmdargs.cc \
				xssembler.cc assembler.cc disassembler.cc \
				s2650.cc opcode_2650.cc

FILENAMES := $(notdir $(basename $(SRCS)))
OBJS	  := $(addsuffix .o,$(addprefix $(OBJDIR)/,$(FILENAMES)))
DEPFILES  := $(addsuffix .d,$(addprefix $(DEPDIR)/,$(FILENAMES)))

CXXFLAGS += -g -I..
LDFLAGS += -g

vpath %.cc ../common

COMPILE.c  = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

OUTPUT_OPTION = -o $(OBJDIR)/$*.o

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

%.o : %.cc
$(OBJDIR)/%.o : %.cc | $(DEPDIR) $(OBJDIR)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

grasm:  $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f grasm $(OBJS)

$(OBJDIR):
	@mkdir -p $@

$(DEPFILES): $(DEPDIR)

$(DEPDIR):
	@mkdir -p $@

include $(wildcard $(DEPFILES))
