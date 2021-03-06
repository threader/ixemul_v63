/*      $NetBSD: sysv_sem.c,v 1.26 1996/02/09 19:00:25 christos Exp $   */
 
/*
 * Implementation of SVID semaphores
 *
 * Author:  Daniel Boulet
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */

#define _KERNEL
#include <ixemul.h>
#include <sys/sem.h>
#include <time.h>

/*
 * Values in support of System V compatible semaphores.
 */
static const struct seminfo seminfo = {
	SEMMAP,         /* # of entries in semaphore map */
	SEMMNI,         /* # of semaphore identifiers */
	SEMMNS,         /* # of semaphores in system */
	SEMMNU,         /* # of undo structures in system */
	SEMMSL,         /* max # of semaphores per id */
	SEMOPM,         /* max # of operations per semop call */
	SEMUME,         /* max # of undo entries per process */
	SEMUSZ,         /* size in bytes of undo structure */
	SEMVMX,         /* semaphore maximum value */
	SEMAEM          /* adjust on exit max value */
};

static int      semtot = 0;
static struct   Task *semlock_holder = NULL;

void semlock __P((struct Task *));
struct sem_undo *semu_alloc __P((struct Task *));
int semundo_adjust __P((struct Task *, struct sem_undo **, int, int, int));
void semundo_clear __P((int, int));

static struct   semid_ds sema[SEMMNI];                  /* semaphore id pool */
static struct   sem sem[SEMMNS];                        /* semaphore pool */
static int      semu[(SEMMNU * SEMUSZ) / sizeof(int)];  /* undo structure pool */
static struct   sem_undo *semu_list;                    /* list of active undo structures */

void
seminit()
{
	register int i;

	for (i = 0; i < seminfo.semmni; i++) {
		sema[i].sem_base = 0;
		sema[i].sem_perm.mode = 0;
	}
	for (i = 0; i < seminfo.semmnu; i++) {
		register struct sem_undo *suptr = SEMU(i);
		suptr->un_proc = NULL;
	}
	semu_list = NULL;
}

void
semlock(struct Task *p)
{
	Forbid();
	while (semlock_holder != NULL && semlock_holder != p)
		ix_sleep((caddr_t)&semlock_holder, "semlock");
	Permit();
}


/*
 * Allocate a new sem_undo structure for a process
 * (returns ptr to structure or NULL if no more room)
 */

struct sem_undo *
semu_alloc(struct Task *p)
{
	register int i;
	register struct sem_undo *suptr;
	register struct sem_undo **supptr;
	int attempt;

	/*
	 * Try twice to allocate something.
	 * (we'll purge any empty structures after the first pass so
	 * two passes are always enough)
	 */

	for (attempt = 0; attempt < 2; attempt++) {
		/*
		 * Look for a free structure.
		 * Fill it in and return it if we find one.
		 */

		for (i = 0; i < seminfo.semmnu; i++) {
			suptr = SEMU(i);
			if (suptr->un_proc == NULL) {
				suptr->un_next = semu_list;
				semu_list = suptr;
				suptr->un_cnt = 0;
				suptr->un_proc = p;
				return(suptr);
			}
		}

		/*
		 * We didn't find a free one, if this is the first attempt
		 * then try to free some structures.
		 */

		if (attempt == 0) {
			/* All the structures are in use - try to free some */
			int did_something = 0;

			supptr = &semu_list;
			while ((suptr = *supptr) != NULL) {
				if (suptr->un_cnt == 0)  {
					suptr->un_proc = NULL;
					*supptr = suptr->un_next;
					did_something = 1;
				} else
					supptr = &(suptr->un_next);
			}

			/* If we didn't free anything then just give-up */
			if (!did_something)
				return(NULL);
		} else {
			/*
			 * The second pass failed even though we freed
			 * something after the first pass!
			 * This is IMPOSSIBLE!
			 */
			panic("semu_alloc - second attempt failed");
		}
	}
	return NULL;
}

/*
 * Adjust a particular entry for a particular proc
 */

