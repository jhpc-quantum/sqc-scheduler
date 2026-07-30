TARGETS_MK	?=	$(shell ls .ut-targets.mk 2>/dev/null)
ifdef TARGETS_MK
include $(TARGETS_MK)
endif

SRCS_MK		?=	$(shell ls .ut-srcs.mk 2>/dev/null)
ifdef SRCS_MK
include $(SRCS_MK)
endif

CPPFLAGS	+=	$(UT_CPPFLAGS)
CFLAGS		+=	$(UT_CFLAGS)
CXXFLAGS	+=	$(UT_CXXFLAGS)
DEP_LIBS	+=	$(UT_DEP_LIBS)

CFLAGS		+=	-Wno-missing-declarations -Wno-missing-prototypes
CXXFLAGS	+=	-Wno-missing-declarations -Wno-missing-prototypes

ifdef SANITIZE
CFLAGS		+=	-fsanitize=address
CXXFLAGS	+=	-fsanitize=address
endif # SANITIZE

CPPFLAGS	+=	-I. -I$(UNITY_INCDIR)
DEP_LIBS	+=	$(UNITY_LIBS)

ENTRIES_MK		?=	$(shell ls .ut-entries.mk 2>/dev/null)
ifdef ENTRIES_MK
include $(ENTRIES_MK)
endif

include $(MKRULES_BUILDDIR)/rules.mk

generate::
	@EXCLUDE_TESTS="$(EXCLUDE_TESTS)" srcdir="$(srcdir)" $(MKRULES_BUILDDIR)/ut-init.sh

ut-run::
	@for i in $(TARGETS) ; do \
		echo "" && echo $$i: && ./$$i ; \
	done

ut-run-val::
	@for i in $(TARGETS) ; do \
		echo "" && echo $$i: && $(VALGRIND) ./.libs/$$i ; \
	done

ut-run-hel::
	@for i in $(TARGETS) ; do \
		echo "" && echo $$i: && $(HELGRIND) ./.libs/$$i ; \
	done

distclean::
	$(RM) -f .ut-*.mk .depend $(SRCS)
