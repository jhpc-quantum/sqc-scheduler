.SUFFIXES:	.a .la .ln .o .lo .s .S .c .cc .cpp .i .y .l .pb.h pb.cc .grpc.pb.h .grpc.pb.cc

.c.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.c.lo:
	$(LTCOMPILE_CC) -c $< -o $@

.c.i:
	LC_ALL=C $(CC) -E $(CPPFLAGS) $< | uniq > $@

.cc.o:
	LC_ALL=C $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.cc.lo:
	$(LTCOMPILE_CXX) -c $< -o $@

.cc.i:
	LC_ALL=C $(CXX) -E $(CPPFLAGS) $< | uniq > $@

.cpp.o:
	LC_ALL=C $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.cpp.lo:
	$(LTCOMPILE_CXX) -c $< -o $@

.cpp.i:
	LC_ALL=C $(CXX) -E $(CPPFLAGS) $< | uniq > $@

.s.lo:
	$(LTCOMPILE_CC) -c $<

.s.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $<

.S.lo:
	$(LTCOMPILE_CC) -c $<

.S.o:
	LC_ALL=C $(CC) $(CFLAGS) $(CPPFLAGS) -c $<

.c.s:
	LC_ALL=C $(CC) -S $(CFLAGS) $(CPPFLAGS) -c $< -o $<

.cc.s:
	LC_ALL=C $(CXX) -S $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.cpp.s:
	LC_ALL=C $(CXX) -S $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.y.c:
	$(YACC) -d -o $@ $?

.l.c:
	$(LEX) -o $@ $?

ifdef HAVE_PROTOC_C
%.pb.c:	%.proto
	$(PROTOC_C) --c_out=. -I$(srcdir) $<

%.pb.h:	%.proto
	$(PROTOC_C) --c_out=. -I$(srcdir) $<
endif
ifdef HAVE_PROTOC
%.pb.cc:	%.proto
	protoc --cpp_out=. -I$(srcdir) $<

%.pb.h:	%.proto
	protoc --cpp_out=. -I$(srcdir) $<

%.grpc.pb.cc:	%.proto
	protoc --grpc_out=. -I$(srcdir) \
		--plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<

%.grpc.pb.h:	%.proto
	protoc --grpc_out=. -I$(srcdir) \
		--plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<
endif


all::		ALL
ALL::		$(TARGETS)

ifdef INSTALL_BIN_EXE_TARGETS
install::	install-exe
install-exe::  $(INSTALL_BIN_EXE_TARGETS)
	@if test ! -z "$(INSTALL_BIN_EXE_TARGETS)" -a \
		! -z "$(INSTALL_BIN_EXE_DIR)" ; then \
		$(MKDIR) $(INSTALL_BIN_EXE_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_BIN_EXE_TARGETS) ; do \
			$(LTINSTALL_EXE) $$i $(INSTALL_BIN_EXE_DIR) ; \
		done ; \
	fi
else
install::	install-exe
install-exe::
	@true
endif

ifdef INSTALL_SBIN_EXE_TARGETS
install::	install-sbin-exe
install-sbin-exe::	$(INSTALL_SBIN_EXE_TARGETS)
	@if test ! -z "$(INSTALL_SBIN_EXE_TARGETS)" -a \
		 ! -z "$(INSTALL_SBIN_EXE_DIR)" ; then \
		$(MKDIR) $(INSTALL_SBIN_EXE_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_SBIN_EXE_TARGETS) ; do \
			$(LTINSTALL_EXE) $$i $(INSTALL_SBIN_EXE_DIR) ; \
		done ; \
	fi
else
install::	install-sbin-exe
install-sbin-exe::
	@true
endif

ifdef INSTALL_LIB_TARGETS
install::	install-lib
install-lib::	$(INSTALL_LIB_TARGETS)
	@if test ! -z "$(INSTALL_LIB_TARGETS)" -a \
		 ! -z "$(INSTALL_LIB_DIR)" ; then \
		$(MKDIR) $(INSTALL_LIB_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_LIB_TARGETS) ; do \
			$(LTINSTALL_LIB) $$i $(INSTALL_LIB_DIR) ; \
		done ; \
	fi
else
install::	install-lib
install-lib::
	@true
endif

