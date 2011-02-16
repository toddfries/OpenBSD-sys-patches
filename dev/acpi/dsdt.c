<<<<<<< HEAD
/* $OpenBSD: dsdt.c,v 1.86 2007/03/23 05:43:46 jordan Exp $ */
=======
/* $OpenBSD: dsdt.c,v 1.181 2011/01/02 04:56:57 jordan Exp $ */
>>>>>>> origin/master
/*
 * Copyright (c) 2005 Jordan Hargrave <jordan@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/bus.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#endif

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/amltypes.h>
#include <dev/acpi/dsdt.h>

#define opsize(opcode) (((opcode) & 0xFF00) ? 2 : 1)

#define AML_FIELD_RESERVED	0x00
#define AML_FIELD_ATTRIB	0x01

#define AML_REVISION		0x01
#define AML_INTSTRLEN		16
#define AML_NAMESEG_LEN		4

struct acpi_q		*acpi_maptable(struct acpi_softc *sc, paddr_t,
			    const char *, const char *,
			    const char *, int);
struct aml_scope	*aml_load(struct acpi_softc *, struct aml_scope *,
			    struct aml_value *, struct aml_value *);

<<<<<<< HEAD
struct aml_scope;

int			aml_cmpvalue(struct aml_value *, struct aml_value *, int);
=======
>>>>>>> origin/master
void			aml_copyvalue(struct aml_value *, struct aml_value *);

void			aml_setvalue(struct aml_scope *, struct aml_value *,
			    struct aml_value *, int64_t);
void			aml_freevalue(struct aml_value *);
struct aml_value	*aml_allocvalue(int, int64_t, const void *);
struct aml_value	*_aml_setvalue(struct aml_value *, int, int64_t,
			    const void *);

u_int64_t		aml_convradix(u_int64_t, int, int);
int64_t			aml_evalexpr(int64_t, int64_t, int);
int			aml_lsb(u_int64_t);
int			aml_msb(u_int64_t);

int			aml_tstbit(const u_int8_t *, int);
void			aml_setbit(u_int8_t *, int, int);

void			aml_xaddref(struct aml_value *, const char *);
void			aml_xdelref(struct aml_value **, const char *);

void			aml_bufcpy(void *, int, const void *, int, int);

int			aml_pc(uint8_t *);

<<<<<<< HEAD
#define aml_delref(x) _aml_delref(x, __FUNCTION__, __LINE__)

struct aml_value	*aml_parseop(struct aml_scope *, struct aml_value *);
=======
struct aml_value	*aml_parseop(struct aml_scope *, struct aml_value *,int);
>>>>>>> origin/master
struct aml_value	*aml_parsetarget(struct aml_scope *, struct aml_value *,
			    struct aml_value **);
struct aml_value	*aml_parseterm(struct aml_scope *, struct aml_value *);

struct aml_value	*aml_evaltarget(struct aml_scope *scope,
			    struct aml_value *res);
int			aml_evalterm(struct aml_scope *scope,
			    struct aml_value *raw, struct aml_value *dst);

struct aml_opcode	*aml_findopcode(int);

#define acpi_os_malloc(sz) _acpi_os_malloc(sz, __FUNCTION__, __LINE__)
#define acpi_os_free(ptr)  _acpi_os_free(ptr, __FUNCTION__, __LINE__)

void			*_acpi_os_malloc(size_t, const char *, int);
void			_acpi_os_free(void *, const char *, int);
void			acpi_stall(int);

struct aml_value	*aml_callosi(struct aml_scope *, struct aml_value *);

const char		*aml_getname(const char *);
int64_t			aml_hextoint(const char *);
void			aml_dump(int, u_int8_t *);
void			_aml_die(const char *fn, int line, const char *fmt, ...);
#define aml_die(x...)	_aml_die(__FUNCTION__, __LINE__, x)

void aml_notify_task(void *, int);
void acpi_poll_notify_task(void *, int);

/*
 * @@@: Global variables
 */
int			aml_intlen = 64;
struct aml_node		aml_root;
struct aml_value	*aml_global_lock;