int
semundo_adjust(struct Task *p, struct sem_undo **supptr, int semid, int semnum, int adjval)
{
	register struct sem_undo *suptr;
	register struct undo *sunptr;
	int i;

	/* Look for and remember the sem_undo if the caller doesn't provide
	   it */

	suptr = *supptr;
	if (suptr == NULL) {
		for (suptr = semu_list; suptr != NULL; suptr = suptr->un_next) {
			if (suptr->un_proc == p) {
				*supptr = suptr;
				break;
			}
		}
		if (suptr == NULL) {
			if (adjval == 0)
				return(0);
			suptr = semu_alloc(p);
			if (suptr == NULL)
				return(ENOSPC);
			*supptr = suptr;
		}
	}

	/*
	 * Look for the requested entry and adjust it (delete if adjval becomes
	 * 0).
	 */
	sunptr = &suptr->un_ent[0];
	for (i = 0; i < suptr->un_cnt; i++, sunptr++) {
		if (sunptr->un_id != semid || sunptr->un_num != semnum)
			continue;
		if (adjval == 0)
			sunptr->un_adjval = 0;
		else
			sunptr->un_adjval += adjval;
		if (sunptr->un_adjval == 0) {
			suptr->un_cnt--;
			if (i < suptr->un_cnt)
				suptr->un_ent[i] =
				    suptr->un_ent[suptr->un_cnt];
		}
		return(0);
	}

	/* Didn't find the right entry - create it */
	if (adjval == 0)
		return(0);
	if (suptr->un_cnt == SEMUME)
		return(EINVAL);

	sunptr = &suptr->un_ent[suptr->un_cnt];
	suptr->un_cnt++;
	sunptr->un_adjval = adjval;
	sunptr->un_id = semid;
	sunptr->un_num = semnum;
	return(0);
}

void
semundo_clear(int semid, int semnum)
{
	register struct sem_undo *suptr;

	for (suptr = semu_list; suptr != NULL; suptr = suptr->un_next) {
		register struct undo *sunptr;
		register int i;

		sunptr = &suptr->un_ent[0];
		for (i = 0; i < suptr->un_cnt; i++, sunptr++) {
			if (sunptr->un_id == semid) {
				if (semnum == -1 || sunptr->un_num == semnum) {
					suptr->un_cnt--;
					if (i < suptr->un_cnt) {
						suptr->un_ent[i] =
						  suptr->un_ent[suptr->un_cnt];
						i--, sunptr--;
					}
				}
				if (semnum != -1)
					break;
			}
		}
	}
}