ifdef INSTALL_HEADER_TARGETS
install::	install-header
install-header::	$(INSTALL_HEADER_TARGETS)
	@if test ! -z "$(INSTALL_HEADER_TARGETS)" -a \
		 ! -z "$(INSTALL_HEADER_DIR)" ; then \
		$(MKDIR) $(INSTALL_HEADER_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_HEADER_TARGETS) ; do \
			if test -f $$i; then \
				$(LTINSTALL_HEADER) $$i $(INSTALL_HEADER_DIR) ; \
			else \
				$(LTINSTALL_HEADER) $(srcdir)/$$i $(INSTALL_HEADER_DIR) ; \
			fi ; \
		done ; \
	fi
else
install::	install-header
install-header::
	@true
endif

ifdef INSTALL_CONFIG_TARGETS
install::	install-config
install-config::	$(INSTALL_CONFIG_TARGETS)
	@if test ! -z "$(INSTALL_CONFIG_TARGETS)" -a \
		 ! -z "$(INSTALL_CONFIG_DIR)" ; then \
		$(MKDIR) $(INSTALL_CONFIG_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_CONFIG_TARGETS) ; do \
			$(INSTALL_DATA) $$i $(INSTALL_CONFIG_DIR) ; \
		done ; \
	fi
else
install::	install-config
install-config::
	@true
endif

ifdef INSTALL_DOC_TARGETS
install::	install-doc
install-doc::	$(INSTALL_DOC_TARGETS)
	@if test ! -z "$(INSTALL_DOC_TARGETS)" -a \
		 ! -z "$(INSTALL_DOC_DIR)" ; then \
		$(MKDIR) $(INSTALL_DOC_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_DOC_TARGETS) ; do \
			$(INSTALL_DATA) $$i $(INSTALL_DOC_DIR) ; \
		done ; \
	fi
else
install::	install-doc
install-doc::
	@true
endif

ifdef INSTALL_EXAMPLE_TARGETS
install::	install-example
install-example::	$(INSTALL_EXAMPLE_TARGETS)
	@if test ! -z "$(INSTALL_EXAMPLE_TARGETS)" -a \
		 ! -z "$(INSTALL_EXAMPLE_DIR)" ; then \
		$(MKDIR) $(INSTALL_EXAMPLE_DIR) > /dev/null 2>&1 ; \
		for i in $(INSTALL_EXAMPLE_TARGETS) ; do \
			$(INSTALL_DATA) $$i $(INSTALL_EXAMPLE_DIR) ; \
		done ; \
	fi
else
install::	install-example
install-example::
	@true
endif

ifdef SRCS
depend::
	@if test ! -z "$(SRCS)"; then \
		> .depend ; \
		for i in $(SRCS); do \
			if test -r $$i ; then \
			    j=$$i ; \
			else \
				j=$(srcdir)/$$i ; \
			fi ; \
			$(CXX) -std=gnu++11 -M $(CPPFLAGS) $$j | \
				sed 's:\.o\::\.lo\::' > .depend ; \
			if test $$? -ne 0 ; then \
				echo depend in `pwd` failed. ; \
			else \
				echo depend in `pwd` succeeded. ; \
			fi ; \
		done ; \
	fi
else
depend::
	@true
endif

ifdef TARGET_LIB_CXX
$(TARGET_LIB_CXX):	$(OBJS)
	$(RM) -f $@ .libs/$(@F:.la=.*)
	$(LTLIB_CXX) -o $@ $(OBJS) $(LDFLAGS) $(DEP_LIBS)
else
$(TARGET_LIB):	$(OBJS)
	$(RM) -f $@ .libs/$(@F:.la=.*)
	$(LTLIB_CC) -o $@ $(OBJS) $(LDFLAGS) $(DEP_LIBS)
endif

ifdef TARGET_EXE_CXX
$(TARGET_EXE_CXX):	$(OBJS)
	$(RM) -f $@ .libs/lt-$@ .libs/$@
	$(LTLINK_CXX) -o $@ $(OBJS) $(DEP_LIBS) $(LDFLAGS) 
else
$(TARGET_EXE):	$(OBJS)
	$(RM) -f $@ .libs/lt-$@ .libs/$@
	$(LTLINK_CC) -o $@ $(OBJS) $(DEP_LIBS) $(LDFLAGS) 
endif