struct aml_value *aml_parsenamed(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsenamedscope(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsemath(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsecompare(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parseif(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsewhile(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsebufpkg(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsemethod(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsesimple(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsefieldunit(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsebufferfield(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsemisc3(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsemuxaction(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsemisc2(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsematch(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parseref(struct aml_scope *, int, struct aml_value *);
struct aml_value *aml_parsestring(struct aml_scope *, int, struct aml_value *);

/* Perfect Hash key */
#define HASH_OFF		6904
#define HASH_SIZE		179
#define HASH_KEY(k)		(((k) ^ HASH_OFF) % HASH_SIZE)

/*
 * XXX this array should be sorted, and then aml_findopcode() should
 * do a binary search
 */
struct aml_opcode **aml_ophash;
struct aml_opcode aml_table[] = {
	/* Simple types */
	{ AMLOP_ZERO,		"Zero",		"c",	aml_parsesimple },
	{ AMLOP_ONE,		"One",		"c",	aml_parsesimple },
	{ AMLOP_ONES,		"Ones",		"c",	aml_parsesimple },
	{ AMLOP_REVISION,	"Revision",	"R",	aml_parsesimple },
	{ AMLOP_BYTEPREFIX,	".Byte",	"b",	aml_parsesimple },
	{ AMLOP_WORDPREFIX,	".Word",	"w",	aml_parsesimple },
	{ AMLOP_DWORDPREFIX,	".DWord",	"d",	aml_parsesimple },
	{ AMLOP_QWORDPREFIX,	".QWord",	"q",	aml_parsesimple },
	{ AMLOP_STRINGPREFIX,	".String",	"a",	aml_parsesimple },
	{ AMLOP_DEBUG,		"DebugOp",	"D",	aml_parsesimple },
	{ AMLOP_BUFFER,		"Buffer",	"piB",	aml_parsebufpkg },
	{ AMLOP_PACKAGE,	"Package",	"pbT",	aml_parsebufpkg },
	{ AMLOP_VARPACKAGE,	"VarPackage",	"piT",	aml_parsebufpkg },

	/* Simple objects */
	{ AMLOP_LOCAL0,		"Local0",	"L",	aml_parseref },
	{ AMLOP_LOCAL1,		"Local1",	"L",	aml_parseref },
	{ AMLOP_LOCAL2,		"Local2",	"L",	aml_parseref },
	{ AMLOP_LOCAL3,		"Local3",	"L",	aml_parseref },
	{ AMLOP_LOCAL4,		"Local4",	"L",	aml_parseref },
	{ AMLOP_LOCAL5,		"Local5",	"L",	aml_parseref },
	{ AMLOP_LOCAL6,		"Local6",	"L",	aml_parseref },
	{ AMLOP_LOCAL7,		"Local7",	"L",	aml_parseref },
	{ AMLOP_ARG0,		"Arg0",		"A",	aml_parseref },
	{ AMLOP_ARG1,		"Arg1",		"A",	aml_parseref },
	{ AMLOP_ARG2,		"Arg2",		"A",	aml_parseref },
	{ AMLOP_ARG3,		"Arg3",		"A",	aml_parseref },
	{ AMLOP_ARG4,		"Arg4",		"A",	aml_parseref },
	{ AMLOP_ARG5,		"Arg5",		"A",	aml_parseref },
	{ AMLOP_ARG6,		"Arg6",		"A",	aml_parseref },

	/* Control flow */
	{ AMLOP_IF,		"If",		"pI",	aml_parseif },
	{ AMLOP_ELSE,		"Else",		"pT" },
<<<<<<< HEAD
	{ AMLOP_WHILE,		"While",	"piT",	aml_parsewhile },
=======
	{ AMLOP_WHILE,		"While",	"piT",	},
>>>>>>> origin/master
	{ AMLOP_BREAK,		"Break",	"" },
	{ AMLOP_CONTINUE,	"Continue",	"" },
	{ AMLOP_RETURN,		"Return",	"t",	aml_parseref },
	{ AMLOP_FATAL,		"Fatal",	"bdi",	aml_parsemisc2 },
	{ AMLOP_NOP,		"Nop",		"",	aml_parsesimple },
	{ AMLOP_BREAKPOINT,	"BreakPoint",	"" },

	/* Arithmetic operations */
<<<<<<< HEAD
	{ AMLOP_INCREMENT,	"Increment",	"t",	aml_parsemath },
	{ AMLOP_DECREMENT,	"Decrement",	"t",	aml_parsemath },
	{ AMLOP_ADD,		"Add",		"iir",	aml_parsemath },
	{ AMLOP_SUBTRACT,	"Subtract",	"iir",	aml_parsemath },
	{ AMLOP_MULTIPLY,	"Multiply",	"iir",	aml_parsemath },
	{ AMLOP_DIVIDE,		"Divide",	"iirr",	aml_parsemath },
	{ AMLOP_SHL,		"ShiftLeft",	"iir",	aml_parsemath },
	{ AMLOP_SHR,		"ShiftRight",	"iir",	aml_parsemath },
	{ AMLOP_AND,		"And",		"iir",	aml_parsemath },
	{ AMLOP_NAND,		"Nand",		"iir",	aml_parsemath },
	{ AMLOP_OR,		"Or",		"iir",	aml_parsemath },
	{ AMLOP_NOR,		"Nor",		"iir",	aml_parsemath },
	{ AMLOP_XOR,		"Xor",		"iir",	aml_parsemath },
	{ AMLOP_NOT,		"Not",		"ir",	aml_parsemath },
	{ AMLOP_MOD,		"Mod",		"iir",	aml_parsemath },
	{ AMLOP_FINDSETLEFTBIT,	"FindSetLeftBit", "ir",	aml_parsemath },
	{ AMLOP_FINDSETRIGHTBIT,"FindSetRightBit", "ir",aml_parsemath },
=======
	{ AMLOP_INCREMENT,	"Increment",	"S",	},
	{ AMLOP_DECREMENT,	"Decrement",	"S",	},
	{ AMLOP_ADD,		"Add",		"iir",	},
	{ AMLOP_SUBTRACT,	"Subtract",	"iir",	},
	{ AMLOP_MULTIPLY,	"Multiply",	"iir",	},
	{ AMLOP_DIVIDE,		"Divide",	"iirr",	},
	{ AMLOP_SHL,		"ShiftLeft",	"iir",	},
	{ AMLOP_SHR,		"ShiftRight",	"iir",	},
	{ AMLOP_AND,		"And",		"iir",	},
	{ AMLOP_NAND,		"Nand",		"iir",	},
	{ AMLOP_OR,		"Or",		"iir",	},
	{ AMLOP_NOR,		"Nor",		"iir",	},
	{ AMLOP_XOR,		"Xor",		"iir",	},
	{ AMLOP_NOT,		"Not",		"ir",	},
	{ AMLOP_MOD,		"Mod",		"iir",	},
	{ AMLOP_FINDSETLEFTBIT,	"FindSetLeftBit", "ir",	},
	{ AMLOP_FINDSETRIGHTBIT,"FindSetRightBit", "ir",},
>>>>>>> origin/master

	/* Logical test operations */
	{ AMLOP_LAND,		"LAnd",		"ii",	aml_parsemath },
	{ AMLOP_LOR,		"LOr",		"ii",	aml_parsemath },
	{ AMLOP_LNOT,		"LNot",		"i",	aml_parsemath },
	{ AMLOP_LNOTEQUAL,	"LNotEqual",	"tt",	aml_parsecompare },
	{ AMLOP_LLESSEQUAL,	"LLessEqual",	"tt",	aml_parsecompare },
	{ AMLOP_LGREATEREQUAL,	"LGreaterEqual", "tt",	aml_parsecompare },
	{ AMLOP_LEQUAL,		"LEqual",	"tt",	aml_parsecompare },
	{ AMLOP_LGREATER,	"LGreater",	"tt",	aml_parsecompare },
	{ AMLOP_LLESS,		"LLess",	"tt",	aml_parsecompare },

	/* Named objects */
<<<<<<< HEAD
	{ AMLOP_NAMECHAR,	".NameRef",	"n",	aml_parsesimple	},
	{ AMLOP_ALIAS,		"Alias",	"nN",	aml_parsenamed },
	{ AMLOP_NAME,		"Name",	"Nt",	aml_parsenamed },
	{ AMLOP_EVENT,		"Event",	"N",	aml_parsenamed },
	{ AMLOP_MUTEX,		"Mutex",	"Nb",	aml_parsenamed },
	{ AMLOP_DATAREGION,	"DataRegion",	"Nttt",	aml_parsenamed },
	{ AMLOP_OPREGION,	"OpRegion",	"Nbii",	aml_parsenamed },
	{ AMLOP_SCOPE,		"Scope",	"pNT",	aml_parsenamedscope },
	{ AMLOP_DEVICE,		"Device",	"pNT",	aml_parsenamedscope },
	{ AMLOP_POWERRSRC,	"Power Resource", "pNbwT", aml_parsenamedscope },
	{ AMLOP_THERMALZONE,	"ThermalZone",	"pNT",	aml_parsenamedscope },
	{ AMLOP_PROCESSOR,	"Processor",	"pNbdbT", aml_parsenamedscope },
	{ AMLOP_METHOD,		"Method",	"pNfM",	aml_parsemethod },
=======
	{ AMLOP_NAMECHAR,	".NameRef",	"n",	},
	{ AMLOP_ALIAS,		"Alias",	"nN",	},
	{ AMLOP_NAME,		"Name",	"Nt",	},
	{ AMLOP_EVENT,		"Event",	"N",	},
	{ AMLOP_MUTEX,		"Mutex",	"Nb",	},
	{ AMLOP_DATAREGION,	"DataRegion",	"Nttt",	},
	{ AMLOP_OPREGION,	"OpRegion",	"Nbii",	},
	{ AMLOP_SCOPE,		"Scope",	"pnT",	},
	{ AMLOP_DEVICE,		"Device",	"pNT",	},
	{ AMLOP_POWERRSRC,	"Power Resource", "pNbwT",},
	{ AMLOP_THERMALZONE,	"ThermalZone",	"pNT",	},
	{ AMLOP_PROCESSOR,	"Processor",	"pNbdbT", },
	{ AMLOP_METHOD,		"Method",	"pNbM",	},
>>>>>>> origin/master

	/* Field operations */
	{ AMLOP_FIELD,		"Field",	"pnfF",		aml_parsefieldunit },
	{ AMLOP_INDEXFIELD,	"IndexField",	"pntfF",	aml_parsefieldunit },
	{ AMLOP_BANKFIELD,	"BankField",	"pnnifF",	aml_parsefieldunit },
	{ AMLOP_CREATEFIELD,	"CreateField",	"tiiN",		aml_parsebufferfield },
	{ AMLOP_CREATEQWORDFIELD, "CreateQWordField","tiN",	aml_parsebufferfield },
	{ AMLOP_CREATEDWORDFIELD, "CreateDWordField","tiN",	aml_parsebufferfield },
	{ AMLOP_CREATEWORDFIELD, "CreateWordField", "tiN",	aml_parsebufferfield },
	{ AMLOP_CREATEBYTEFIELD, "CreateByteField", "tiN",	aml_parsebufferfield },
	{ AMLOP_CREATEBITFIELD,	"CreateBitField", "tiN",	aml_parsebufferfield },

	/* Conversion operations */
	{ AMLOP_TOINTEGER,	"ToInteger",	"tr",	aml_parsemath },
	{ AMLOP_TOBUFFER,	"ToBuffer",	"tr",	},
	{ AMLOP_TODECSTRING,	"ToDecString",	"ir",	aml_parsestring },
	{ AMLOP_TOHEXSTRING,	"ToHexString",	"ir",	aml_parsestring },
	{ AMLOP_TOSTRING,	"ToString",	"t",	aml_parsestring },
	{ AMLOP_MID,		"Mid",		"tiir",	aml_parsestring },
	{ AMLOP_FROMBCD,	"FromBCD",	"ir",	aml_parsemath },
	{ AMLOP_TOBCD,		"ToBCD",	"ir",	aml_parsemath },

	/* Mutex/Signal operations */
	{ AMLOP_ACQUIRE,	"Acquire",	"tw",	aml_parsemuxaction },
	{ AMLOP_RELEASE,	"Release",	"t",	aml_parsemuxaction },
	{ AMLOP_SIGNAL,		"Signal",	"t",	aml_parsemuxaction },
	{ AMLOP_WAIT,		"Wait",		"ti",	aml_parsemuxaction },
	{ AMLOP_RESET,		"Reset",	"t",	aml_parsemuxaction },

	{ AMLOP_INDEX,		"Index",	"tir",	aml_parseref },
	{ AMLOP_DEREFOF,	"DerefOf",	"t",	aml_parseref },
	{ AMLOP_REFOF,		"RefOf",	"t",	aml_parseref },
	{ AMLOP_CONDREFOF,	"CondRef",	"nr",	aml_parseref },

	{ AMLOP_LOADTABLE,	"LoadTable",	"tttttt" },
	{ AMLOP_STALL,		"Stall",	"i",	aml_parsemisc2 },
	{ AMLOP_SLEEP,		"Sleep",	"i",	aml_parsemisc2 },
	{ AMLOP_LOAD,		"Load",		"nt",	aml_parseref },
	{ AMLOP_UNLOAD,		"Unload",	"t" },
	{ AMLOP_STORE,		"Store",	"tr",	aml_parseref },
	{ AMLOP_CONCAT,		"Concat",	"ttr",	aml_parsestring },
	{ AMLOP_CONCATRES,	"ConcatRes",	"ttt" },
	{ AMLOP_NOTIFY,		"Notify",	"ti",	aml_parsemisc2 },
	{ AMLOP_SIZEOF,		"Sizeof",	"t",	aml_parsemisc3 },
	{ AMLOP_MATCH,		"Match",	"tbibii", aml_parsematch },
	{ AMLOP_OBJECTTYPE,	"ObjectType",	"t",	aml_parsemisc3 },
	{ AMLOP_COPYOBJECT,	"CopyObject",	"tr",	aml_parseref },
};

int aml_pc(uint8_t *src)
{
	return src - aml_root.start;
}

struct aml_scope *aml_lastscope;

void _aml_die(const char *fn, int line, const char *fmt, ...)
{
	struct aml_scope *root;
<<<<<<< HEAD
=======
	struct aml_value *sp;
	int idx;
#endif /* SMALL_KERNEL */
>>>>>>> origin/master
	va_list ap;
	int idx;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

	for (root = aml_lastscope; root && root->pos; root = root->parent) {
		printf("%.4x Called: %s\n", aml_pc(root->pos),
		    aml_nodename(root->node));
		for (idx = 0; idx < AML_MAX_ARG; idx++) {
			sp = aml_getstack(root, AMLOP_ARG0+idx);
			if (sp && sp->type) {
				printf("  arg%d: ", idx);
				aml_showvalue(sp, 0);
			}
		}
		for (idx = 0; idx < AML_MAX_LOCAL; idx++) {
			sp = aml_getstack(root, AMLOP_LOCAL0+idx);
			if (sp && sp->type) {
				printf("  local%d: ", idx);
				aml_showvalue(sp, 0);
			}
		}
	}

	/* XXX: don't panic */
	panic("aml_die %s:%d", fn, line);
}

void
aml_hashopcodes(void)
{
	int i;

	/* Dynamically allocate hash table */
	aml_ophash = (struct aml_opcode **)acpi_os_malloc(HASH_SIZE *
	    sizeof(struct aml_opcode *));
	for (i = 0; i < sizeof(aml_table) / sizeof(aml_table[0]); i++)
		aml_ophash[HASH_KEY(aml_table[i].opcode)] = &aml_table[i];
}

struct aml_opcode *
aml_findopcode(int opcode)
{
	struct aml_opcode *hop;

	hop = aml_ophash[HASH_KEY(opcode)];
	if (hop && hop->opcode == opcode)
		return hop;
	return NULL;
}

<<<<<<< HEAD
=======
#if defined(DDB) || !defined(SMALL_KERNEL)
>>>>>>> origin/master
const char *
aml_mnem(int opcode, uint8_t *pos)
{
	struct aml_opcode *tab;
	static char mnemstr[32];

	if ((tab = aml_findopcode(opcode)) != NULL) {
		strlcpy(mnemstr, tab->mnem, sizeof(mnemstr));
		if (pos != NULL) {
			switch (opcode) {
			case AMLOP_STRINGPREFIX:
				snprintf(mnemstr, sizeof(mnemstr), "\"%s\"", pos);
				break;
			case AMLOP_BYTEPREFIX:
				snprintf(mnemstr, sizeof(mnemstr), "0x%.2x",
					 *(uint8_t *)pos);
				break;
			case AMLOP_WORDPREFIX:
				snprintf(mnemstr, sizeof(mnemstr), "0x%.4x",
					 *(uint16_t *)pos);
				break;
			case AMLOP_DWORDPREFIX:
				snprintf(mnemstr, sizeof(mnemstr), "0x%.4x",
					 *(uint16_t *)pos);
				break;
			case AMLOP_NAMECHAR:
				strlcpy(mnemstr, aml_getname(pos), sizeof(mnemstr));
				break;
			}
		}
		return mnemstr;
	}
	return ("xxx");
}
<<<<<<< HEAD

const char *
aml_args(int opcode)
{
	struct aml_opcode *tab;

	if ((tab = aml_findopcode(opcode)) != NULL)
		return tab->args;
	return ("");
}
=======
#endif /* defined(DDB) || !defined(SMALL_KERNEL) */
>>>>>>> origin/master

struct aml_notify_data {
	struct aml_node		*node;
	char			pnpid[20];
	void			*cbarg;
	int			(*cbproc)(struct aml_node *, int, void *);
	int			poll;

	SLIST_ENTRY(aml_notify_data) link;
};

SLIST_HEAD(aml_notify_head, aml_notify_data);
struct aml_notify_head aml_notify_list =
    LIST_HEAD_INITIALIZER(&aml_notify_list);

/*
 *  @@@: Memory management functions
 */

long acpi_nalloc;

struct acpi_memblock {
	size_t size;
#ifdef ACPI_MEMDEBUG
	const char *fn;
	int         line;
	int         sig;
	LIST_ENTRY(acpi_memblock) link;
#endif
};

#ifdef ACPI_MEMDEBUG
LIST_HEAD(, acpi_memblock)	acpi_memhead;
int				acpi_memsig;

int
acpi_walkmem(int sig, const char *lbl)
{
	struct acpi_memblock *sptr;

	printf("--- walkmem:%s %x --- %x bytes alloced\n", lbl, sig, acpi_nalloc);
	LIST_FOREACH(sptr, &acpi_memhead, link) {
		if (sptr->sig < sig)
			break;
		printf("%.4x Alloc %.8lx bytes @ %s:%d\n",
			sptr->sig, sptr->size, sptr->fn, sptr->line);
	}
	return acpi_memsig;
}
#endif /* ACPI_MEMDEBUG */

void *
_acpi_os_malloc(size_t size, const char *fn, int line)
{
	struct acpi_memblock *sptr;

<<<<<<< HEAD
	sptr = malloc(size+sizeof(*sptr), M_DEVBUF, M_WAITOK);
	dnprintf(99, "alloc: %x %s:%d\n", sptr, fn, line);
	if (sptr) {
		acpi_nalloc += size;
		sptr->size = size;
		memset(&sptr[1], 0, size);
		return &sptr[1];
	}
	return NULL;
=======
	sptr = malloc(size+sizeof(*sptr), M_ACPI, M_WAITOK | M_ZERO);
	dnprintf(99, "alloc: %x %s:%d\n", sptr, fn, line);
	acpi_nalloc += size;
	sptr->size = size;
#ifdef ACPI_MEMDEBUG
	sptr->line = line;
	sptr->fn = fn;
	sptr->sig = ++acpi_memsig;

	LIST_INSERT_HEAD(&acpi_memhead, sptr, link);
#endif

	return &sptr[1];
>>>>>>> origin/master
}

void
_acpi_os_free(void *ptr, const char *fn, int line)
{
	struct acpi_memblock *sptr;

	if (ptr != NULL) {
		sptr = &(((struct acpi_memblock *)ptr)[-1]);
		acpi_nalloc -= sptr->size;

#ifdef ACPI_MEMDEBUG
		LIST_REMOVE(sptr, link);
#endif

		dnprintf(99, "free: %x %s:%d\n", sptr, fn, line);
		free(sptr, M_ACPI);
	}
}

void
acpi_sleep(int ms, char *reason)
{
	static int acpinowait;
	int to = ms * hz / 1000;

	if (cold)
		delay(ms * 1000);
	else {
		if (to <= 0)
			to = 1;
		tsleep(&acpinowait, PWAIT, reason, to);
	}
}

void
acpi_stall(int us)
{
	delay(us);
}

int
acpi_mutex_acquire(struct aml_value *val, int timeout)
{
	/* XXX we currently do not have concurrency so assume mutex succeeds */
	dnprintf(50, "acpi_mutex_acquire\n");

	return (0);
#if 0
	struct acpi_mutex *mtx = val->v_mutex;
	int rv = 0, ts, tries = 0;

	if (val->type != AML_OBJTYPE_MUTEX) {
		printf("acpi_mutex_acquire: invalid mutex\n");
		return (1);
	}

	if (timeout == 0xffff)
		timeout = 0;

	/* lock recursion be damned, panic if that happens */
	rw_enter_write(&mtx->amt_lock);
	while (mtx->amt_ref_count) {
		rw_exit_write(&mtx->amt_lock);
		/* block access */
		ts = tsleep(mtx, PWAIT, mtx->amt_name, timeout / hz);
		if (ts == EWOULDBLOCK) {
			rv = 1; /* mutex not acquired */
			goto done;
		}
		tries++;
		rw_enter_write(&mtx->amt_lock);
	}

	mtx->amt_ref_count++;
	rw_exit_write(&mtx->amt_lock);
done:
	return (rv);
#endif
}

void
acpi_mutex_release(struct aml_value *val)
{
	dnprintf(50, "acpi_mutex_release\n");
#if 0
	struct acpi_mutex *mtx = val->v_mutex;

	/* sanity */
	if (val->type != AML_OBJTYPE_MUTEX) {
		printf("acpi_mutex_acquire: invalid mutex\n");
		return;
	}

	rw_enter_write(&mtx->amt_lock);

	if (mtx->amt_ref_count == 0) {
		printf("acpi_mutex_release underflow %s\n", mtx->amt_name);
		goto done;
	}

	mtx->amt_ref_count--;
	wakeup(mtx); /* wake all of them up */
done:
	rw_exit_write(&mtx->amt_lock);
#endif
}

/*
 * @@@: Misc utility functions
 */

void
aml_dump(int len, u_int8_t *buf)
{
	int		idx;

	dnprintf(50, "{ ");
	for (idx = 0; idx < len; idx++) {
		dnprintf(50, "%s0x%.2x", idx ? ", " : "", buf[idx]);
	}
	dnprintf(50, " }\n");
}

/* Bit mangling code */
int
aml_tstbit(const u_int8_t *pb, int bit)
{
	pb += aml_bytepos(bit);

	return (*pb & aml_bitmask(bit));
}

void
aml_setbit(u_int8_t *pb, int bit, int val)
{
	pb += aml_bytepos(bit);

	if (val)
		*pb |= aml_bitmask(bit);
	else
		*pb &= ~aml_bitmask(bit);
}

/* Read/Write to hardware I/O fields */
void
aml_gasio(struct acpi_softc *sc, int type, uint64_t base, uint64_t length,
    int bitpos, int bitlen, int size, void *buf, int mode)
{
	dnprintf(10, "-- aml_gasio: %.2x"
	    " base:%llx len:%llx bpos:%.4x blen:%.4x sz:%.2x mode=%s\n",
	    type, base, length, bitpos, bitlen, size,
	    mode==ACPI_IOREAD?"read":"write");
	acpi_gasio(sc, mode, type, base+(bitpos>>3),
	    (size>>3), (bitlen>>3), buf);
#ifdef ACPI_DEBUG
	while (bitlen > 0) {
		dnprintf(10, "%.2x ", *(uint8_t *)buf);
		buf++;
		bitlen -=8;
	}
	dnprintf(10, "\n");
#endif
}

/*
 * @@@: Notify functions
 */
#ifndef SMALL_KERNEL
void
acpi_poll(void *arg)
{
	int s;

	s = spltty();
	acpi_addtask(acpi_softc, acpi_poll_notify_task, NULL, 0);
	acpi_softc->sc_threadwaiting = 0;
	wakeup(acpi_softc);
	splx(s);

	timeout_add_sec(&acpi_softc->sc_dev_timeout, 10);
}
#endif

void
aml_notify_task(void *node, int notify_value)
{
	struct aml_notify_data	*pdata = NULL;

	dnprintf(10,"run notify: %s %x\n", aml_nodename(node), notify_value);
	SLIST_FOREACH(pdata, &aml_notify_list, link)
		if (pdata->node == node)
			pdata->cbproc(pdata->node, notify_value, pdata->cbarg);
}

void
aml_register_notify(struct aml_node *node, const char *pnpid,
    int (*proc)(struct aml_node *, int, void *), void *arg, int poll)
{
	struct aml_notify_data	*pdata;
	extern int acpi_poll_enabled;

	dnprintf(10, "aml_register_notify: %s %s %x\n",
	    node->name, pnpid ? pnpid : "", proc);

	pdata = acpi_os_malloc(sizeof(struct aml_notify_data));
	pdata->node = node;
	pdata->cbarg = arg;
	pdata->cbproc = proc;
	pdata->poll = poll;

	if (pnpid)
		strlcpy(pdata->pnpid, pnpid, sizeof(pdata->pnpid));

	SLIST_INSERT_HEAD(&aml_notify_list, pdata, link);

	if (poll && !acpi_poll_enabled)
		timeout_add_sec(&acpi_softc->sc_dev_timeout, 10);
}

void
aml_notify(struct aml_node *node, int notify_value)
{
	if (node == NULL)
		return;

	dnprintf(10,"queue notify: %s %x\n", aml_nodename(node), notify_value);
	acpi_addtask(acpi_softc, aml_notify_task, node, notify_value);
}

#ifndef SMALL_KERNEL
void
aml_notify_dev(const char *pnpid, int notify_value)
{
	struct aml_notify_data	*pdata = NULL;

	if (pnpid == NULL)
		return;

	SLIST_FOREACH(pdata, &aml_notify_list, link)
		if (pdata->pnpid && !strcmp(pdata->pnpid, pnpid))
			pdata->cbproc(pdata->node, notify_value, pdata->cbarg);
}

void
acpi_poll_notify_task(void *arg0, int arg1)
{
	struct aml_notify_data	*pdata = NULL;

	SLIST_FOREACH(pdata, &aml_notify_list, link)
		if (pdata->cbproc && pdata->poll)
			pdata->cbproc(pdata->node, 0, pdata->cbarg);
}
#endif

/*
 * @@@: Namespace functions
 */

struct aml_node *__aml_search(struct aml_node *, uint8_t *);
void aml_delchildren(struct aml_node *);


/* Search for a name in children nodes */
struct aml_node *
__aml_search(struct aml_node *root, uint8_t *nameseg)
{
<<<<<<< HEAD
	if (root == NULL)
		return NULL;
	for (root = root->child; root; root = root->sibling) {
		if (!memcmp(root->name, nameseg, AML_NAMESEG_LEN))
			return root;
	}
	return NULL;
=======
	struct aml_node *node;

	/* XXX: Replace with SLIST/SIMPLEQ routines */
	if (root == NULL)
		return NULL;
	SIMPLEQ_FOREACH(node, &root->son, sib) {
		if (!strncmp(node->name, nameseg, AML_NAMESEG_LEN))
			return node;
	}
	if (create) {
		node = acpi_os_malloc(sizeof(struct aml_node));
		memcpy((void *)node->name, nameseg, AML_NAMESEG_LEN);
		node->value = aml_allocvalue(0,0,NULL);
		node->value->node = node;
		node->parent = root;

		SIMPLEQ_INIT(&node->son);
		SIMPLEQ_INSERT_TAIL(&root->son, node, sib);
	}
	return node;
>>>>>>> origin/master
}

/* Get absolute pathname of AML node */
const char *
aml_nodename(struct aml_node *node)
{
	static char namebuf[128];

	namebuf[0] = 0;
	if (node) {
		aml_nodename(node->parent);
		if (node->parent != &aml_root)
			strlcat(namebuf, ".", sizeof(namebuf));
		strlcat(namebuf, node->name, sizeof(namebuf));
		return namebuf+1;
	}
	return namebuf;
}

const char *
aml_getname(const char *name)
{
	static char namebuf[128], *p;
	int count;

	p = namebuf;
	while (*name == AMLOP_ROOTCHAR || *name == AMLOP_PARENTPREFIX)
		*(p++) = *(name++);
	switch (*name) {
	case 0x00:
		count = 0;
		break;
	case AMLOP_MULTINAMEPREFIX:
		count = name[1];
		name += 2;
		break;
	case AMLOP_DUALNAMEPREFIX:
		count = 2;
		name += 1;
		break;
	default:
		count = 1;
	}
	while (count--) {
		memcpy(p, name, 4);
		p[4] = '.';
		p += 5;
		name += 4;
		if (*name == '.') name++;
	}
	*(--p) = 0;
	return namebuf;
}

<<<<<<< HEAD
/* Create name/value pair in namespace */
struct aml_node *
aml_createname(struct aml_node *root, const void *vname, struct aml_value *value)
{
	struct aml_node *node, **pp;
	uint8_t *name = (uint8_t *)vname;
	int count;

	if (*name == AMLOP_ROOTCHAR) {
		root = &aml_root;
		name++;
	}
	while (*name == AMLOP_PARENTPREFIX && root) {
		root = root->parent;
		name++;
	}
	switch (*name) {
	case 0x00:
		return root;
	case AMLOP_MULTINAMEPREFIX:
		count = name[1];
		name += 2;
		break;
	case AMLOP_DUALNAMEPREFIX:
		count = 2;
		name += 1;
		break;
	default:
		count = 1;
		break;
	}
	node = NULL;
	while (count-- && root) {
		/* Create new name if it does not exist */
		if ((node = __aml_search(root, name)) == NULL) {
			node = acpi_os_malloc(sizeof(struct aml_node));

			memcpy((void *)node->name, name, AML_NAMESEG_LEN);
			for (pp = &root->child; *pp; pp = &(*pp)->sibling)
				;
			node->parent = root;
			node->sibling = NULL;
			*pp = node;
		}
		root = node;
		name += AML_NAMESEG_LEN;
	}
	/* If node created, set value pointer */
	if (node && value) {
		node->value = value;
		value->node = node;
	}
	return node;
}

/* Search namespace for a named node */
struct aml_node *
aml_searchname(struct aml_node *root, const void *vname)
{
	struct aml_node *node;
	uint8_t *name = (uint8_t *)vname;
	int count;

	if (*name == AMLOP_ROOTCHAR) {
		root = &aml_root;
		name++;
	}
	while (*name == AMLOP_PARENTPREFIX && root) {
		root = root->parent;
		name++;
	}
	if (strlen(name) < AML_NAMESEG_LEN) {
		aml_die("bad name");
	}
	switch (*name) {
	case 0x00:
		return root;
	case AMLOP_MULTINAMEPREFIX:
		count = name[1];
		name += 2;
		break;
	case AMLOP_DUALNAMEPREFIX:
		count = 2;
		name += 1;
		break;
	default:
		if (name[4] == '.') {
			/* Called from user code */
			while (*name && (root = __aml_search(root, name)) != NULL) {
				name += AML_NAMESEG_LEN+1;
			}
			return root;
		}
		/* Special case.. search relative for name */
		while (root && (node = __aml_search(root, name)) == NULL) {
			root = root->parent;
		}
		return node;
	}
	/* Search absolute for name*/
	while (count-- && (root = __aml_search(root, name)) != NULL) {
		name += AML_NAMESEG_LEN;
	}
	return root;
}

=======
>>>>>>> origin/master
/* Free all children nodes/values */
void
aml_delchildren(struct aml_node *node)
{
	struct aml_node *onode;

	if (node == NULL)
		return;
	while ((onode = SIMPLEQ_FIRST(&node->son)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&node->son, sib);

		aml_delchildren(onode);

		/* Don't delete values that have references */
		if (onode->value && onode->value->refcnt > 1)
			onode->value->node = NULL;

		/* Decrease reference count */
		aml_xdelref(&onode->value, "");

		/* Delete node */
		acpi_os_free(onode);
	}
}

/*
 * @@@: Value functions
 */

<<<<<<< HEAD
struct aml_value	*aml_alloctmp(struct aml_scope *, int);
struct aml_scope	*aml_pushscope(struct aml_scope *, uint8_t *,
			    uint8_t *, struct aml_node *);
struct aml_scope	*aml_popscope(struct aml_scope *);
int			aml_parsenode(struct aml_scope *, struct aml_node *,
			    uint8_t *, uint8_t **, struct aml_value *);

#define AML_LHS		0
#define AML_RHS		1
#define AML_DST		2
#define AML_DST2	3

/* Allocate temporary storage in this scope */
struct aml_value *
aml_alloctmp(struct aml_scope *scope, int narg)
{
	struct aml_vallist *tmp;

	/* Allocate array of temp values */
	tmp = (struct aml_vallist *)acpi_os_malloc(sizeof(struct aml_vallist) +
	    narg * sizeof(struct aml_value));

	tmp->obj = (struct aml_value *)&tmp[1];
	tmp->nobj = narg;

	/* Link into scope */
	tmp->next = scope->tmpvals;
	scope->tmpvals = tmp;

	/* Return array of values */
	return tmp->obj;
}

/* Allocate+push parser scope */
struct aml_scope *
aml_pushscope(struct aml_scope *parent, uint8_t *start, uint8_t  *end,
    struct aml_node *node)
{
	struct aml_scope *scope;

	scope = acpi_os_malloc(sizeof(struct aml_scope));
	scope->pos = start;
	scope->end = end;
	scope->node = node;
	scope->parent = parent;
	scope->sc = dsdt_softc;

	aml_lastscope = scope;

	return scope;
}

struct aml_scope *
aml_popscope(struct aml_scope *scope)
{
	struct aml_scope *nscope;
	struct aml_vallist *ol;
	int idx;

	if (scope == NULL)
		return NULL;
	nscope = scope->parent;

	/* Free temporary values */
	while ((ol = scope->tmpvals) != NULL) {
		scope->tmpvals = ol->next;
		for (idx = 0; idx < ol->nobj; idx++) {
			aml_freevalue(&ol->obj[idx]);
		}
		acpi_os_free(ol);
	}
	acpi_os_free(scope);

	aml_lastscope = nscope;

	return nscope;
}

int
aml_parsenode(struct aml_scope *parent, struct aml_node *node, uint8_t *start,
    uint8_t **end, struct aml_value *res)
{
	struct aml_scope *scope;

	/* Don't parse zero-length scope */
	if (start == *end)
		return 0;
	scope = aml_pushscope(parent, start, *end, node);
	if (res == NULL)
		res = aml_alloctmp(scope, 1);
	while (scope != parent) {
		while (scope->pos < scope->end)
			aml_parseop(scope, res);
		scope = aml_popscope(scope);
	}
	return 0;
}

=======
>>>>>>> origin/master
/*
 * Field I/O code
 */
void aml_setbufint(struct aml_value *, int, int, struct aml_value *);
void aml_getbufint(struct aml_value *, int, int, struct aml_value *);
void aml_fieldio(struct aml_scope *, struct aml_value *, struct aml_value *, int);
void aml_unlockfield(struct aml_scope *, struct aml_value *);
void aml_lockfield(struct aml_scope *, struct aml_value *);

<<<<<<< HEAD
/* Copy from a bufferfield to an integer/buffer */
void
aml_setbufint(struct aml_value *dst, int bitpos, int bitlen,
    struct aml_value *src)
{
	if (src->type != AML_OBJTYPE_BUFFER)
		aml_die("wrong setbufint type\n");

#if 1
	/* Return buffer type */
	_aml_setvalue(dst, AML_OBJTYPE_BUFFER, (bitlen+7)>>3, NULL);
	aml_bufcpy(dst->v_buffer, 0, src->v_buffer, bitpos, bitlen);
#else
	if (bitlen < aml_intlen) {
		/* XXX: Endian issues?? */
		/* Return integer type */
		_aml_setvalue(dst, AML_OBJTYPE_INTEGER, 0, NULL);
		aml_bufcpy(&dst->v_integer, 0, src->v_buffer, bitpos, bitlen);
	} else {
		/* Return buffer type */
		_aml_setvalue(dst, AML_OBJTYPE_BUFFER, (bitlen+7)>>3, NULL);
		aml_bufcpy(dst->v_buffer, 0, src->v_buffer, bitpos, bitlen);
	}
#endif
}

/* Copy from a string/integer/buffer to a bufferfield */
void
aml_getbufint(struct aml_value *src, int bitpos, int bitlen,
    struct aml_value *dst)
{
	if (dst->type != AML_OBJTYPE_BUFFER)
		aml_die("wrong getbufint type\n");
	switch (src->type) {
	case AML_OBJTYPE_INTEGER:
		if (bitlen >= aml_intlen)
			bitlen = aml_intlen;
		aml_bufcpy(dst->v_buffer, bitpos, &src->v_integer, 0, bitlen);
		break;
	case AML_OBJTYPE_BUFFER:
		if (bitlen >= 8*src->length)
			bitlen = 8*src->length;
		aml_bufcpy(dst->v_buffer, bitpos, src->v_buffer, 0, bitlen);
		break;
	case AML_OBJTYPE_STRING:
		if (bitlen >= 8*src->length)
			bitlen = 8*src->length;
		aml_bufcpy(dst->v_buffer, bitpos, src->v_string, 0, bitlen);
		break;
	}
}

=======
long acpi_acquire_global_lock(void*);
long acpi_release_global_lock(void*);
static long global_lock_count = 0;
#define acpi_acquire_global_lock(x) 1
#define acpi_release_global_lock(x) 0
>>>>>>> origin/master
void
aml_lockfield(struct aml_scope *scope, struct aml_value *field)
{
	if (AML_FIELD_LOCK(field->v_field.flags) == AML_FIELD_LOCK_ON) {
		/* XXX: do locking here */
	}
}

void
aml_unlockfield(struct aml_scope *scope, struct aml_value *field)
{
	if (AML_FIELD_LOCK(field->v_field.flags) == AML_FIELD_LOCK_ON) {
		/* XXX: do unlocking here */
	}
}

void *aml_getbuffer(struct aml_value *, int *);

void *
aml_getbuffer(struct aml_value *val, int *bitlen)
{
	switch (val->type) {
	case AML_OBJTYPE_INTEGER:
	case AML_OBJTYPE_STATICINT:
		*bitlen = aml_intlen;
		return (&val->v_integer);

	case AML_OBJTYPE_BUFFER:
	case AML_OBJTYPE_STRING:
		*bitlen = val->length<<3;
		return (val->v_buffer);

<<<<<<< HEAD
	default:
		aml_die("getvbi");
=======
	/* Spin to acquire lock */
	while (!st) {
		st = acpi_acquire_global_lock(&acpi_softc->sc_facs->global_lock);
		/* XXX - yield/delay? */
>>>>>>> origin/master
	}

	return (NULL);
}

/*
 * Buffer/Region: read/write to bitfields
 */
void
aml_fieldio(struct aml_scope *scope, struct aml_value *field,
    struct aml_value *res, int mode)
{
<<<<<<< HEAD
	struct aml_value *pop, tf;
	int bpos, blen, aligned, mask;
	void    *iobuf, *iobuf2;
	uint64_t iobase;
=======
	int st, x, s;
>>>>>>> origin/master

	pop = field->v_field.ref1;
	bpos = field->v_field.bitpos;
	blen = field->v_field.bitlen;

	dnprintf(55,"--fieldio: %s [%s] bp:%.4x bl:%.4x\n",
	    mode == ACPI_IOREAD ? "rd" : "wr",
	    aml_nodename(field->node), bpos, blen);

<<<<<<< HEAD
	aml_lockfield(scope, field);
	switch (field->v_field.type) {
	case AMLOP_INDEXFIELD:
		/* Set Index */
		memcpy(&tf, field->v_field.ref2, sizeof(struct aml_value));
		tf.v_field.bitpos += (bpos & 7);
		tf.v_field.bitlen  = blen;

		aml_setvalue(scope, pop, NULL, bpos>>3);
		aml_fieldio(scope, &tf, res, mode);
#ifdef ACPI_DEBUG
		dnprintf(55, "-- post indexfield %x,%x @ %x,%x\n", 
		    bpos & 3, blen, 
		    field->v_field.ref2->v_field.bitpos, 
		    field->v_field.ref2->v_field.bitlen);

		iobuf = aml_getbuffer(res, &aligned);
		aml_dump(aligned >> 3, iobuf);
#endif
		break;
	case AMLOP_BANKFIELD:
		/* Set Bank */
		memcpy(&tf, field->v_field.ref2, sizeof(struct aml_value));
		tf.v_field.bitpos += (bpos & 7);
		tf.v_field.bitlen  = blen;

		aml_setvalue(scope, pop, NULL, field->v_field.ref3);
		aml_fieldio(scope, &tf, res, mode);
#ifdef ACPI_DEBUG
		dnprintf(55, "-- post bankfield %x,%x @ %x,%x\n", 
		    bpos & 3, blen, 
		    field->v_field.ref2->v_field.bitpos, 
		    field->v_field.ref2->v_field.bitlen);

		iobuf = aml_getbuffer(res, &aligned);
		aml_dump(aligned >> 3, iobuf);
#endif
		break;
	case AMLOP_FIELD:
		/* This is an I/O field */
		if (pop->type != AML_OBJTYPE_OPREGION)
			aml_die("Not an opregion!\n");

		/* Get field access size */
		switch (AML_FIELD_ACCESS(field->v_field.flags)) {
		case AML_FIELD_ANYACC:
		case AML_FIELD_BYTEACC:
			mask = 7;
			break;
		case AML_FIELD_WORDACC:
			mask = 15;
			break;
		case AML_FIELD_DWORDACC:
			mask = 31;
			break;
		case AML_FIELD_QWORDACC:
			mask = 63;
			break;
		}

		/* Pre-allocate return value for reads */
		if (mode == ACPI_IOREAD)
			_aml_setvalue(res, AML_OBJTYPE_BUFFER,
			    (field->v_field.bitlen+7)>>3, NULL);
		
		/* Get aligned bitpos/bitlength */
		blen = ((bpos & mask) + blen + mask) & ~mask;
		bpos = bpos & ~mask;
		aligned = (bpos == field->v_field.bitpos &&
		    blen == field->v_field.bitlen);
		iobase = pop->v_opregion.iobase;

		/* Check for aligned reads/writes */
		if (aligned) {
			iobuf = aml_getbuffer(res, &aligned);
			aml_gasio(scope->sc, pop->v_opregion.iospace,
			    iobase, pop->v_opregion.iolen, bpos, blen,
			    mask + 1, iobuf, mode); 
#ifdef ACPI_DEBUG
			dnprintf(55, "aligned: %s @ %.4x:%.4x + %.4x\n",
			    mode == ACPI_IOREAD ? "rd" : "wr", 
			    bpos, blen, aligned);

			aml_dump(blen >> 3, iobuf);
#endif
		}
		else if (mode == ACPI_IOREAD) {
			iobuf = acpi_os_malloc(blen>>3);
			aml_gasio(scope->sc, pop->v_opregion.iospace,
			    iobase, pop->v_opregion.iolen, bpos, blen,
			    mask + 1, iobuf, mode);

			/* ASSERT: res is buffer type as it was set above */
			aml_bufcpy(res->v_buffer, 0, iobuf, 
			    field->v_field.bitpos & mask,
			    field->v_field.bitlen);

#ifdef ACPI_DEBUG
			dnprintf(55,"non-aligned read: %.4x:%.4x : ", 
			    field->v_field.bitpos & mask,
			    field->v_field.bitlen);

			aml_dump(blen >> 3, iobuf);
			dnprintf(55,"post-read: ");
			aml_dump((field->v_field.bitlen+7)>>3, res->v_buffer);
#endif
			acpi_os_free(iobuf);
		}
		else {
			iobuf = acpi_os_malloc(blen >> 3);
			switch (AML_FIELD_UPDATE(field->v_field.flags)) {
			case AML_FIELD_WRITEASONES:
				memset(iobuf, 0xFF, blen >> 3);
				break;
			case AML_FIELD_PRESERVE:
				aml_gasio(scope->sc, pop->v_opregion.iospace,
				    iobase, pop->v_opregion.iolen, bpos, blen,
				    mask + 1, iobuf, ACPI_IOREAD);
				break;
			}
			/* Copy into IOBUF */
			iobuf2 = aml_getbuffer(res, &aligned);
			aml_bufcpy(iobuf, field->v_field.bitpos & mask,
			    iobuf2, 0, field->v_field.bitlen);

#ifdef ACPI_DEBUG
			dnprintf(55,"non-aligned write: %.4x:%.4x : ", 
			    field->v_field.bitpos & mask,
			    field->v_field.bitlen);
=======
	/* Release lock */
	st = acpi_release_global_lock(&acpi_softc->sc_facs->global_lock);
	if (!st)
		return;

	/* Signal others if someone waiting */
	s = spltty();
	x = acpi_read_pmreg(acpi_softc, ACPIREG_PM1_CNT, 0);
	x |= ACPI_PM1_GBL_RLS;
	acpi_write_pmreg(acpi_softc, ACPIREG_PM1_CNT, 0, x);
	splx(s);
>>>>>>> origin/master

			aml_dump(blen >> 3, iobuf);
#endif
			aml_gasio(scope->sc, pop->v_opregion.iospace,
			    iobase, pop->v_opregion.iolen, bpos, blen,
			    mask + 1, iobuf, mode);

			acpi_os_free(iobuf);
		}
		/* Verify that I/O is in range */
#if 0
		/*
		 * XXX: some I/O ranges are on dword boundaries, but their
		 * length is incorrect eg. dword access, but length of
		 * opregion is 2 bytes.
		 */
		if ((bpos+blen) >= (pop->v_opregion.iolen * 8)) {
			aml_die("Out of bounds I/O!!! region:%x:%llx:%x %x\n",
			    pop->v_opregion.iospace, pop->v_opregion.iobase,
			    pop->v_opregion.iolen, bpos+blen);
		}
#endif
		break;
	default:
		/* This is a buffer field */
		if (mode == ACPI_IOREAD)
			aml_setbufint(res, bpos, blen, pop);
		else
			aml_getbufint(res, bpos, blen, pop);
		break;
	}
	aml_unlockfield(scope, field);
}

/*
 * @@@: Value set/compare/alloc/free routines
 */
<<<<<<< HEAD
int64_t aml_str2int(const char *, int);
struct aml_value *aml_derefvalue(struct aml_scope *, struct aml_value *, int);
#define aml_dereftarget(s, v)	aml_derefvalue(s, v, ACPI_IOWRITE)
#define aml_derefterm(s, v, m)	aml_derefvalue(s, v, ACPI_IOREAD)
=======
>>>>>>> origin/master

void
aml_showvalue(struct aml_value *val, int lvl)
{
	int idx;

	if (val == NULL)
		return;

	if (val->node)
		printf(" [%s]", aml_nodename(val->node));
	printf(" %p cnt:%.2x stk:%.2x", val, val->refcnt, val->stack);
	switch (val->type) {
	case AML_OBJTYPE_INTEGER:
		printf(" integer: %llx\n", val->v_integer);
		break;
	case AML_OBJTYPE_STRING:
		printf(" string: %s\n", val->v_string);
		break;
	case AML_OBJTYPE_METHOD:
		printf(" method: %.2x\n", val->v_method.flags);
		break;
	case AML_OBJTYPE_PACKAGE:
		printf(" package: %.2x\n", val->length);
		for (idx = 0; idx < val->length; idx++)
			aml_showvalue(val->v_package[idx], lvl);
		break;
	case AML_OBJTYPE_BUFFER:
		printf(" buffer: %.2x {", val->length);
		for (idx = 0; idx < val->length; idx++)
			printf("%s%.2x", idx ? ", " : "", val->v_buffer[idx]);
		printf("}\n");
		break;
	case AML_OBJTYPE_FIELDUNIT:
	case AML_OBJTYPE_BUFFERFIELD:
		printf(" field: bitpos=%.4x bitlen=%.4x ref1:%x ref2:%x [%s]\n",
		    val->v_field.bitpos, val->v_field.bitlen,
		    val->v_field.ref1, val->v_field.ref2,
		    aml_mnem(val->v_field.type, NULL));
		aml_showvalue(val->v_field.ref1, lvl);
		aml_showvalue(val->v_field.ref2, lvl);
		break;
	case AML_OBJTYPE_MUTEX:
		printf(" mutex: %s ref: %d\n",
		    val->v_mutex ?  val->v_mutex->amt_name : "",
		    val->v_mutex ?  val->v_mutex->amt_ref_count : 0);
		break;
	case AML_OBJTYPE_EVENT:
		printf(" event:\n");
		break;
	case AML_OBJTYPE_OPREGION:
		printf(" opregion: %.2x,%.8llx,%x\n",
		    val->v_opregion.iospace, val->v_opregion.iobase,
		    val->v_opregion.iolen);
		break;
	case AML_OBJTYPE_NAMEREF:
		printf(" nameref: %s\n", aml_getname(val->v_nameref));
		break;
	case AML_OBJTYPE_DEVICE:
		printf(" device:\n");
		break;
	case AML_OBJTYPE_PROCESSOR:
		printf(" cpu: %.2x,%.4x,%.2x\n",
		    val->v_processor.proc_id, val->v_processor.proc_addr,
		    val->v_processor.proc_len);
		break;
	case AML_OBJTYPE_THERMZONE:
		printf(" thermzone:\n");
		break;
	case AML_OBJTYPE_POWERRSRC:
		printf(" pwrrsrc: %.2x,%.2x\n",
		    val->v_powerrsrc.pwr_level, val->v_powerrsrc.pwr_order);
		break;
	case AML_OBJTYPE_OBJREF:
		printf(" objref: %p index:%x\n", val->v_objref.ref,
		    val->v_objref.index);
		aml_showvalue(val->v_objref.ref, lvl);
		break;
	default:
		printf(" !!type: %x\n", val->type);
	}
}

/* Perform DeRef on value. If ACPI_IOREAD, will perform buffer/IO field read */
struct aml_value *
aml_derefvalue(struct aml_scope *scope, struct aml_value *ref, int mode)
{
	struct aml_node *node;
	struct aml_value *tmp;
	int64_t tmpint;
	int argc, index;

	for (;;) {
		switch (ref->type) {
		case AML_OBJTYPE_NAMEREF:
			node = aml_searchname(scope->node, ref->v_nameref);
			if (node == NULL || node->value == NULL)
				return ref;
			ref = node->value;
			break;

		case AML_OBJTYPE_OBJREF:
			index = ref->v_objref.index;
			ref = aml_dereftarget(scope, ref->v_objref.ref);
			if (index != -1) {
				if (index >= ref->length)
					aml_die("index.buf out of bounds: "
					    "%d/%d\n", index, ref->length);
				switch (ref->type) {
				case AML_OBJTYPE_PACKAGE:
					ref = ref->v_package[index];
					break;
				case AML_OBJTYPE_STATICINT:
				case AML_OBJTYPE_INTEGER:
					/* Convert to temporary buffer */
					if (ref->node)
						aml_die("named integer index\n");
					tmpint = ref->v_integer;
					_aml_setvalue(ref, AML_OBJTYPE_BUFFER,
					    aml_intlen>>3, &tmpint);
					/* FALLTHROUGH */
				case AML_OBJTYPE_BUFFER:
				case AML_OBJTYPE_STRING:
					/* Return contents at this index */
					tmp = aml_alloctmp(scope, 1);
					if (mode == ACPI_IOREAD) {
						/* Shortcut: return integer
						 * contents of buffer at index */
						_aml_setvalue(tmp,
						    AML_OBJTYPE_INTEGER,
						    ref->v_buffer[index], NULL);
					} else {
						_aml_setvalue(tmp,
						    AML_OBJTYPE_BUFFERFIELD,
						    0, NULL);
						tmp->v_field.type =
						    AMLOP_CREATEBYTEFIELD;
						tmp->v_field.bitpos = index * 8;
						tmp->v_field.bitlen = 8;
						tmp->v_field.ref1 = ref;
						aml_addref(ref);
					}
					return tmp;
				default:
					aml_die("unknown index type: %d", ref->type);
					break;
				}
			}
			break;

		case AML_OBJTYPE_METHOD:
			/* Read arguments from current scope */
			argc = AML_METHOD_ARGCOUNT(ref->v_method.flags);
			tmp = aml_alloctmp(scope, argc+1);
			for (index = 0; index < argc; index++) {
				aml_parseop(scope, &tmp[index]);
				aml_addref(&tmp[index]);
			}
			ref = aml_evalmethod(scope, ref->node, argc, tmp, &tmp[argc]);
			break;

		case AML_OBJTYPE_BUFFERFIELD:
		case AML_OBJTYPE_FIELDUNIT:
			if (mode == ACPI_IOREAD) {
				/* Read I/O field into temporary storage */
				tmp = aml_alloctmp(scope, 1);
				aml_fieldio(scope, ref, tmp, ACPI_IOREAD);
				return tmp;
			}
			return ref;

		default:
			return ref;
		}

	}
}

int64_t
<<<<<<< HEAD
aml_str2int(const char *str, int radix)
{
	/* XXX: fixme */
	return 0;
}

int64_t
=======
>>>>>>> origin/master
aml_val2int(struct aml_value *rval)
{
	int64_t ival = 0;

	if (rval == NULL) {
		dnprintf(50, "null val2int\n");
		return (0);
	}
	switch (rval->type) {
	case AML_OBJTYPE_INTEGER:
		ival = rval->v_integer;
		break;
	case AML_OBJTYPE_BUFFER:
		aml_bufcpy(&ival, 0, rval->v_buffer, 0,
		    min(aml_intlen, rval->length*8));
		break;
	case AML_OBJTYPE_STRING:
		ival = aml_hextoint(rval->v_string);
		break;
	}
	return (ival);
}

/* Sets value into LHS: lhs must already be cleared */
struct aml_value *
_aml_setvalue(struct aml_value *lhs, int type, int64_t ival, const void *bval)
{
	memset(&lhs->_, 0x0, sizeof(lhs->_));

	lhs->type = type;
	switch (lhs->type) {
	case AML_OBJTYPE_INTEGER:
		lhs->length = aml_intlen>>3;
		lhs->v_integer = ival;
		break;
	case AML_OBJTYPE_METHOD:
		lhs->v_method.flags = ival;
		lhs->v_method.fneval = bval;
		break;
	case AML_OBJTYPE_NAMEREF:
		lhs->v_nameref = (uint8_t *)bval;
		break;
	case AML_OBJTYPE_OBJREF:
		lhs->v_objref.type = ival;
		lhs->v_objref.ref = (struct aml_value *)bval;
		break;
	case AML_OBJTYPE_BUFFER:
		lhs->length = ival;
		lhs->v_buffer = (uint8_t *)acpi_os_malloc(ival);
		if (bval)
			memcpy(lhs->v_buffer, bval, ival);
		break;
	case AML_OBJTYPE_STRING:
		if (ival == -1)
			ival = strlen((const char *)bval);
		lhs->length = ival;
		lhs->v_string = (char *)acpi_os_malloc(ival+1);
		if (bval)
			strncpy(lhs->v_string, (const char *)bval, ival);
		break;
	case AML_OBJTYPE_PACKAGE:
		lhs->length = ival;
		lhs->v_package = (struct aml_value **)acpi_os_malloc(ival *
		    sizeof(struct aml_value *));
		for (ival = 0; ival < lhs->length; ival++)
			lhs->v_package[ival] = aml_allocvalue(
			    AML_OBJTYPE_UNINITIALIZED, 0, NULL);
		break;
	}
	return lhs;
}

/* Copy object to another value: lhs must already be cleared */
void
aml_copyvalue(struct aml_value *lhs, struct aml_value *rhs)
{
	int idx;

	lhs->type = rhs->type;
	switch (lhs->type) {
	case AML_OBJTYPE_UNINITIALIZED:
		break;
	case AML_OBJTYPE_INTEGER:
		lhs->length = aml_intlen>>3;
		lhs->v_integer = rhs->v_integer;
		break;
	case AML_OBJTYPE_MUTEX:
		lhs->v_mutex = rhs->v_mutex;
		break;
	case AML_OBJTYPE_POWERRSRC:
		lhs->v_powerrsrc = rhs->v_powerrsrc;
		break;
	case AML_OBJTYPE_METHOD:
		lhs->v_method = rhs->v_method;
		break;
	case AML_OBJTYPE_BUFFER:
		_aml_setvalue(lhs, rhs->type, rhs->length, rhs->v_buffer);
		break;
	case AML_OBJTYPE_STRING:
		_aml_setvalue(lhs, rhs->type, rhs->length, rhs->v_string);
		break;
	case AML_OBJTYPE_OPREGION:
		lhs->v_opregion = rhs->v_opregion;
		break;
	case AML_OBJTYPE_PROCESSOR:
		lhs->v_processor = rhs->v_processor;
		break;
	case AML_OBJTYPE_NAMEREF:
		lhs->v_nameref = rhs->v_nameref;
		break;
	case AML_OBJTYPE_PACKAGE:
		_aml_setvalue(lhs, rhs->type, rhs->length, NULL);
		for (idx = 0; idx < rhs->length; idx++)
			aml_copyvalue(lhs->v_package[idx], rhs->v_package[idx]);
		break;
	case AML_OBJTYPE_OBJREF:
		lhs->v_objref = rhs->v_objref;
		aml_xaddref(lhs->v_objref.ref, "");
		break;
	default:
		printf("copyvalue: %x", rhs->type);
		break;
	}
}

int is_local(struct aml_scope *, struct aml_value *);

<<<<<<< HEAD
int is_local(struct aml_scope *scope, struct aml_value *val)
{
	return val->stack;
=======
	rv = (struct aml_value *)acpi_os_malloc(sizeof(struct aml_value));
	if (rv != NULL) {
		aml_xaddref(rv, "");
		return _aml_setvalue(rv, type, ival, bval);
	}
	return NULL;
>>>>>>> origin/master
}

/* Guts of the code: Assign one value to another.  LHS may contain a previous value */
void
aml_setvalue(struct aml_scope *scope, struct aml_value *lhs,
    struct aml_value *rhs, int64_t ival)
{
	struct aml_value tmpint;

	/* Use integer as result */
	if (rhs == NULL) {
		memset(&tmpint, 0, sizeof(tmpint));
		rhs = _aml_setvalue(&tmpint, AML_OBJTYPE_INTEGER, ival, NULL);
	}

	if (is_local(scope, lhs)) {
		/* ACPI: Overwrite writing to LocalX */
		aml_freevalue(lhs);
	}
	else {
		lhs = aml_dereftarget(scope, lhs);
	}

	switch (lhs->type) {
	case AML_OBJTYPE_UNINITIALIZED:
		aml_copyvalue(lhs, rhs);
		break;
	case AML_OBJTYPE_BUFFERFIELD:
	case AML_OBJTYPE_FIELDUNIT:
		aml_fieldio(scope, lhs, rhs, ACPI_IOWRITE);
		break;
	case AML_OBJTYPE_DEBUGOBJ:
#ifdef ACPI_DEBUG
		printf("-- debug --\n");
		aml_showvalue(rhs, 50);
#endif
		break;
	case AML_OBJTYPE_STATICINT:
		if (lhs->node) {
			lhs->v_integer = aml_val2int(rhs);
		}
		break;
	case AML_OBJTYPE_INTEGER:
		lhs->v_integer = aml_val2int(rhs);
		break;
	case AML_OBJTYPE_BUFFER:
		if (lhs->node)
			dnprintf(40, "named.buffer\n");
		aml_freevalue(lhs);
		if (rhs->type == AML_OBJTYPE_BUFFER)
			_aml_setvalue(lhs, AML_OBJTYPE_BUFFER, rhs->length,
			    rhs->v_buffer);
		else if (rhs->type == AML_OBJTYPE_INTEGER ||
			    rhs->type == AML_OBJTYPE_STATICINT)
			_aml_setvalue(lhs, AML_OBJTYPE_BUFFER,
			    sizeof(rhs->v_integer), &rhs->v_integer);
		else if (rhs->type == AML_OBJTYPE_STRING)
			_aml_setvalue(lhs, AML_OBJTYPE_BUFFER, rhs->length+1,
			    rhs->v_string);
		else {
			/* aml_showvalue(rhs); */
			aml_die("setvalue.buf : %x", aml_pc(scope->pos));
		}
		break;
	case AML_OBJTYPE_STRING:
		if (lhs->node)
			dnprintf(40, "named string\n");
		aml_freevalue(lhs);
		if (rhs->type == AML_OBJTYPE_STRING)
			_aml_setvalue(lhs, AML_OBJTYPE_STRING, rhs->length,
			    rhs->v_string);
		else if (rhs->type == AML_OBJTYPE_BUFFER)
			_aml_setvalue(lhs, AML_OBJTYPE_STRING, rhs->length,
			    rhs->v_buffer);
		else if (rhs->type == AML_OBJTYPE_INTEGER || rhs->type == AML_OBJTYPE_STATICINT) {
			_aml_setvalue(lhs, AML_OBJTYPE_STRING, 10, NULL);
			snprintf(lhs->v_string, lhs->length, "%lld",
			    rhs->v_integer);
		} else {
			//aml_showvalue(rhs);
			aml_die("setvalue.str");
		}
		break;
	default:
		/* XXX: */
		dnprintf(10, "setvalue.unknown: %x", lhs->type);
		break;
	}
}

/* Allocate dynamic AML value
 *   type : Type of object to allocate (AML_OBJTYPE_XXXX)
 *   ival : Integer value (action depends on type)
 *   bval : Buffer value (action depends on type)
 */
struct aml_value *
aml_allocvalue(int type, int64_t ival, const void *bval)
{
	struct aml_value *rv;

	rv = (struct aml_value *)acpi_os_malloc(sizeof(struct aml_value));
	if (rv != NULL) {
		aml_addref(rv);
		return _aml_setvalue(rv, type, ival, bval);
	}
	return NULL;
}

void
aml_freevalue(struct aml_value *val)
{
	int idx;

	if (val == NULL)
		return;
	switch (val->type) {
	case AML_OBJTYPE_STRING:
		acpi_os_free(val->v_string);
		break;
	case AML_OBJTYPE_BUFFER:
		acpi_os_free(val->v_buffer);
		break;
	case AML_OBJTYPE_PACKAGE:
		for (idx = 0; idx < val->length; idx++) {
			aml_freevalue(val->v_package[idx]);
			acpi_os_free(val->v_package[idx]);
		}
		acpi_os_free(val->v_package);
		break;
	case AML_OBJTYPE_OBJREF:
		aml_xdelref(&val->v_objref.ref, "");
		break;
	case AML_OBJTYPE_BUFFERFIELD:
	case AML_OBJTYPE_FIELDUNIT:
		aml_xdelref(&val->v_field.ref1, "");
		aml_xdelref(&val->v_field.ref2, "");
		break;
	}
	val->type = 0;
	memset(&val->_, 0, sizeof(val->_));
}

/*
 * @@@: Math eval routines
 */

/* Convert number from one radix to another
 * Used in BCD conversion routines */
u_int64_t
aml_convradix(u_int64_t val, int iradix, int oradix)
{
	u_int64_t rv = 0, pwr;

	rv = 0;
	pwr = 1;
	while (val) {
		rv += (val % iradix) * pwr;
		val /= iradix;
		pwr *= oradix;
	}
	return rv;
}

/* Calculate LSB */
int
aml_lsb(u_int64_t val)
{
	int		lsb;

	if (val == 0)
		return (0);

	for (lsb = 1; !(val & 0x1); lsb++)
		val >>= 1;

	return (lsb);
}

/* Calculate MSB */
int
aml_msb(u_int64_t val)
{
	int		msb;

	if (val == 0)
		return (0);

	for (msb = 1; val != 0x1; msb++)
		val >>= 1;

	return (msb);
}

/* Evaluate Math operands */
int64_t
aml_evalexpr(int64_t lhs, int64_t rhs, int opcode)
{
	int64_t res;

	switch (opcode) {
		/* Math operations */
	case AMLOP_INCREMENT:
	case AMLOP_ADD:
		res = (lhs + rhs);
		break;
	case AMLOP_DECREMENT:
	case AMLOP_SUBTRACT:
		res = (lhs - rhs);
		break;
	case AMLOP_MULTIPLY:
		res = (lhs * rhs);
		break;
	case AMLOP_DIVIDE:
		res = (lhs / rhs);
		break;
	case AMLOP_MOD:
		res = (lhs % rhs);
		break;
	case AMLOP_SHL:
		res = (lhs << rhs);
		break;
	case AMLOP_SHR:
		res = (lhs >> rhs);
		break;
	case AMLOP_AND:
		res = (lhs & rhs);
		break;
	case AMLOP_NAND:
		res = ~(lhs & rhs);
		break;
	case AMLOP_OR:
		res = (lhs | rhs);
		break;
	case AMLOP_NOR:
		res = ~(lhs | rhs);
		break;
	case AMLOP_XOR:
		res = (lhs ^ rhs);
		break;
	case AMLOP_NOT:
		res = ~(lhs);
		break;

		/* Conversion/misc */
	case AMLOP_FINDSETLEFTBIT:
		res = aml_msb(lhs);
		break;
	case AMLOP_FINDSETRIGHTBIT:
		res = aml_lsb(lhs);
		break;
	case AMLOP_TOINTEGER:
		res = (lhs);
		break;
	case AMLOP_FROMBCD:
		res = aml_convradix(lhs, 16, 10);
		break;
	case AMLOP_TOBCD:
		res = aml_convradix(lhs, 10, 16);
		break;

		/* Logical/Comparison */
	case AMLOP_LAND:
		res = (lhs && rhs);
		break;
	case AMLOP_LOR:
		res = (lhs || rhs);
		break;
	case AMLOP_LNOT:
		res = (!lhs);
		break;
	case AMLOP_LNOTEQUAL:
		res = (lhs != rhs);
		break;
	case AMLOP_LLESSEQUAL:
		res = (lhs <= rhs);
		break;
	case AMLOP_LGREATEREQUAL:
		res = (lhs >= rhs);
		break;
	case AMLOP_LEQUAL:
		res = (lhs == rhs);
		break;
	case AMLOP_LGREATER:
		res = (lhs > rhs);
		break;
	case AMLOP_LLESS:
		res = (lhs < rhs);
		break;
	}

	dnprintf(50,"aml_evalexpr: %s %llx %llx = %llx\n",
		 aml_mnem(opcode, NULL), lhs, rhs, res);

	return res;
}

<<<<<<< HEAD
int
aml_cmpvalue(struct aml_value *lhs, struct aml_value *rhs, int opcode)
{
	int rc, lt, rt;

	rc = 0;
	lt = lhs->type & ~AML_STATIC;
	rt = rhs->type & ~AML_STATIC;
	if (lt == rt) {
		switch (lt) {
		case AML_OBJTYPE_STATICINT:
		case AML_OBJTYPE_INTEGER:
			rc = (lhs->v_integer - rhs->v_integer);
			break;
		case AML_OBJTYPE_STRING:
			rc = strncmp(lhs->v_string, rhs->v_string,
			    min(lhs->length, rhs->length));
			if (rc == 0)
				rc = lhs->length - rhs->length;
			break;
		case AML_OBJTYPE_BUFFER:
			rc = memcmp(lhs->v_buffer, rhs->v_buffer,
			    min(lhs->length, rhs->length));
			if (rc == 0)
				rc = lhs->length - rhs->length;
			break;
		}
	} else if (lt == AML_OBJTYPE_INTEGER) {
		rc = lhs->v_integer - aml_val2int(rhs);
	} else if (rt == AML_OBJTYPE_INTEGER) {
		rc = aml_val2int(lhs) - rhs->v_integer;
	} else {
		aml_die("mismatched compare\n");
	}
	return aml_evalexpr(rc, 0, opcode);
}

=======
>>>>>>> origin/master
/*
 * aml_bufcpy copies/shifts buffer data, special case for aligned transfers
 * dstPos/srcPos are bit positions within destination/source buffers
 */
void
aml_bufcpy(void *pvDst, int dstPos, const void *pvSrc, int srcPos, int len)
{
	const u_int8_t *pSrc = pvSrc;
	u_int8_t *pDst = pvDst;
	int		idx;

	if (aml_bytealigned(dstPos|srcPos|len)) {
		/* Aligned transfer: use memcpy */
		memcpy(pDst+aml_bytepos(dstPos), pSrc+aml_bytepos(srcPos),
		    aml_bytelen(len));
		return;
	}

	/* Misaligned transfer: perform bitwise copy (slow) */
	for (idx = 0; idx < len; idx++)
		aml_setbit(pDst, idx + dstPos, aml_tstbit(pSrc, idx + srcPos));
}

struct aml_value *
aml_callmethod(struct aml_scope *scope, struct aml_value *val)
{
	while (scope->pos < scope->end)
		aml_parseterm(scope, val);
	return val;
}

/*
 * Evaluate an AML method
 *
 * Returns a copy of the result in res (must be freed by user)
 */
struct aml_value *
aml_evalmethod(struct aml_scope *parent, struct aml_node *node,
    int argc, struct aml_value *argv, struct aml_value *res)
{
	struct aml_scope *scope;

	scope = aml_pushscope(parent, node->value->v_method.start,
	    node->value->v_method.end, node);
	scope->args = argv;
	scope->nargs = argc;

	if (res == NULL)
		res = aml_alloctmp(scope, 1);

#ifdef ACPI_DEBUG
	dnprintf(10, "calling [%s] (%d args)\n",
	    aml_nodename(node), scope->nargs);
	for (argc = 0; argc < scope->nargs; argc++) {
		dnprintf(10, "  arg%d: ", argc);
		aml_showvalue(&scope->args[argc], 10);
	}
	node->value->v_method.fneval(scope, res);
	dnprintf(10, "[%s] returns: ", aml_nodename(node));
	aml_showvalue(res, 10);
#else
	node->value->v_method.fneval(scope, res);
#endif
	/* Free any temporary children nodes */
	aml_delchildren(node);
	aml_popscope(scope);

	return res;
}

/*
 * @@@: External API
 *
 * evaluate an AML node
 * Returns a copy of the value in res  (must be freed by user)
 */
int
aml_evalnode(struct acpi_softc *sc, struct aml_node *node,
    int argc, struct aml_value *argv, struct aml_value *res)
{
	static int lastck;
	struct aml_node *ref;

	if (res)
		memset(res, 0, sizeof(struct aml_value));
	if (node == NULL || node->value == NULL)
		return (ACPI_E_BADVALUE);

	switch (node->value->type) {
	case AML_OBJTYPE_METHOD:
		aml_evalmethod(NULL, node, argc, argv, res);
		if (acpi_nalloc > lastck) {
			/* Check if our memory usage has increased */
			dnprintf(10, "Leaked: [%s] %d\n",
			    aml_nodename(node), acpi_nalloc);
			lastck = acpi_nalloc;
		}
		break;
	case AML_OBJTYPE_STATICINT:
	case AML_OBJTYPE_INTEGER:
	case AML_OBJTYPE_STRING:
	case AML_OBJTYPE_BUFFER:
	case AML_OBJTYPE_PACKAGE:
	case AML_OBJTYPE_EVENT:
	case AML_OBJTYPE_DEVICE:
	case AML_OBJTYPE_MUTEX:
	case AML_OBJTYPE_OPREGION:
	case AML_OBJTYPE_POWERRSRC:
	case AML_OBJTYPE_PROCESSOR:
	case AML_OBJTYPE_THERMZONE:
	case AML_OBJTYPE_DEBUGOBJ:
		if (res)
			aml_copyvalue(res, node->value);
		break;
	case AML_OBJTYPE_NAMEREF:
		if (res == NULL)
			break;
		if ((ref = aml_searchname(node, node->value->v_nameref)) != NULL)
			_aml_setvalue(res, AML_OBJTYPE_OBJREF, -1, ref);
		else
			aml_copyvalue(res, node->value);
		break;
	default:
		break;
	}
	return (0);
}

/*
 * evaluate an AML name
 * Returns a copy of the value in res  (must be freed by user)
 */
int
aml_evalname(struct acpi_softc *sc, struct aml_node *parent, const char *name,
    int argc, struct aml_value *argv, struct aml_value *res)
{
	return aml_evalnode(sc, aml_searchname(parent, name), argc, argv, res);
}

int
aml_evalinteger(struct acpi_softc *sc, struct aml_node *parent,
    const char *name, int argc, struct aml_value *argv, int64_t *ival)
{
	struct aml_value res;

	if (name != NULL)
		parent = aml_searchname(parent, name);
	if (aml_evalnode(sc, parent, argc, argv, &res) == 0) {
		*ival = aml_val2int(&res);
		aml_freevalue(&res);
		return 0;
	}
	return 1;
}

void
aml_walknodes(struct aml_node *node, int mode,
    int (*nodecb)(struct aml_node *, void *), void *arg)
{
	struct aml_node *child;

	if (node == NULL)
		return;
	if (mode == AML_WALK_PRE)
		if (nodecb(node, arg))
			return;
	SIMPLEQ_FOREACH(child, &node->son, sib)
		aml_walknodes(child, mode, nodecb, arg);
	if (mode == AML_WALK_POST)
		nodecb(node, arg);
}

void
aml_walktree(struct aml_node *node)
{
	while (node) {
		aml_showvalue(node->value, 0);
		aml_walktree(node->child);
		node = node->sibling;
	}
}

void
aml_walkroot(void)
{
	aml_walktree(aml_root.child);
}

int
aml_find_node(struct aml_node *node, const char *name,
    void (*cbproc)(struct aml_node *, void *arg), void *arg)
{
	const char *nn;

	while (node) {
		if ((nn = node->name) != NULL) {
			if (*nn == AMLOP_ROOTCHAR) nn++;
			while (*nn == AMLOP_PARENTPREFIX) nn++;
			if (!strcmp(name, nn))
				cbproc(node, arg);
		}
<<<<<<< HEAD
		aml_find_node(node->child, name, cbproc, arg);
		node = node->sibling;
=======
		/* Only recurse if cbproc() wants us to */
		if (!st)
			aml_find_node(SIMPLEQ_FIRST(&node->son), name, cbproc, arg);
		node = SIMPLEQ_NEXT(node, sib);
>>>>>>> origin/master
	}
	return (0);
}

/*
 * @@@: Parser functions
 */
uint8_t *aml_parsename(struct aml_node *, uint8_t *, struct aml_value **, int);
uint8_t *aml_parseend(struct aml_scope *scope);
int	aml_parselength(struct aml_scope *);
int	aml_parseopcode(struct aml_scope *);

/* Get AML Opcode */
int
aml_parseopcode(struct aml_scope *scope)
{
	int opcode = (scope->pos[0]);
	int twocode = (scope->pos[0]<<8) + scope->pos[1];

	/* Check if this is an embedded name */
	switch (opcode) {
	case AMLOP_ROOTCHAR:
	case AMLOP_PARENTPREFIX:
	case AMLOP_MULTINAMEPREFIX:
	case AMLOP_DUALNAMEPREFIX:
	case AMLOP_NAMECHAR:
<<<<<<< HEAD
	case 'A' ... 'Z':
=======
		return AMLOP_NAMECHAR;
	}
	if (opcode >= 'A' && opcode <= 'Z')
>>>>>>> origin/master
		return AMLOP_NAMECHAR;
	if (twocode == AMLOP_LNOTEQUAL || twocode == AMLOP_LLESSEQUAL ||
	    twocode == AMLOP_LGREATEREQUAL || opcode == AMLOP_EXTPREFIX) {
		scope->pos += 2;
		return twocode;
	}
	scope->pos += 1;
	return opcode;
}

/* Decode embedded AML Namestring */
uint8_t *
aml_parsename(struct aml_node *inode, uint8_t *pos, struct aml_value **rval, int create)
{
	struct aml_node *relnode, *node = inode;
	uint8_t	*start = pos;
	int i;

	if (*pos == AMLOP_ROOTCHAR) {
		pos++;
		node = &aml_root;
	}
	while (*pos == AMLOP_PARENTPREFIX) {
		pos++;
		if ((node = node->parent) == NULL)
			node = &aml_root;
	}
	switch (*pos) {
	case 0x00:
		pos++;
		break;
	case AMLOP_MULTINAMEPREFIX:
		for (i=0; i<pos[1]; i++)
			node = __aml_search(node, pos+2+i*AML_NAMESEG_LEN,
			    create);
		pos += 2+i*AML_NAMESEG_LEN;
		break;
	case AMLOP_DUALNAMEPREFIX:
		node = __aml_search(node, pos+1, create);
		node = __aml_search(node, pos+1+AML_NAMESEG_LEN, create);
		pos += 1+2*AML_NAMESEG_LEN;
		break;
	default:
		/* If Relative Search (pos == start), recursively go up root */
		relnode = node;
		do {
			node = __aml_search(relnode, pos, create);
			relnode = relnode->parent;
		} while (!node && pos == start && relnode);
		pos += AML_NAMESEG_LEN;
		break;
	}
	if (node) {
		*rval = node->value;

		/* Dereference ALIAS here */
		if ((*rval)->type == AML_OBJTYPE_OBJREF &&
		    (*rval)->v_objref.type == AMLOP_ALIAS) {
			dnprintf(10, "deref alias: %s\n", aml_nodename(node));
			*rval = (*rval)->v_objref.ref;
		}
		aml_xaddref(*rval, 0);

		dnprintf(10, "parsename: %s %x\n", aml_nodename(node),
		    (*rval)->type);
	} else {
		*rval = aml_allocvalue(AML_OBJTYPE_NAMEREF, 0, start);

		dnprintf(10, "%s:%s not found\n", aml_nodename(inode),
		    aml_getname(start));
	}

	return pos;
}

/* Decode AML Length field */
int
aml_parselength(struct aml_scope *scope)
{
<<<<<<< HEAD
	int len = (*scope->pos & 0xF);

	switch (*scope->pos >> 6) {
	case 0x00:
		len = scope->pos[0] & 0x3F;
		scope->pos += 1;
		break;
	case 0x01:
		len += (scope->pos[1]<<4L);
		scope->pos += 2;
		break;
	case 0x02:
		len += (scope->pos[1]<<4L) + (scope->pos[2]<<12L);
		scope->pos += 3;
		break;
	case 0x03:
		len += (scope->pos[1]<<4L) + (scope->pos[2]<<12L) +
		    (scope->pos[3]<<20L);
		scope->pos += 4;
		break;
	}
=======
	int len;
	uint8_t lcode;

	lcode = *(scope->pos++);
	if (lcode <= 0x3F)
		return lcode;

	/* lcode >= 0x40, multibyte length, get first byte of extended length */
	len = lcode & 0xF;
	len += *(scope->pos++) << 4L;
	if (lcode >= 0x80)
		len += *(scope->pos++) << 12L;
	if (lcode >= 0xC0)
		len += *(scope->pos++) << 20L;
>>>>>>> origin/master
	return len;
}

/* Get address of end of scope; based on current address */
uint8_t *
aml_parseend(struct aml_scope *scope)
{
	uint8_t *pos = scope->pos;
	int len;

	len = aml_parselength(scope);
	if (pos+len > scope->end) {
		dnprintf(10,
		    "Bad scope... runover pos:%.4x new end:%.4x scope "
		    "end:%.4x\n", aml_pc(pos), aml_pc(pos+len),
		    aml_pc(scope->end));
		pos = scope->end;
	}
	return pos+len;
}

/*
 * @@@: Opcode utility functions
 */
int		aml_match(int, int64_t, struct aml_value *);
void		aml_fixref(struct aml_value **);
int64_t		aml_parseint(struct aml_scope *, int);
void		aml_resize(struct aml_value *val, int newsize);

void
aml_resize(struct aml_value *val, int newsize)
{
	void *oldptr;
	int oldsize;

	if (val->length >= newsize)
		return;
	oldsize = val->length;
	switch (val->type) {
	case AML_OBJTYPE_BUFFER:
		oldptr = val->v_buffer;
		_aml_setvalue(val, val->type, newsize, NULL);
		memcpy(val->v_buffer, oldptr, oldsize);
		acpi_os_free(oldptr);
		break;
	case AML_OBJTYPE_STRING:
		oldptr = val->v_string;
		_aml_setvalue(val, val->type, newsize+1, NULL);
		memcpy(val->v_string, oldptr, oldsize);
		acpi_os_free(oldptr);
		break;
	}
}


int
aml_match(int op, int64_t mv1, struct aml_value *mv2)
{
	struct aml_value tmpint;

	memset(&tmpint, 0, sizeof(tmpint));
	_aml_setvalue(&tmpint, AML_OBJTYPE_INTEGER, mv1, NULL);
	switch (op) {
	case AML_MATCH_EQ:
		return aml_cmpvalue(&tmpint, mv2, AMLOP_LEQUAL);
	case AML_MATCH_LT:
		return aml_cmpvalue(&tmpint, mv2, AMLOP_LLESS);
	case AML_MATCH_LE:
		return aml_cmpvalue(&tmpint, mv2, AMLOP_LLESSEQUAL);
	case AML_MATCH_GE:
		return aml_cmpvalue(&tmpint, mv2, AMLOP_LGREATEREQUAL);
	case AML_MATCH_GT:
		return aml_cmpvalue(&tmpint, mv2, AMLOP_LGREATER);
	}
	return (1);
}

<<<<<<< HEAD
int amlop_delay;

u_int64_t
aml_getpciaddr(struct acpi_softc *sc, struct aml_node *root)
{
	struct aml_value tmpres;
	u_int64_t pciaddr;

	/* PCI */
	pciaddr = 0;
	if (!aml_evalname(dsdt_softc, root, "_ADR", 0, NULL, &tmpres)) {
		/* Device:Function are bits 16-31,32-47 */
		pciaddr += (aml_val2int(&tmpres) << 16L);
		aml_freevalue(&tmpres);
		dnprintf(20, "got _adr [%s]\n", aml_nodename(root));
	} else {
		/* Mark invalid */
		pciaddr += (0xFFFF << 16L);
		return pciaddr;
	}

	if (!aml_evalname(dsdt_softc, root, "_BBN", 0, NULL, &tmpres)) {
		/* PCI bus is in bits 48-63 */
		pciaddr += (aml_val2int(&tmpres) << 48L);
		aml_freevalue(&tmpres);
		dnprintf(20, "got _bbn [%s]\n", aml_nodename(root));
	}
	dnprintf(20, "got pciaddr: %s:%llx\n", aml_nodename(root), pciaddr);
	return pciaddr;
}

/* Fixup references for BufferFields/FieldUnits */
void
aml_fixref(struct aml_value **res)
{
	struct aml_value *oldres;

	while (*res && (*res)->type == AML_OBJTYPE_OBJREF &&
	    (*res)->v_objref.index == -1) {
		oldres = (*res)->v_objref.ref;
		aml_delref(res);
		aml_addref(oldres);
		*res = oldres;
	}
}

int64_t
aml_parseint(struct aml_scope *scope, int opcode)
{
	uint8_t *np = scope->pos;
	struct aml_value *tmpval;
	int64_t rval;
=======
/*
 * @@@: Opcode functions
 */
>>>>>>> origin/master

	if (opcode == AML_ANYINT)
		opcode = aml_parseopcode(scope);
	switch (opcode) {
	case AMLOP_ZERO:
		rval = 0;
		break;
	case AMLOP_ONE:
		rval = 1;
		break;
	case AMLOP_ONES:
		rval = -1;
		break;
	case AMLOP_REVISION:
		rval = AML_REVISION;
		break;
	case AMLOP_BYTEPREFIX:
		np = scope->pos;
		rval = *(uint8_t *)scope->pos;
		scope->pos += 1;
		break;
	case AMLOP_WORDPREFIX:
		np = scope->pos;
		rval = aml_letohost16(*(uint16_t *)scope->pos);
		scope->pos += 2;
		break;
	case AMLOP_DWORDPREFIX:
		np = scope->pos;
		rval = aml_letohost32(*(uint32_t *)scope->pos);
		scope->pos += 4;
		break;
	case AMLOP_QWORDPREFIX:
		np = scope->pos;
		rval = aml_letohost64(*(uint64_t *)scope->pos);
		scope->pos += 8;
		break;
	default:
		scope->pos = np;
		tmpval = aml_alloctmp(scope, 1);
		aml_parseterm(scope, tmpval);
		return aml_val2int(tmpval);
	}
	dnprintf(15, "%.4x: [%s] %s\n", aml_pc(scope->pos-opsize(opcode)),
	    aml_nodename(scope->node), aml_mnem(opcode, np));
	return rval;
}

struct aml_value *
aml_evaltarget(struct aml_scope *scope, struct aml_value *res)
{
	return res;
}

int
aml_evalterm(struct aml_scope *scope, struct aml_value *raw,
    struct aml_value *dst)
{
	struct aml_value *deref;

	aml_freevalue(dst);
	deref = aml_derefterm(scope, raw, 0);
	aml_copyvalue(dst, deref);
	return 0;
}

<<<<<<< HEAD

/*
 * @@@: Opcode functions
 */

/* Parse named objects */
struct aml_value *
aml_parsenamed(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	uint8_t *name;
	int s, offs = 0;

	AML_CHECKSTACK();
	name = aml_parsename(scope);

	res = aml_allocvalue(AML_OBJTYPE_UNINITIALIZED, 0, NULL);
	switch (opcode) {
	case AMLOP_NAME:
		aml_parseop(scope, res);
		break;
	case AMLOP_ALIAS:
		_aml_setvalue(res, AML_OBJTYPE_NAMEREF, 0, name);
		name = aml_parsename(scope);
		break;
	case AMLOP_EVENT:
		_aml_setvalue(res, AML_OBJTYPE_EVENT, 0, NULL);
		break;
	case AMLOP_MUTEX:
		/* XXX mutex is unused since we don't have concurrency */
		_aml_setvalue(res, AML_OBJTYPE_MUTEX, 0, NULL);
		res->v_mutex = (struct acpi_mutex *)acpi_os_malloc(
		    sizeof(struct acpi_mutex));
		res->v_mutex->amt_synclevel = aml_parseint(scope,
		    AMLOP_BYTEPREFIX);
		s = strlen(aml_getname(name));
		if (s > 4)
			offs = s - 4;
		strlcpy(res->v_mutex->amt_name, aml_getname(name) + offs,
		    ACPI_MTX_MAXNAME);
		rw_init(&res->v_mutex->amt_lock, res->v_mutex->amt_name);
		break;
	case AMLOP_OPREGION:
		_aml_setvalue(res, AML_OBJTYPE_OPREGION, 0, NULL);
		res->v_opregion.iospace = aml_parseint(scope, AMLOP_BYTEPREFIX);
		res->v_opregion.iobase = aml_parseint(scope, AML_ANYINT);
		res->v_opregion.iolen = aml_parseint(scope, AML_ANYINT);
		if (res->v_opregion.iospace == GAS_PCI_CFG_SPACE) {
			res->v_opregion.iobase += aml_getpciaddr(dsdt_softc,
			    scope->node);
			dnprintf(20, "got ioaddr: %s.%s:%llx\n",
			    aml_nodename(scope->node), aml_getname(name),
			    res->v_opregion.iobase);
		}
		break;
	}
	aml_createname(scope->node, name, res);

	return res;
=======
/*
 * @@@: Default Object creation
 */
static char osstring[] = "Macrosift Windogs MT";
struct aml_defval {
	const char		*name;
	int			type;
	int64_t			ival;
	const void		*bval;
	struct aml_value	**gval;
} aml_defobj[] = {
	{ "_OS_", AML_OBJTYPE_STRING, -1, osstring },
	{ "_REV", AML_OBJTYPE_INTEGER, 2, NULL },
	{ "_GL", AML_OBJTYPE_MUTEX, 1, NULL, &aml_global_lock },
	{ "_OSI", AML_OBJTYPE_METHOD, 1, aml_callosi },

	/* Create default scopes */
	{ "_GPE" },
	{ "_PR_" },
	{ "_SB_" },
	{ "_TZ_" },
	{ "_SI_" },

	{ NULL }
};

/* _OSI Default Method:
 * Returns True if string argument matches list of known OS strings
 * We return True for Windows to fake out nasty bad AML
 */
char *aml_valid_osi[] = {
	"Windows 2000",
	"Windows 2001",
	"Windows 2001.1",
	"Windows 2001 SP0",
	"Windows 2001 SP1",
	"Windows 2001 SP2",
	"Windows 2001 SP3",
	"Windows 2001 SP4",
	"Windows 2006",
	"Windows 2009",
	NULL
};

struct aml_value *
aml_callosi(struct aml_scope *scope, struct aml_value *val)
{
	int idx, result=0;
	struct aml_value *fa;

	fa = aml_getstack(scope, AMLOP_ARG0);
	for (idx=0; !result && aml_valid_osi[idx] != NULL; idx++) {
		dnprintf(10,"osi: %s,%s\n", fa->v_string, aml_valid_osi[idx]);
		result = !strcmp(fa->v_string, aml_valid_osi[idx]);
	}
	dnprintf(10,"@@ OSI found: %x\n", result);
	return aml_allocvalue(AML_OBJTYPE_INTEGER, result, NULL);
>>>>>>> origin/master
}

/* Parse Named objects with scope */
struct aml_value *
aml_parsenamedscope(struct aml_scope *scope, int opcode, struct aml_value *res)
{
<<<<<<< HEAD
	uint8_t *end, *name;
	struct aml_node *node;

	AML_CHECKSTACK();
	end = aml_parseend(scope);
	name = aml_parsename(scope);

	switch (opcode) {
	case AMLOP_DEVICE:
		res = aml_allocvalue(AML_OBJTYPE_DEVICE, 0, NULL);
		break;
	case AMLOP_SCOPE:
		res = NULL;
		break;
	case AMLOP_PROCESSOR:
		res = aml_allocvalue(AML_OBJTYPE_PROCESSOR, 0, NULL);
		res->v_processor.proc_id = aml_parseint(scope, AMLOP_BYTEPREFIX);
		res->v_processor.proc_addr = aml_parseint(scope, AMLOP_DWORDPREFIX);
		res->v_processor.proc_len = aml_parseint(scope, AMLOP_BYTEPREFIX);
		break;
	case AMLOP_POWERRSRC:
		res = aml_allocvalue(AML_OBJTYPE_POWERRSRC, 0, NULL);
		res->v_powerrsrc.pwr_level = aml_parseint(scope, AMLOP_BYTEPREFIX);
		res->v_powerrsrc.pwr_order = aml_parseint(scope, AMLOP_BYTEPREFIX);
		break;
	case AMLOP_THERMALZONE:
		res = aml_allocvalue(AML_OBJTYPE_THERMZONE, 0, NULL);
		break;
=======
	struct aml_value *tmp;
	struct aml_defval *def;

#ifdef ACPI_MEMDEBUG
	LIST_INIT(&acpi_memhead);
#endif

	osstring[1] = 'i';
	osstring[6] = 'o';
	osstring[15] = 'w';
	osstring[18] = 'N';

	SIMPLEQ_INIT(&aml_root.son);
	strlcpy(aml_root.name, "\\", sizeof(aml_root.name));
	aml_root.value = aml_allocvalue(0, 0, NULL);
	aml_root.value->node = &aml_root;

	for (def = aml_defobj; def->name; def++) {
		/* Allocate object value + add to namespace */
		aml_parsename(&aml_root, (uint8_t *)def->name, &tmp, 1);
		_aml_setvalue(tmp, def->type, def->ival, def->bval);
		if (def->gval) {
			/* Set root object pointer */
			*def->gval = tmp;
		}
		aml_xdelref(&tmp, 0);
>>>>>>> origin/master
	}
	node = aml_createname(scope->node, name, res);
	aml_parsenode(scope, node, scope->pos, &end, NULL);
	scope->pos = end;

	return res;
}

/* Parse math opcodes */
struct aml_value *
aml_parsemath(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	struct aml_value *tmparg;
	int64_t i1, i2, i3;

<<<<<<< HEAD
	tmparg = aml_alloctmp(scope, 1);
	AML_CHECKSTACK();
	switch (opcode) {
	case AMLOP_LNOT:
		i2 = 0;
		i1 = aml_parseint(scope, AML_ANYINT);
=======
	switch (typ) {
	case LR_EXTIRQ:
		printf("extirq\tflags:%.2x len:%.2x irq:%.4x\n",
		    crs->lr_extirq.flags, crs->lr_extirq.irq_count,
		    letoh32(crs->lr_extirq.irq[0]));
		break;
	case SR_IRQ:
		printf("irq\t%.4x %.2x\n", letoh16(crs->sr_irq.irq_mask),
		    crs->sr_irq.irq_flags);
		break;
	case SR_DMA:
		printf("dma\t%.2x %.2x\n", crs->sr_dma.channel,
		    crs->sr_dma.flags);
		break;
	case SR_IOPORT:
		printf("ioport\tflags:%.2x _min:%.4x _max:%.4x _aln:%.2x _len:%.2x\n",
		    crs->sr_ioport.flags, crs->sr_ioport._min,
		    crs->sr_ioport._max, crs->sr_ioport._aln,
		    crs->sr_ioport._len);
		break;
	case SR_STARTDEP:
		printf("startdep\n");
>>>>>>> origin/master
		break;
	case AMLOP_LAND:
	case AMLOP_LOR:
		i1 = aml_parseint(scope, AML_ANYINT);
		i2 = aml_parseint(scope, AML_ANYINT);
		break;
	case AMLOP_NOT:
	case AMLOP_TOBCD:
	case AMLOP_FROMBCD:
	case AMLOP_TOINTEGER:
	case AMLOP_FINDSETLEFTBIT:
	case AMLOP_FINDSETRIGHTBIT:
		i2 = 0;
		i1 = aml_parseint(scope, AML_ANYINT);
		aml_parsetarget(scope, tmparg, NULL);
		break;
	case AMLOP_INCREMENT:
	case AMLOP_DECREMENT:
		aml_parsetarget(scope, tmparg, NULL);
		i1 = aml_val2int(aml_derefterm(scope, tmparg, 0));
		i2 = 1;
		break;
	case AMLOP_DIVIDE:
		i1 = aml_parseint(scope, AML_ANYINT);
		i2 = aml_parseint(scope, AML_ANYINT);

		aml_parsetarget(scope, tmparg, NULL);	// remainder
		aml_setvalue(scope, tmparg, NULL, (i1 % i2));

		aml_parsetarget(scope, tmparg, NULL);	// quotient
		break;
	default:
		i1 = aml_parseint(scope, AML_ANYINT);
		i2 = aml_parseint(scope, AML_ANYINT);
		aml_parsetarget(scope, tmparg, NULL);
		break;
	}
	i3 = aml_evalexpr(i1, i2, opcode);
	aml_setvalue(scope, res, NULL, i3);
	aml_setvalue(scope, tmparg, NULL, i3);
	return (res);
}

/* Parse logical comparison opcodes */
struct aml_value *
aml_parsecompare(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	struct aml_value *tmparg;
	int rc;

	AML_CHECKSTACK();
	tmparg = aml_alloctmp(scope, 2);
	aml_parseterm(scope, &tmparg[AML_LHS]);
	aml_parseterm(scope, &tmparg[AML_RHS]);

	/* Compare both values */
	rc = aml_cmpvalue(&tmparg[AML_LHS], &tmparg[AML_RHS], opcode);
	aml_setvalue(scope, res, NULL, rc);

	return res;
}

<<<<<<< HEAD
/* Parse IF/ELSE opcodes */
struct aml_value *
aml_parseif(struct aml_scope *scope, int opcode, struct aml_value *res)
=======
int
aml_parse_resource(struct aml_value *res,
    int (*crs_enum)(union acpi_resource *, void *), void *arg)
>>>>>>> origin/master
{
	int64_t test;
	uint8_t *end;

<<<<<<< HEAD
	AML_CHECKSTACK();
	end = aml_parseend(scope);
	test = aml_parseint(scope, AML_ANYINT);

	dnprintf(40, "@ iftest: %llx\n", test);
	while (test && scope->pos < end) {
		/* Parse if scope */
		aml_parseterm(scope, res);
	}
	if (scope->pos >= scope->end)
		return res;
=======
	if (res->type != AML_OBJTYPE_BUFFER || res->length < 5)
		return (-1);
	for (off = 0; off < res->length; off += rlen) {
		crs = (union acpi_resource *)(res->v_buffer+off);

		rlen = AML_CRSLEN(crs);
		if (crs->hdr.typecode == 0x79 || !rlen)
			break;
>>>>>>> origin/master

	if (*end == AMLOP_ELSE) {
		scope->pos = ++end;
		end = aml_parseend(scope);
		while (!test && scope->pos < end) {
			/* Parse ELSE scope */
			aml_parseterm(scope, res);
		}
	}
	if (scope->pos < end)
		scope->pos = end;
	return res;
}

<<<<<<< HEAD
struct aml_value *
aml_parsewhile(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	uint8_t *end, *start;
	int test, cnt;

	AML_CHECKSTACK();
	end = aml_parseend(scope);
	start = scope->pos;
	cnt = 0;
	do {
		test = 1;
		if (scope->pos == start || scope->pos == end) {
			scope->pos = start;
			test = aml_parseint(scope, AML_ANYINT);
			dnprintf(40, "@whiletest = %d %x\n", test, cnt++);
		} else if (*scope->pos == AMLOP_BREAK) {
			scope->pos++;
			test = 0;
		} else if (*scope->pos == AMLOP_CONTINUE) {
			scope->pos = start;
		} else {
			aml_parseterm(scope, res);
		}
	} while (test && scope->pos <= end && cnt < 0x199);
	/* XXX: shouldn't need breakout counter */

	dnprintf(40, "Set While end : %x\n", cnt);
	if (scope->pos < end)
		scope->pos = end;
	return res;
=======
	return (0);
>>>>>>> origin/master
}

/* Parse Buffer/Package opcodes */
struct aml_value *
aml_parsebufpkg(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	uint8_t *end;
	int len;

	AML_CHECKSTACK();
	end = aml_parseend(scope);
	len = aml_parseint(scope, (opcode == AMLOP_PACKAGE) ?
	    AMLOP_BYTEPREFIX : AML_ANYINT);

	switch (opcode) {
	case AMLOP_BUFFER:
		_aml_setvalue(res, AML_OBJTYPE_BUFFER, len, NULL);
		if (scope->pos < end) {
			memcpy(res->v_buffer, scope->pos, end-scope->pos);
		}
		if (len != end-scope->pos) {
			dnprintf(99, "buffer: %.4x %.4x\n", len, end-scope->pos);
		}
		break;
	case AMLOP_PACKAGE:
	case AMLOP_VARPACKAGE:
		_aml_setvalue(res, AML_OBJTYPE_PACKAGE, len, NULL);
		for (len = 0; len < res->length && scope->pos < end; len++) {
			aml_parseop(scope, res->v_package[len]);
		}
		if (scope->pos != end) {
			dnprintf(99, "Package not equiv!! %.4x %.4x %d of %d\n",
			    aml_pc(scope->pos), aml_pc(end), len, res->length);
		}
		break;
	}
	scope->pos = end;
	return res;
}

<<<<<<< HEAD
struct aml_value *
aml_parsemethod(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	uint8_t *end, *name;

	AML_CHECKSTACK();
	end = aml_parseend(scope);
	name = aml_parsename(scope);

	res = aml_allocvalue(AML_OBJTYPE_METHOD, 0, NULL);
	res->v_method.flags = aml_parseint(scope, AMLOP_BYTEPREFIX);
	res->v_method.start = scope->pos;
	res->v_method.end = end;
	res->v_method.fneval = aml_callmethod;
	aml_createname(scope->node, name, res);

	scope->pos = end;

	return res;
}
=======
/*
 * Walk nodes and perform fixups for nameref
 */
int aml_fixup_node(struct aml_node *, void *);
>>>>>>> origin/master

/* Parse simple type opcodes */
struct aml_value *
aml_parsesimple(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	struct aml_node *node;

<<<<<<< HEAD
	AML_CHECKSTACK();
	switch (opcode) {
	case AMLOP_ZERO:
		_aml_setvalue(res, AML_OBJTYPE_INTEGER+AML_STATIC,
		    aml_parseint(scope, opcode), NULL);
		break;
	case AMLOP_ONE:
	case AMLOP_ONES:
	case AMLOP_BYTEPREFIX:
	case AMLOP_WORDPREFIX:
	case AMLOP_DWORDPREFIX:
	case AMLOP_QWORDPREFIX:
	case AMLOP_REVISION:
		_aml_setvalue(res, AML_OBJTYPE_INTEGER,
		    aml_parseint(scope, opcode), NULL);
		break;
	case AMLOP_DEBUG:
		_aml_setvalue(res, AML_OBJTYPE_DEBUGOBJ, 0, NULL);
		break;
	case AMLOP_STRINGPREFIX:
		_aml_setvalue(res, AML_OBJTYPE_STRING, -1, scope->pos);
		scope->pos += res->length+1;
		break;
	case AMLOP_NAMECHAR:
		_aml_setvalue(res, AML_OBJTYPE_NAMEREF, 0, NULL);
		res->v_nameref = aml_parsename(scope);
		node = aml_searchname(scope->node, res->v_nameref);
		if (node && node->value)
			_aml_setvalue(res, AML_OBJTYPE_OBJREF, -1, node->value);
		break;
=======
	if (node->value == NULL)
		return (0);
	if (arg == NULL)
		aml_fixup_node(node, node->value);
	else if (val->type == AML_OBJTYPE_NAMEREF) {
		node = aml_searchname(node, val->v_nameref);
		if (node && node->value) {
			_aml_setvalue(val, AML_OBJTYPE_OBJREF, AMLOP_NAMECHAR,
			    node->value);
		}
	} else if (val->type == AML_OBJTYPE_PACKAGE) {
		for (i = 0; i < val->length; i++)
			aml_fixup_node(node, val->v_package[i]);
>>>>>>> origin/master
	}
	return res;
}

<<<<<<< HEAD
/* Parse field unit opcodes */
struct aml_value *
aml_parsefieldunit(struct aml_scope *scope, int opcode, struct aml_value *res)
=======
#ifndef SMALL_KERNEL
const char *
aml_val_to_string(const struct aml_value *val)
>>>>>>> origin/master
{
	uint8_t *end, *name;
	struct aml_value *fld;

	AML_CHECKSTACK();
	end = aml_parseend(scope);

	switch (opcode) {
	case AMLOP_FIELD:
		aml_parsetarget(scope, NULL, &res->v_field.ref1);
		break;
	case AMLOP_INDEXFIELD:
		aml_parsetarget(scope, NULL, &res->v_field.ref1);
		aml_parsetarget(scope, NULL, &res->v_field.ref2);
		break;
	case AMLOP_BANKFIELD:
		aml_parsetarget(scope, NULL, &res->v_field.ref1);
		aml_parsetarget(scope, NULL, &res->v_field.ref2);
		res->v_field.ref3 = aml_parseint(scope, AML_ANYINT);
		break;
	}
	res->v_field.flags = aml_parseint(scope, AMLOP_BYTEPREFIX);
	res->v_field.type = opcode;

<<<<<<< HEAD
	aml_fixref(&res->v_field.ref1);
	aml_fixref(&res->v_field.ref2);

	while (scope->pos < end) {
		switch (*scope->pos) {
		case 0x00: // reserved
			scope->pos++;
			res->v_field.bitlen = aml_parselength(scope);
			break;
		case 0x01: // attrib
			scope->pos++;
			/* XXX: do something with this */
			aml_parseint(scope, AMLOP_BYTEPREFIX);
			aml_parseint(scope, AMLOP_BYTEPREFIX);
			res->v_field.bitlen = 0;
			break;
		default:
			name = aml_parsename(scope);
			res->v_field.bitlen = aml_parselength(scope);

			/* Allocate new fieldunit */
			fld = aml_allocvalue(AML_OBJTYPE_FIELDUNIT, 0, NULL);

			/* Increase reference count on field */
			fld->v_field = res->v_field;
			aml_addref(fld->v_field.ref1);
			aml_addref(fld->v_field.ref2);

			aml_createname(scope->node, name, fld);
			break;
		}
		res->v_field.bitpos += res->v_field.bitlen;
	}
	/* Delete redundant reference */
	aml_delref(&res->v_field.ref1);
	aml_delref(&res->v_field.ref2);
	return res;
}

/* Parse CreateXXXField opcodes */
struct aml_value *
aml_parsebufferfield(struct aml_scope *scope, int opcode,
    struct aml_value *res)
{
	uint8_t *name;
=======
	return (buffer);
}
#endif /* SMALL_KERNEL */

int aml_error;

struct aml_value *aml_gettgt(struct aml_value *, int);
struct aml_value *aml_xeval(struct aml_scope *, struct aml_value *, int, int,
    struct aml_value *);
struct aml_value *aml_xparsesimple(struct aml_scope *, char,
    struct aml_value *);
struct aml_value *aml_xparse(struct aml_scope *, int, const char *);
struct aml_value *aml_seterror(struct aml_scope *, const char *, ...);

struct aml_scope *aml_xfindscope(struct aml_scope *, int, int);
struct aml_scope *aml_xpushscope(struct aml_scope *, struct aml_value *,
    struct aml_node *, int);
struct aml_scope *aml_xpopscope(struct aml_scope *);

void		aml_showstack(struct aml_scope *);
struct aml_value *aml_xconvert(struct aml_value *, int, int);

int		aml_xmatchtest(int64_t, int64_t, int);
int		aml_xmatch(struct aml_value *, int, int, int, int, int);

int		aml_xcompare(struct aml_value *, struct aml_value *, int);
struct aml_value *aml_xconcat(struct aml_value *, struct aml_value *);
struct aml_value *aml_xconcatres(struct aml_value *, struct aml_value *);
struct aml_value *aml_xmid(struct aml_value *, int, int);
int		aml_ccrlen(union acpi_resource *, void *);

void		aml_xstore(struct aml_scope *, struct aml_value *, int64_t,
    struct aml_value *);

/*
 * Reference Count functions
 */
void
aml_xaddref(struct aml_value *val, const char *lbl)
{
	if (val == NULL)
		return;
	dnprintf(50, "XAddRef: %p %s:[%s] %d\n",
	    val, lbl,
	    val->node ? aml_nodename(val->node) : "INTERNAL",
	    val->refcnt);
	val->refcnt++;
}
>>>>>>> origin/master

	AML_CHECKSTACK();
	res = aml_allocvalue(AML_OBJTYPE_BUFFERFIELD, 0, NULL);
	res->v_field.type = opcode;
	aml_parsetarget(scope, NULL, &res->v_field.ref1);
	res->v_field.bitpos = aml_parseint(scope, AML_ANYINT);

<<<<<<< HEAD
	aml_fixref(&res->v_field.ref1);

	switch (opcode) {
	case AMLOP_CREATEFIELD:
		res->v_field.bitlen = aml_parseint(scope, AML_ANYINT);
		break;
	case AMLOP_CREATEBITFIELD:
		res->v_field.bitlen = 1;
		break;
	case AMLOP_CREATEBYTEFIELD:
		res->v_field.bitlen = 8;
		res->v_field.bitpos *= 8;
		break;
	case AMLOP_CREATEWORDFIELD:
		res->v_field.bitlen = 16;
		res->v_field.bitpos *= 8;
		break;
	case AMLOP_CREATEDWORDFIELD:
		res->v_field.bitlen = 32;
		res->v_field.bitpos *= 8;
		break;
	case AMLOP_CREATEQWORDFIELD:
		res->v_field.bitlen = 64;
		res->v_field.bitpos *= 8;
		break;
=======
	if (pv == NULL || *pv == NULL)
		return;
	val = *pv;
	val->refcnt--;
	if (val->refcnt == 0) {
		dnprintf(50, "XDelRef: %p %s %2d [%s] %s\n",
		    val, lbl,
		    val->refcnt,
		    val->node ? aml_nodename(val->node) : "INTERNAL",
		    val->refcnt ? "" : "---------------- FREEING");

		aml_freevalue(val);
		acpi_os_free(val);
		*pv = NULL;
>>>>>>> origin/master
	}
	name = aml_parsename(scope);
	aml_createname(scope->node, name, res);

	return res;
}

/* Parse Mutex/Event action */
struct aml_value *
aml_parsemuxaction(struct aml_scope *scope, int opcode, struct aml_value *res)
{
<<<<<<< HEAD
	struct aml_value *tmparg;
	int64_t i1;
	int rv;

	AML_CHECKSTACK();

	tmparg = aml_alloctmp(scope, 1);
	aml_parsetarget(scope, tmparg, NULL);
	switch (opcode) {
	case AMLOP_ACQUIRE:
		/* Assert: tmparg is AML_OBJTYPE_MUTEX */
		i1 = aml_parseint(scope, AMLOP_WORDPREFIX);
		rv = acpi_mutex_acquire(tmparg->v_objref.ref, i1);
		/* Return true if timed out */
		aml_setvalue(scope, res, NULL, rv);
		break;
	case AMLOP_RELEASE:
		acpi_mutex_release(tmparg->v_objref.ref);
		break;

	case AMLOP_WAIT:
		/* Assert: tmparg is AML_OBJTYPE_EVENT */
		i1 = aml_parseint(scope, AML_ANYINT);

		/* Return true if timed out */
		aml_setvalue(scope, res, NULL, 0);
		break;
	case AMLOP_SIGNAL:
		break;
	case AMLOP_RESET:
		break;
=======
	while (scope) {
		switch (endscope) {
		case AMLOP_RETURN:
			scope->pos = scope->end;
			if (scope->type == AMLOP_WHILE)
				scope->pos = NULL;
			break;
		case AMLOP_CONTINUE:
			scope->pos = scope->end;
			break;
		case AMLOP_BREAK:
			scope->pos = scope->end;
			if (scope->type == type)
				scope->parent->pos = scope->end;
			break;
		}
		if (scope->type == type)
			break;
		scope = scope->parent;
>>>>>>> origin/master
	}

	return res;
}

<<<<<<< HEAD
/* Parse Miscellaneous opcodes */
struct aml_value *
aml_parsemisc2(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	struct aml_value *tmparg, *dev;
	int i1, i2, i3;

	AML_CHECKSTACK();

	switch (opcode) {
	case AMLOP_NOTIFY:
		/* Assert: tmparg is nameref or objref */
		tmparg = aml_alloctmp(scope, 1);
		aml_parseop(scope, tmparg);
		dev = aml_dereftarget(scope, tmparg);

		i1 = aml_parseint(scope, AML_ANYINT);
		if (dev && dev->node) {
			dnprintf(10, "Notify: [%s] %.2x\n",
			    aml_nodename(dev->node), i1);
			aml_notify(dev->node, i1);
		}
		break;
	case AMLOP_SLEEP:
		i1 = aml_parseint(scope, AML_ANYINT);
		dnprintf(50, "SLEEP: %x\n", i1);
		if (i1)
			acpi_sleep(i1);
		else {
			dnprintf(10, "acpi_sleep(0)\n");
		}
		break;
	case AMLOP_STALL:
		i1 = aml_parseint(scope, AML_ANYINT);
		dnprintf(50, "STALL: %x\n", i1);
		if (i1)
			acpi_stall(i1);
		else {
			dnprintf(10, "acpi_stall(0)\n");
=======
struct aml_value *
aml_getstack(struct aml_scope *scope, int opcode)
{
	struct aml_value *sp;

	sp = NULL;
	scope = aml_xfindscope(scope, AMLOP_METHOD, 0);
	if (scope == NULL)
		return NULL;
	if (opcode >= AMLOP_LOCAL0 && opcode <= AMLOP_LOCAL7) {
		if (scope->locals == NULL)
			scope->locals = aml_allocvalue(AML_OBJTYPE_PACKAGE, 8, NULL);
		sp = scope->locals->v_package[opcode - AMLOP_LOCAL0];
		sp->stack = opcode;
	} else if (opcode >= AMLOP_ARG0 && opcode <= AMLOP_ARG6) {
		if (scope->args == NULL)
			scope->args = aml_allocvalue(AML_OBJTYPE_PACKAGE, 7, NULL);
		sp = scope->args->v_package[opcode - AMLOP_ARG0];
		if (sp->type == AML_OBJTYPE_OBJREF)
			sp = sp->v_objref.ref;
	}
	return sp;
}

#ifdef ACPI_DEBUG
/* Dump AML Stack */
void
aml_showstack(struct aml_scope *scope)
{
	struct aml_value *sp;
	int idx;

	dnprintf(10, "===== Stack %s:%s\n", aml_nodename(scope->node),
	    aml_mnem(scope->type, 0));
	for (idx=0; scope->args && idx<7; idx++) {
		sp = aml_getstack(scope, AMLOP_ARG0+idx);
		if (sp && sp->type) {
			dnprintf(10," Arg%d: ", idx);
			aml_showvalue(sp, 10);
		}
	}
	for (idx=0; scope->locals && idx<8; idx++) {
		sp = aml_getstack(scope, AMLOP_LOCAL0+idx);
		if (sp && sp->type) {
			dnprintf(10," Local%d: ", idx);
			aml_showvalue(sp, 10);
>>>>>>> origin/master
		}
		break;
	case AMLOP_FATAL:
		i1 = aml_parseint(scope, AMLOP_BYTEPREFIX);
		i2 = aml_parseint(scope, AMLOP_DWORDPREFIX);
		i3 = aml_parseint(scope, AML_ANYINT);
		aml_die("FATAL: %x %x %x\n", i1, i2, i3);
		break;
	}
	return res;
}

/* Parse Miscellaneous opcodes */
struct aml_value *
aml_parsemisc3(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	struct aml_value *tmparg;

<<<<<<< HEAD
	AML_CHECKSTACK();
	tmparg = aml_alloctmp(scope, 1);
	aml_parseterm(scope, tmparg);
	switch (opcode) {
	case AMLOP_SIZEOF:
		aml_setvalue(scope, res, NULL, tmparg->length);
		break;
	case AMLOP_OBJECTTYPE:
		aml_setvalue(scope, res, NULL, tmparg->type);
		break;
	}
=======
	if (range->type == AML_OBJTYPE_METHOD) {
		start = range->v_method.start;
		end = range->v_method.end;
	} else {
		start = range->v_buffer;
		end = start + range->length;
		if (start == end)
			return NULL;
	}
	scope = acpi_os_malloc(sizeof(struct aml_scope));
	if (scope == NULL)
		return NULL;

	scope->node = node;
	scope->start = start;
	scope->end = end;
	scope->pos = scope->start;
	scope->parent = parent;
	scope->type = type;
	scope->sc = acpi_softc;
>>>>>>> origin/master

	return res;
}

/* Parse AMLOP_MATCH */
struct aml_value *
aml_parsematch(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	struct aml_value *pkg;
	int op1, op2, idx, mv1, mv2;

	AML_CHECKSTACK();
	pkg = aml_parseterm(scope, NULL);
	op1 = aml_parseint(scope, AMLOP_BYTEPREFIX);
	mv1 = aml_parseint(scope, AML_ANYINT);
	op2 = aml_parseint(scope, AMLOP_BYTEPREFIX);
	mv2 = aml_parseint(scope, AML_ANYINT);
	idx = aml_parseint(scope, AML_ANYINT);

	aml_setvalue(scope, res, NULL, -1);
	while (idx < pkg->length) {
		if (aml_match(op1, mv1, pkg->v_package[idx]) ||
		    aml_match(op2, mv2, pkg->v_package[idx])) {
			aml_setvalue(scope, res, NULL, idx);
			break;
		}
		idx++;
	}
	aml_delref(&pkg);
	return res;
}

/* Parse referenced objects */
struct aml_value *
aml_parseref(struct aml_scope *scope, int opcode, struct aml_value *res)
{
<<<<<<< HEAD
	struct aml_value *tmparg;
=======
	struct aml_scope *nscope;
>>>>>>> origin/master

	AML_CHECKSTACK();

	switch (opcode) {
	case AMLOP_INDEX:
		tmparg = aml_alloctmp(scope, 1);
		_aml_setvalue(res, AML_OBJTYPE_OBJREF, -1, NULL);
		aml_parsetarget(scope, tmparg, NULL);

<<<<<<< HEAD
		res->v_objref.index = aml_parseint(scope, AML_ANYINT);
		res->v_objref.ref = aml_dereftarget(scope, tmparg);

		aml_parsetarget(scope, tmparg, NULL);
		aml_setvalue(scope, tmparg, res, 0);
		break;
	case AMLOP_DEREFOF:
		aml_parseop(scope, res);
		break;
	case AMLOP_RETURN:
		tmparg = aml_alloctmp(scope, 1);
		aml_parseterm(scope, tmparg);
		aml_setvalue(scope, res, tmparg, 0);
		scope->pos = scope->end;
		break;
	case AMLOP_ARG0 ... AMLOP_ARG6:
		opcode -= AMLOP_ARG0;
		if (scope->args == NULL || opcode >= scope->nargs)
			aml_die("arg %d out of range", opcode);

		/* Create OBJREF to stack variable */
		_aml_setvalue(res, AML_OBJTYPE_OBJREF, -1,
		    &scope->args[opcode]);
		break;
	case AMLOP_LOCAL0 ... AMLOP_LOCAL7:
		opcode -= AMLOP_LOCAL0;

		/* No locals exist.. lazy allocate */
		if (scope->locals == NULL) {
			dnprintf(10, "Lazy alloc locals\n");
			scope->locals = aml_alloctmp(scope, AML_MAX_LOCAL);
		}

		/* Create OBJREF to stack variable */
		_aml_setvalue(res, AML_OBJTYPE_OBJREF, -1,
		    &scope->locals[opcode]);
		res->v_objref.ref->stack = opcode+AMLOP_LOCAL0;
		break;
	case AMLOP_LOAD:
		tmparg = aml_alloctmp(scope, 2);
		aml_parseop(scope, &tmparg[0]);
		aml_parseop(scope, &tmparg[1]);
		break;
	case AMLOP_STORE:
		tmparg = aml_alloctmp(scope, 1);
		aml_parseterm(scope, res);
		aml_parsetarget(scope, tmparg, NULL);

		while (tmparg->type == AML_OBJTYPE_OBJREF) {
			if (tmparg->v_objref.index != -1)
		    		break;
			tmparg = tmparg->v_objref.ref;
		}
		aml_setvalue(scope, tmparg, res, 0);
		break;
	case AMLOP_REFOF:
		_aml_setvalue(res, AML_OBJTYPE_OBJREF, -1, NULL);
		aml_parsetarget(scope, NULL, &res->v_objref.ref);
		break;
	case AMLOP_CONDREFOF:
		/* Returns true if object exists */
		tmparg = aml_alloctmp(scope, 2);
		aml_parsetarget(scope, &tmparg[0], NULL);
		aml_parsetarget(scope, &tmparg[1], NULL);
		if (tmparg[0].type != AML_OBJTYPE_NAMEREF) {
			/* Object exists */
			aml_freevalue(&tmparg[1]);
			aml_setvalue(scope, &tmparg[1], &tmparg[0], 0);
			aml_setvalue(scope, res, NULL, 1);
		} else {
			/* Object doesn't exist */
			aml_setvalue(scope, res, NULL, 0);
		}
		break;
=======
	if (scope->type == AMLOP_METHOD)
		aml_delchildren(scope->node);
	if (scope->locals) {
		aml_freevalue(scope->locals);
		acpi_os_free(scope->locals);
		scope->locals = NULL;
	}
	if (scope->args) {
		aml_freevalue(scope->args);
		acpi_os_free(scope->args);
		scope->args = NULL;
>>>>>>> origin/master
	}

	return res;
}

struct aml_value *
aml_parsestring(struct aml_scope *scope, int opcode, struct aml_value *res)
{
	struct aml_value *tmpval;
	int i1, i2;

	AML_CHECKSTACK();
	switch (opcode) {
	case AMLOP_CONCAT:
		tmpval = aml_alloctmp(scope, 4);
		aml_parseterm(scope, &tmpval[AML_LHS]);
		aml_parseterm(scope, &tmpval[AML_RHS]);
		aml_parsetarget(scope, &tmpval[AML_DST], NULL);
		if (tmpval[AML_LHS].type == AML_OBJTYPE_BUFFER &&
		    tmpval[AML_RHS].type == AML_OBJTYPE_BUFFER) {
			aml_resize(&tmpval[AML_LHS],
			    tmpval[AML_LHS].length+tmpval[AML_RHS].length);
			memcpy(&tmpval[AML_LHS].v_buffer+tmpval[AML_LHS].length,
			    tmpval[AML_RHS].v_buffer, tmpval[AML_RHS].length);
			aml_setvalue(scope, &tmpval[AML_DST], &tmpval[AML_LHS], 0);
		}
		if (tmpval[AML_LHS].type == AML_OBJTYPE_STRING &&
		    tmpval[AML_RHS].type == AML_OBJTYPE_STRING) {
			aml_resize(&tmpval[AML_LHS],
			    tmpval[AML_LHS].length+tmpval[AML_RHS].length);
			memcpy(&tmpval[AML_LHS].v_string+tmpval[AML_LHS].length,
			    tmpval[AML_RHS].v_buffer, tmpval[AML_RHS].length);
			aml_setvalue(scope, &tmpval[AML_DST], &tmpval[AML_LHS], 0);
		} else {
			aml_die("concat");
		}
		break;
	case AMLOP_MID:
		tmpval = aml_alloctmp(scope, 2);
		aml_parseterm(scope, &tmpval[0]);
		i1 = aml_parseint(scope, AML_ANYINT); // start
		i2 = aml_parseint(scope, AML_ANYINT); // length
		aml_parsetarget(scope, &tmpval[1], NULL);
		if (i1 > tmpval[0].length)
			i1 = tmpval[0].length;
		if (i1+i2 > tmpval[0].length)
			i2 = tmpval[0].length-i1;
		_aml_setvalue(res, AML_OBJTYPE_STRING, i2, tmpval[0].v_string+i1);
		break;
	case AMLOP_TODECSTRING:
	case AMLOP_TOHEXSTRING:
		i1 = aml_parseint(scope, AML_ANYINT);
		_aml_setvalue(res, AML_OBJTYPE_STRING, 20, NULL);
		snprintf(res->v_string, res->length,
		    ((opcode == AMLOP_TODECSTRING) ? "%d" : "%x"), i1);
		break;
	default:
		aml_die("to_string");
		break;
	}
<<<<<<< HEAD

	return res;
=======
	return (0);
>>>>>>> origin/master
}

struct aml_value *
aml_parseterm(struct aml_scope *scope, struct aml_value *res)
{
<<<<<<< HEAD
	struct aml_value *tmpres;

	/* If no value specified, allocate dynamic */
	if (res == NULL)
		res = aml_allocvalue(AML_OBJTYPE_UNINITIALIZED, 0, NULL);
	tmpres = aml_alloctmp(scope, 1);
	aml_parseop(scope, tmpres);
	aml_evalterm(scope, tmpres, res);
	return res;
}

struct aml_value *
aml_parsetarget(struct aml_scope *scope, struct aml_value *res,
    struct aml_value **opt)
{
	struct aml_value *dummy;

	/* If no value specified, allocate dynamic */
	if (res == NULL)
		res = aml_allocvalue(AML_OBJTYPE_UNINITIALIZED, 0, NULL);
	aml_parseop(scope, res);
	if (opt == NULL)
		opt = &dummy;

	*opt = aml_evaltarget(scope, res);

	return res;
}

int odp;

/* Main Opcode Parser/Evaluator */
struct aml_value *
aml_parseop(struct aml_scope *scope, struct aml_value *res)
{
	int opcode;
	struct aml_opcode *htab;
	struct aml_value *rv = NULL;

	if (odp++ > 25)
		panic("depth");
	
	aml_freevalue(res);
	opcode = aml_parseopcode(scope);
	dnprintf(15, "%.4x: [%s] %s\n", aml_pc(scope->pos-opsize(opcode)),
	    aml_nodename(scope->node), aml_mnem(opcode, scope->pos));
	delay(amlop_delay);

	htab = aml_findopcode(opcode);
	if (htab && htab->handler) {
		rv = htab->handler(scope, opcode, res);
	} else {
		/* No opcode handler */
		aml_die("Unknown opcode: %.4x @ %.4x", opcode,
		    aml_pc(scope->pos - opsize(opcode)));
	}
	odp--;
	return rv;
}

const char hext[] = "0123456789ABCDEF";

const char *
aml_eisaid(u_int32_t pid)
{
	static char id[8];

	id[0] = '@' + ((pid >> 2) & 0x1F);
	id[1] = '@' + ((pid << 3) & 0x18) + ((pid >> 13) & 0x7);
	id[2] = '@' + ((pid >> 8) & 0x1F);
	id[3] = hext[(pid >> 20) & 0xF];
	id[4] = hext[(pid >> 16) & 0xF];
	id[5] = hext[(pid >> 28) & 0xF];
	id[6] = hext[(pid >> 24) & 0xF];
	id[7] = 0;
	return id;
}

/*
 * @@@: Fixup DSDT code
=======
	struct aml_value *tmp;
	int flag;

	while (index < pkg->length) {
		/* Convert package value to integer */
		tmp = aml_xconvert(pkg->v_package[index],
		    AML_OBJTYPE_INTEGER, -1);

		/* Perform test */
		flag = aml_xmatchtest(tmp->v_integer, v1, op1) &&
		    aml_xmatchtest(tmp->v_integer, v2, op2);
		aml_xdelref(&tmp, "xmatch");

		if (flag)
			return index;
		index++;
	}
	return -1;
}

/*
 * Conversion routines
>>>>>>> origin/master
 */
struct aml_fixup {
	int		offset;
	u_int8_t	oldv, newv;
} __ibm300gl[] = {
	{ 0x19, 0x3a, 0x3b },
	{ -1 }
};

struct aml_blacklist {
	const char	*oem, *oemtbl;
	struct aml_fixup *fixtab;
	u_int8_t	cksum;
} amlfix_list[] = {
	{ "IBM   ", "CDTPWSNH", __ibm300gl, 0x41 },
	{ NULL },
};

void
aml_fixup_dsdt(u_int8_t *acpi_hdr, u_int8_t *base, int len)
{
	struct acpi_table_header *hdr = (struct acpi_table_header *)acpi_hdr;
	struct aml_blacklist *fixlist;
	struct aml_fixup *fixtab;

<<<<<<< HEAD
	for (fixlist = amlfix_list; fixlist->oem; fixlist++) {
		if (!memcmp(fixlist->oem, hdr->oemid, 6) &&
		    !memcmp(fixlist->oemtbl, hdr->oemtableid, 8) &&
		    fixlist->cksum == hdr->checksum) {
			/* Found a potential fixup entry */
			for (fixtab = fixlist->fixtab; fixtab->offset != -1;
			    fixtab++) {
				if (base[fixtab->offset] == fixtab->oldv)
					base[fixtab->offset] = fixtab->newv;
			}
		}
=======
	while (*str) {
		if (*str >= '0' && *str <= '9')
			c = *(str++) - '0';
		else if (*str >= 'a' && *str <= 'f')
			c = *(str++) - 'a' + 10;
		else if (*str >= 'A' && *str <= 'F')
			c = *(str++) - 'A' + 10;
		else
			break;
		v = (v << 4) + c;
>>>>>>> origin/master
	}
}

<<<<<<< HEAD
/*
 * @@@: Default Object creation
 */
struct aml_defval {
	const char		*name;
	int			type;
	int64_t			ival;
	const void		*bval;
	struct aml_value	**gval;
} aml_defobj[] = {
	{ "_OS_", AML_OBJTYPE_STRING, -1, "OpenBSD" },
	{ "_REV", AML_OBJTYPE_INTEGER, 2, NULL },
	{ "_GL", AML_OBJTYPE_MUTEX, 1, NULL, &aml_global_lock },
	{ "_OSI", AML_OBJTYPE_METHOD, 1, aml_callosi },
	{ NULL }
};

/* _OSI Default Method:
 * Returns True if string argument matches list of known OS strings
 * We return True for Windows to fake out nasty bad AML
 */
char *aml_valid_osi[] = { 
	"OpenBSD",
	"Windows 2000",
	"Windows 2001",
	"Windows 2001.1",
	"Windows 2001 SP0",
	"Windows 2001 SP1",
	"Windows 2001 SP2",
	"Windows 2001 SP3",
	"Windows 2001 SP4",
	"Windows 2006",
	NULL
};

struct aml_value *
aml_callosi(struct aml_scope *scope, struct aml_value *val)
=======
struct aml_value *
aml_xconvert(struct aml_value *a, int ctype, int clen)
>>>>>>> origin/master
{
	struct aml_value tmpstr, *arg;
	int idx, result;

<<<<<<< HEAD
	/* Perform comparison with valid strings */
	result = 0;
	memset(&tmpstr, 0, sizeof(tmpstr));
	tmpstr.type = AML_OBJTYPE_STRING;
	arg = aml_derefvalue(scope, &scope->args[0], ACPI_IOREAD);

	for (idx=0; !result && aml_valid_osi[idx] != NULL; idx++) {
		tmpstr.v_string = aml_valid_osi[idx];
		tmpstr.length = strlen(tmpstr.v_string);
		
		result = aml_cmpvalue(arg, &tmpstr, AMLOP_LEQUAL);
	}
	aml_setvalue(scope, val, NULL, result);
	return val;
=======
	/* Object is already this type */
	if (clen == -1)
		clen = a->length;
	if (a->type == ctype) {
		aml_xaddref(a, "XConvert");
		return a;
	}
	switch (ctype) {
	case AML_OBJTYPE_BUFFER:
		dnprintf(10,"convert to buffer\n");
		switch (a->type) {
		case AML_OBJTYPE_INTEGER:
			c = aml_allocvalue(AML_OBJTYPE_BUFFER, a->length,
			    &a->v_integer);
			break;
		case AML_OBJTYPE_STRING:
			c = aml_allocvalue(AML_OBJTYPE_BUFFER, a->length,
			    a->v_string);
			break;
		}
		break;
	case AML_OBJTYPE_INTEGER:
		dnprintf(10,"convert to integer : %x\n", a->type);
		switch (a->type) {
		case AML_OBJTYPE_BUFFER:
			c = aml_allocvalue(AML_OBJTYPE_INTEGER, 0, NULL);
			memcpy(&c->v_integer, a->v_buffer,
			    min(a->length, c->length));
			break;
		case AML_OBJTYPE_STRING:
			c = aml_allocvalue(AML_OBJTYPE_INTEGER, 0, NULL);
			c->v_integer = aml_hextoint(a->v_string);
			break;
		case AML_OBJTYPE_UNINITIALIZED:
			c = aml_allocvalue(AML_OBJTYPE_INTEGER, 0, NULL);
			break;
		}
		break;
	case AML_OBJTYPE_STRING:
	case AML_OBJTYPE_HEXSTRING:
	case AML_OBJTYPE_DECSTRING:
		dnprintf(10,"convert to string\n");
		switch (a->type) {
		case AML_OBJTYPE_INTEGER:
			c = aml_allocvalue(AML_OBJTYPE_STRING, 20, NULL);
			snprintf(c->v_string, c->length, (ctype == AML_OBJTYPE_HEXSTRING) ?
			    "0x%llx" : "%lld", a->v_integer);
			break;
		case AML_OBJTYPE_BUFFER:
			c = aml_allocvalue(AML_OBJTYPE_STRING, a->length,
			    a->v_buffer);
			break;
		case AML_OBJTYPE_STRING:
			aml_xaddref(a, "XConvert");
			return a;
		}
		break;
	}
	if (c == NULL) {
#ifndef SMALL_KERNEL
		aml_showvalue(a, 0);
#endif
		aml_die("Could not convert %x to %x\n", a->type, ctype);
	}
	return c;
>>>>>>> origin/master
}

void
aml_create_defaultobjects()
{
	struct aml_value *tmp;
	struct aml_defval *def;

<<<<<<< HEAD
	for (def = aml_defobj; def->name; def++) {
		/* Allocate object value + add to namespace */
		tmp = aml_allocvalue(def->type, def->ival, def->bval);
		aml_createname(&aml_root, def->name, tmp);
		if (def->gval) {
			/* Set root object pointer */
			*def->gval = tmp;
=======
	/* Convert A2 to type of A1 */
	a2 = aml_xconvert(a2, a1->type, -1);
	if (a1->type == AML_OBJTYPE_INTEGER)
		rc = aml_evalexpr(a1->v_integer, a2->v_integer, opcode);
	else {
		/* Perform String/Buffer comparison */
		rc = memcmp(a1->v_buffer, a2->v_buffer,
		    min(a1->length, a2->length));
		if (rc == 0) {
			/* If buffers match, which one is longer */
			rc = a1->length - a2->length;
>>>>>>> origin/master
		}
	}
}

<<<<<<< HEAD
int
aml_print_resource(union acpi_resource *crs, void *arg)
=======
/* Concatenate two objects, returning pointer to new object */
struct aml_value *
aml_xconcat(struct aml_value *a1, struct aml_value *a2)
>>>>>>> origin/master
{
	int typ = AML_CRSTYPE(crs);

<<<<<<< HEAD
	switch (typ) {
	case LR_EXTIRQ:
		printf("extirq\tflags:%.2x len:%.2x irq:%.4x\n",
		    crs->lr_extirq.flags, crs->lr_extirq.irq_count,
		    aml_letohost32(crs->lr_extirq.irq[0]));
		break;
	case SR_IRQ:
		printf("irq\t%.4x %.2x\n", aml_letohost16(crs->sr_irq.irq_mask),
		    crs->sr_irq.irq_flags);
		break;
	case SR_DMA:
		printf("dma\t%.2x %.2x\n", crs->sr_dma.channel,
		    crs->sr_dma.flags);
		break;
	case SR_IOPORT:
		printf("ioport\tflags:%.2x _min:%.4x _max:%.4x _aln:%.2x _len:%.2x\n",
		    crs->sr_ioport.flags, crs->sr_ioport._min,
		    crs->sr_ioport._max, crs->sr_ioport._aln,
		    crs->sr_ioport._len);
		break;
	case SR_STARTDEP:
		printf("startdep\n");
		break;
	case SR_ENDDEP:
		printf("enddep\n");
		break;
	case LR_WORD:
	  	printf("word\ttype:%.2x flags:%.2x tflag:%.2x gra:%.4x min:%.4x max:%.4x tra:%.4x len:%.4x\n",
			crs->lr_word.type, crs->lr_word.flags, crs->lr_word.tflags,
			crs->lr_word._gra, crs->lr_word._min, crs->lr_word._max,
			crs->lr_word._tra, crs->lr_word._len);
		break;
	case LR_DWORD:
	  	printf("dword\ttype:%.2x flags:%.2x tflag:%.2x gra:%.8x min:%.8x max:%.8x tra:%.8x len:%.8x\n",
			crs->lr_dword.type, crs->lr_dword.flags, crs->lr_dword.tflags,
			crs->lr_dword._gra, crs->lr_dword._min, crs->lr_dword._max,
			crs->lr_dword._tra, crs->lr_dword._len);
		break;
	case LR_QWORD:
	  	printf("dword\ttype:%.2x flags:%.2x tflag:%.2x gra:%.16llx min:%.16llx max:%.16llx tra:%.16llx len:%.16llx\n",
			crs->lr_qword.type, crs->lr_qword.flags, crs->lr_qword.tflags,
			crs->lr_qword._gra, crs->lr_qword._min, crs->lr_qword._max,
			crs->lr_qword._tra, crs->lr_qword._len);
=======
	/* Convert arg2 to type of arg1 */
	a2 = aml_xconvert(a2, a1->type, -1);
	switch (a1->type) {
	case AML_OBJTYPE_INTEGER:
		c = aml_allocvalue(AML_OBJTYPE_BUFFER,
		    a1->length + a2->length, NULL);
		memcpy(c->v_buffer, &a1->v_integer, a1->length);
		memcpy(c->v_buffer+a1->length, &a2->v_integer, a2->length);
		break;
	case AML_OBJTYPE_BUFFER:
		c = aml_allocvalue(AML_OBJTYPE_BUFFER,
		    a1->length + a2->length, NULL);
		memcpy(c->v_buffer, a1->v_buffer, a1->length);
		memcpy(c->v_buffer+a1->length, a2->v_buffer, a2->length);
		break;
	case AML_OBJTYPE_STRING:
		c = aml_allocvalue(AML_OBJTYPE_STRING,
		    a1->length + a2->length, NULL);
		memcpy(c->v_string, a1->v_string, a1->length);
		memcpy(c->v_string+a1->length, a2->v_string, a2->length);
>>>>>>> origin/master
		break;
	default:
		printf("unknown type: %x\n", typ);
		break;
	}
<<<<<<< HEAD
	return (0);
}

union acpi_resource *aml_mapresource(union acpi_resource *);

union acpi_resource *
aml_mapresource(union acpi_resource *crs)
{
	static union acpi_resource map;
	int rlen;

	rlen = AML_CRSLEN(crs);
	if (rlen >= sizeof(map))
		return crs;

	memset(&map, 0, sizeof(map));
	memcpy(&map, crs, rlen);

	return &map;
}

int
aml_parse_resource(int length, uint8_t *buffer,
    int (*crs_enum)(union acpi_resource *, void *), void *arg)
{
	int off, rlen;
	union acpi_resource *crs;

	for (off = 0; off < length; off += rlen) {
		crs = (union acpi_resource *)(buffer+off);
	
		rlen = AML_CRSLEN(crs);
		if (crs->hdr.typecode == 0x79 || rlen <= 3)
			break;

		crs = aml_mapresource(crs);
#ifdef ACPI_DEBUG
		aml_print_resource(crs, NULL);
#endif
		crs_enum(crs, arg);
	}

	return 0;
}
=======
	/* Either deletes temp buffer, or decrease refcnt on original A2 */
	aml_xdelref(&a2, "xconcat");
	return c;
}

/* Calculate length of Resource Template */
int
aml_ccrlen(union acpi_resource *rs, void *arg)
{
	int *plen = arg;

	*plen += AML_CRSLEN(rs);
	return (0);
}

/* Concatenate resource templates, returning pointer to new object */
struct aml_value *
aml_xconcatres(struct aml_value *a1, struct aml_value *a2)
{
	struct aml_value *c;
	int l1 = 0, l2 = 0, l3 = 2;
	uint8_t a3[] = { 0x79, 0x00 };

	if (a1->type != AML_OBJTYPE_BUFFER || a2->type != AML_OBJTYPE_BUFFER)
		aml_die("concatres: not buffers\n");

	/* Walk a1, a2, get length minus end tags, concatenate buffers, add end tag */
	aml_parse_resource(a1, aml_ccrlen, &l1);
	aml_parse_resource(a2, aml_ccrlen, &l2);

	/* Concatenate buffers, add end tag */
	c = aml_allocvalue(AML_OBJTYPE_BUFFER, l1+l2+l3, NULL);
	memcpy(c->v_buffer,    a1->v_buffer, l1);
	memcpy(c->v_buffer+l1, a2->v_buffer, l2);
	memcpy(c->v_buffer+l1+l2, a3,           l3);

	return c;
}

/* Extract substring from string or buffer */
struct aml_value *
aml_xmid(struct aml_value *src, int index, int length)
{
	if (index > src->length)
		index = 0;
	if ((index + length) > src->length)
		length = src->length - index;
	return aml_allocvalue(src->type, length, src->v_buffer + index);
}

/*
 * Field I/O utility functions
 */
void aml_xcreatefield(struct aml_value *, int, struct aml_value *, int, int,
    struct aml_value *, int, int);
void aml_xparsefieldlist(struct aml_scope *, int, int,
    struct aml_value *, struct aml_value *, int);

#define GAS_PCI_CFG_SPACE_UNEVAL  0xCC
>>>>>>> origin/master

void
aml_foreachpkg(struct aml_value *pkg, int start, 
	       void (*fn)(struct aml_value *, void *), 
	       void *arg)
{
<<<<<<< HEAD
	int idx;
	
	if (pkg->type != AML_OBJTYPE_PACKAGE)
		return;
	for (idx=start; idx<pkg->length; idx++) 
		fn(pkg->v_package[idx], arg);
=======
	if (aml_evalname(acpi_softc, node, "_HID", 0, NULL, val))
		return (-1);

	/* Integer _HID: convert to EISA ID */
	if (val->type == AML_OBJTYPE_INTEGER)
		_aml_setvalue(val, AML_OBJTYPE_STRING, -1, aml_eisaid(val->v_integer));
	return (0);
>>>>>>> origin/master
}

void aml_rwfield(struct aml_value *, int, int, struct aml_value *, int);
void aml_rwgas(struct aml_value *, int, int, struct aml_value *, int, int);

/* Get PCI address for opregion objects */
int
<<<<<<< HEAD
acpi_parse_aml(struct acpi_softc *sc, u_int8_t *start, u_int32_t length)
{
	u_int8_t *end;

	dsdt_softc = sc;

	strlcpy(aml_root.name, "\\", sizeof(aml_root.name));
	if (aml_root.start == NULL) {
		aml_root.start = start;
		aml_root.end = start+length;
	}
	end = start+length;
	aml_parsenode(NULL, &aml_root, start, &end, NULL);
	dnprintf(50, " : parsed %d AML bytes\n", length);

	return (0);
}

/*
 * Walk nodes and perform fixups for nameref
 */
int aml_fixup_node(struct aml_node *, void *);

int aml_fixup_node(struct aml_node *node, void *arg)
{
	struct aml_value *val = arg;
	int i;

	if (node->value == NULL)
		return (0);
	if (arg == NULL)
		aml_fixup_node(node, node->value);
	else if (val->type == AML_OBJTYPE_NAMEREF) {
		node = aml_searchname(node, val->v_nameref);
		if (node && node->value) {
			_aml_setvalue(val, AML_OBJTYPE_OBJREF, -1,
			    node->value);
=======
aml_rdpciaddr(struct aml_node *pcidev, union amlpci_t *addr)
{
	int64_t res;

	if (aml_evalinteger(acpi_softc, pcidev, "_ADR", 0, NULL, &res) == 0) {
		addr->fun = res & 0xFFFF;
		addr->dev = res >> 16;
	}
	while (pcidev != NULL) {
		/* HID device (PCI or PCIE root): eval _BBN */
		if (__aml_search(pcidev, "_HID", 0)) {
			if (aml_evalinteger(acpi_softc, pcidev, "_BBN", 0, NULL, &res) == 0) {
				addr->bus = res;
				break;
			}
		}
		pcidev = pcidev->parent;
	}
	return (0);
}

/* Read/Write from opregion object */
void
aml_rwgas(struct aml_value *rgn, int bpos, int blen, struct aml_value *val, int mode, int flag)
{
	struct aml_value tmp;
	union amlpci_t pi;
	void *tbit, *vbit;
	int slen, type, sz;

	dnprintf(10," %5s %.2x %.8llx %.4x [%s]\n",
		mode == ACPI_IOREAD ? "read" : "write",
		rgn->v_opregion.iospace,
		rgn->v_opregion.iobase + (bpos >> 3),
		blen, aml_nodename(rgn->node));
	memset(&tmp, 0, sizeof(tmp));
	pi.addr = rgn->v_opregion.iobase + (bpos >> 3);
	if (rgn->v_opregion.iospace == GAS_PCI_CFG_SPACE)
	{
		/* Get PCI Root Address for this opregion */
		aml_rdpciaddr(rgn->node->parent, &pi);
	}

	/* Get field access size */
	switch (AML_FIELD_ACCESS(flag)) {
	case AML_FIELD_WORDACC:
		sz = 2;
		break;
	case AML_FIELD_DWORDACC:
		sz = 4;
		break;
	case AML_FIELD_QWORDACC:
		sz = 8;
		break;
	default:
		sz = 1;
		break;
	}

	tbit = &tmp.v_integer;
	vbit = &val->v_integer;
	slen = (blen + 7) >> 3;
	type = rgn->v_opregion.iospace;

	/* Allocate temporary storage */
	if (blen > aml_intlen) {
		if (mode == ACPI_IOREAD) {
			/* Read from a large field:  create buffer */
			_aml_setvalue(val, AML_OBJTYPE_BUFFER, slen, 0);
		} else {
			/* Write to a large field.. create or convert buffer */
			val = aml_xconvert(val, AML_OBJTYPE_BUFFER, -1);
		}
		_aml_setvalue(&tmp, AML_OBJTYPE_BUFFER, slen, 0);
		tbit = tmp.v_buffer;
		vbit = val->v_buffer;
	} else if (mode == ACPI_IOREAD) {
		/* Read from a short field.. initialize integer */
		_aml_setvalue(val, AML_OBJTYPE_INTEGER, 0, 0);
	} else {
		/* Write to a short field.. convert to integer */
		val = aml_xconvert(val, AML_OBJTYPE_INTEGER, -1);
	}

	if (mode == ACPI_IOREAD) {
		/* Read bits from opregion */
		acpi_gasio(acpi_softc, ACPI_IOREAD, type, pi.addr, sz, slen, tbit);
		aml_bufcpy(vbit, 0, tbit, bpos & 7, blen);
	} else {
		/* Write bits to opregion */
		if (val->length < slen) {
			dnprintf(0,"writetooshort: %d %d %s\n", val->length, slen, aml_nodename(rgn->node));
			slen = val->length;
		}
		if (AML_FIELD_UPDATE(flag) == AML_FIELD_PRESERVE && ((bpos|blen) & 7)) {
			/* If not aligned and preserve, read existing value */
			acpi_gasio(acpi_softc, ACPI_IOREAD, type, pi.addr, sz, slen, tbit);
		} else if (AML_FIELD_UPDATE(flag) == AML_FIELD_WRITEASONES) {
			memset(tbit, 0xFF, tmp.length);
		}
		/* Copy target bits, then write to region */
		aml_bufcpy(tbit, bpos & 7, vbit, 0, blen);
		acpi_gasio(acpi_softc, ACPI_IOWRITE, type, pi.addr, sz, slen, tbit);

		aml_xdelref(&val, "fld.write");
	}
	aml_freevalue(&tmp);
}

void
aml_rwfield(struct aml_value *fld, int bpos, int blen, struct aml_value *val, int mode)
{
	struct aml_value tmp, *ref1, *ref2;

	ref2 = fld->v_field.ref2;
	ref1 = fld->v_field.ref1;
	if (blen > fld->v_field.bitlen)
		blen = fld->v_field.bitlen;

	aml_lockfield(NULL, fld);
	memset(&tmp, 0, sizeof(tmp));
	aml_xaddref(&tmp, "fld.write");
	if (fld->v_field.type == AMLOP_INDEXFIELD) {
		_aml_setvalue(&tmp, AML_OBJTYPE_INTEGER, fld->v_field.ref3, 0);
		aml_rwfield(ref2, 0, aml_intlen, &tmp, ACPI_IOWRITE);
		aml_rwfield(ref1, fld->v_field.bitpos, fld->v_field.bitlen, val, mode);
	} else if (fld->v_field.type == AMLOP_BANKFIELD) {
		_aml_setvalue(&tmp, AML_OBJTYPE_INTEGER, fld->v_field.ref3, 0);
		aml_rwfield(ref2, 0, aml_intlen, &tmp, ACPI_IOWRITE);
		aml_rwgas(ref1, fld->v_field.bitpos, fld->v_field.bitlen, val, mode, fld->v_field.flags);
	} else if (fld->v_field.type == AMLOP_FIELD) {
		aml_rwgas(ref1, fld->v_field.bitpos+bpos, blen, val, mode, fld->v_field.flags);
	} else if (mode == ACPI_IOREAD) {
		/* bufferfield:read */
		_aml_setvalue(val, AML_OBJTYPE_INTEGER, 0, 0);
		aml_bufcpy(&val->v_integer, 0, ref1->v_buffer, fld->v_field.bitpos, fld->v_field.bitlen);
	} else {
		/* bufferfield:write */
		val = aml_xconvert(val, AML_OBJTYPE_INTEGER, -1);
		aml_bufcpy(ref1->v_buffer, fld->v_field.bitpos, &val->v_integer, 0, fld->v_field.bitlen);
		aml_xdelref(&val, "wrbuffld");
	}
	aml_unlockfield(NULL, fld);
}

/* Create Field Object          data		index
 *   AMLOP_FIELD		n:OpRegion	NULL
 *   AMLOP_INDEXFIELD		n:Field		n:Field
 *   AMLOP_BANKFIELD		n:OpRegion	n:Field
 *   AMLOP_CREATEFIELD		t:Buffer	NULL
 *   AMLOP_CREATEBITFIELD	t:Buffer	NULL
 *   AMLOP_CREATEBYTEFIELD	t:Buffer	NULL
 *   AMLOP_CREATEWORDFIELD	t:Buffer	NULL
 *   AMLOP_CREATEDWORDFIELD	t:Buffer	NULL
 *   AMLOP_CREATEQWORDFIELD	t:Buffer	NULL
 *   AMLOP_INDEX		t:Buffer	NULL
 */
void
aml_xcreatefield(struct aml_value *field, int opcode,
		struct aml_value *data, int bpos, int blen,
		struct aml_value *index, int indexval, int flags)
{
	dnprintf(10, "## %s(%s): %s %.4x-%.4x\n",
	    aml_mnem(opcode, 0),
	    blen > aml_intlen ? "BUF" : "INT",
	    aml_nodename(field->node), bpos, blen);
	if (index) {
		dnprintf(10, "  index:%s:%.2x\n", aml_nodename(index->node),
		    indexval);
	}
	dnprintf(10, "  data:%s\n", aml_nodename(data->node));
	field->type = (opcode == AMLOP_FIELD ||
	    opcode == AMLOP_INDEXFIELD ||
	    opcode == AMLOP_BANKFIELD) ?
	    AML_OBJTYPE_FIELDUNIT :
	    AML_OBJTYPE_BUFFERFIELD;
	if (opcode == AMLOP_INDEXFIELD) {
		indexval = bpos >> 3;
		bpos &= 7;
	}

	if (field->type == AML_OBJTYPE_BUFFERFIELD &&
	    data->type != AML_OBJTYPE_BUFFER)
	{
		printf("WARN: %s not buffer\n",
		    aml_nodename(data->node));
		data = aml_xconvert(data, AML_OBJTYPE_BUFFER, -1);
	}
	field->v_field.type = opcode;
	field->v_field.bitpos = bpos;
	field->v_field.bitlen = blen;
	field->v_field.ref3 = indexval;
	field->v_field.ref2 = index;
	field->v_field.ref1 = data;
	field->v_field.flags = flags;

	/* Increase reference count */
	aml_xaddref(data, "Field.Data");
	aml_xaddref(index, "Field.Index");
}

/* Parse Field/IndexField/BankField scope */
void
aml_xparsefieldlist(struct aml_scope *mscope, int opcode, int flags,
    struct aml_value *data, struct aml_value *index, int indexval)
{
	struct aml_value *rv;
	int bpos, blen;

	if (mscope == NULL)
		return;
	bpos = 0;
	while (mscope->pos < mscope->end) {
		switch (*mscope->pos) {
		case 0x00: // reserved, length
			mscope->pos++;
			blen = aml_parselength(mscope);
			break;
		case 0x01: // flags
			mscope->pos += 3;
			blen = 0;
			break;
		default: // 4-byte name, length
			mscope->pos = aml_parsename(mscope->node, mscope->pos,
			    &rv, 1);
			blen = aml_parselength(mscope);
			aml_xcreatefield(rv, opcode, data, bpos, blen, index,
				indexval, flags);
			aml_xdelref(&rv, 0);
			break;
		}
		bpos += blen;
	}
	aml_xpopscope(mscope);
}

/*
 * Mutex/Event utility functions
 */
int	acpi_xmutex_acquire(struct aml_scope *, struct aml_value *, int);
void	acpi_xmutex_release(struct aml_scope *, struct aml_value *);
int	acpi_xevent_wait(struct aml_scope *, struct aml_value *, int);
void	acpi_xevent_signal(struct aml_scope *, struct aml_value *);
void	acpi_xevent_reset(struct aml_scope *, struct aml_value *);

int
acpi_xmutex_acquire(struct aml_scope *scope, struct aml_value *mtx,
    int timeout)
{
	int err;

	if (mtx->v_mtx.owner == NULL || scope == mtx->v_mtx.owner) {
		/* We are now the owner */
		mtx->v_mtx.owner = scope;
		if (mtx == aml_global_lock) {
			dnprintf(10,"LOCKING GLOBAL\n");
			err = acpi_acquire_global_lock(&acpi_softc->sc_facs->global_lock);
		}
		dnprintf(5,"%s acquires mutex %s\n", scope->node->name,
		    mtx->node->name);
		return (0);
	} else if (timeout == 0) {
		return (1);
	}
	/* Wait for mutex */
	return (0);
}

void
acpi_xmutex_release(struct aml_scope *scope, struct aml_value *mtx)
{
	int err;

	if (mtx == aml_global_lock) {
		dnprintf(10,"UNLOCKING GLOBAL\n");
		err=acpi_release_global_lock(&acpi_softc->sc_facs->global_lock);
	}
	dnprintf(5, "%s releases mutex %s\n", scope->node->name,
	    mtx->node->name);
	mtx->v_mtx.owner = NULL;
	/* Wakeup waiters */
}

int
acpi_xevent_wait(struct aml_scope *scope, struct aml_value *evt, int timeout)
{
	/* Wait for event to occur; do work in meantime */
	evt->v_evt.state = 0;
	while (!evt->v_evt.state) {
		if (!acpi_dotask(acpi_softc) && !cold)
			tsleep(evt, PWAIT, "acpievt", 1);
		else
			delay(100);
	}
	if (evt->v_evt.state == 1) {
		/* Object is signaled */
		return (0);
	} else if (timeout == 0) {
		/* Zero timeout */
		return (1);
	}
	/* Wait for timeout or signal */
	return (0);
}

void
acpi_xevent_signal(struct aml_scope *scope, struct aml_value *evt)
{
	evt->v_evt.state = 1;
	/* Wakeup waiters */
}

void
acpi_xevent_reset(struct aml_scope *scope, struct aml_value *evt)
{
	evt->v_evt.state = 0;
}

/* Store result value into an object */
void
aml_xstore(struct aml_scope *scope, struct aml_value *lhs , int64_t ival,
    struct aml_value *rhs)
{
	struct aml_value tmp;
	int mlen;

	/* Already set */
	if (lhs == rhs || lhs == NULL || lhs->type == AML_OBJTYPE_NOTARGET) {
		return;
	}
	memset(&tmp, 0, sizeof(tmp));
	tmp.refcnt=99;
	if (rhs == NULL) {
		rhs = _aml_setvalue(&tmp, AML_OBJTYPE_INTEGER, ival, NULL);
	}
	if (rhs->type == AML_OBJTYPE_BUFFERFIELD ||
	    rhs->type == AML_OBJTYPE_FIELDUNIT) {
		aml_rwfield(rhs, 0, rhs->v_field.bitlen, &tmp, ACPI_IOREAD);
		rhs = &tmp;
	}
	/* Store to LocalX: free value */
	if (lhs->stack >= AMLOP_LOCAL0 && lhs->stack <= AMLOP_LOCAL7)
		aml_freevalue(lhs);

	lhs = aml_gettgt(lhs, AMLOP_STORE);
	switch (lhs->type) {
	case AML_OBJTYPE_UNINITIALIZED:
		aml_copyvalue(lhs, rhs);
		break;
	case AML_OBJTYPE_BUFFERFIELD:
	case AML_OBJTYPE_FIELDUNIT:
		aml_rwfield(lhs, 0, lhs->v_field.bitlen, rhs, ACPI_IOWRITE);
		break;
	case AML_OBJTYPE_DEBUGOBJ:
		break;
	case AML_OBJTYPE_INTEGER:
		rhs = aml_xconvert(rhs, lhs->type, -1);
		lhs->v_integer = rhs->v_integer;
		aml_xdelref(&rhs, "store.int");
		break;
	case AML_OBJTYPE_BUFFER:
	case AML_OBJTYPE_STRING:
		rhs = aml_xconvert(rhs, lhs->type, -1);
		if (lhs->length < rhs->length) {
			dnprintf(10,"Overrun! %d,%d\n", lhs->length, rhs->length);
			aml_freevalue(lhs);
			_aml_setvalue(lhs, rhs->type, rhs->length, NULL);
		}
		mlen = min(lhs->length, rhs->length);
		memset(lhs->v_buffer, 0x00, lhs->length);
		memcpy(lhs->v_buffer, rhs->v_buffer, mlen);
		aml_xdelref(&rhs, "store.bufstr");
		break;
	case AML_OBJTYPE_PACKAGE:
		/* Convert to LHS type, copy into LHS */
		if (rhs->type != AML_OBJTYPE_PACKAGE) {
			aml_die("Copy non-package into package?");
>>>>>>> origin/master
		}
	} else if (val->type == AML_OBJTYPE_PACKAGE) {
		for (i = 0; i < val->length; i++)
			aml_fixup_node(node, val->v_package[i]);
	} else if (val->type == AML_OBJTYPE_OPREGION) {
		if (val->v_opregion.iospace != GAS_PCI_CFG_SPACE)
			return (0);
		if (ACPI_PCI_FN(val->v_opregion.iobase) != 0xFFFF)
			return (0);
		val->v_opregion.iobase =
		    ACPI_PCI_REG(val->v_opregion.iobase) +
		    aml_getpciaddr(dsdt_softc, node);
		dnprintf(20, "late ioaddr : %s:%llx\n",
		    aml_nodename(node), val->v_opregion.iobase);
	}
<<<<<<< HEAD
	return (0);
}

void
aml_postparse()
{
	aml_walknodes(&aml_root, AML_WALK_PRE, aml_fixup_node, NULL);
=======
	aml_freevalue(&tmp);
}

#ifdef DDB
/* Disassembler routines */
void aml_disprintf(void *arg, const char *fmt, ...);

void
aml_disprintf(void *arg, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
aml_disasm(struct aml_scope *scope, int lvl,
    void (*dbprintf)(void *, const char *, ...),
    void *arg)
{
	int pc, opcode;
	struct aml_opcode *htab;
	uint64_t ival;
	struct aml_value *rv, tmp;
	uint8_t *end;
	struct aml_scope *ms;
	char *ch;
	char  mch[64];

	if (dbprintf == NULL)
		dbprintf = aml_disprintf;

	pc = aml_pc(scope->pos);
	opcode = aml_parseopcode(scope);
	htab = aml_findopcode(opcode);

	/* Display address + indent */
	if (lvl <= 0x7FFF) {
		dbprintf(arg, "%.4x ", pc);
		for (pc=0; pc<lvl; pc++) {
			dbprintf(arg, "	 ");
		}
	}
	ch = NULL;
	switch (opcode) {
	case AMLOP_NAMECHAR:
		scope->pos = aml_parsename(scope->node, scope->pos, &rv, 0);
		if (rv->type == AML_OBJTYPE_NAMEREF) {
			ch = "@@@";
			aml_xdelref(&rv, "disasm");
			break;
		}
		/* if this is a method, get arguments */
		strlcpy(mch, aml_nodename(rv->node), sizeof(mch));
		if (rv->type == AML_OBJTYPE_METHOD) {
			strlcat(mch, "(", sizeof(mch));
			for (ival=0; ival<AML_METHOD_ARGCOUNT(rv->v_method.flags); ival++) {
				strlcat(mch, ival ? ", %z" : "%z",
				    sizeof(mch));
			}
			strlcat(mch, ")", sizeof(mch));
		}
		aml_xdelref(&rv, "");
		ch = mch;
		break;

	case AMLOP_ZERO:
	case AMLOP_ONE:
	case AMLOP_ONES:
	case AMLOP_LOCAL0:
	case AMLOP_LOCAL1:
	case AMLOP_LOCAL2:
	case AMLOP_LOCAL3:
	case AMLOP_LOCAL4:
	case AMLOP_LOCAL5:
	case AMLOP_LOCAL6:
	case AMLOP_LOCAL7:
	case AMLOP_ARG0:
	case AMLOP_ARG1:
	case AMLOP_ARG2:
	case AMLOP_ARG3:
	case AMLOP_ARG4:
	case AMLOP_ARG5:
	case AMLOP_ARG6:
	case AMLOP_NOP:
	case AMLOP_REVISION:
	case AMLOP_DEBUG:
	case AMLOP_CONTINUE:
	case AMLOP_BREAKPOINT:
	case AMLOP_BREAK:
		ch="%m";
		break;
	case AMLOP_BYTEPREFIX:
		ch="%b";
		break;
	case AMLOP_WORDPREFIX:
		ch="%w";
		break;
	case AMLOP_DWORDPREFIX:
		ch="%d";
		break;
	case AMLOP_QWORDPREFIX:
		ch="%q";
		break;
	case AMLOP_STRINGPREFIX:
		ch="%a";
		break;

	case AMLOP_INCREMENT:
	case AMLOP_DECREMENT:
	case AMLOP_LNOT:
	case AMLOP_SIZEOF:
	case AMLOP_DEREFOF:
	case AMLOP_REFOF:
	case AMLOP_OBJECTTYPE:
	case AMLOP_UNLOAD:
	case AMLOP_RELEASE:
	case AMLOP_SIGNAL:
	case AMLOP_RESET:
	case AMLOP_STALL:
	case AMLOP_SLEEP:
	case AMLOP_RETURN:
		ch="%m(%n)";
		break;
	case AMLOP_OR:
	case AMLOP_ADD:
	case AMLOP_AND:
	case AMLOP_NAND:
	case AMLOP_XOR:
	case AMLOP_SHL:
	case AMLOP_SHR:
	case AMLOP_NOR:
	case AMLOP_MOD:
	case AMLOP_SUBTRACT:
	case AMLOP_MULTIPLY:
	case AMLOP_INDEX:
	case AMLOP_CONCAT:
	case AMLOP_CONCATRES:
	case AMLOP_TOSTRING:
		ch="%m(%n, %n, %n)";
		break;
	case AMLOP_CREATEBYTEFIELD:
	case AMLOP_CREATEWORDFIELD:
	case AMLOP_CREATEDWORDFIELD:
	case AMLOP_CREATEQWORDFIELD:
	case AMLOP_CREATEBITFIELD:
		ch="%m(%n, %n, %N)";
		break;
	case AMLOP_CREATEFIELD:
		ch="%m(%n, %n, %n, %N)";
		break;
	case AMLOP_DIVIDE:
	case AMLOP_MID:
		ch="%m(%n, %n, %n, %n)";
		break;
	case AMLOP_LAND:
	case AMLOP_LOR:
	case AMLOP_LNOTEQUAL:
	case AMLOP_LLESSEQUAL:
	case AMLOP_LLESS:
	case AMLOP_LEQUAL:
	case AMLOP_LGREATEREQUAL:
	case AMLOP_LGREATER:
	case AMLOP_NOT:
	case AMLOP_FINDSETLEFTBIT:
	case AMLOP_FINDSETRIGHTBIT:
	case AMLOP_TOINTEGER:
	case AMLOP_TOBUFFER:
	case AMLOP_TOHEXSTRING:
	case AMLOP_TODECSTRING:
	case AMLOP_FROMBCD:
	case AMLOP_TOBCD:
	case AMLOP_WAIT:
	case AMLOP_LOAD:
	case AMLOP_STORE:
	case AMLOP_NOTIFY:
	case AMLOP_COPYOBJECT:
		ch="%m(%n, %n)";
		break;
	case AMLOP_ACQUIRE:
		ch = "%m(%n, %w)";
		break;
	case AMLOP_CONDREFOF:
		ch="%m(%R, %n)";
		break;
	case AMLOP_ALIAS:
		ch="%m(%n, %N)";
		break;
	case AMLOP_NAME:
		ch="%m(%N, %n)";
		break;
	case AMLOP_EVENT:
		ch="%m(%N)";
		break;
	case AMLOP_MUTEX:
		ch = "%m(%N, %b)";
		break;
	case AMLOP_OPREGION:
		ch = "%m(%N, %b, %n, %n)";
		break;
	case AMLOP_DATAREGION:
		ch="%m(%N, %n, %n, %n)";
		break;
	case AMLOP_FATAL:
		ch = "%m(%b, %d, %n)";
		break;
	case AMLOP_IF:
	case AMLOP_WHILE:
	case AMLOP_SCOPE:
	case AMLOP_THERMALZONE:
	case AMLOP_VARPACKAGE:
		end = aml_parseend(scope);
		ch = "%m(%n) {\n%T}";
		break;
	case AMLOP_DEVICE:
		end = aml_parseend(scope);
		ch = "%m(%N) {\n%T}";
		break;
	case AMLOP_POWERRSRC:
		end = aml_parseend(scope);
		ch = "%m(%N, %b, %w) {\n%T}";
		break;
	case AMLOP_PROCESSOR:
		end = aml_parseend(scope);
		ch = "%m(%N, %b, %d, %b) {\n%T}";
		break;
	case AMLOP_METHOD:
		end = aml_parseend(scope);
		ch = "%m(%N, %b) {\n%T}";
		break;
	case AMLOP_PACKAGE:
		end = aml_parseend(scope);
		ch = "%m(%b) {\n%T}";
		break;
	case AMLOP_ELSE:
		end = aml_parseend(scope);
		ch = "%m {\n%T}";
		break;
	case AMLOP_BUFFER:
		end = aml_parseend(scope);
		ch = "%m(%n) { %B }";
		break;
	case AMLOP_INDEXFIELD:
		end = aml_parseend(scope);
		ch = "%m(%n, %n, %b) {\n%F}";
		break;
	case AMLOP_BANKFIELD:
		end = aml_parseend(scope);
		ch = "%m(%n, %n, %n, %b) {\n%F}";
		break;
	case AMLOP_FIELD:
		end = aml_parseend(scope);
		ch = "%m(%n, %b) {\n%F}";
		break;
	case AMLOP_MATCH:
		ch = "%m(%n, %b, %n, %b, %n, %n)";
		break;
	case AMLOP_LOADTABLE:
		ch = "%m(%n, %n, %n, %n, %n, %n)";
		break;
	default:
		aml_die("opcode = %x\n", opcode);
		break;
	}

	/* Parse printable buffer args */
	while (ch && *ch) {
		char c;

		if (*ch != '%') {
			dbprintf(arg,"%c", *(ch++));
			continue;
		}
		c = *(++ch);
		switch (c) {
		case 'b':
		case 'w':
		case 'd':
		case 'q':
			/* Parse simple object: don't allocate */
			aml_xparsesimple(scope, c, &tmp);
			dbprintf(arg,"0x%llx", tmp.v_integer);
			break;
		case 'a':
			dbprintf(arg, "\'%s\'", scope->pos);
			scope->pos += strlen(scope->pos)+1;
			break;
		case 'N':
			/* Create Name */
			rv = aml_xparsesimple(scope, c, NULL);
			dbprintf(arg,aml_nodename(rv->node));
			break;
		case 'm':
			/* display mnemonic */
			dbprintf(arg,htab->mnem);
			break;
		case 'R':
			/* Search name */
			printf("%s", aml_getname(scope->pos));
			scope->pos = aml_parsename(scope->node, scope->pos,
			    &rv, 0);
			aml_xdelref(&rv, 0);
			break;
		case 'z':
		case 'n':
			/* generic arg: recurse */
			aml_disasm(scope, lvl | 0x8000, dbprintf, arg);
			break;
		case 'B':
			/* Buffer */
			scope->pos = end;
			break;
		case 'F':
			/* Field List */
			tmp.v_buffer = scope->pos;
			tmp.length   = end - scope->pos;

			ms = aml_xpushscope(scope, &tmp, scope->node, 0);
			while (ms && ms->pos < ms->end) {
				if (*ms->pos == 0x00) {
					ms->pos++;
					aml_parselength(ms);
				} else if (*ms->pos == 0x01) {
					ms->pos+=3;
				} else {
					ms->pos = aml_parsename(ms->node,
					     ms->pos, &rv, 1);
					aml_parselength(ms);
					dbprintf(arg,"	%s\n",
					    aml_nodename(rv->node));
					aml_xdelref(&rv, 0);
				}
			}
			aml_xpopscope(ms);

			/* Display address and closing bracket */
			dbprintf(arg,"%.4x ", aml_pc(scope->pos));
			for (pc=0; pc<(lvl & 0x7FFF); pc++) {
				dbprintf(arg,"	");
			}
			scope->pos = end;
			break;
		case 'T':
			/* Scope: Termlist */
			tmp.v_buffer = scope->pos;
			tmp.length   = end - scope->pos;

			ms = aml_xpushscope(scope, &tmp, scope->node, 0);
			while (ms && ms->pos < ms->end) {
				aml_disasm(ms, (lvl + 1) & 0x7FFF,
				    dbprintf, arg);
			}
			aml_xpopscope(ms);

			/* Display address and closing bracket */
			dbprintf(arg,"%.4x ", aml_pc(scope->pos));
			for (pc=0; pc<(lvl & 0x7FFF); pc++) {
				dbprintf(arg,"	");
			}
			scope->pos = end;
			break;
		}
		ch++;
	}
	if (lvl <= 0x7FFF) {
		dbprintf(arg,"\n");
	}
}
#endif /* DDB */

int aml_busy;

/* Evaluate method or buffervalue objects */
struct aml_value *
aml_xeval(struct aml_scope *scope, struct aml_value *my_ret, int ret_type,
    int argc, struct aml_value *argv)
{
	struct aml_value *tmp = my_ret;
	struct aml_scope *ms;
	int idx;

	switch (tmp->type) {
	case AML_OBJTYPE_NAMEREF:
		my_ret = aml_seterror(scope, "Undefined name: %s",
		    aml_getname(my_ret->v_nameref));
		break;
	case AML_OBJTYPE_METHOD:
		dnprintf(10,"\n--== Eval Method [%s, %d args] to %c ==--\n",
		    aml_nodename(tmp->node),
		    AML_METHOD_ARGCOUNT(tmp->v_method.flags),
		    ret_type);
		ms = aml_xpushscope(scope, tmp, tmp->node, AMLOP_METHOD);

		/* Parse method arguments */
		for (idx=0; idx<AML_METHOD_ARGCOUNT(tmp->v_method.flags); idx++) {
			struct aml_value *sp;

			sp = aml_getstack(ms, AMLOP_ARG0+idx);
			if (argv) {
				aml_copyvalue(sp, &argv[idx]);
			} else {
				_aml_setvalue(sp, AML_OBJTYPE_OBJREF, AMLOP_ARG0 + idx, 0);
				sp->v_objref.ref = aml_xparse(scope, 't', "ARGX");
			}
		}
#ifdef ACPI_DEBUG
		aml_showstack(ms);
#endif

		/* Evaluate method scope */
		aml_root.start = tmp->v_method.base;
		if (tmp->v_method.fneval != NULL) {
			my_ret = tmp->v_method.fneval(ms, NULL);
		} else {
			aml_xparse(ms, 'T', "METHEVAL");
			my_ret = ms->retv;
		}
		dnprintf(10,"\n--==Finished evaluating method: %s %c\n",
		    aml_nodename(tmp->node), ret_type);
#ifdef ACPI_DEBUG
		aml_showvalue(my_ret, 0);
		aml_showstack(ms);
#endif
		aml_xpopscope(ms);
		break;
	case AML_OBJTYPE_BUFFERFIELD:
	case AML_OBJTYPE_FIELDUNIT:
		my_ret = aml_allocvalue(0,0,NULL);
		dnprintf(20,"quick: Convert Bufferfield to %c 0x%x\n",
		    ret_type, my_ret);
		aml_rwfield(tmp, 0, tmp->v_field.bitlen, my_ret, ACPI_IOREAD);
		break;
	}
	if (ret_type == 'i' && my_ret && my_ret->type != AML_OBJTYPE_INTEGER) {
#ifndef SMALL_KERNEL
		aml_showvalue(my_ret, 8-100);
#endif
		aml_die("Not Integer");
	}
	return my_ret;
}

/*
 * The following opcodes produce return values
 *   TOSTRING	-> Str
 *   TOHEXSTR	-> Str
 *   TODECSTR	-> Str
 *   STRINGPFX	-> Str
 *   BUFFER	-> Buf
 *   CONCATRES	-> Buf
 *   TOBUFFER	-> Buf
 *   MID	-> Buf|Str
 *   CONCAT	-> Buf|Str
 *   PACKAGE	-> Pkg
 *   VARPACKAGE -> Pkg
 *   LOCALx	-> Obj
 *   ARGx	-> Obj
 *   NAMECHAR	-> Obj
 *   REFOF	-> ObjRef
 *   INDEX	-> ObjRef
 *   DEREFOF	-> DataRefObj
 *   COPYOBJECT -> DataRefObj
 *   STORE	-> DataRefObj

 *   ZERO	-> Int
 *   ONE	-> Int
 *   ONES	-> Int
 *   REVISION	-> Int
 *   B/W/D/Q	-> Int
 *   OR		-> Int
 *   AND	-> Int
 *   ADD	-> Int
 *   NAND	-> Int
 *   XOR	-> Int
 *   SHL	-> Int
 *   SHR	-> Int
 *   NOR	-> Int
 *   MOD	-> Int
 *   SUBTRACT	-> Int
 *   MULTIPLY	-> Int
 *   DIVIDE	-> Int
 *   NOT	-> Int
 *   TOBCD	-> Int
 *   FROMBCD	-> Int
 *   FSLEFTBIT	-> Int
 *   FSRIGHTBIT -> Int
 *   INCREMENT	-> Int
 *   DECREMENT	-> Int
 *   TOINTEGER	-> Int
 *   MATCH	-> Int
 *   SIZEOF	-> Int
 *   OBJECTTYPE -> Int
 *   TIMER	-> Int

 *   CONDREFOF	-> Bool
 *   ACQUIRE	-> Bool
 *   WAIT	-> Bool
 *   LNOT	-> Bool
 *   LAND	-> Bool
 *   LOR	-> Bool
 *   LLESS	-> Bool
 *   LEQUAL	-> Bool
 *   LGREATER	-> Bool
 *   LNOTEQUAL	-> Bool
 *   LLESSEQUAL -> Bool
 *   LGREATEREQ -> Bool

 *   LOADTABLE	-> DDB
 *   DEBUG	-> Debug

 *   The following opcodes do not generate a return value:
 *   NOP
 *   BREAKPOINT
 *   RELEASE
 *   RESET
 *   SIGNAL
 *   NAME
 *   ALIAS
 *   OPREGION
 *   DATAREGION
 *   EVENT
 *   MUTEX
 *   SCOPE
 *   DEVICE
 *   THERMALZONE
 *   POWERRSRC
 *   PROCESSOR
 *   METHOD
 *   CREATEFIELD
 *   CREATEBITFIELD
 *   CREATEBYTEFIELD
 *   CREATEWORDFIELD
 *   CREATEDWORDFIELD
 *   CREATEQWORDFIELD
 *   FIELD
 *   INDEXFIELD
 *   BANKFIELD
 *   STALL
 *   SLEEP
 *   NOTIFY
 *   FATAL
 *   LOAD
 *   UNLOAD
 *   IF
 *   ELSE
 *   WHILE
 *   BREAK
 *   CONTINUE
 */

/* Parse a simple object from AML Bytestream */
struct aml_value *
aml_xparsesimple(struct aml_scope *scope, char ch, struct aml_value *rv)
{
	if (rv == NULL)
		rv = aml_allocvalue(0,0,NULL);
	switch (ch) {
	case AML_ARG_REVISION:
		_aml_setvalue(rv, AML_OBJTYPE_INTEGER, AML_REVISION, NULL);
		break;
	case AML_ARG_DEBUG:
		_aml_setvalue(rv, AML_OBJTYPE_DEBUGOBJ, 0, NULL);
		break;
	case AML_ARG_BYTE:
		_aml_setvalue(rv, AML_OBJTYPE_INTEGER,
		    aml_get8(scope->pos), NULL);
		scope->pos += 1;
		break;
	case AML_ARG_WORD:
		_aml_setvalue(rv, AML_OBJTYPE_INTEGER,
		    aml_get16(scope->pos), NULL);
		scope->pos += 2;
		break;
	case AML_ARG_DWORD:
		_aml_setvalue(rv, AML_OBJTYPE_INTEGER,
		    aml_get32(scope->pos), NULL);
		scope->pos += 4;
		break;
	case AML_ARG_QWORD:
		_aml_setvalue(rv, AML_OBJTYPE_INTEGER,
		    aml_get64(scope->pos), NULL);
		scope->pos += 8;
		break;
	case AML_ARG_STRING:
		_aml_setvalue(rv, AML_OBJTYPE_STRING, -1, scope->pos);
		scope->pos += rv->length+1;
		break;
	}
	return rv;
}

/*
 * Main Opcode Parser/Evaluator
 *
 * ret_type is expected type for return value
 *   'o' = Data Object (Int/Str/Buf/Pkg/Name)
 *   'i' = Integer
 *   't' = TermArg     (Int/Str/Buf/Pkg)
 *   'r' = Target      (NamedObj/Local/Arg/Null)
 *   'S' = SuperName   (NamedObj/Local/Arg)
 *   'T' = TermList
 */
#define aml_debugger(x)

int maxdp;

struct aml_value *
aml_gettgt(struct aml_value *val, int opcode)
{
	while (val && val->type == AML_OBJTYPE_OBJREF) {
		val = val->v_objref.ref;
	}
	return val;
}

struct aml_value *
aml_seterror(struct aml_scope *scope, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("### AML PARSE ERROR (0x%x): ", aml_pc(scope->pos));
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

	while (scope) {
		scope->pos = scope->end;
		scope = scope->parent;
	}
	aml_error++;
	return aml_allocvalue(AML_OBJTYPE_INTEGER, 0, 0);
}

/* Load new SSDT scope from memory address */
struct aml_scope *
aml_load(struct acpi_softc *sc, struct aml_scope *scope,
    struct aml_value *rgn, struct aml_value *ddb)
{
	struct acpi_q *entry;
	struct acpi_dsdt *p_ssdt;
	struct aml_value tmp;

	ddb->type = AML_OBJTYPE_DDBHANDLE;
	ddb->v_integer = 0;

	memset(&tmp, 0, sizeof(tmp));
	if (rgn->type != AML_OBJTYPE_OPREGION ||
	    rgn->v_opregion.iospace != GAS_SYSTEM_MEMORY)
		goto fail;

	/* Load SSDT from memory */
	entry = acpi_maptable(sc, rgn->v_opregion.iobase, "SSDT", NULL, NULL, 1);
	if (entry == NULL)
		goto fail;

	dnprintf(10, "%s: loaded SSDT %s @ %llx\n", sc->sc_dev.dv_xname,
	    aml_nodename(rgn->node), rgn->v_opregion.iobase);
	ddb->v_integer = entry->q_id;

	p_ssdt = entry->q_table;
	tmp.v_buffer = p_ssdt->aml;
	tmp.length   = p_ssdt->hdr_length - sizeof(p_ssdt->hdr);

	return aml_xpushscope(scope, &tmp, scope->node,
	    AMLOP_LOAD);
fail:
	printf("%s: unable to load %s\n", sc->sc_dev.dv_xname,
	    aml_nodename(rgn->node));
	return NULL;
}

struct aml_value *
aml_xparse(struct aml_scope *scope, int ret_type, const char *stype)
{
	int    opcode, idx, pc;
	struct aml_opcode *htab;
	struct aml_value *opargs[8], *my_ret, *rv;
	struct aml_scope *mscope, *iscope;
	uint8_t *start, *end;
	const char *ch;
	int64_t ival;

	my_ret = NULL;
	if (scope == NULL || scope->pos >= scope->end) {
		return NULL;
	}
	if (odp++ > 125)
		panic("depth");
	if (odp > maxdp) {
		maxdp = odp;
		dnprintf(10, "max depth: %d\n", maxdp);
	}
	end = NULL;
	iscope = scope;
 start:
	/* --== Stage 0: Get Opcode ==-- */
	start = scope->pos;
	pc = aml_pc(scope->pos);
	aml_debugger(scope);

	opcode = aml_parseopcode(scope);
	htab = aml_findopcode(opcode);
	if (htab == NULL) {
		/* No opcode handler */
		aml_die("Unknown opcode: %.4x @ %.4x", opcode, pc);
	}
	dnprintf(18,"%.4x %s\n", pc, aml_mnem(opcode, scope->pos));

	/* --== Stage 1: Process opcode arguments ==-- */
	memset(opargs, 0, sizeof(opargs));
	idx = 0;
	for (ch = htab->args; *ch; ch++) {
		rv = NULL;
		switch (*ch) {
		case AML_ARG_OBJLEN:
			end = aml_parseend(scope);
			break;
		case AML_ARG_IFELSE:
                        /* Special Case: IF-ELSE:piTbpT or IF:piT */
			ch = (*end == AMLOP_ELSE && end < scope->end) ?
			    "-TbpT" : "-T";
			break;

			/* Complex arguments */
		case 's':
		case 'S':
		case AML_ARG_TARGET:
		case AML_ARG_TERMOBJ:
		case AML_ARG_INTEGER:
			if (*ch == 'r' && *scope->pos == AMLOP_ZERO) {
				/* Special case: NULL Target */
				rv = aml_allocvalue(AML_OBJTYPE_NOTARGET, 0, NULL);
				scope->pos++;
			}
			else {
				rv = aml_xparse(scope, *ch, htab->mnem);
				if (rv == NULL || aml_error)
					goto parse_error;
			}
			break;

			/* Simple arguments */
		case AML_ARG_BUFFER:
		case AML_ARG_METHOD:
		case AML_ARG_FIELDLIST:
		case AML_ARG_TERMOBJLIST:
			rv = aml_allocvalue(AML_OBJTYPE_SCOPE, 0, NULL);
			rv->v_buffer = scope->pos;
			rv->length = end - scope->pos;
			scope->pos = end;
			break;
		case AML_ARG_CONST:
			rv = aml_allocvalue(AML_OBJTYPE_INTEGER,
			    (char)opcode, NULL);
			break;
		case AML_ARG_CREATENAME:
			scope->pos = aml_parsename(scope->node, scope->pos,
			    &rv, 1);
			break;
		case AML_ARG_SEARCHNAME:
			scope->pos = aml_parsename(scope->node, scope->pos,
			    &rv, 0);
			break;
		case AML_ARG_BYTE:
		case AML_ARG_WORD:
		case AML_ARG_DWORD:
		case AML_ARG_QWORD:
		case AML_ARG_DEBUG:
		case AML_ARG_STRING:
		case AML_ARG_REVISION:
			rv = aml_xparsesimple(scope, *ch, NULL);
			break;
		case AML_ARG_STKLOCAL:
		case AML_ARG_STKARG:
			rv = aml_getstack(scope, opcode);
			break;
		default:
			aml_die("Unknown arg type: %c\n", *ch);
			break;
		}
		if (rv != NULL)
			opargs[idx++] = rv;
		}

	/* --== Stage 2: Process opcode ==-- */
	ival = 0;
	my_ret = NULL;
	mscope = NULL;
	switch (opcode) {
	case AMLOP_NOP:
	case AMLOP_BREAKPOINT:
		break;
	case AMLOP_LOCAL0:
	case AMLOP_LOCAL1:
	case AMLOP_LOCAL2:
	case AMLOP_LOCAL3:
	case AMLOP_LOCAL4:
	case AMLOP_LOCAL5:
	case AMLOP_LOCAL6:
	case AMLOP_LOCAL7:
	case AMLOP_ARG0:
	case AMLOP_ARG1:
	case AMLOP_ARG2:
	case AMLOP_ARG3:
	case AMLOP_ARG4:
	case AMLOP_ARG5:
	case AMLOP_ARG6:
		my_ret = opargs[0];
		aml_xaddref(my_ret, htab->mnem);
		break;
	case AMLOP_NAMECHAR:
		/* opargs[0] = named object (node != NULL), or nameref */
		my_ret = opargs[0];
		if (scope->type == AMLOP_PACKAGE) {
			/* Special case for package */
			if (my_ret->type == AML_OBJTYPE_NAMEREF)
				my_ret = aml_allocvalue(AML_OBJTYPE_STRING, -1,
				    aml_getname(my_ret->v_nameref));
			else if (my_ret->node)
				my_ret = aml_allocvalue(AML_OBJTYPE_STRING, -1,
				    aml_nodename(my_ret->node));
			break;
		}
		if (my_ret->type == AML_OBJTYPE_OBJREF) {
			my_ret = my_ret->v_objref.ref;
			aml_xaddref(my_ret, "de-alias");
		}
		if (ret_type == 'i' || ret_type == 't' || ret_type == 'T') {
			/* Return TermArg or Integer: Evaluate object */
			my_ret = aml_xeval(scope, my_ret, ret_type, 0, NULL);
		} else if (my_ret->type == AML_OBJTYPE_METHOD) {
			/* This should only happen with CondRef */
			dnprintf(12,"non-termarg method : %s\n", stype);
			aml_xaddref(my_ret, "zoom");
		}
		break;

	case AMLOP_ZERO:
	case AMLOP_ONE:
	case AMLOP_ONES:
	case AMLOP_DEBUG:
	case AMLOP_REVISION:
	case AMLOP_BYTEPREFIX:
	case AMLOP_WORDPREFIX:
	case AMLOP_DWORDPREFIX:
	case AMLOP_QWORDPREFIX:
	case AMLOP_STRINGPREFIX:
		my_ret = opargs[0];
		break;

	case AMLOP_BUFFER:
		/* Buffer: iB => Buffer */
		my_ret = aml_allocvalue(AML_OBJTYPE_BUFFER,
		    opargs[0]->v_integer, NULL);
		memcpy(my_ret->v_buffer, opargs[1]->v_buffer,
		    opargs[1]->length);
		break;
	case AMLOP_PACKAGE:
	case AMLOP_VARPACKAGE:
		/* Package/VarPackage: bT/iT => Package */
		my_ret = aml_allocvalue(AML_OBJTYPE_PACKAGE,
		    opargs[0]->v_integer, 0);
		mscope = aml_xpushscope(scope, opargs[1], scope->node,
		    AMLOP_PACKAGE);

		/* Recursively parse package contents */
		for (idx=0; idx<my_ret->length; idx++) {
			rv = aml_xparse(mscope, 'o', "Package");
			if (rv != NULL) {
				aml_xdelref(&my_ret->v_package[idx], "pkginit");
				my_ret->v_package[idx] = rv;
			}
		}
		aml_xpopscope(mscope);
		mscope = NULL;
		break;

		/* Math/Logical operations */
	case AMLOP_OR:
	case AMLOP_ADD:
	case AMLOP_AND:
	case AMLOP_NAND:
	case AMLOP_XOR:
	case AMLOP_SHL:
	case AMLOP_SHR:
	case AMLOP_NOR:
	case AMLOP_MOD:
	case AMLOP_SUBTRACT:
	case AMLOP_MULTIPLY:
		/* XXX: iir => I */
		ival = aml_evalexpr(opargs[0]->v_integer,
		    opargs[1]->v_integer, opcode);
		aml_xstore(scope, opargs[2], ival, NULL);
		break;
	case AMLOP_DIVIDE:
		/* Divide: iirr => I */
		if (opargs[1]->v_integer == 0) {
			my_ret = aml_seterror(scope, "Divide by Zero!");
			break;
		}
		ival = aml_evalexpr(opargs[0]->v_integer,
		    opargs[1]->v_integer, AMLOP_MOD);
		aml_xstore(scope, opargs[2], ival, NULL);

		ival = aml_evalexpr(opargs[0]->v_integer,
		    opargs[1]->v_integer, AMLOP_DIVIDE);
		aml_xstore(scope, opargs[3], ival, NULL);
		break;
	case AMLOP_NOT:
	case AMLOP_TOBCD:
	case AMLOP_FROMBCD:
	case AMLOP_FINDSETLEFTBIT:
	case AMLOP_FINDSETRIGHTBIT:
		/* XXX: ir => I */
		ival = aml_evalexpr(opargs[0]->v_integer, 0, opcode);
		aml_xstore(scope, opargs[1], ival, NULL);
		break;
	case AMLOP_INCREMENT:
	case AMLOP_DECREMENT:
		/* Inc/Dec: S => I */
		my_ret = aml_xeval(scope, opargs[0], AML_ARG_INTEGER, 0, NULL);
		ival = aml_evalexpr(my_ret->v_integer, 1, opcode);
		aml_xstore(scope, opargs[0], ival, NULL);
		break;
	case AMLOP_LNOT:
		/* LNot: i => Bool */
		ival = aml_evalexpr(opargs[0]->v_integer, 0, opcode);
		break;
	case AMLOP_LOR:
	case AMLOP_LAND:
		/* XXX: ii => Bool */
		ival = aml_evalexpr(opargs[0]->v_integer,
		    opargs[1]->v_integer, opcode);
		break;
	case AMLOP_LLESS:
	case AMLOP_LEQUAL:
	case AMLOP_LGREATER:
	case AMLOP_LNOTEQUAL:
	case AMLOP_LLESSEQUAL:
	case AMLOP_LGREATEREQUAL:
		/* XXX: tt => Bool */
		ival = aml_xcompare(opargs[0], opargs[1], opcode);
		break;

		/* Reference/Store operations */
	case AMLOP_CONDREFOF:
		/* CondRef: rr => I */
		ival = 0;
		if (opargs[0]->node != NULL) {
			/* Create Object Reference */
			opargs[2] = aml_allocvalue(AML_OBJTYPE_OBJREF, opcode,
				opargs[0]);
			aml_xaddref(opargs[0], "CondRef");
			aml_xstore(scope, opargs[1], 0, opargs[2]);

			/* Mark that we found it */
			ival = -1;
		}
		break;
	case AMLOP_REFOF:
		/* RefOf: r => ObjRef */
		my_ret = aml_allocvalue(AML_OBJTYPE_OBJREF, opcode, opargs[0]);
		aml_xaddref(my_ret->v_objref.ref, "RefOf");
		break;
	case AMLOP_INDEX:
		/* Index: tir => ObjRef */
		idx = opargs[1]->v_integer;
		if (idx >= opargs[0]->length || idx < 0) {
#ifndef SMALL_KERNEL
			aml_showvalue(opargs[0], 0);
#endif
			aml_die("Index out of bounds %d/%d\n", idx,
			    opargs[0]->length);
		}
		switch (opargs[0]->type) {
		case AML_OBJTYPE_PACKAGE:
			/* Don't set opargs[0] to NULL */
			if (ret_type == 't' || ret_type == 'i' || ret_type == 'T') {
				my_ret = opargs[0]->v_package[idx];
				aml_xaddref(my_ret, "Index.Package");
			} else {
				my_ret = aml_allocvalue(AML_OBJTYPE_OBJREF, AMLOP_PACKAGE,
				    opargs[0]->v_package[idx]);
				aml_xaddref(my_ret->v_objref.ref,
				    "Index.Package");
			}
			break;
		case AML_OBJTYPE_BUFFER:
		case AML_OBJTYPE_STRING:
		case AML_OBJTYPE_INTEGER:
			rv = aml_xconvert(opargs[0], AML_OBJTYPE_BUFFER, -1);
			if (ret_type == 't' || ret_type == 'i' || ret_type == 'T') {
				dnprintf(12,"Index.Buf Term: %d = %x\n",
				    idx, rv->v_buffer[idx]);
				ival = rv->v_buffer[idx];
			} else {
				dnprintf(12, "Index.Buf Targ\n");
				my_ret = aml_allocvalue(0,0,NULL);
				aml_xcreatefield(my_ret, AMLOP_INDEX, rv,
				    8 * idx, 8, NULL, 0, AML_FIELD_BYTEACC);
			}
			aml_xdelref(&rv, "Index.BufStr");
			break;
		default:
			aml_die("Unknown index : %x\n", opargs[0]->type);
			break;
		}
		aml_xstore(scope, opargs[2], ival, my_ret);
		break;
	case AMLOP_DEREFOF:
		/* DerefOf: t:ObjRef => DataRefObj */
		if (opargs[0]->type == AML_OBJTYPE_OBJREF) {
			my_ret = opargs[0]->v_objref.ref;
			aml_xaddref(my_ret, "DerefOf");
		} else {
			my_ret = opargs[0];
			//aml_xaddref(my_ret, "DerefOf");
		}
		break;
	case AMLOP_COPYOBJECT:
		/* CopyObject: t:DataRefObj, s:implename => DataRefObj */
		my_ret = opargs[0];
		aml_freevalue(opargs[1]);
		aml_copyvalue(opargs[1], opargs[0]);
		break;
	case AMLOP_STORE:
		/* Store: t:DataRefObj, S:upername => DataRefObj */
		my_ret = opargs[0];
		aml_xstore(scope, opargs[1], 0, opargs[0]);
		break;

		/* Conversion */
	case AMLOP_TOINTEGER:
		/* Source:CData, Result => Integer */
		my_ret = aml_xconvert(opargs[0], AML_OBJTYPE_INTEGER, -1);
		aml_xstore(scope, opargs[1], 0, my_ret);
		break;
	case AMLOP_TOBUFFER:
		/* Source:CData, Result => Buffer */
		my_ret = aml_xconvert(opargs[0], AML_OBJTYPE_BUFFER, -1);
		aml_xstore(scope, opargs[1], 0, my_ret);
		break;
	case AMLOP_TOHEXSTRING:
		/* Source:CData, Result => String */
		my_ret = aml_xconvert(opargs[0], AML_OBJTYPE_HEXSTRING, -1);
		aml_xstore(scope, opargs[1], 0, my_ret);
		break;
	case AMLOP_TODECSTRING:
		/* Source:CData, Result => String */
		my_ret = aml_xconvert(opargs[0], AML_OBJTYPE_DECSTRING, -1);
		aml_xstore(scope, opargs[1], 0, my_ret);
		break;
	case AMLOP_TOSTRING:
		/* Source:B, Length:I, Result => String */
		my_ret = aml_xconvert(opargs[0], AML_OBJTYPE_STRING,
		    opargs[1]->v_integer);
		aml_xstore(scope, opargs[2], 0, my_ret);
		break;
	case AMLOP_CONCAT:
		/* Source1:CData, Source2:CData, Result => CData */
		my_ret = aml_xconcat(opargs[0], opargs[1]);
		aml_xstore(scope, opargs[2], 0, my_ret);
		break;
	case AMLOP_CONCATRES:
		/* Concat two resource buffers: buf1, buf2, result => Buffer */
		my_ret = aml_xconcatres(opargs[0], opargs[1]);
		aml_xstore(scope, opargs[2], 0, my_ret);
		break;
	case AMLOP_MID:
		/* Source:BS, Index:I, Length:I, Result => BS */
		my_ret = aml_xmid(opargs[0], opargs[1]->v_integer,
		    opargs[2]->v_integer);
		aml_xstore(scope, opargs[3], 0, my_ret);
		break;
	case AMLOP_MATCH:
		/* Match: Pkg, Op1, Val1, Op2, Val2, Index */
		ival = aml_xmatch(opargs[0], opargs[5]->v_integer,
		    opargs[1]->v_integer, opargs[2]->v_integer,
		    opargs[3]->v_integer, opargs[4]->v_integer);
		break;
	case AMLOP_SIZEOF:
		/* Sizeof: S => i */
		rv = aml_gettgt(opargs[0], opcode);
		ival = rv->length;
		break;
	case AMLOP_OBJECTTYPE:
		/* ObjectType: S => i */
		rv = aml_gettgt(opargs[0], opcode);
		ival = rv->type;
		break;

		/* Mutex/Event handlers */
	case AMLOP_ACQUIRE:
		/* Acquire: Sw => Bool */
		rv = aml_gettgt(opargs[0], opcode);
		ival = acpi_xmutex_acquire(scope, rv,
		    opargs[1]->v_integer);
		break;
	case AMLOP_RELEASE:
		/* Release: S */
		rv = aml_gettgt(opargs[0], opcode);
		acpi_xmutex_release(scope, rv);
		break;
	case AMLOP_WAIT:
		/* Wait: Si => Bool */
		rv = aml_gettgt(opargs[0], opcode);
		ival = acpi_xevent_wait(scope, rv,
		    opargs[1]->v_integer);
		break;
	case AMLOP_RESET:
		/* Reset: S */
		rv = aml_gettgt(opargs[0], opcode);
		acpi_xevent_reset(scope, rv);
		break;
	case AMLOP_SIGNAL:
		/* Signal: S */
		rv = aml_gettgt(opargs[0], opcode);
		acpi_xevent_signal(scope, rv);
		break;

		/* Named objects */
	case AMLOP_NAME:
		/* Name: Nt */
		rv = opargs[0];
		aml_freevalue(rv);
			aml_copyvalue(rv, opargs[1]);
		break;
	case AMLOP_ALIAS:
		/* Alias: nN */
		rv = _aml_setvalue(opargs[1], AML_OBJTYPE_OBJREF, opcode, 0);
		rv->v_objref.ref = aml_gettgt(opargs[0], opcode);
		aml_xaddref(rv->v_objref.ref, "Alias");
		break;
	case AMLOP_OPREGION:
		/* OpRegion: Nbii */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_OPREGION, 0, 0);
		rv->v_opregion.iospace = opargs[1]->v_integer;
		rv->v_opregion.iobase = opargs[2]->v_integer;
		rv->v_opregion.iolen = opargs[3]->v_integer;
		rv->v_opregion.flag = 0;
		break;
	case AMLOP_DATAREGION:
		/* DataTableRegion: N,t:SigStr,t:OemIDStr,t:OemTableIDStr */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_OPREGION, 0, 0);
		rv->v_opregion.iospace = GAS_SYSTEM_MEMORY;
		rv->v_opregion.iobase = 0;
		rv->v_opregion.iolen = 0;
		aml_die("AML-DataTableRegion\n");
		break;
	case AMLOP_EVENT:
		/* Event: N */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_EVENT, 0, 0);
		rv->v_integer = 0;
		break;
	case AMLOP_MUTEX:
		/* Mutex: Nw */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_MUTEX, 0, 0);
		rv->v_mtx.synclvl = opargs[1]->v_integer;
		break;
	case AMLOP_SCOPE:
		/* Scope: NT */
		rv = opargs[0];
		if (rv->type == AML_OBJTYPE_NAMEREF) {
			printf("Undefined scope: %s\n", aml_getname(rv->v_nameref));
			break;
		}
		mscope = aml_xpushscope(scope, opargs[1], rv->node, opcode);
		break;
	case AMLOP_DEVICE:
		/* Device: NT */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_DEVICE, 0, 0);
		mscope = aml_xpushscope(scope, opargs[1], rv->node, opcode);
		break;
	case AMLOP_THERMALZONE:
		/* ThermalZone: NT */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_THERMZONE, 0, 0);
		mscope = aml_xpushscope(scope, opargs[1], rv->node, opcode);
		break;
	case AMLOP_POWERRSRC:
		/* PowerRsrc: NbwT */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_POWERRSRC, 0, 0);
		rv->v_powerrsrc.pwr_level = opargs[1]->v_integer;
		rv->v_powerrsrc.pwr_order = opargs[2]->v_integer;
		mscope = aml_xpushscope(scope, opargs[3], rv->node, opcode);
		break;
	case AMLOP_PROCESSOR:
		/* Processor: NbdbT */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_PROCESSOR, 0, 0);
		rv->v_processor.proc_id = opargs[1]->v_integer;
		rv->v_processor.proc_addr = opargs[2]->v_integer;
		rv->v_processor.proc_len = opargs[3]->v_integer;
		mscope = aml_xpushscope(scope, opargs[4], rv->node, opcode);
		break;
	case AMLOP_METHOD:
		/* Method: NbM */
		rv = _aml_setvalue(opargs[0], AML_OBJTYPE_METHOD, 0, 0);
		rv->v_method.flags = opargs[1]->v_integer;
		rv->v_method.start = opargs[2]->v_buffer;
		rv->v_method.end = rv->v_method.start + opargs[2]->length;
		rv->v_method.base = aml_root.start;
		break;

		/* Field objects */
	case AMLOP_CREATEFIELD:
		/* Source:B, BitIndex:I, NumBits:I, FieldName */
		rv = _aml_setvalue(opargs[3], AML_OBJTYPE_BUFFERFIELD, 0, 0);
		aml_xcreatefield(rv, opcode, opargs[0], opargs[1]->v_integer,
		    opargs[2]->v_integer, NULL, 0, 0);
		break;
	case AMLOP_CREATEBITFIELD:
		/* Source:B, BitIndex:I, FieldName */
		rv = _aml_setvalue(opargs[2], AML_OBJTYPE_BUFFERFIELD, 0, 0);
		aml_xcreatefield(rv, opcode, opargs[0], opargs[1]->v_integer,
		    1, NULL, 0, 0);
		break;
	case AMLOP_CREATEBYTEFIELD:
		/* Source:B, ByteIndex:I, FieldName */
		rv = _aml_setvalue(opargs[2], AML_OBJTYPE_BUFFERFIELD, 0, 0);
		aml_xcreatefield(rv, opcode, opargs[0], opargs[1]->v_integer*8,
		    8, NULL, 0, AML_FIELD_BYTEACC);
		break;
	case AMLOP_CREATEWORDFIELD:
		/* Source:B, ByteIndex:I, FieldName */
		rv = _aml_setvalue(opargs[2], AML_OBJTYPE_BUFFERFIELD, 0, 0);
		aml_xcreatefield(rv, opcode, opargs[0], opargs[1]->v_integer*8,
		    16, NULL, 0, AML_FIELD_WORDACC);
		break;
	case AMLOP_CREATEDWORDFIELD:
		/* Source:B, ByteIndex:I, FieldName */
		rv = _aml_setvalue(opargs[2], AML_OBJTYPE_BUFFERFIELD, 0, 0);
		aml_xcreatefield(rv, opcode, opargs[0], opargs[1]->v_integer*8,
		    32, NULL, 0, AML_FIELD_DWORDACC);
		break;
	case AMLOP_CREATEQWORDFIELD:
		/* Source:B, ByteIndex:I, FieldName */
		rv = _aml_setvalue(opargs[2], AML_OBJTYPE_BUFFERFIELD, 0, 0);
		aml_xcreatefield(rv, opcode, opargs[0], opargs[1]->v_integer*8,
		    64, NULL, 0, AML_FIELD_QWORDACC);
		break;
	case AMLOP_FIELD:
		/* Field: n:OpRegion, b:Flags, F:ieldlist */
		mscope = aml_xpushscope(scope, opargs[2], scope->node, opcode);
		aml_xparsefieldlist(mscope, opcode, opargs[1]->v_integer,
		    opargs[0], NULL, 0);
		mscope = NULL;
		break;
	case AMLOP_INDEXFIELD:
		/* IndexField: n:Index, n:Data, b:Flags, F:ieldlist */
		mscope = aml_xpushscope(scope, opargs[3], scope->node, opcode);
		aml_xparsefieldlist(mscope, opcode, opargs[2]->v_integer,
		    opargs[1], opargs[0], 0);
		mscope = NULL;
		break;
	case AMLOP_BANKFIELD:
		/* BankField: n:OpRegion, n:Field, i:Bank, b:Flags, F:ieldlist */
		mscope = aml_xpushscope(scope, opargs[4], scope->node, opcode);
		aml_xparsefieldlist(mscope, opcode, opargs[3]->v_integer,
		    opargs[0], opargs[1], opargs[2]->v_integer);
		mscope = NULL;
		break;

		/* Misc functions */
	case AMLOP_STALL:
		/* Stall: i */
		acpi_stall(opargs[0]->v_integer);
		break;
	case AMLOP_SLEEP:
		/* Sleep: i */
		acpi_sleep(opargs[0]->v_integer, "amlsleep");
		break;
	case AMLOP_NOTIFY:
		/* Notify: Si */
		rv = aml_gettgt(opargs[0], opcode);
		dnprintf(50,"Notifying: %s %x\n",
		    aml_nodename(rv->node),
		    opargs[1]->v_integer);
		aml_notify(rv->node, opargs[1]->v_integer);
		break;
	case AMLOP_TIMER:
		/* Timer: => i */
		ival = 0xDEADBEEF;
		break;
	case AMLOP_FATAL:
		/* Fatal: bdi */
		aml_die("AML FATAL ERROR: %x,%x,%x\n",
		    opargs[0]->v_integer, opargs[1]->v_integer,
		    opargs[2]->v_integer);
		break;
	case AMLOP_LOADTABLE:
		/* LoadTable(Sig:Str, OEMID:Str, OEMTable:Str, [RootPath:Str], [ParmPath:Str],
		   [ParmData:DataRefObj]) => DDBHandle */
		aml_die("LoadTable");
		break;
	case AMLOP_LOAD:
		/* Load(Object:NameString, DDBHandle:SuperName) */
		mscope = aml_load(acpi_softc, scope, opargs[0], opargs[1]);
		break;
	case AMLOP_UNLOAD:
		/* DDBHandle */
		aml_die("Unload");
		break;

		/* Control Flow */
	case AMLOP_IF:
		/* Arguments: iT or iTbT */
		if (opargs[0]->v_integer) {
			dnprintf(10,"parse-if @ %.4x\n", pc);
			mscope = aml_xpushscope(scope, opargs[1], scope->node,
			    AMLOP_IF);
		} else if (opargs[3] != NULL) {
			dnprintf(10,"parse-else @ %.4x\n", pc);
			mscope = aml_xpushscope(scope, opargs[3], scope->node,
			    AMLOP_ELSE);
		}
		break;
	case AMLOP_WHILE:
		if (opargs[0]->v_integer) {
			/* Set parent position to start of WHILE */
			scope->pos = start;
			mscope = aml_xpushscope(scope, opargs[1], scope->node,
			    AMLOP_WHILE);
		}
		break;
	case AMLOP_BREAK:
		/* Break: Find While Scope parent, mark type as null */
		aml_xfindscope(scope, AMLOP_WHILE, AMLOP_BREAK);
		break;
	case AMLOP_CONTINUE:
		/* Find Scope.. mark all objects as invalid on way to root */
		aml_xfindscope(scope, AMLOP_WHILE, AMLOP_CONTINUE);
		break;
	case AMLOP_RETURN:
		mscope = aml_xfindscope(scope, AMLOP_METHOD, AMLOP_RETURN);
		if (mscope->retv) {
			aml_die("already allocated\n");
		}
		mscope->retv = aml_allocvalue(0,0,NULL);
		aml_copyvalue(mscope->retv, opargs[0]);
		mscope = NULL;
		break;
	default:
		/* may be set direct result */
		aml_die("Unknown opcode: %x:%s\n", opcode, htab->mnem);
		break;
	}
	if (mscope != NULL) {
		/* Change our scope to new scope */
		scope = mscope;
	}
	if ((ret_type == 'i' || ret_type == 't') && my_ret == NULL) {
		dnprintf(10,"quick: %.4x [%s] alloc return integer = 0x%llx\n",
		    pc, htab->mnem, ival);
		my_ret = aml_allocvalue(AML_OBJTYPE_INTEGER, ival, NULL);
	}
	if (ret_type == 'i' && my_ret && my_ret->type != AML_OBJTYPE_INTEGER) {
		dnprintf(10,"quick: %.4x convert to integer %s -> %s\n",
		    pc, htab->mnem, stype);
		my_ret = aml_xconvert(my_ret, AML_OBJTYPE_INTEGER, -1);
	}
	if (my_ret != NULL) {
		/* Display result */
		dnprintf(20,"quick: %.4x %18s %c %.4x\n", pc, stype,
		    ret_type, my_ret->stack);
	}

	/* End opcode: display/free arguments */
parse_error:
	for (idx=0; idx<8; idx++) {
		if (opargs[idx] == my_ret)
			opargs[idx] = NULL;
		aml_xdelref(&opargs[idx], "oparg");
	}

	/* If parsing whole scope and not done, start again */
	if (ret_type == 'T') {
		aml_xdelref(&my_ret, "scope.loop");
		while (scope->pos >= scope->end && scope != iscope) {
			/* Pop intermediate scope */
			scope = aml_xpopscope(scope);
		}
		if (scope->pos && scope->pos < scope->end)
			goto start;
	}

	odp--;
	dnprintf(50, ">>return [%s] %s %c %p\n", aml_nodename(scope->node),
	    stype, ret_type, my_ret);

	return my_ret;
}

int
acpi_parse_aml(struct acpi_softc *sc, u_int8_t *start, u_int32_t length)
{
	struct aml_scope *scope;
	struct aml_value res;

	aml_root.start = start;
	memset(&res, 0, sizeof(res));
	res.type = AML_OBJTYPE_SCOPE;
	res.length = length;
	res.v_buffer = start;

	/* Push toplevel scope, parse AML */
	aml_error = 0;
	scope = aml_xpushscope(NULL, &res, &aml_root, AMLOP_SCOPE);
	aml_busy++;
	aml_xparse(scope, 'T', "TopLevel");
	aml_busy--;
	aml_xpopscope(scope);

	if (aml_error) {
		printf("error in acpi_parse_aml\n");
		return -1;
	}
	return (0);
}

/*
 * @@@: External API
 *
 * evaluate an AML node
 * Returns a copy of the value in res  (must be freed by user)
 */
int
aml_evalnode(struct acpi_softc *sc, struct aml_node *node,
    int argc, struct aml_value *argv, struct aml_value *res)
{
	struct aml_value *xres;

	if (res)
		memset(res, 0, sizeof(*res));
	if (node == NULL || node->value == NULL)
		return (ACPI_E_BADVALUE);
	dnprintf(12,"EVALNODE: %s %d\n", aml_nodename(node), acpi_nalloc);

	aml_error = 0;
	xres = aml_xeval(NULL, node->value, 't', argc, argv);
	if (xres) {
		if (res)
			aml_copyvalue(res, xres);
		if (xres != node->value)
			aml_xdelref(&xres, "evalnode");
	}
	if (aml_error) {
		printf("error evaluating: %s\n", aml_nodename(node));
		return (-1);
	}
	return (0);
}

/*
 * evaluate an AML name
 * Returns a copy of the value in res  (must be freed by user)
 */
int
aml_evalname(struct acpi_softc *sc, struct aml_node *parent, const char *name,
    int argc, struct aml_value *argv, struct aml_value *res)
{
	parent = aml_searchname(parent, name);
	return aml_evalnode(sc, parent, argc, argv, res);
}

/*
 * evaluate an AML integer object
 */
int
aml_evalinteger(struct acpi_softc *sc, struct aml_node *parent,
    const char *name, int argc, struct aml_value *argv, int64_t *ival)
{
	struct aml_value res;
	int rc;

	parent = aml_searchname(parent, name);
	rc = aml_evalnode(sc, parent, argc, argv, &res);
	if (rc == 0) {
		*ival = aml_val2int(&res);
		aml_freevalue(&res);
	}
	return rc;
}

/*
 * Search for an AML name in namespace.. root only
 */
struct aml_node *
aml_searchname(struct aml_node *root, const void *vname)
{
	char *name = (char *)vname;
	char  nseg[AML_NAMESEG_LEN + 1];
	int   i;

	dnprintf(25,"Searchname: %s:%s = ", aml_nodename(root), vname);
	if (*name == AMLOP_ROOTCHAR) {
		root = &aml_root;
		name++;
	}
	while (*name != 0) {
		/* Ugh.. we can have short names here: append '_' */
		strlcpy(nseg, "____", sizeof(nseg));
		for (i=0; i < AML_NAMESEG_LEN && *name && *name != '.'; i++)
			nseg[i] = *name++;
		if (*name == '.')
			name++;
		root = __aml_search(root, nseg, 0);
	}
	dnprintf(25,"%p %s\n", root, aml_nodename(root));
	return root;
}

/*
 * Search for relative name
 */
struct aml_node *
aml_searchrel(struct aml_node *root, const void *vname)
{
	struct aml_node *res;

	while (root) {
		res = aml_searchname(root, vname);
		if (res != NULL)
			return res;
		root = root->parent;
	}
	return NULL;
>>>>>>> origin/master
}

#ifndef SMALL_KERNEL

void
acpi_getdevlist(struct acpi_devlist_head *list, struct aml_node *root,
    struct aml_value *pkg, int off)
{
	struct acpi_devlist *dl;
	struct aml_node *node;
	int idx;

	for (idx=off; idx<pkg->length; idx++) {
		node = aml_searchname(root, pkg->v_package[idx]->v_string);
		if (node) {
			dl = acpi_os_malloc(sizeof(*dl));
			if (dl) {
				dl->dev_node = node;
				TAILQ_INSERT_TAIL(list, dl, dev_link);
			}
		}
	}
}

void
acpi_freedevlist(struct acpi_devlist_head *list)
{
	struct acpi_devlist *dl;

	while ((dl = TAILQ_FIRST(list)) != NULL) {
		TAILQ_REMOVE(list, dl, dev_link);
		acpi_os_free(dl);
	}
}
#endif /* SMALL_KERNEL */