static int
ix_semctl(int semid, int semnum, int cmd, union semun arg)
{
	usetup;
	struct ucred cred;
	int i, rval, eval;
	register struct semid_ds *semaptr;
	struct Task *p = SysBase->ThisTask;

	cred.cr_uid = geteuid();
	cred.cr_gid = getegid();

	semlock(p);

	semid = IPCID_TO_IX(semid);
	if (semid < 0 || semid >= seminfo.semmsl)
		errno_return(-1, EINVAL);

	semaptr = &sema[semid];
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm.seq != IPCID_TO_SEQ(semid))
		errno_return(-1, EINVAL);

	eval = 0;
	rval = 0;

	switch (cmd) {
	case IPC_RMID:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_M)) != 0)
			errno_return(-1, eval);
		semaptr->sem_perm.cuid = cred.cr_uid;
		semaptr->sem_perm.uid = cred.cr_uid;
		semtot -= semaptr->sem_nsems;
		for (i = semaptr->sem_base - sem; i < semtot; i++)
			sem[i] = sem[i + semaptr->sem_nsems];
		for (i = 0; i < seminfo.semmni; i++) {
			if ((sema[i].sem_perm.mode & SEM_ALLOC) &&
			    sema[i].sem_base > semaptr->sem_base)
				sema[i].sem_base -= semaptr->sem_nsems;
		}
		semaptr->sem_perm.mode = 0;
		semundo_clear(semid, -1);
		ix_wakeup((u_int)semaptr);
		break;

	case IPC_SET:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_M)))
			errno_return(-1, eval);
		semaptr->sem_perm.uid = arg.buf->sem_perm.uid;
		semaptr->sem_perm.gid = arg.buf->sem_perm.gid;
		semaptr->sem_perm.mode = (semaptr->sem_perm.mode & ~0777) |
		    (arg.buf->sem_perm.mode & 0777);
		semaptr->sem_ctime = time(NULL);
		break;

	case IPC_STAT:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_R)))
			errno_return(-1, eval);
		memcpy(arg.buf, semaptr, sizeof(struct semid_ds));
		break;

	case GETNCNT:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_R)))
			errno_return(-1, eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			errno_return(-1, EINVAL);
		rval = semaptr->sem_base[semnum].semncnt;
		break;

	case GETPID:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_R)))
			errno_return(-1, eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			errno_return(-1, EINVAL);
		rval = semaptr->sem_base[semnum].sempid;
		break;

	case GETVAL:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_R)))
			errno_return(-1, eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			errno_return(-1, EINVAL);
		rval = semaptr->sem_base[semnum].semval;
		break;

	case GETALL:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_R)))
			errno_return(-1, eval);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			memcpy(&arg.array[i], (caddr_t)&semaptr->sem_base[i].semval,
			    sizeof(arg.array[0]));
		}
		break;

	case GETZCNT:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_R)))
			errno_return(-1, eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			errno_return(-1, EINVAL);
		rval = semaptr->sem_base[semnum].semzcnt;
		break;

	case SETVAL:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_W)))
			errno_return(-1, eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			errno_return(-1, EINVAL);
		semaptr->sem_base[semnum].semval = arg.val;
		semundo_clear(semid, semnum);
		ix_wakeup((u_int)semaptr);
		break;

	case SETALL:
		if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_W)))
			errno_return(-1, eval);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			memcpy((caddr_t)&semaptr->sem_base[i].semval, &arg.array[i],
			    sizeof(arg.array[0]));
		}
		semundo_clear(semid, -1);
		ix_wakeup((u_int)semaptr);
		break;

	default:
		errno_return(-1, EINVAL);
	}

	if (eval == 0)
		return rval;
	return(eval);
}

static int
ix_semget(key_t key, int nsems, int semflg)
{
	int semid, eval;
	usetup;
	struct Task *p = SysBase->ThisTask;
	struct ucred cred;

	cred.cr_uid = geteuid();
	cred.cr_gid = getegid();

	semlock(p);

	if (key != IPC_PRIVATE) {
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].sem_perm.mode & SEM_ALLOC) &&
			    sema[semid].sem_perm.key == key)
				break;
		}
		if (semid < seminfo.semmni) {
			if ((eval = ipcperm(&cred, &sema[semid].sem_perm,
			    semflg & 0700)))
				errno_return(-1, eval);
			if (nsems > 0 && sema[semid].sem_nsems < nsems) {
				errno_return(-1, EINVAL);
			}
			if ((semflg & IPC_CREAT) && (semflg & IPC_EXCL)) {
				errno_return(-1, EEXIST);
			}
			goto found;
		}
	}

	if (key == IPC_PRIVATE || (semflg & IPC_CREAT)) {
		if (nsems <= 0 || nsems > seminfo.semmsl) {
			errno_return(-1, EINVAL);
		}
		if (nsems > seminfo.semmns - semtot) {
			errno_return(-1, ENOSPC);
		}
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].sem_perm.mode & SEM_ALLOC) == 0)
				break;
		}
		if (semid == seminfo.semmni) {
			errno_return(-1, ENOSPC);
		}
		sema[semid].sem_perm.key = key;
		sema[semid].sem_perm.cuid = cred.cr_uid;
		sema[semid].sem_perm.uid = cred.cr_uid;
		sema[semid].sem_perm.cgid = cred.cr_gid;
		sema[semid].sem_perm.gid = cred.cr_gid;
		sema[semid].sem_perm.mode = (semflg & 0777) | SEM_ALLOC;
		sema[semid].sem_perm.seq =
		    (sema[semid].sem_perm.seq + 1) & 0x7fff;
		sema[semid].sem_nsems = nsems;
		sema[semid].sem_otime = 0;
		sema[semid].sem_ctime = time(NULL);
		sema[semid].sem_base = &sem[semtot];
		semtot += nsems;
		bzero(sema[semid].sem_base,
		    sizeof(sema[semid].sem_base[0])*nsems);
	} else {
		errno_return(-1, ENOENT);
	}