.PHONY:	prerequisite
prerequisite::
	@( \
		$(MAKE) clean && \
		$(MAKE) generate && \
		$(MAKE) symlink && \
		$(MAKE) depend; \
		exit $$?; \
	)

clean::
	$(LTCLEAN) $(OBJS) *.i *~ *.~*~ core core.* *.core $(TARGETS) \
		*.o *.lo *.gcda *.gcno *.gcov
	$(RM) -rf ./html ./scan-result ./clang.mk ./scan-build.mk ./icc.mk \
	./fortify.mk ./fortify.fpr ./fortify.rtf ./fortify.pdf \
	./und.mk ./und.txt ./und.udb ./und_html ./cov \
	*gcov.mk *.gcno *.gcov gcovr_result.xml .libs

distclean::	clean
	$(RM) Makefile .depend

beautify-for-py::
	@find . -type f -name '*.py' -o -name '*.py.in'| \
	egrep -v 'framework/wip-or-deprecate|test/AutomaticVerificationTool' | \
	xargs sh $(MKRULES_SRCDIR)/beautify_for_py

beautify::
	@sh $(MKRULES_SRCDIR)/beautify 'framework/*.c' 'framework/*.cpp' 'framework/*.h'

revert::
	@git status $(srcdir) | grep modified: | awk '{ print $$NF }' | \
	xargs git checkout

Makefiles::
	@if test -x $(top_builddir)/config.status; then \
		(cd $(top_builddir); LC_ALL=C sh ./config.status; \
			$(RM) config.log) ; \
	fi

ifdef LIBTOOL_DEPS
libtool::	$(LIBTOOL_DEPS)
	@if test -x $(top_builddir)/config.status; then \
		(cd $(top_builddir); LC_ALL=C sh ./config.status libtool; \
			$(RM) config.log) ; \
	fi
endif

.PHONY: check-syntax
check-syntax:
	${CC} -Wall -fsyntax-only $(CHK_SOURCES)

doxygen::
	sh $(MKRULES_SRCDIR)/mkfiles.sh
	$(RM) -rf ./html ./latex
	doxygen $(MKRULES_SRCDIR)/doxygen.conf
	$(RM) -rf ./latex ./.files

fortify::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./fortify.mk ./fortify.fpr ; \
		echo 'ifeq ($$(__SITECONF__),.pre.)' > ./fortify.mk ; \
		echo 'ORGCC=$(CC)' >> ./fortify.mk ; \
		echo 'endif' >> ./fortify.mk ; \
		echo 'ifeq ($$(__SITECONF__),.post.)' >> ./fortify.mk ; \
		echo 'CC=sourceanalyzer -scan -f $$(top_srcdir)/fortify.fpr -append $$(ORGCC)' >> ./fortify.mk ; \
		echo 'LINK_CC=$$(ORGCC)' >> ./fortify.mk ; \
		echo 'endif' >> ./fortify.mk ; \
		(SITECONF_MK=`pwd`/fortify.mk $(MAKE)) ; \
		$(RM) -f ./fortify.mk ; \
		if test -f $(srcdir)/fortify.fpr; then \
			ReportGenerator -format rtf -source ./fortify.fpr \
				-f ./fortify.rtf ; \
		fi ; \
	)

scan-build::
	@( \
		$(MAKE) prerequisite > /dev/null 2>&1 ; \
		$(RM) -rf ./scan-build.mk ./scan-result ; \
		scan-build sh -c 'echo CC = $${CC} > ./scan-build.mk' \
			> /dev/null 2>&1 ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/scan-build.mk \
				scan-build -o ./scan-result \
				sh -c "$(MAKE)" ; ) ; \
		fi ; \
		$(RM) -f ./scan-build.mk ; \
	)

scan-build-blame::
	@( \
		$(RM) -f ./m.out ; \
		$(MAKE) scan-build > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -r ./m.out ; then \
			sh $(MKRULES_SRCDIR)/warn-blame.sh ./m.out ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

gcc-full-opt::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./gcc-full-opt.mk ; \
		echo "DEBUG_CFLAGS = -g0" > ./gcc-full-opt.mk ; \
		echo "DEBUG_CXXFLAGS = -g0" >> ./gcc-full-opt.mk ; \
		echo "OPT_CFLAGS = -O6" >> ./gcc-full-opt.mk ; \
		echo "OPT_CXXFLAGS = -O6" >> ./gcc-full-opt.mk ; \
		echo "CODEGEN_CFLAGS = -fno-keep-inline-functions" >> ./gcc-full-opt.mk ; \
		echo "CODEGEN_CXXFLAGS = -fno-keep-inline-functions" >> ./gcc-full-opt.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/gcc-full-opt.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./gcc-full-opt.mk ; \
	)

