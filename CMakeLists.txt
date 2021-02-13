cmake_minimum_required(VERSION 3.12)
project(gnupg
		VERSION 2.2.27
		LANGUAGES C)

add_compile_definitions(HAVE_CONFIG_H)

add_compile_definitions(LOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")
if(UNIX)
add_compile_definitions(
	GNUPG_BINDIR="${CMAKE_INSTALL_PREFIX}/bin"
	GNUPG_LIBEXECDIR="${CMAKE_INSTALL_PREFIX}/libexec"
	GNUPG_LIBDIR="${CMAKE_INSTALL_PREFIX}/lib/gnupg"
	GNUPG_DATADIR="${CMAKE_INSTALL_PREFIX}/share/gnupg"
	GNUPG_SYSCONFDIR="${CMAKE_INSTALL_PREFIX}/etc/gnupg"
	GNUPG_LOCALSTATEDIR="${CMAKE_INSTALL_PREFIX}/var")
endif()

set(PACKAGE \"${PROJECT_NAME}\")
set(PACKAGE_BUGREPORT \"https://bugs.gnupg.org\")
set(PACKAGE_NAME \"${PROJECT_NAME}\")
set(PACKAGE_GT \"${PROJECT_NAME}\")
set(PACKAGE_STRING \"${PROJECT_NAME}\ ${PROJECT_VERSION}\")
set(PACKAGE_TARNAME \"${PROJECT_NAME}\")
set(PACKAGE_URL \"\")
set(PACKAGE_VERSION \"${PROJECT_VERSION}\")
set(VERSION \"${PROJECT_VERSION}\")

include(UtilityFunctions)
include(CheckCSourceCompiles)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(visibility)
include(LargeFiles)
include(fork)
include(inline)
include(TimeWithSysTime)
include(CTest)

#find_package(GPGRT REQUIRED)
#find_package(ASSUAN REQUIRED)
#find_package(KSBA REQUIRED)
#find_package(NPTH REQUIRED)
#find_package(GCRYPT REQUIRED)
set(NEED_LIBGCRYPT_VERSION \"1.9.1\")
set(NEED_KSBA_VERSION \"1.5.0\")

find_program(GAWK_EXE gawk REQUIRED)
file(TO_NATIVE_PATH ${GAWK_EXE} GAWK_EXE)

find_package(Iconv)
find_package(Intl)
find_package(BZip2)
find_package(ZLIB)

set(BUILD_REVISION \"\")
set(BUILD_TIMESTAMP \"\")
set(EXEEXT \"${CMAKE_EXECUTABLE_SUFFIX}\")

set(AGENT_S2K_CALIBRATION 100)
set(BUILD_WITH_AGENT 1)
set(BUILD_WITH_DIRMNGR 1)
set(BUILD_WITH_G13 1)
set(BUILD_WITH_GPG 1)
set(BUILD_WITH_GPGSM 1)
set(BUILD_WITH_SCDAEMON 1)
set(ENABLE_CARD_SUPPORT 1)
#set(DEFAULT_TRUST_STORE_FILE) #figure out what it does
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC" AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
	set(USE_LIBDNS 0)
else()
	set(USE_LIBDNS 1)
endif()
set(USE_TOFU 0)

set(DIRMNGR_DISP_NAME \"DirMngr\")
set(DIRMNGR_DEFAULT_KEYSERVER \"hkps://hkps.pool.sks-keyservers.net\")
set(DIRMNGR_INFO_NAME \"DIRMNGR_INFO\")
set(DIRMNGR_NAME \"dirmngr\")
set(DIRMNGR_SOCK_NAME \"S.dirmngr\")
set(G13_DISP_NAME \"G13\")
set(G13_NAME \"g13\")
set(GNUPG_NAME \"GnuPG\")
set(GPG_DISP_NAME \"GnuPG\")
set(GNUPG_SWDB_TAG \"gnupg22\")
set(GPGCONF_DISP_NAME \"GPGConf\")
set(GPGCONF_NAME \"gpgconf\")
set(GPGEXT_GPG \"gpg\")
set(GPG_NAME \"gpg\")
set(GPGSM_DISP_NAME \"GPGSM\")
set(GPGSM_NAME \"gpgsm\")
set(GPGTAR_NAME \"gpgtar\")
set(GPG_AGENT_DISP_NAME \"GPG\ Agent\")
set(GPG_AGENT_NAME \"gpg-agent\")
set(GPG_AGENT_BROWSER_SOCK_NAME \"S.gpg-agent.browser\")
set(GPG_AGENT_EXTRA_SOCK_NAME \"S.gpg-agent.extra\")
set(GPG_AGENT_SOCK_NAME \"S.gpg-agent\")
set(GPG_AGENT_SSH_SOCK_NAME \"S.gpg-agent.ssh\")
set(SCDAEMON_DISP_NAME \"SCDaemon\")
set(SCDAEMON_NAME \"scdaemon\")
set(SCDAEMON_SOCK_NAME \"S.scdaemon\")

set(GPG_USE_AES128 1)
set(GPG_USE_AES192 1)
set(GPG_USE_AES256 1)
set(GPG_USE_BLOWFISH 1)
set(GPG_USE_CAMELLIA128 1)
set(GPG_USE_CAMELLIA192 1)
set(GPG_USE_CAMELLIA256 1)
set(GPG_USE_CAST5 1)
set(GPG_USE_ECDH 1)
set(GPG_USE_ECDSA 1)
set(GPG_USE_EDDSA 1)
set(GPG_USE_IDEA 1)
set(GPG_USE_MD5 1)
set(GPG_USE_RMD160 1)
set(GPG_USE_RSA 1)
set(GPG_USE_SHA224 1)
set(GPG_USE_SHA384 1)
set(GPG_USE_SHA512 1)
set(GPG_USE_TWOFISH 1)

if(Iconv_FOUND)
	set(HAVE_ICONV 1)
endif()

if(Intl_FOUND)
	set(ENABLE_NLS 1)
	set(HAVE_GETTEXT 1)
	set(HAVE_DCGETTEXT 1)
endif()

if(BZip2_FOUND)
	set(HAVE_BZIP2 1)
endif()

if(ZLIB_FOUND)
	set(HAVE_ZIP 1)
endif()

if(WIN32)
	set(HAVE_VISIBILITY 0)
	set(TIME_WITH_SYS_TIME 1)
	set(HAVE_W32_SYSTEM 1)
	set(HAVE_W64_SYSTEM 1)
	set(MKDIR_TAKES_ONE_ARG 1)
	set(HAVE_DRIVE_LETTERS 1)
	set(HAVE_DOSISH_SYSTEM 1)
	set(HAVE_ISASCII 1)
	set(HAVE_FSEEKO 1)
	set(HAVE_FTELLO 1)
	set(HAVE_STRCASECMP 1)
	set(HAVE_STRNCASECMP 1)
	set(LITTLE_ENDIAN_HOST 1)
	set(USE_LDAP 1)
	set(USE_LDAPWRAPPER 1)
	if(NOT Intl_FOUND)
		set(USE_SIMPLE_GETTEXT 1)
	endif()
endif()

if(WIN32)
	set(USE_WINDOWS_THREADS 1)
	set(HOST_TRIPLET_STRING "x86_64-windows")
	set(ENCFS \"encfs.exe\")
	set(FUSERMOUNT \"fusermount.exe\")
	set(GNUPG_REGISTRY_DIR \"Software\\\\GNU\\\\GnuPG\")
	set(PRINTABLE_OS_NAME \"Windows\ 10\")
else()
	set(USE_POSIX_THREADS 1)
	set(HOST_TRIPLET_STRING "x86_64-pc-linux-gnu")
	set(ENCFS \"/usr/bin/encfs\")
	set(FUSERMOUNT \"/usr/bin/fusermount\")
	set(PRINTABLE_OS_NAME \"GNU/Linux\")
	set(HAVE_WORKING_FORK 1)
endif()

set(NAME_OF_SENDMAIL \"\")
set(PK_UID_CACHE_SIZE 4096)
set(SECMEM_BUFFER_SIZE 32768)
set(USE_DIRMNGR_AUTO_START 1)
set(HAVE_NPTH 1)
set(USE_NPTH 1)

check_c_headers(direct.h getopt.h inttypes.h langinfo.h langinfo.h locale.h libusb.h libutil.h locale.h
memory.h pty.h pwd.h signal.h stdint.h stdlib.h strings.h string.h sys/mkdev.h sys/select.h sys/socket.h
sys/stat.h sys/sysmacros.h sys/types.h sys/ucred.h termios.h termio.h ucred.h unistd.h util.h utmp.h
vfork.h winsock2.h ws2tcpip.h
sys/time.h time.h sys/types.h sys/types.h sys/types.h)

if(HAVE_LIBUTIL_H)
	set(HAVE_LIBUTIL 1)
endif()

if(HAVE_LIBUSB_H)
	set(HAVE_LIBUSB 1)
endif()

if(WIN32)
	set(CMAKE_EXTRA_INCLUDE_FILES "winsock2.h;ws2tcpip.h;winldap.h")
	set(CMAKE_REQUIRED_LIBRARIES ws2_32 Wldap32)
endif()
check_functions(atexit canonicalize_file_name clock_gettime ctermid explicit_bzero fcntl flockfile
fork fsync ftello ftruncate funlockfile getaddrinfo getenv getpagesize getpeerucred getpwnam
getpwuid getrlimit getrusage gettimeofday gmtime_r inet_ntop inet_pton inotify_init
isascii ldap_get_option ldap_set_option ldap_start_tls_s ldap_start_tls_sA lstat
memicmp memmove memrchr mmap nanosleep nl_langinfo pipe raise rand setenv setlocale
setrlimit sigaction sigprocmask stat stpcpy strcasecmp strchr strerror strftime
stricmp strlwr strncasecmp strpbrk strsep strtol strtoul strtoull tcgetattr
timegm times ttyname unsetenv vfork vprintf wait4 waitpid usleep)

unset(CMAKE_EXTRA_INCLUDE_FILES)
unset(CMAKE_REQUIRED_LIBRARIES)

if(NOT HAVE_VPRINTF)
	check_function_exists(_doprnt HAVE_DOPRNT)
endif()

set(CMAKE_EXTRA_INCLUDE_FILES signal.h)
check_functions_declarations(getpagesize sys_siglist)
unset(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES time.h)
check_types(time_t "unsigned int" "unsigned long" "unsigned long long" "unsigned short"
mode_t pid_t size_t)
unset(CMAKE_EXTRA_INCLUDE_FILES)

if(WIN32)
set(CMAKE_EXTRA_INCLUDE_FILES "winsock2.h;ws2tcpip.h")
else()
set(CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
endif()
check_types(socklen_t)
unset(CMAKE_EXTRA_INCLUDE_FILES)

if(NOT HAVE_MODE_T)
	set(mode_t int)
endif()

if(NOT HAVE_PID_T)
	set(pid_t int)
endif()

if(NOT HAVE_SIZE_T)
	set(size_t "unsigned\ int")
endif()

if(NOT HAVE_SOCKLEN_T)
	set(socklen_t "unsigned\ int")
endif()

check_library_exists(lber ber_free "" HAVE_LBER)
check_library_exists(readline rl_completion_matches "" HAVE_LIBREADLINE)

set(CMAKE_EXTRA_INCLUDE_FILES signal.h)
check_type_size("struct sigaction" SIGACTION)
if(HAVE_SIGACTION)
	set(HAVE_STRUCT_SIGACTION 1)
endif()
unset(CMAKE_EXTRA_INCLUDE_FILES)

check_struct_has_member("struct sockpeercred" "pid" "sys/types.h;sys/socket.h" HAVE_STRUCT_SOCKPEERCRED_PID)
check_struct_has_member("struct ucred" "cr_pid" "sys/types.h;sys/socket.h" HAVE_STRUCT_UCRED_CR_PID)
check_struct_has_member("struct ucred" "pid" "sys/types.h;sys/socket.h" HAVE_STRUCT_UCRED_PID)



check_c_source_compiles("
#include <locale.h>
int main ()
{
	return LC_MESSAGES;
}"
HAVE_LC_MESSAGES)

set(CMAKE_REQUIRED_LIBRARIES resolv)
check_c_source_compiles("
#define BIND_8_COMPAT
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
int
main ()
{
unsigned char answer[PACKETSZ];
  res_query(\"foo.bar\",C_IN,T_A,answer,PACKETSZ);
  dn_skipname(0,0); dn_expand(0,0,0,0,0);

  ;
  return 0;
}"
HAVE_SYSTEM_RESOLVER)
unset(CMAKE_REQUIRED_LIBRARIES)

check_c_source_compiles("
#if ${TIME_WITH_SYS_TIME}
# include <sys/time.h>
# include <time.h>
#else
# if ${HAVE_SYS_TIME_H}
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

int
main ()
{
static int test_array [1 - 2 * !(((time_t)-1) < 0)];
test_array [0] = 0;
return test_array [0];

  ;
  return 0;
}"
HAVE_UNSIGNED_TIME_T)

check_c_source_compiles("
#include <sys/types.h>
#include <signal.h>

int
main ()
{
return *(signal (0, 0)) (0) == 1;
  ;
  return 0;
}"
RETSIGTYPE)
if(RETSIGTYPE)
	set(RETSIGTYPE int)
else()
	set(RETSIGTYPE void)
endif()

check_c_source_compiles("
int main ()
{

volatile int x;
int * volatile y = (int *) 0;
return !x && !y;
  ;
  return 0;
}"
VOLATILE_WORKS)
if(NOT VOLATILE_WORKS)
	set(volatile /**/)
endif()

foreach(t byte u16 u32 ulong ushort)
string(TOUPPER ${t} ut)
	check_c_source_compiles("
		#define _GNU_SOURCE 1
		#include <stdlib.h>
		#include <sys/types.h>
		int
		main ()
		{
			#undef ${t}
			int a = sizeof(${t});
			return 0;
		}"
	HAVE_${ut}_TYPEDEF)
endforeach()


configure_file(gnupg-config.h.in config.h)
if(WIN32)
	if(NOT HAVE_DIRENT_H)
		configure_file(dirent.h.in dirent.h)
	endif()
	if(NOT HAVE_SYS_TIME_H)
		configure_file(sys_time.h.in sys/time.h)
	endif()
	if(NOT HAVE_STRINGS_H)
		configure_file(strings.h.in strings.h)
	endif()
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

#############
## common ###
#############

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/audit-events.h
		COMMAND ${GAWK_EXE} -f ${CMAKE_CURRENT_SOURCE_DIR}/common/exaudit.awk ${CMAKE_CURRENT_SOURCE_DIR}/common/audit.h
		| ${GAWK_EXE} -f ${CMAKE_CURRENT_SOURCE_DIR}/common/mkstrtable.awk -v textidx=3 -v nogettext=1 -v pkg_namespace=eventstr_
		> ${CMAKE_CURRENT_BINARY_DIR}/audit-events.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/common/exaudit.awk
		${CMAKE_CURRENT_SOURCE_DIR}/common/audit.h ${CMAKE_CURRENT_SOURCE_DIR}/common/mkstrtable.awk)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/status-codes.h
		COMMAND ${GAWK_EXE} -f ${CMAKE_CURRENT_SOURCE_DIR}/common/exstatus.awk ${CMAKE_CURRENT_SOURCE_DIR}/common/status.h
		| ${GAWK_EXE} -f ${CMAKE_CURRENT_SOURCE_DIR}/common/mkstrtable.awk -v textidx=3 -v nogettext=1 -v pkg_namespace=statusstr_
		> ${CMAKE_CURRENT_BINARY_DIR}/status-codes.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/common/exstatus.awk
		${CMAKE_CURRENT_SOURCE_DIR}/common/status.h ${CMAKE_CURRENT_SOURCE_DIR}/common/mkstrtable.awk)

add_custom_target(gen_header_gnupg DEPENDS
${CMAKE_CURRENT_BINARY_DIR}/audit-events.h ${CMAKE_CURRENT_BINARY_DIR}/status-codes.h)

set(common_SOURCES
common-defs.h
util.h utilproto.h fwddecl.h i18n.c i18n.h
types.h host2net.h dynload.h w32help.h
mapstrings.c stringhelp.c stringhelp.h
strlist.c strlist.h
utf8conv.c utf8conv.h
argparse.c argparse.h
logging.c logging.h
dotlock.c dotlock.h
mischelp.c mischelp.h
status.c status.h
shareddefs.h
openpgpdefs.h
gc-opt-flags.h
keyserver.h
sexp-parse.h
tlv.c tlv.h
init.c init.h
sexputil.c
sysutils.c sysutils.h
homedir.c
gettime.c gettime.h
yesno.c
b64enc.c b64dec.c zb32.c zb32.h
convert.c
percent.c
mbox-util.c mbox-util.h
miscellaneous.c
xasprintf.c
xreadline.c
membuf.c membuf.h
ccparray.c ccparray.h
iobuf.c iobuf.h
ttyio.c ttyio.h
asshelp.c asshelp2.c asshelp.h
exechelp.h
signal.c
audit.c audit.h
localename.c
session-env.c session-env.h
userids.c userids.h
openpgp-oid.c
ssh-utils.c ssh-utils.h
agent-opt.c
helpfile.c
mkdir_p.c mkdir_p.h
strlist.c strlist.h
exectool.c exectool.h
server-help.c server-help.h
name-value.c name-value.h
recsel.c recsel.h
ksba-io-support.c ksba-io-support.h
compliance.c compliance.h)

if(WIN32)
	list(APPEND common_SOURCES w32-reg.c exechelp-w32.c)
	if(NOT HAVE_DIRENT_H)
		list(APPEND common_SOURCES dirent-w32.c)
	endif()
	if(NOT HAVE_GETTIMEOFDAY)
		list(APPEND common_SOURCES gettimeofday-w32.c)
	endif()
	if(NOT HAVE_USLEEP)
		list(APPEND common_SOURCES sleep-w32.c)
	endif()
elseif(UNIX)
	list(APPEND common_SOURCES exechelp-posix.c)
endif()

list(TRANSFORM common_SOURCES PREPEND "common/")

set(without_npth_sources common/get-passphrase.c common/get-passphrase.h)
set(with_npth_sources common/call-gpg.c common/call-gpg.h)

add_library(common STATIC ${common_SOURCES} ${without_npth_sources})
target_compile_definitions(common PRIVATE WITHOUT_NPTH=1)

add_library(commonpth STATIC ${common_SOURCES} ${with_npth_sources})

add_dependencies(common gen_header_gnupg)
add_dependencies(commonpth gen_header_gnupg)

set(required_libs GPGRT::GPGRT ASSUAN::ASSUAN KSBA::KSBA GCRYPT::GCRYPT)
if(Iconv_FOUND)
	list(APPEND required_libs Iconv::Iconv)
endif()
if(Intl_FOUND)
	list(APPEND required_libs ${Intl_LIBRARIES})
endif()
if(BZip2_FOUND)
	list(APPEND required_libs BZip2::BZip2)
endif()
if(ZLIB_FOUND)
	list(APPEND required_libs ZLIB::ZLIB)
endif()
if(WIN32)
	list(APPEND required_libs ws2_32 iphlpapi Wldap32)
endif()

target_link_libraries(common ${required_libs})
target_link_libraries(commonpth ${required_libs} NPTH::NPTH)
if(WIN32)
	if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
		target_link_options(common PUBLIC -Wl,invalidcontinue.obj)
		target_link_options(commonpth PUBLIC -Wl,invalidcontinue.obj)
	elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
		target_link_options(common PUBLIC /IGNORE:4217 /IGNORE:4049 /NOLOGO /SUBSYSTEM:CONSOLE invalidcontinue.obj)
		target_link_options(commonpth PUBLIC /IGNORE:4217 /IGNORE:4049 /NOLOGO /SUBSYSTEM:CONSOLE invalidcontinue.obj)
	endif()
endif()

set(pquery_SOURCES simple-pwquery.c simple-pwquery.h asshelp.c asshelp.h)
list(TRANSFORM pquery_SOURCES PREPEND "common/")
add_library(pquery STATIC ${pquery_SOURCES})
target_link_libraries(pquery ASSUAN::ASSUAN Iconv::Iconv ${Intl_LIBRARIES})

add_library(gpgrl STATIC common/gpgrlhelp.c)
target_link_libraries(gpgrl GCRYPT::GCRYPT)

if(BUILD_TESTING)

set(common_tests
t-stringhelp t-timestuff
t-convert t-percent t-gettime t-sysutils t-sexputil
t-session-env t-openpgp-oid t-ssh-utils
t-mapstrings t-zb32 t-mbox-util t-iobuf t-strlist
t-name-value t-ccparray t-recsel t-exechelp t-exectool)

#maintainer tests -> t-b64 t-helpfile

if(WIN32)
	list(APPEND common_tests t-w32-reg)
endif()

foreach(t ${common_tests})
	add_executable(${t} common/${t}.c)
	target_link_libraries(${t} common)
	set_target_properties(${t} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/common)
	add_test(NAME ${t} COMMAND ${t})
	if(WIN32)
		set_tests_properties(${t} PROPERTIES ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}\;${NEW_PATH}")
	endif()
endforeach()

if(WIN32)
	# fails due to dummy function close_all_fds
	# Not to worry, this functions isn't used anywhere else
	set_tests_properties(t-exechelp PROPERTIES WILL_FAIL TRUE) 
	set_tests_properties(t-session-env PROPERTIES ENVIRONMENT "HOME=$ENV{USERPROFILE};PATH=${CMAKE_BINARY_DIR}\;${NEW_PATH}")
endif()

endif()

#############
## regexp ###
#############
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_unicode_mapping.c
		COMMAND ${GAWK_EXE} -n -f ${CMAKE_CURRENT_SOURCE_DIR}/regexp/parse-unidata.awk ${CMAKE_CURRENT_SOURCE_DIR}/regexp/UnicodeData.txt
		> ${CMAKE_CURRENT_BINARY_DIR}/_unicode_mapping.c
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/regexp/parse-unidata.awk ${CMAKE_CURRENT_SOURCE_DIR}/regexp/UnicodeData.txt)

add_custom_target(gen_unicode_mapping DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_unicode_mapping.c)

set(regexp_SOURCES
jimregexp.h utf8.h jimregexp.c utf8.c)

list(TRANSFORM regexp_SOURCES PREPEND "regexp/")

add_library(regexp STATIC ${regexp_SOURCES})
target_compile_definitions(regexp PRIVATE JIM_REGEXP JIM_UTF8 USE_UTF8)
add_dependencies(regexp gen_unicode_mapping)

#############
## kbx ######
#############

set(keybox_SOURCES
keybox.h keybox-defs.h keybox-search-desc.h
keybox-util.c
keybox-init.c
keybox-blob.c
keybox-file.c
keybox-search.c
keybox-update.c
keybox-openpgp.c
keybox-dump.c)

list(TRANSFORM keybox_SOURCES PREPEND "kbx/")

add_library(keybox STATIC ${keybox_SOURCES})
add_library(keybox509 STATIC ${keybox_SOURCES})
target_compile_definitions(keybox509 PUBLIC KEYBOX_WITH_X509=1)

target_link_libraries(keybox GPGRT::GPGRT)
target_link_libraries(keybox509 GPGRT::GPGRT)

add_executable(kbxutil kbx/kbxutil.c)
target_link_libraries(kbxutil keybox509 common)
set_target_properties(kbxutil PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/kbx)

#############
## g10 ######
#############

set(gpg_lib_SOURCES
gpg.h
dek.h
build-packet.c
compress.c
filter.h
free-packet.c
getkey.c
keydb.c keydb.h
keyring.c keyring.h
seskey.c
kbnode.c
main.h
mainproc.c
armor.c
mdfilter.c
textfilter.c
progress.c
misc.c
rmd160.c rmd160.h
options.h
openfile.c
keyid.c
packet.h
parse-packet.c
cpr.c
plaintext.c
sig-check.c
keylist.c
pkglue.c pkglue.h
ecdh.c
server.c
pkclist.c
skclist.c
pubkey-enc.c
passphrase.c
decrypt.c
decrypt-data.c
cipher.c
encrypt.c
sign.c
verify.c
revoke.c
dearmor.c
import.c
export.c
migrate.c
delkey.c
keygen.c
helptext.c
keyserver.c
keyserver-internal.h
call-dirmngr.c call-dirmngr.h
photoid.c photoid.h
call-agent.c call-agent.h
trust.c exec.c exec.h
key-clean.c key-clean.h
key-check.c key-check.h
#trust
trustdb.c trustdb.h tdbdump.c tdbio.c tdbio.h)

if(BZip2_FOUND)
	list(APPEND gpg_lib_SOURCES compress-bz2.c)
endif()

if(USE_TOFU)
	list(APPEND gpg_lib_SOURCES tofu.h tofu.c gpgsql.c gpgsql.h)
endif()

if(ENABLE_CARD_SUPPORT)
	list(APPEND gpg_lib_SOURCES card-util.c)
endif()

list(TRANSFORM gpg_lib_SOURCES PREPEND "g10/")

add_library(gpg_lib STATIC ${gpg_lib_SOURCES})
target_link_libraries(gpg_lib common gpgrl keybox regexp)

set(gpg_SOURCES
gpg.c keyedit.c keyedit.h)
list(TRANSFORM gpg_SOURCES PREPEND "g10/")

add_executable(gpg ${gpg_SOURCES})
target_link_libraries(gpg gpg_lib)

add_executable(gpgcompose g10/gpgcompose.c)
target_link_libraries(gpgcompose gpg_lib)

add_executable(gpgv g10/gpgv.c g10/verify.c)
target_link_libraries(gpgv gpg_lib)

set_target_properties(gpg gpgcompose gpgv PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/g10)

if(BUILD_TESTING)

set(g10_tests
t-rmd160 t-keydb t-keydb-get-keyblock t-stutter)

foreach(t ${g10_tests})
	add_executable(${t} g10/${t}.c g10/test-stubs.c)
	target_link_libraries(${t} gpg_lib)
	set_target_properties(${t} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/g10)
	add_test(NAME ${t} COMMAND ${t})
	if(WIN32)
		set_tests_properties(${t} PROPERTIES ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}\;${NEW_PATH}")
	endif()
endforeach()
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/g10)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/g10/t-keydb-get-keyblock.gpg ${CMAKE_CURRENT_BINARY_DIR}/g10/t-keydb-get-keyblock.gpg)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/g10/t-keydb-keyring.kbx ${CMAKE_CURRENT_BINARY_DIR}/g10/t-keydb-keyring.kbx)
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/g10/t-stutter-data.asc ${CMAKE_CURRENT_BINARY_DIR}/g10/t-stutter-data.asc)

endif()

#############
## sm #######
#############

set(gpgsm_SOURCES
gpgsm.c	gpgsm.h
misc.c
keydb.c keydb.h
server.c
call-agent.c
call-dirmngr.c
fingerprint.c
certlist.c
certdump.c
certcheck.c
certchain.c
keylist.c
verify.c
sign.c
encrypt.c
decrypt.c
import.c
export.c
delete.c
certreqgen.c
certreqgen-ui.c
minip12.c minip12.h
qualified.c
passphrase.c passphrase.h)

list(TRANSFORM gpgsm_SOURCES PREPEND "sm/")

add_executable(gpgsm ${gpgsm_SOURCES})
target_link_libraries(gpgsm keybox509 common gpgrl)
set_target_properties(gpgsm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/sm)

#############
## agent ####
#############

set(gpg_agent_SOURCES
gpg-agent.c agent.h
command.c command-ssh.c
call-pinentry.c
cache.c
trans.c
findkey.c
pksign.c
pkdecrypt.c
genkey.c
protect.c
trustlist.c
divert-scd.c
cvt-openpgp.c cvt-openpgp.h
call-scd.c
learncard.c)

list(TRANSFORM gpg_agent_SOURCES PREPEND "agent/")

set(gpg_protect_tool_SOURCES
protect-tool.c
protect.c cvt-openpgp.c)

list(TRANSFORM gpg_protect_tool_SOURCES PREPEND "agent/")

set(gpg_preset_passphrase_SOURCES
agent/preset-passphrase.c)

add_executable(gpg-agent ${gpg_agent_SOURCES})
target_link_libraries(gpg-agent commonpth)
add_executable(gpg-protect-tool ${gpg_protect_tool_SOURCES})
target_link_libraries(gpg-protect-tool common)
add_executable(gpg-preset-passphrase ${gpg_preset_passphrase_SOURCES})
target_link_libraries(gpg-preset-passphrase common pquery)

set_target_properties(gpg-agent gpg-protect-tool gpg-preset-passphrase PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/agent)

if(BUILD_TESTING)

add_executable(t-protect agent/t-protect.c agent/protect.c)
target_link_libraries(t-protect common)
add_test(NAME t-protect COMMAND t-protect)
set_target_properties(t-protect PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/agent)
if(WIN32)
	set_tests_properties(t-protect PROPERTIES ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}\;${NEW_PATH}")
endif()

endif()

#############
## scd ######
#############

set(scdaemon_SOURCES
scdaemon.c scdaemon.h
command.c
atr.c atr.h
apdu.c apdu.h
ccid-driver.c ccid-driver.h
iso7816.c iso7816.h
app.c app-common.h app-help.c
#card_apps
app-openpgp.c app-nks.c app-dinsig.c app-p15.c app-geldkarte.c app-sc-hsm.c)

list(TRANSFORM scdaemon_SOURCES PREPEND "scd/")

add_executable(scdaemon ${scdaemon_SOURCES})
target_link_libraries(scdaemon commonpth)
set_target_properties(scdaemon PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/scd)

#############
## dirmngr ##
#############

set(dirmngr_SOURCES
dirmngr.c dirmngr.h server.c crlcache.c crlfetch.c
certcache.c certcache.h
domaininfo.c
workqueue.c
loadswdb.c
cdb.h cdblib.c misc.c dirmngr-err.h dirmngr-status.h
ocsp.c ocsp.h validate.c validate.h
dns-stuff.c dns-stuff.h
http.c http.h http-common.c http-common.h http-ntbtls.c
ks-action.c ks-action.h ks-engine.h
ks-engine-hkp.c ks-engine-http.c ks-engine-finger.c ks-engine-kdns.c)

if(USE_LDAP)
	list(APPEND dirmngr_SOURCES
	ldapserver.h ldapserver.c ldap.c w32-ldap-help.h
	ldap-wrapper.h ldap-parse-uri.c ldap-parse-uri.h
	ks-engine-ldap.c ldap-wrapper.c)

	if(WIN32)
		list(APPEND ldap_url_SOURCES ldap-url.h ldap-url.c)
	endif()
endif()

if(USE_LIBDNS)
	list(APPEND dirmngr_SOURCES dns.c dns.h)
endif()

list(TRANSFORM dirmngr_SOURCES PREPEND "dirmngr/")
list(TRANSFORM ldap_url_SOURCES PREPEND "dirmngr/")

add_executable(dirmngr ${dirmngr_SOURCES} ${ldap_url_SOURCES})
target_link_libraries(dirmngr commonpth)

add_executable(dirmngr-client dirmngr/dirmngr-client.c)
target_link_libraries(dirmngr-client common)

set_target_properties(dirmngr dirmngr-client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dirmngr)

if(USE_LDAP)
	add_executable(dirmngr-ldap dirmngr/dirmngr_ldap.c ${ldap_url_SOURCES})
	target_link_libraries(dirmngr-ldap common)
	set_target_properties(dirmngr-ldap PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dirmngr)
endif()

if(BUILD_TESTING)

set(dirmngr_testlib_SOURCES
http.c dns-stuff.c http-common.c t-support.h t-support.c)
if(USE_LIBDNS)
	list(APPEND dirmngr_testlib_SOURCES dns.c dns.h)
endif()

list(TRANSFORM dirmngr_testlib_SOURCES PREPEND "dirmngr/")
add_library(dirmngr_testlib STATIC ${dirmngr_testlib_SOURCES})
target_link_libraries(dirmngr_testlib commonpth)

set(dirmngr_tests
t-http-basic)

#maintainer tests -> t-http t-dns-stuff

if(USE_LDAP)
	add_executable(t-ldap-parse-uri
	dirmngr/t-ldap-parse-uri.c dirmngr/ldap-parse-uri.c dirmngr/ldap-parse-uri.h ${ldap_url_SOURCES})
	target_compile_definitions(t-ldap-parse-uri PRIVATE WITHOUT_NPTH=1)
	target_link_libraries(t-ldap-parse-uri common dirmngr_testlib)
	set_target_properties(t-ldap-parse-uri PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dirmngr)
	add_test(NAME t-ldap-parse-uri COMMAND t-ldap-parse-uri)
	if(WIN32)
		set_tests_properties(t-ldap-parse-uri PROPERTIES ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}\;${NEW_PATH}")
	endif()
endif()

foreach(t ${dirmngr_tests})
	add_executable(${t} dirmngr/${t}.c)
	target_compile_definitions(${t} PRIVATE WITHOUT_NPTH=1)
	target_link_libraries(${t} common dirmngr_testlib)
	set_target_properties(${t} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dirmngr)
	add_test(NAME ${t} COMMAND ${t})
	if(WIN32)
		set_tests_properties(${t} PROPERTIES ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}\;${NEW_PATH}")
	endif()
endforeach()

endif()

#############
## tools ####
#############

set(gpgconf_SOURCES
gpgconf.c gpgconf.h gpgconf-comp.c)
list(TRANSFORM gpgconf_SOURCES PREPEND "tools/")

add_executable(gpgconf ${gpgconf_SOURCES})
target_link_libraries(gpgconf common)

add_executable(gpg-connect-agent tools/gpg-connect-agent.c)
target_link_libraries(gpg-connect-agent common gpgrl)

add_executable(gpg-check-pattern tools/gpg-check-pattern.c)
target_link_libraries(gpg-check-pattern common regexp)

set(gpgtar_SOURCES
gpgtar.c gpgtar.h
gpgtar-create.c
gpgtar-extract.c
gpgtar-list.c)
list(TRANSFORM gpgtar_SOURCES PREPEND "tools/")

add_executable(gpgtar ${gpgtar_SOURCES})
target_link_libraries(gpgtar common)

add_executable(gpgsplit tools/gpgsplit.c)
target_link_libraries(gpgsplit common)

#programs not ported to windows currently
if(NOT WIN32)

set(gpgparsemail_SOURCES
gpgparsemail.c rfc822parse.c rfc822parse.h)
list(TRANSFORM gpgparsemail_SOURCES PREPEND "tools/")
add_executable(gpgparsemail ${gpgparsemail_SOURCES})
target_link_libraries(gpgparsemail common)

add_executable(watchgnupg tools/watchgnupg.c)
target_link_libraries(watchgnupg common)

endif()

set(gpg_wks_server_SOURCES
gpg-wks-server.c
gpg-wks.h
wks-util.c
wks-receive.c
rfc822parse.c rfc822parse.h
mime-parser.c mime-parser.h
mime-maker.c  mime-maker.h
send-mail.c   send-mail.h)

list(TRANSFORM gpg_wks_server_SOURCES PREPEND "tools/")
add_executable(gpg-wks-server ${gpg_wks_server_SOURCES})
target_link_libraries(gpg-wks-server common)

set(gpg_wks_client_SOURCES
gpg-wks-client.c
gpg-wks.h
wks-util.c
wks-receive.c
rfc822parse.c rfc822parse.h
mime-parser.c mime-parser.h
mime-maker.h  mime-maker.c
send-mail.c   send-mail.h
call-dirmngr.c call-dirmngr.h)

list(TRANSFORM gpg_wks_client_SOURCES PREPEND "tools/")
add_executable(gpg-wks-client ${gpg_wks_client_SOURCES})
target_link_libraries(gpg-wks-client common)

set_target_properties(gpgconf gpg-connect-agent gpg-check-pattern gpgtar gpgsplit gpg-wks-server gpg-wks-client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools)

#############
## tests ####
#############

set(gpgscm_SOURCES
main.c private.h ffi.c ffi.h ffi-private.h
scheme-config.h scheme.c scheme.h scheme-private.h
opdefines.h small-integers.h)
list(TRANSFORM gpgscm_SOURCES PREPEND "tests/gpgscm/")

add_executable(gpgscm ${gpgscm_SOURCES})
target_precompile_headers(gpgscm PRIVATE tests/gpgscm/scheme-config.h)
target_include_directories(gpgscm PRIVATE tests)
target_link_libraries(gpgscm common)
if(WIN32)
	target_link_options(gpgscm PRIVATE -Wl,-ignore:4217)
endif()

add_executable(t-child tests/gpgscm/t-child.c)
set_target_properties(gpgscm t-child PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests/gpgscm)

add_test(NAME t-child COMMAND gpgscm ${CMAKE_CURRENT_SOURCE_DIR}/tests/gpgscm/t-child.scm)
if(WIN32)
	set_tests_properties(t-child PROPERTIES ENVIRONMENT "EXEEXT=${CMAKE_EXECUTABLE_SUFFIX};GPGSCM_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/gpgscm;PATH=${CMAKE_BINARY_DIR}\;${NEW_PATH}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests/gpgscm)
else()
	set_tests_properties(t-child PROPERTIES ENVIRONMENT "EXEEXT=${CMAKE_EXECUTABLE_SUFFIX};GPGSCM_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/gpgscm"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests/gpgscm)
endif()

if(UNIX)
	add_executable(asschk tests/asschk.c)
endif()
add_executable(fake-pinentry tests/openpgp/fake-pinentry.c)
set_target_properties(fake-pinentry PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests/openpgp)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests/gpgsm ${CMAKE_CURRENT_BINARY_DIR}/tests/migrations ${CMAKE_CURRENT_BINARY_DIR}/tests/pkits)

add_test(NAME gpgsm COMMAND gpgscm ${CMAKE_CURRENT_SOURCE_DIR}/tests/gpgsm/run-tests.scm --parallel WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests/gpgsm)
add_test(NAME migrations COMMAND gpgscm ${CMAKE_CURRENT_SOURCE_DIR}/tests/migrations/run-tests.scm --parallel WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests/migrations)
add_test(NAME openpgp COMMAND gpgscm ${CMAKE_CURRENT_SOURCE_DIR}/tests/openpgp/run-tests.scm --parallel WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests/openpgp)

if(WIN32)
	set_tests_properties(gpgsm migrations openpgp PROPERTIES ENVIRONMENT
	"EXEEXT=${CMAKE_EXECUTABLE_SUFFIX};GPGSCM_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/gpgscm;abs_top_srcdir=${CMAKE_CURRENT_SOURCE_DIR};objdir=${CMAKE_CURRENT_BINARY_DIR};PATH=${CMAKE_BINARY_DIR}\;${NEW_PATH}"
	RUN_SERIAL TRUE)
else()
	set_tests_properties(gpgsm migrations openpgp PROPERTIES ENVIRONMENT
	"EXEEXT=${CMAKE_EXECUTABLE_SUFFIX};GPGSCM_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/gpgscm;abs_top_srcdir=${CMAKE_CURRENT_SOURCE_DIR};objdir=${CMAKE_CURRENT_BINARY_DIR}"
	RUN_SERIAL TRUE)
endif()

###########################################

set(bin_programs 
kbxutil gpg gpgv gpgsm gpg-agent dirmngr dirmngr-client #gpgparsemail watchgnupg PORT to Windows
gpgconf gpg-connect-agent gpgtar gpgcompose gpgsplit gpg-wks-server)

set(libexec_programs
gpg-protect-tool gpg-preset-passphrase gpg-check-pattern scdaemon  gpg-wks-client)

if(USE_LDAP)
	list(APPEND libexec_program sdirmngr-ldap)
endif()

if(WIN32)
	install(TARGETS ${bin_programs} ${libexec_programs}	RUNTIME DESTINATION bin)
else()
	install(TARGETS ${bin_programs}	RUNTIME DESTINATION bin)
	install(TARGETS ${libexec_programs}	RUNTIME DESTINATION libexec)
endif()

install(PROGRAMS tools/addgnupghome tools/applygnupgdefaults
		DESTINATION sbin)

if(NOT WIN32)

install(TARGETS gpgparsemail watchgnupg
		RUNTIME DESTINATION bin)

endif()

file(GLOB gmo_files "po/*.gmo")
foreach(gmo ${gmo_files})
	string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/po "" gmo ${gmo})
	string(REPLACE ".gmo" "" gmo ${gmo})
	install(FILES po/${gmo}.gmo DESTINATION share/locale/${gmo}/LC_MESSAGES/)
	install(CODE "file(RENAME ${CMAKE_INSTALL_PREFIX}/share/locale/${gmo}/LC_MESSAGES/${gmo}.gmo ${CMAKE_INSTALL_PREFIX}/share/locale/${gmo}/LC_MESSAGES/${PROJECT_NAME}.mo)")
endforeach()

#install(FILES doc/gnupg.info doc/gnupg.info-1 doc/gnupg.info-2 DESTINATION share/info)

file(GLOB help_files "doc/help*.txt")
install(FILES ${help_files} DESTINATION share/gnupg)

install(DIRECTORY doc/examples DESTINATION share/doc/gnupg)
install(FILES doc/DCO doc/DETAILS doc/FAQ doc/HACKING doc/KEYSERVER doc/OpenPGP doc/TRANSLATE
		DESTINATION share/doc/gnupg)
install(FILES g10/distsigkey.gpg dirmngr/sks-keyservers.netCA.pem DESTINATION share/gnupg)