found:
	return IXSEQ_TO_IPCID(semid, sema[semid].sem_perm);
}

static int
ix_semop(int semid, struct sembuf *sops, int nsops)
{
	usetup;
	register struct semid_ds *semaptr;
	register struct sembuf *sopptr = NULL;
	register struct sem *semptr = NULL;
	struct sem_undo *suptr = NULL;
	int i, j, eval;
	int do_wakeup, do_undos;
	struct Task *p = SysBase->ThisTask;
	struct ucred cred;

	cred.cr_uid = geteuid();
	cred.cr_gid = getegid();

	semlock(p);

	semid = IPCID_TO_IX(semid);     /* Convert back to zero origin */

	if (semid < 0 || semid >= seminfo.semmsl)
		errno_return(-1, EINVAL);

	semaptr = &sema[semid];
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm.seq != IPCID_TO_SEQ(semid))
		errno_return(-1, EINVAL);

	if ((eval = ipcperm(&cred, &semaptr->sem_perm, IPC_W))) {
		errno_return(-1, eval);
	}

	if (nsops > MAX_SOPS) {
		errno_return(-1, E2BIG);
	}

	/* 
	 * Loop trying to satisfy the vector of requests.
	 * If we reach a point where we must wait, any requests already
	 * performed are rolled back and we go to sleep until some other
	 * process wakes us up.  At this point, we start all over again.
	 *
	 * This ensures that from the perspective of other tasks, a set
	 * of requests is atomic (never partially satisfied).
	 */
	do_undos = 0;

	for (;;) {
		do_wakeup = 0;

		for (i = 0; i < nsops; i++) {
			sopptr = &sops[i];

			if (sopptr->sem_num >= semaptr->sem_nsems)
				errno_return(-1, EFBIG);

			semptr = &semaptr->sem_base[sopptr->sem_num];

			if (sopptr->sem_op < 0) {
				if ((int)(semptr->semval +
					  sopptr->sem_op) < 0) {
					break;
				} else {
					semptr->semval += sopptr->sem_op;
					if (semptr->semval == 0 &&
					    semptr->semzcnt > 0)
						do_wakeup = 1;
				}
				if (sopptr->sem_flg & SEM_UNDO)
					do_undos = 1;
			} else if (sopptr->sem_op == 0) {
				if (semptr->semval > 0) {
					break;
				}
			} else {
				if (semptr->semncnt > 0)
					do_wakeup = 1;
				semptr->semval += sopptr->sem_op;
				if (sopptr->sem_flg & SEM_UNDO)
					do_undos = 1;
			}
		}

		/*
		 * Did we get through the entire vector?
		 */
		if (i >= nsops)
			goto done;

		/*
		 * No ... rollback anything that we've already done
		 */
		for (j = 0; j < i; j++)
			semaptr->sem_base[sops[j].sem_num].semval -=
			    sops[j].sem_op;

		/*
		 * If the request that we couldn't satisfy has the
		 * NOWAIT flag set then return with EAGAIN.
		 */
		if (sopptr->sem_flg & IPC_NOWAIT)
			errno_return(-1, EAGAIN);

		if (sopptr->sem_op == 0)
			semptr->semzcnt++;
		else
			semptr->semncnt++;

		eval = ix_sleep((caddr_t)semaptr, "semlock");

		suptr = NULL;   /* sem_undo may have been reallocated */

		if (eval != 0)
			errno_return(-1, EINTR);

		/*
		 * Make sure that the semaphore still exists
		 */
		if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
		    semaptr->sem_perm.seq != IPCID_TO_SEQ(semid)) {
			/* The man page says to return EIDRM. */
			/* Unfortunately, BSD doesn't define that code! */
#ifdef EIDRM
			errno_return(-1, EIDRM);
#else
			errno_return(-1, EINVAL);
#endif
		}

		/*
		 * The semaphore is still alive.  Readjust the count of
		 * waiting processes.
		 */
		if (sopptr->sem_op == 0)
			semptr->semzcnt--;
		else
			semptr->semncnt--;
	}