clang::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./clang.mk ; \
		echo "CC = clang" > ./clang.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/clang.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./clang.mk ; \
	)

icc::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./icc.mk ; \
		echo "CC = icc" > ./icc.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/icc.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./icc.mk ; \
	)

gcov::
	@( \
		echo "CFLAGS += -fprofile-arcs -ftest-coverage" > ./gcov.mk ; \
		echo "LDFLAGS += -fprofile-arcs" >> ./gcov.mk ; \
		if test $$? -eq 0; then \
			(SITECONF_MK=`pwd`/gcov.mk $(MAKE)) ; \
		fi ; \
		$(RM) -f ./gcov.mk ; \
	)

und::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./und.mk ./und.udb ./und.txt und_html ; \
		echo "CC = gccwrapper" > ./und.mk ; \
		if test $$? -eq 0; then \
			(LC_ALL=C SITECONF_MK=`pwd`/und.mk \
				buildspy -db `pwd`/und.udb -cmd make) ; \
		fi ; \
		if test -f ./und.udb; then \
			und -db `pwd`/und.udb analyze ; \
		fi ; \
		if test $$? -eq 0; then \
			und -db `pwd`/und.udb report ; \
		fi ; \
		$(RM) -f ./und.mk ; \
	)

cov::
	@( \
		$(MAKE) prerequisite ; \
		$(RM) -rf ./cov ; \
		cov-build --dir ./cov sh -c "$(MAKE)" > /dev/null 2>&1 && \
		cov-analyze --dir ./cov --all > /dev/null 2>&1 && \
		cov-format-errors --dir ./cov > /dev/null 2>&1 ; \
	)

wc::
	@find . -type f -name '*.c' -o -name '*.cpp' -o -name '*.h' | \
	xargs wc

warn-check::
	@( \
		$(MAKE) prerequisite > /dev/null 2>&1 ; \
		$(MAKE) > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -f ./m.out ; then \
			grep ' warning:' ./m.out | awk -F: '{ print $$1 }' | \
			sort | uniq ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

warn-blame::
	@( \
		$(MAKE) prerequisite > /dev/null 2>&1 ; \
		$(MAKE) > ./m.out 2>&1 ; \
		if test $$? -eq 0 -a -f ./m.out ; then \
			sh $(MKRULES_SRCDIR)/warn-blame.sh ./m.out ; \
		fi ; \
		$(RM) -f ./m.out ; \
	)

dostext::
	sh $(MKRULES_SRCDIR)/doDosText.sh

unixtext::
	sh $(MKRULES_SRCDIR)/doUnixText.sh

check-srcs::
	@echo $(SRCS)

check-objs::
	@echo $(OBJS)

VALGRIND	=	valgrind \
			--leak-check=full \
			--leak-resolution=high \
			--show-leak-kinds=all
HELGRIND	=	valgrind --tool=helgrind

ifdef HAVE_UNITY
HAVE_TEST	?=	$(shell ls -d ./test 2>/dev/null)
ifdef HAVE_TEST
DIRS    +=	$(HAVE_TEST)
endif # HAVE_TEST
endif # HAVE_UNITY

ifdef DIRS
all depend clean distclean check check-nocolor valgrind helgrind install install-exe install-lib install-header install-config install-sbin exe symlink generate ut-run ut-run-val ut-run-hel::
	@for i in / $(DIRS) ; do \
		case $$i in \
			/) continue ;; \
			*) (cd $$i && $(MAKE) $@) || exit 1;; \
		esac ; \
	done
endif

check::
	@true

summary::
	@true

valgrind::
	@true

helgrind::
	@true

generate::
	@true

symlink::
	@true

ut-run::
	@true

ut-run-val::
	@true

ut-run-hel::
	@true

HAVE_DEPEND	?=	$(shell ls ./.depend 2>/dev/null)
ifdef HAVE_DEPEND
include $(HAVE_DEPEND)
endif # HAVE_DEPEND
