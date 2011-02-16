<<<<<<< HEAD
/*	$OpenBSD: nfs_node.c,v 1.31 2006/01/09 12:43:16 pedro Exp $	*/
=======
/*	$OpenBSD: nfs_node.c,v 1.55 2010/12/21 20:14:43 thib Exp $	*/
>>>>>>> origin/master
/*	$NetBSD: nfs_node.c,v 1.16 1996/02/18 11:53:42 fvdl Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)nfs_node.c	8.6 (Berkeley) 5/22/95
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pool.h>
<<<<<<< HEAD
#include <sys/hash.h>
=======
#include <sys/rwlock.h>
#include <sys/queue.h>
>>>>>>> origin/master

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <nfs/nfsmount.h>
#include <nfs/nfs_var.h>

<<<<<<< HEAD
LIST_HEAD(nfsnodehashhead, nfsnode) *nfsnodehashtbl;
u_long nfsnodehash;
struct lock nfs_hashlock;

=======
>>>>>>> origin/master
struct pool nfs_node_pool;
extern int prtactive;

struct rwlock nfs_hashlock = RWLOCK_INITIALIZER("nfshshlk");

/* XXX */
extern struct vops nfs_vops;

/* filehandle to node lookup. */
static __inline int
nfsnode_cmp(const struct nfsnode *a, const struct nfsnode *b)
{
<<<<<<< HEAD
	nfsnodehashtbl = hashinit(desiredvnodes, M_NFSNODE, M_WAITOK, &nfsnodehash);
	lockinit(&nfs_hashlock, PINOD, "nfs_hashlock", 0, 0);

	pool_init(&nfs_node_pool, sizeof(struct nfsnode), 0, 0, 0, "nfsnodepl",
	    &pool_allocator_nointr);
=======
	if (a->n_fhsize != b->n_fhsize)
		return (a->n_fhsize - b->n_fhsize);
	return (memcmp(a->n_fhp, b->n_fhp, a->n_fhsize));
>>>>>>> origin/master
}

RB_PROTOTYPE(nfs_nodetree, nfsnode, n_entry, nfsnode_cmp);
RB_GENERATE(nfs_nodetree, nfsnode, n_entry, nfsnode_cmp);

/*
 * Look up a vnode/nfsnode by file handle.
 * Callers must check for mount points!!
 * In all cases, a pointer to a
 * nfsnode structure is returned.
 */