done:
	/*
	 * Process any SEM_UNDO requests.
	 */
	if (do_undos) {
		for (i = 0; i < nsops; i++) {
			/*
			 * We only need to deal with SEM_UNDO's for non-zero
			 * op's.
			 */
			int adjval;

			if ((sops[i].sem_flg & SEM_UNDO) == 0)
				continue;
			adjval = sops[i].sem_op;
			if (adjval == 0)
				continue;
			eval = semundo_adjust(p, &suptr, semid,
			    sops[i].sem_num, -adjval);
			if (eval == 0)
				continue;

			/*
			 * Oh-Oh!  We ran out of either sem_undo's or undo's.
			 * Rollback the adjustments to this point and then
			 * rollback the semaphore ups and down so we can return
			 * with an error with all structures restored.  We
			 * rollback the undo's in the exact reverse order that
			 * we applied them.  This guarantees that we won't run
			 * out of space as we roll things back out.
			 */
			for (j = i - 1; j >= 0; j--) {
				if ((sops[j].sem_flg & SEM_UNDO) == 0)
					continue;
				adjval = sops[j].sem_op;
				if (adjval == 0)
					continue;
				if (semundo_adjust(p, &suptr, semid,
				    sops[j].sem_num, adjval) != 0)
					panic("semop - can't undo undos");
			}

			for (j = 0; j < nsops; j++)
				semaptr->sem_base[sops[j].sem_num].semval -=
				    sops[j].sem_op;
			errno_return(-1, eval);
		} /* loop through the sops */
	} /* if (do_undos) */

	/* We're definitely done - set the sempid's */
	for (i = 0; i < nsops; i++) {
		sopptr = &sops[i];
		semptr = &semaptr->sem_base[sopptr->sem_num];
		semptr->sempid = (pid_t)p;
	}

	/* Do a wakeup if any semaphore was up'd. */
	if (do_wakeup) {
		ix_wakeup((u_int)semaptr);
	}
	return(0);
}

/*
 * Go through the undo structures for this process and apply the adjustments to
 * semaphores.
 */
