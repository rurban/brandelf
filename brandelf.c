/*-
 * Copyright (c) 1996 Søren Schmidt
 * Copyright (c) 2022 Reini Urban
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/usr.bin/brandelf/brandelf.c,v 1.16 2000/07/02 03:34:08 imp Exp $
 */

#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <err.h>

// values from include/llvm/Support/ELF.h and GNU Linux elf.h
#ifndef ELFOSABI_SYSV
#define ELFOSABI_SYSV 0
#endif
#ifndef ELFOSABI_HPUX
#define ELFOSABI_HPUX 1
#endif
#ifndef ELFOSABI_NETBSD
#define ELFOSABI_NETBSD 2
#endif
#ifndef ELFOSABI_LINUX
#define ELFOSABI_LINUX 3
#endif
#ifndef ELFOSABI_HURD
#define ELFOSABI_HURD 4
#endif
#ifndef ELFOSABI_SOLARIS
#define ELFOSABI_SOLARIS 6
#endif
#ifndef ELFOSABI_AIX
#define ELFOSABI_AIX 7
#endif
#ifndef ELFOSABI_IRI
#define ELFOSABI_IRIX 8
#endif
#ifndef ELFOSABI_FREEBSD
#define ELFOSABI_FREEBSD 9
#endif
#ifndef ELFOSABI_TRU64
#define ELFOSABI_TRU64 10
#endif
#ifndef ELFOSABI_MODESTO
#define ELFOSABI_MODESTO 11
#endif
#ifndef ELFOSABI_OPENBSD
#define ELFOSABI_OPENBSD 12
#endif
#ifndef ELFOSABI_OPENVMS
#define ELFOSABI_OPENVMS 13
#endif
#ifndef ELFOSABI_NSK
#define ELFOSABI_NSK 14
#endif
#ifndef ELFOSABI_AROS
#define ELFOSABI_AROS 15
#endif
#ifndef ELFOSABI_FENIXOS
#define ELFOSABI_FENIXOS 16
#endif
#ifndef ELFOSABI_ARM_AEABI
#define ELFOSABI_ARM_AEABI 64
#endif
#ifndef ELFOSABI_C6000_LINUX
#define ELFOSABI_C6000_LINUX 65
#endif
#ifndef ELFOSABI_
#define ELFOSABI_ARM 97
#endif
#ifndef ELFOSABI_
#define ELFOSABI_STANDALONE 255
#endif

static int elftype(const char *);
static const char *iselftype(int);
static void printelftypes(void);
static void usage __P((void));

struct ELFtypes {
	const char *const str;
	const unsigned value;
};
static struct ELFtypes elftypes[] = {
	{ "SysV",	ELFOSABI_SYSV }, // NONE
	{ "HP-UX",	ELFOSABI_HPUX },
	{ "NetBSD",	ELFOSABI_NETBSD },
	{ "Linux",	ELFOSABI_LINUX },
	{ "Hurd",	ELFOSABI_HURD },
	{ "Solaris",	ELFOSABI_SOLARIS }, // Sun Solaris
	{ "AIX",	ELFOSABI_AIX },
	{ "IRIX",	ELFOSABI_IRIX },
	{ "FreeBSD",	ELFOSABI_FREEBSD },
	{ "TRU64",	ELFOSABI_TRU64 }, // Compaq TRU64 UNIX
	{ "Modesto",	ELFOSABI_MODESTO }, // Novell Modesto 
	{ "OpenBSD",	ELFOSABI_OPENBSD },
	{ "OpenVMS",	ELFOSABI_OPENVMS },
	{ "NSK",	ELFOSABI_NSK }, // Hewlett-Packard Non-Stop Kernel 
	{ "AROS",	ELFOSABI_AROS }, // Amiga Research OS
	{ "FenixOS",	ELFOSABI_FENIXOS },
	{ "ARM EABI",   ELFOSABI_ARM_AEABI }, // I.e. bare-metal TMS320C6000
	{ "TMS320C6000 Linux",ELFOSABI_C6000_LINUX }, // Linux TMS320C6000
	{ "ARM",	ELFOSABI_ARM },
	{ "Standalone",	ELFOSABI_STANDALONE }, // embedded
};