int
nfs_nget(struct mount *mnt, nfsfh_t *fh, int fhsize, struct nfsnode **npp)
{
	struct nfsmount		*nmp;
	struct nfsnode		*np, find, *np2;
	struct vnode		*vp, *nvp;
	struct proc		*p = curproc;		/* XXX */
	int			 error;

	nmp = VFSTONFS(mnt);

loop:
	rw_enter_write(&nfs_hashlock);
	find.n_fhp = fh;
	find.n_fhsize = fhsize;
	np = RB_FIND(nfs_nodetree, &nmp->nm_ntree, &find);
	if (np != NULL) {
		rw_exit_write(&nfs_hashlock);
		vp = NFSTOV(np);
		error = vget(vp, LK_EXCLUSIVE, p);
		if (error)
			goto loop;
		*npp = np;
		return (0);
	}
<<<<<<< HEAD
	if (lockmgr(&nfs_hashlock, LK_EXCLUSIVE|LK_SLEEPFAIL, NULL))
		goto loop;
	error = getnewvnode(VT_NFS, mntp, nfsv2_vnodeop_p, &nvp);
	if (error) {
		*npp = 0;
		lockmgr(&nfs_hashlock, LK_RELEASE, NULL);
=======

	/*
	 * getnewvnode() could recycle a vnode, potentially formerly
	 * owned by NFS. This will cause a VOP_RECLAIM() to happen,
	 * which will cause recursive locking, so we unlock before
	 * calling getnewvnode() lock again afterwards, but must check
	 * to see if this nfsnode has been added while we did not hold
	 * the lock.
	 */
	rw_exit_write(&nfs_hashlock);
	error = getnewvnode(VT_NFS, mnt, &nfs_vops, &nvp);
	/* note that we don't have this vnode set up completely yet */
	rw_enter_write(&nfs_hashlock);
	if (error) {
		*npp = NULL;
		rw_exit_write(&nfs_hashlock);
>>>>>>> origin/master
		return (error);
	}
	nvp->v_flag |= VLARVAL;
	np = RB_FIND(nfs_nodetree, &nmp->nm_ntree, &find);
	if (np != NULL) {
		vgone(nvp);
		rw_exit_write(&nfs_hashlock);
		goto loop;
	}

	vp = nvp;
	np = pool_get(&nfs_node_pool, PR_WAITOK | PR_ZERO);
	vp->v_data = np;
	/* we now have an nfsnode on this vnode */
	vp->v_flag &= ~VLARVAL;
	np->n_vnode = vp;

	rw_init(&np->n_commitlock, "nfs_commitlk");

	/* 
	 * Are we getting the root? If so, make sure the vnode flags
	 * are correct 
	 */
	if ((fhsize == nmp->nm_fhsize) && !bcmp(fh, nmp->nm_fh, fhsize)) {
		if (vp->v_type == VNON)
			vp->v_type = VDIR;
		vp->v_flag |= VROOT;
	}

	np->n_fhp = &np->n_fh;
	bcopy(fh, np->n_fhp, fhsize);
	np->n_fhsize = fhsize;
<<<<<<< HEAD
	lockmgr(&nfs_hashlock, LK_RELEASE, NULL);
=======
	np2 = RB_INSERT(nfs_nodetree, &nmp->nm_ntree, np);
	KASSERT(np2 == NULL);
	np->n_accstamp = -1;
	rw_exit(&nfs_hashlock);
>>>>>>> origin/master
	*npp = np;

	return (0);
}

int
nfs_inactive(void *v)
{
<<<<<<< HEAD
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct nfsnode *np;
	struct sillyrename *sp;
	struct proc *p = curproc;	/* XXX */

	np = VTONFS(ap->a_vp);
=======
	struct vop_inactive_args	*ap = v;
	struct nfsnode			*np;
	struct sillyrename		*sp;
>>>>>>> origin/master

#ifdef DIAGNOSTIC
	if (prtactive && ap->a_vp->v_usecount != 0)
		vprint("nfs_inactive: pushing active", ap->a_vp);
#endif
	if (ap->a_vp->v_flag & VLARVAL)
		/*
		 * vnode was incompletely set up, just return
		 * as we are throwing it away.
		 */
		return(0);
#ifdef DIAGNOSTIC
	if (ap->a_vp->v_data == NULL)
		panic("NULL v_data (no nfsnode set up?) in vnode %p",
		    ap->a_vp);
#endif
	np = VTONFS(ap->a_vp);
	if (ap->a_vp->v_type != VDIR) {
		sp = np->n_sillyrename;
		np->n_sillyrename = NULL;
	} else
		sp = NULL;
	if (sp) {
		/*
		 * Remove the silly file that was rename'd earlier
		 */
		nfs_vinvalbuf(ap->a_vp, 0, sp->s_cred, curproc);
		nfs_removeit(sp);
		crfree(sp->s_cred);
		vrele(sp->s_dvp);
		FREE((caddr_t)sp, M_NFSREQ);
	}
	np->n_flag &= (NMODIFIED | NFLUSHINPROG | NFLUSHWANT);

	VOP_UNLOCK(ap->a_vp, 0, ap->a_p);
	return (0);
}

/*
 * Reclaim an nfsnode so that it can be used for other purposes.
 */
int
nfs_reclaim(void *v)
{
<<<<<<< HEAD
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct nfsnode *np = VTONFS(vp);
	struct nfsdmap *dp, *dp2;
=======
	struct vop_reclaim_args	*ap = v;
	struct vnode		*vp = ap->a_vp;
	struct nfsmount		*nmp;
	struct nfsnode		*np = VTONFS(vp);
>>>>>>> origin/master

#ifdef DIAGNOSTIC
	if (prtactive && vp->v_usecount != 0)
		vprint("nfs_reclaim: pushing active", vp);
#endif
<<<<<<< HEAD

	if (np->n_hash.le_prev != NULL)
		LIST_REMOVE(np, n_hash);

	/*
	 * Free up any directory cookie structures and
	 * large file handle structures that might be associated with
	 * this nfs node.
	 */
	if (vp->v_type == VDIR) {
		dp = LIST_FIRST(&np->n_cookies);
		while (dp) {
			dp2 = dp;
			dp = LIST_NEXT(dp, ndm_list);
			FREE((caddr_t)dp2, M_NFSDIROFF);
		}
	}
	if (np->n_fhsize > NFS_SMALLFH) {
		free(np->n_fhp, M_NFSBIGFH);
	}
=======
	if (ap->a_vp->v_flag & VLARVAL)
		/*
		 * vnode was incompletely set up, just return
		 * as we are throwing it away.
		 */
		return(0);
#ifdef DIAGNOSTIC
	if (ap->a_vp->v_data == NULL)
		panic("NULL v_data (no nfsnode set up?) in vnode %p",
		    ap->a_vp);
#endif
	nmp = VFSTONFS(vp->v_mount);
	rw_enter_write(&nfs_hashlock);
	RB_REMOVE(nfs_nodetree, &nmp->nm_ntree, np);
	rw_exit_write(&nfs_hashlock);
>>>>>>> origin/master

	if (np->n_rcred)
		crfree(np->n_rcred);
	if (np->n_wcred)
<<<<<<< HEAD
		crfree(np->n_wcred);	
=======
		crfree(np->n_wcred);

>>>>>>> origin/master
	cache_purge(vp);
	pool_put(&nfs_node_pool, vp->v_data);
	vp->v_data = NULL;

	return (0);
}