void
semexit(struct Task *p)
{
	register struct sem_undo *suptr;
	register struct sem_undo **supptr;

	/*
	 * Go through the chain of undo vectors looking for one associated with
	 * this process.
	 */

	for (supptr = &semu_list; (suptr = *supptr) != NULL;
	    supptr = &suptr->un_next) {
		if (suptr->un_proc == p)
			break;
	}

	/*
	 * There are a few possibilities to consider here ...
	 *
	 * 1) The semaphore facility isn't currently locked.  In this case,
	 *    this call should proceed normally.
	 * 2) The semaphore facility is locked by this process (i.e. the one
	 *    that is exiting).  In this case, this call should proceed as
	 *    usual and the facility should be unlocked at the end of this
	 *    routine (since the locker is exiting).
	 * 3) The semaphore facility is locked by some other process and this
	 *    process doesn't have an undo structure allocated for it.  In this
	 *    case, this call should proceed normally (i.e. not accomplish
	 *    anything and, most importantly, not block since that is
	 *    unnecessary and could result in a LOT of processes blocking in
	 *    here if the facility is locked for a long time).
	 * 4) The semaphore facility is locked by some other process and this
	 *    process has an undo structure allocated for it.  In this case,
	 *    this call should block until the facility has been unlocked since
	 *    the holder of the lock may be examining this process's proc entry
	 *    (the ipcs utility does this when printing out the information
	 *    from the allocated sem undo elements).
	 *
	 * This leads to the conclusion that we should not block unless we
	 * discover that the someone else has the semaphore facility locked and
	 * this process has an undo structure.  Let's do that...
	 *
	 * Note that we do this in a separate pass from the one that processes
	 * any existing undo structure since we don't want to risk blocking at
	 * that time (it would make the actual unlinking of the element from
	 * the chain of allocated undo structures rather messy).
	 */

	/*
	 * Does someone else hold the semaphore facility's lock?
	 */

	if (semlock_holder != NULL && semlock_holder != p) {
		/*
		 * Yes (i.e. we are in case 3 or 4).
		 *
		 * If we didn't find an undo vector associated with this
		 * process than we can just return (i.e. we are in case 3).
		 *
		 * Note that we know that someone else is holding the lock so
		 * we don't even have to see if we're holding it...
		 */

		if (suptr == NULL)
			return;

		/*
		 * We are in case 4.
		 *
		 * Go to sleep as long as someone else is locking the semaphore
		 * facility (note that we won't get here if we are holding the
		 * lock so we don't need to check for that possibility).
		 */

		while (semlock_holder != NULL)
			ix_sleep((caddr_t)&semlock_holder, "semlock");

		/*
		 * Nobody is holding the facility (i.e. we are now in case 1).
		 * We can proceed safely according to the argument outlined
		 * above.
		 *
		 * We look up the undo vector again, in case the list changed
		 * while we were asleep, and the parent is now different.
		 */

		for (supptr = &semu_list; (suptr = *supptr) != NULL;
		    supptr = &suptr->un_next) {
			if (suptr->un_proc == p)
				break;
		}

		if (suptr == NULL)
			panic("semexit: undo vector disappeared");
	} else {
		/*
		 * No (i.e. we are in case 1 or 2).
		 *
		 * If there is no undo vector, skip to the end and unlock the
		 * semaphore facility if necessary.
		 */

		if (suptr == NULL)
			goto unlock;
	}

	/*
	 * We are now in case 1 or 2, and we have an undo vector for this
	 * process.
	 */

	/*
	 * If there are any active undo elements then process them.
	 */
	if (suptr->un_cnt > 0) {
		int i;

		for (i = 0; i < suptr->un_cnt; i++) {
			int semid = suptr->un_ent[i].un_id;
			int semnum = suptr->un_ent[i].un_num;
			int adjval = suptr->un_ent[i].un_adjval;
			struct semid_ds *semaptr;

			semaptr = &sema[semid];
			if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0)
				panic("semexit - semid not allocated");
			if (semnum >= semaptr->sem_nsems)
				panic("semexit - semnum out of range");

			if (adjval < 0 &&
			    semaptr->sem_base[semnum].semval < -adjval)
				semaptr->sem_base[semnum].semval = 0;
			else
				semaptr->sem_base[semnum].semval += adjval;

			ix_wakeup((u_int)semaptr);
		}
	}

	/*
	 * Deallocate the undo vector.
	 */
	suptr->un_proc = NULL;
	*supptr = suptr->un_next;

unlock:
	/*
	 * If the exiting process is holding the global semaphore facility
	 * lock (i.e. we are in case 2) then release it.
	 */
	if (semlock_holder == p) {
		semlock_holder = NULL;
		ix_wakeup((u_int)&semlock_holder);
	}
}


int
semop(int semid, struct sembuf *sops, int nsops)
{
  int result;

  Forbid();
  result = ix_semop(semid, sops, nsops);
  Permit();
  return result;
}

int
semctl(int semid, int semnum, int cmd, union semun arg)
{
  int result = 0;

  if (cmd == GETSEMINFO)
    return (int)&seminfo;
  if (cmd == GETSEMA)
    return (int)&sema;
  Forbid();
  if (cmd == SEMLCK)
  {
    struct Task *p = SysBase->ThisTask;

    semlock(p);
    semlock_holder = p;
  }
  else if (cmd == SEMUNLK)
  {
    semlock_holder = NULL;
    ix_wakeup((u_int)&semlock_holder);
  }
  else
  {
    result = ix_semctl(semid, semnum, cmd, arg);
  }
  Permit();
  return result;
}

int
semget(key_t key, int nsems, int semflg)
{
  int result;

  Forbid();
  result = ix_semget(key, nsems, semflg);
  Permit();
  return result;
}
