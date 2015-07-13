include nall/Makefile

qtlibs := QtCore QtGui
include nall/qt/Makefile

c     := $(compiler) -std=gnu99
cpp   := $(subst cc,++,$(compiler)) -std=gnu++0x
flags := -O3 -I. -Iobj -fomit-frame-pointer
link  := -s

ifeq ($(platform),win)
  link += -mwindows -mthreads
  link += -luuid -lkernel32 -luser32 -lgdi32 -lshell32
  link += -enable-stdcall-fixup -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc
endif

objects := upset

compile = \
  $(strip \
    $(if $(filter %.c,$<), \
      $(c) $(flags) $1 -c $< -o $@, \
      $(if $(filter %.cpp,$<), \
        $(cpp) $(flags) $1 -c $< -o $@ \
      ) \
    ) \
  )

%.o: $<; $(call compile)

all: build;

objects := $(patsubst %,obj/%.o,$(objects))
moc_headers := $(call rwildcard,./,%.moc.hpp)
moc_objects := $(foreach f,$(moc_headers),obj/$(notdir $(patsubst %.moc.hpp,%.moc,$f)))

# automatically run moc on all .moc.hpp (MOC header) files
%.moc: $<; $(moc) -i $< -o $@

# automatically generate %.moc build rules
__list = $(moc_headers)
$(foreach f,$(moc_objects), \
  $(eval __file = $(word 1,$(__list))) \
  $(eval __list = $(wordlist 2,$(words $(__list)),$(__list))) \
  $(eval $f: $(__file)) \
)

#############
### upset ###
#############

build: $(moc_objects) $(objects)
	$(strip $(cpp) -o upset $(objects) $(link) $(qtlib))

install:
	install -D -m 755 ./upset $(DESTDIR)$(prefix)/bin/upset
	install -D -m 644 ./upset.desktop $(DESTDIR)$(prefix)/share/applications/upset.desktop

uninstall:
	rm $(DESTDIR)$(prefix)/bin/upset
	rm $(DESTDIR)$(prefix)/share/applications/upset.desktop

obj/upset.o: upset.cpp *.cpp *.hpp
	$(call compile,$(qtinc))

clean:
	-@$(call delete,obj/*.o)
	-@$(call delete,obj/*.moc)