int
main(int argc, char **argv)
{

	const char *strtype = "FreeBSD";
	int type = ELFOSABI_FREEBSD;
	int retval = 0;
	int ch, change = 0, verbose = 0, force = 0, listed = 0;

	while ((ch = getopt(argc, argv, "f:lt:v")) != -1)
		switch (ch) {
		case 'f':
			if (change)
				errx(1, "f option incompatable with t option");
			force = 1;
			type = atoi(optarg);
			if (errno == ERANGE || type < 0 || type > 255) {
				warnx("invalid argument to option f: %s",
				    optarg);
				usage();
			}
			break;
		case 'l':
			printelftypes();
			listed = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 't':
			if (force)
				errx(1, "t option incompatable with f option");
			change = 1;
			strtype = optarg;
			break;
		default:
			usage();
	}
	argc -= optind;
	argv += optind;
	if (!argc) {
		if (listed)
			exit(0);
		else {
			warnx("no file(s) specified");
			usage();
		}
	}

	if (!force && (type = elftype(strtype)) == -1) {
		warnx("invalid ELF type '%s'", strtype);
		printelftypes();
		usage();
	}

	while (argc) {
		int fd;
		char buffer[EI_NIDENT];

		if ((fd = open(argv[0], change || force ? O_RDWR : O_RDONLY, 0)) < 0) {
			warn("error opening file %s", argv[0]);
			retval = 1;
			goto fail;
		}
		if (read(fd, buffer, EI_NIDENT) < EI_NIDENT) {
			warnx("file '%s' too short", argv[0]);
			retval = 1;
			goto fail;
		}
		if (buffer[0] != ELFMAG0 || buffer[1] != ELFMAG1 ||
		    buffer[2] != ELFMAG2 || buffer[3] != ELFMAG3) {
			warnx("file '%s' is not ELF format", argv[0]);
			retval = 1;
			goto fail;
		}
		if (!change && !force) {
			fprintf(stdout,
				"File '%s' is of brand '%s' (%u).\n",
				argv[0], iselftype(buffer[EI_OSABI]),
				buffer[EI_OSABI]);
			if (!iselftype(type)) {
				warnx("ELF ABI Brand '%u' is unknown",
				      type);
				printelftypes();
			}
		}
		else {
			buffer[EI_OSABI] = type;
			lseek(fd, 0, SEEK_SET);
			if (write(fd, buffer, EI_NIDENT) != EI_NIDENT) {
				warn("error writing %s %d", argv[0], fd);
				retval = 1;
				goto fail;
			}
		}
fail:
		close(fd);
		argc--;
		argv++;
	}

	return retval;
}

static void
usage()
{
fprintf(stderr, "usage: brandelf [-f ELF ABI number] [-v] [-l] [-t string] file ...\n");
	exit(1);
}

static const char *
iselftype(int elftype)
{
	int elfwalk;

	for (elfwalk = 0;
	     elfwalk < sizeof(elftypes)/sizeof(elftypes[0]);
	     elfwalk++)
		if (elftype == elftypes[elfwalk].value)
			return elftypes[elfwalk].str;
	return 0;
}

static int
elftype(const char *elfstrtype)
{
	int elfwalk;

	for (elfwalk = 0;
	     elfwalk < sizeof(elftypes)/sizeof(elftypes[0]);
	     elfwalk++)
		if (strcmp(elfstrtype, elftypes[elfwalk].str) == 0)
			return elftypes[elfwalk].value;
	return -1;
}

static void
printelftypes()
{
	int elfwalk;

	fprintf(stderr, "known ELF types are: ");
	for (elfwalk = 0;
	     elfwalk < sizeof(elftypes)/sizeof(elftypes[0]);
	     elfwalk++)
		fprintf(stderr, "%s(%u) ", elftypes[elfwalk].str,
			elftypes[elfwalk].value);
	fprintf(stderr, "\n");
}
