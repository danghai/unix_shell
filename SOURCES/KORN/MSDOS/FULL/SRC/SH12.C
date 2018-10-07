/*
 * MS-DOS SHELL - TSEARCH functions
 *
 * MS-DOS SHELL - Copyright (c) 1990,4 Data Logic Limited
 *
 * This code is subject to the following copyright restrictions.  The
 * code for the extended (non BASIC) tsearch.3 functions is based on code
 * written by Peter Valkenburg & Michiel Huisjes.  The following copyright
 * conditions apply:
 *
 * 1.  Redistribution and use in source and binary forms are permitted
 *     provided that the above copyright notice is duplicated in the
 *     source form and the copyright notice in file sh6.c is displayed
 *     on entry to the program.
 *
 * 2.  The sources (or parts thereof) or objects generated from the sources
 *     (or parts of sources) cannot be sold under any circumstances.
 *
 *    $Header: /usr/users/istewart/shell/sh2.3/Release/RCS/sh12.c,v 2.7 1994/08/25 20:49:11 istewart Exp $
 *
 *    $Log: sh12.c,v $
 * Revision 2.7  1994/08/25  20:49:11  istewart
 * MS Shell 2.3 Release
 *
 * Revision 2.6  1994/02/01  10:25:20  istewart
 * Release 2.3 Beta 2, including first NT port
 *
 * Revision 2.5  1993/08/25  16:03:57  istewart
 * Beta 225 - see Notes file
 *
 * Revision 2.4  1993/07/02  10:21:35  istewart
 * 224 Beta fixes
 *
 * Revision 2.3  1993/06/02  09:52:35  istewart
 * Beta 223 Updates - see Notes file
 *
 * Revision 2.2  1993/02/16  16:03:15  istewart
 * Beta 2.22 Release
 *
 * Revision 2.1  1993/01/26  18:35:09  istewart
 * Release 2.2 beta 0
 *
 * Revision 2.0  1992/07/10  10:52:48  istewart
 * 211 Beta updates
 *
 *
 * Start of original notes for the non-BASIC tsearch functions.
 *
 * "tsearch.c", Peter Valkenburg & Michiel Huisjes, november '89.
 *
 * Standard tsearch(3) compatible implementation of AVL height balanced trees.
 * Performance is slightly better than SUN OS tsearch(3) for average case
 * delete/add operations, but worst case behaviour (i.e. when ordinary trees
 * get unbalanced) is much better.  Tsearch/tdelete/tfind run in O(2log(n)),
 * twalk in O(n), where n is the size of binary tree.
 *
 * Entry points:
 *
 *	_ts_NODE_t *tsearch((void *)key, (_ts_NODE_t **)rootp, int (*compar)());
 *	_ts_NODE_t *tdelete((void *)key, (_ts_NODE_t **)rootp, int (*compar)());
 *	_ts_NODE_t *tfind((void *)key, (_ts_NODE_t **)rootp, int (*compar)());
 *	void twalk((_ts_NODE_t *root), void (*action)());
 *
 * Here key is a pointer to any datum (to be) held in the tree rooted by *rootp
 * or root.  Keys are compared by calling compar, which gets two key arguments
 * and should return a value < 0, 0, or > 0, if the first parameter is to be
 * considered less than , equal to, or greater than the second, respectively.
 * Users can declare type (_ts_NODE_t *) as (char *) and (_ts_NODE_t **) as
 * (char **).  The return values of tsearch/tdelete/tfind can be used as
 * pointers to the key pointers of their _ts_NODE_t, by casting them to
 * (char **).
 *
 * See manual page tsearch(3) for more extensive user documentation.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include <limits.h>
#include "sh.h"

/*
 * Type for doubly linked AVL tree.
 */

#ifndef BASIC
typedef struct node_s {
    void		*key;		/* ptr to datum			*/
    struct node_s	*parent;	/* ptr to parent ancestor	*/
    struct node_s	*sibls[2];	/* ptrs to L/R siblings		*/
    int			balance;	/* balance value (-1, 0 or +1)	*/
} _ts_NODE_t;

typedef int		_SibIndex_t;	/* type for indexing siblings	*/
#define	L		((_SibIndex_t) 0)
#define	R		((_SibIndex_t) 1)

#define	LEFT		sibls[L]/* left sibling pointer _ts_NODE_t field */
#define	RIGHT		sibls[R]/* right sibling pointer _ts_NODE_t field */

/*
 * Direction gives direction in which child _ts_NODE_t c is under parent
 * _ts_NODE_t p.
 */

#define	direction(p, c)	((_SibIndex_t) ((p)->RIGHT == (c)))

/*
 * Cmp_dir gives direction corresponding with compare value v (R iff v > 0).
 */

#define	cmp_dir(v)	((_SibIndex_t) ((v) > 0))

/*
 * Siblingp yields ptr to left (d == L) or right (d == R) child of _ts_NODE_t n.
 */

#define	siblingp(n, d)	((n)->sibls + (d))

/*
 * Parentp yields ptr to parent's ptr to _ts_NODE_t n, or root ptr r if n is 0.
 */

#define	parentp(r, n)	((n)->parent == (_ts_NODE_t *)NULL ? (r) : \
			     siblingp((n)->parent, direction((n)->parent, (n))))

/*
 * Dir_bal yields balance value corresponding to _SibIndex_t d.
 */

#define	dir_bal(d)	((d) == L ? -1 : 1)

static _ts_NODE_t	*balance (_ts_NODE_t **, _ts_NODE_t *, int);
static _ts_NODE_t	*single_rotation (_ts_NODE_t **, _ts_NODE_t *,
					  _ts_NODE_t *, _SibIndex_t);
static _ts_NODE_t	*double_rotation (_ts_NODE_t **, _ts_NODE_t *,
					  _ts_NODE_t *, _SibIndex_t);

/*
 * Tsearch adds node key to tree rooted by *rootp, using compar for
 * comparing elements.  It returns the pointer to the _ts_NODE_t in which
 * the (possibly already existing) key pointer resides.
 */

void	*tsearch (key, root, compar)
void	*key;
void	**root;
int	(*compar)(const void *, const void *);
{
    register _ts_NODE_t		*parent, *child;
    _ts_NODE_t			*nnode;
    register _SibIndex_t	d;
    register int		cmp;
    register _ts_NODE_t		**rootp = (_ts_NODE_t **)root;

    if (rootp == (_ts_NODE_t **)NULL)
	return (_ts_NODE_t *)NULL;

    /* find place where key should go */

    parent = (_ts_NODE_t *)NULL;
    child = *rootp;

    while (child != (_ts_NODE_t *)NULL)
    {
	if ((cmp = compar (key, child->key)) == 0)
	    return child;

	parent = child;
	child = *siblingp (child, cmp_dir (cmp));
    }

    /* create new node and set its parent's sibling pointer */

    nnode = (_ts_NODE_t *) GetAllocatedSpace (sizeof (_ts_NODE_t));

    if (nnode == (_ts_NODE_t *)NULL)
	return (_ts_NODE_t *)NULL;

    SetMemoryAreaNumber ((void *) nnode, 0);
    nnode->key = key;
    nnode->balance = 0;
    nnode->parent = parent;
    nnode->LEFT = nnode->RIGHT = (_ts_NODE_t *)NULL;

    if (parent == (_ts_NODE_t *)NULL)
    {
	*rootp = nnode;
	return nnode;			/* just created tree */
    }

    *siblingp (parent, cmp_dir(cmp)) = nnode;
    child = nnode;

/*
 * Back up until tree is balanced.  This is achieved when either
 * the tree is balanced by rotation or a node's balance becomes 0.
 */

    do
    {
	d = direction (parent, child);

	if (parent->balance == dir_bal(d))
	{
	    (void) balance (rootp, parent, d);
	    return nnode;
	}

	else if ((parent->balance += dir_bal(d)) == 0)
	    return nnode;

	child = parent;
	parent = parent->parent;

    } while (parent != (_ts_NODE_t *)NULL);

    return nnode;
}

/*
 * Tdelete removes key from the tree rooted by *rootp, using compar for
 * comparing elements.  It returns the pointer to the _ts_NODE_t that was the
 * parent of the _ts_NODE_t that contained the key pointer, 0 if it did not exist.
 */

void	*tdelete (key, root, compar)
void	*key;
void 	**root;
int	(*compar)(const void *, const void *);
{
    register _ts_NODE_t		*parent, *child;
    _ts_NODE_t			*dnode, *dparent;
    register _SibIndex_t	d;
    register int		cont_bal;
    register int		cmp;
    register _ts_NODE_t		**rootp = (_ts_NODE_t **)root;

    if (rootp == (_ts_NODE_t **)NULL)
	return (void *)NULL;

/* find node to delete */

    child = *rootp;

    while (child != (_ts_NODE_t *)NULL)
    {
	if ((cmp = compar (key, child->key)) == 0)
	    break;

	child = *siblingp (child, cmp_dir (cmp));
    }

    if (child == (_ts_NODE_t *)NULL)
	return (void *)NULL;		/* key not in tree */

/* the node was found; get its successor (if any) to replace it */

    dnode = child;
    dparent = dnode->parent;
    child = child->RIGHT;

    if (child == (_ts_NODE_t *)NULL)		/* no successor for key */
    {
	if ((child = dnode->LEFT) != (_ts_NODE_t *)NULL)
	    child->parent = dparent;	/* set new parent */

	if (dparent == (_ts_NODE_t *)NULL)
	{
	    ReleaseMemoryCell ((void *) dnode);
	    *rootp = child;
	    return (void *)NULL;	/* just deleted the root */
	}

	d = direction (dparent, dnode);	/* for back up */
	*siblingp(dparent, d) = child;	/* replace by left child */
	ReleaseMemoryCell ((void *) dnode);
	parent = dparent;
    }

    else				/* key's successor exists */
    {
	while (child->LEFT != (_ts_NODE_t *)NULL)
	    child = child->LEFT;

	parent = child->parent;
	d = direction(parent, child);		/* for back up */
	*siblingp(parent, d) = child->RIGHT;

	if (child->RIGHT != (_ts_NODE_t *)NULL)
	    child->RIGHT->parent = parent;	/* set new parent */

	dnode->key = child->key;	/* successor replaces key */
	ReleaseMemoryCell ((void *) child);
    }

/*
 * Back up until tree is balanced.  This is achieved when either the tree is
 * balanced by rotation but not made shorter, or a node's balance was 0 before
 * deletion.
 */

    do
    {
	if (parent->balance == dir_bal(!d))
	{
	    cont_bal = ((*siblingp(parent, !d))->balance != 0);
	    parent = balance(rootp, parent, !d);
	}

	else
	{
	    cont_bal = (parent->balance != 0);
	    parent->balance += dir_bal(!d);
	}

	child = parent;

	if ((parent = parent->parent) == (_ts_NODE_t *)NULL)
	    return dparent;		/* we reached the root */

	d = direction(parent, child);

    } while (cont_bal);

    return dparent;
}

/*
 * Balance the subtree rooted at sb that has become to heavy on side d.
 * Also adjusts sibling pointer of the parent of sb, or *rootp if sb is
 * the top of the entire tree.
 */

static _ts_NODE_t	*balance(rootp, sb, d)
_ts_NODE_t		**rootp;
_ts_NODE_t		*sb;
_SibIndex_t		d;
{
    _ts_NODE_t	*sb_next = *siblingp (sb, d);

    if (sb_next->balance == -dir_bal(d))
	return double_rotation (rootp, sb, sb_next, d);

    else
	return single_rotation (rootp, sb, sb_next, d);
}

/*
 * Balance the subtree rooted at sb that has become to heavy on side d
 * by single rotation of sb and sb_next.
 * Also adjusts sibling pointer of the parent of sb, or *rootp if sb is
 * the top of the entire tree.
 *
 *		sb		sb_next		Single rotation: Adding x or
 *	       /  \	       /       \	deleting under 3 caused
 *	sb_next	   3	      1		sb	rotation.  Same holds for mirror
 *     /       \	      |	       /  \	image.  Single_rotation returns
 *    1	        2	==>   x       2    3	top of new balanced tree.
 *    |		|		      |
 *    x		y		      y
 */

static _ts_NODE_t	*single_rotation (rootp, sb, sb_next, d)
_ts_NODE_t		**rootp;
register _ts_NODE_t	*sb, *sb_next;
register _SibIndex_t	d;
{
    *siblingp (sb, d)       = *siblingp(sb_next, !d);
    *siblingp (sb_next, !d) = sb;
    sb->balance             -= sb_next->balance;
    sb_next->balance        = -sb->balance;
    *parentp (rootp, sb)    = sb_next;
    sb_next->parent         = sb->parent;
    sb->parent              = sb_next;

    if (*siblingp (sb, d) != (_ts_NODE_t *)NULL)
	(*siblingp (sb, d))->parent = sb;

    return sb_next;
}

/*
 * Balance the subtree rooted at sb that has become to heavy on side d
 * by double rotation of sb and sb_next.
 * Also adjusts sibling pointer of the parent of sb, or *rootp if sb is
 * the top of the entire tree.
 *
 *		sb		    sb_next2	   Double rotation: Adding x or
 *	       /  \	           /	    \	   x', or deleting under 4
 *	sb_next    \		sb_next	     sb    caused rotation. Same holds
 *     /       \    \	       /       \    /  \   for the mirror image.
 *    1   sb_next2   4	 ==>  1		2  3    4  Double_rotation returns top
 *       /        \			|  |	   of new balanced tree.
 *      2          3			x  x'
 *      |  	   |
 *      x	   x'
 */

static _ts_NODE_t	*double_rotation (rootp, sb, sb_next, d)
_ts_NODE_t		**rootp;
register _ts_NODE_t	*sb, *sb_next;
register _SibIndex_t	d;
{
    register _ts_NODE_t *sb_next2 = *siblingp(sb_next, !d);

    *siblingp (sb_next, !d)  = *siblingp (sb_next2, d);
    *siblingp (sb, d)        = *siblingp (sb_next2, !d);
    *siblingp (sb_next2, d)  = sb_next;
    *siblingp (sb_next2, !d) = sb;

    if (sb_next2->balance == sb_next->balance)
	sb_next->balance = -sb_next->balance;

    else
	sb_next->balance = 0;

    if (sb_next2->balance == sb->balance)
	sb->balance = -sb->balance;

    else
	sb->balance = 0;

    sb_next2->balance    = 0;
    *parentp (rootp, sb) = sb_next2;
    sb_next2->parent     = sb->parent;
    sb->parent           = sb_next->parent = sb_next2;

    if (*siblingp (sb_next, !d) != (_ts_NODE_t *)NULL)
	(*siblingp (sb_next, !d))->parent = sb_next;

    if (*siblingp (sb, d) != (_ts_NODE_t *)NULL)
	(*siblingp (sb, d))->parent = sb;

    return sb_next2;
}

/*
 * Tfind searches node key in the tree rooted by *rootp, using compar for
 * comparing elements.  It returns the pointer to the _ts_NODE_t in which
 * the key pointer resides, or 0 if key is not present.
 */

void	*tfind(key, root, compar)
void	*key;
void	**root;
int	(*compar)(const void *, const void *);
{
    register _ts_NODE_t	*node;
    _ts_NODE_t		**rootp = (_ts_NODE_t **)root;
    register int	cmp;

    if (rootp == (_ts_NODE_t **)NULL)
	return (void *)NULL;

    node = *rootp;
    while (node != (_ts_NODE_t *)NULL)
    {
	if ((cmp = compar (key, node->key)) == 0)
	    return node;

	node = *siblingp (node, cmp_dir (cmp));
    }

    return (void *)NULL;
}

/*
 * Twalk walks the tree rooted by node, from top to bottom and left to right,
 * calling action with the _ts_NODE_t pointer, visit order, and level in the tree
 * (0 is root).  Leafs are visited only once and action is then called with
 * `leaf' as the second parameter.  For nodes with children action is called
 * three times with visit order parameter `preorder' before, `postorder' in
 * between, and `endorder' after visiting the nodes children.
 */

void		twalk (start, action)
const void	*start;
register void	(*action)(const void *, VISIT, int);
{
    register VISIT	visit;
    register int	level;
    register _ts_NODE_t	*node = (_ts_NODE_t *)start;

    if ((node == (_ts_NODE_t *)NULL) || (action == 0))
	return;

/* run down tree from top to bottom, left to right */

    visit = preorder;
    level = 0;

    while (node != (_ts_NODE_t *)NULL)
    {
	if ((visit == preorder) &&
	    (node->LEFT == (_ts_NODE_t *)NULL) &&
	    (node->RIGHT == (_ts_NODE_t *)NULL))
	    visit = leaf;		/* node turns out to be leaf */

	action (node, visit, level);

	switch (visit)
	{
	    case preorder:			/* before visiting left child */
		if (node->LEFT != (_ts_NODE_t *)NULL)
		{
		    node = node->LEFT;
		    level++;
		}

		else
		    visit = postorder;

		break;

	    case postorder:		/* between visiting children */
		if (node->RIGHT != (_ts_NODE_t *)NULL)
		{
		    node = node->RIGHT;
		    visit = preorder;
		    level++;
		}

		else
		    visit = endorder;

		break;

	    case endorder:		/* after visiting children */
	    case leaf:			/* node has no children */
		if (node->parent != (_ts_NODE_t *)NULL)
		{
		    if (direction (node->parent, node) == L)
			visit = postorder;

		    else
			visit = endorder;
		}

		node = node->parent;
		level--;

		break;
	}
    }
}
#else

/*
 * Definition for t.... functions
 */

typedef struct _t_node {
    char		*key;
    struct _t_node	*llink;
    struct _t_node	*rlink;
} _t_NODE;

static void		_twalk (_t_NODE *,
				void (*)(const void *, VISIT, int), int);
/*
 * Basic Tree Processing Functions
 */

/*
 * Delete node with key
 */

void	*tdelete (key, rootcp, compar)
void	*key;
void	**rootcp;
int	(*compar)(const void *, const void *);
{
    _t_NODE		*p;		/* Parent of node to be deleted */
    register _t_NODE	*q;		/* Successor node		*/
    register _t_NODE	*r;		/* Right son node		*/
    int			ans;		/* Result of comparison		*/
    register _t_NODE	**rootp = (_t_NODE **)rootcp;

    if ((rootp == (_t_NODE **)NULL) || ((p = *rootp) == (_t_NODE *)NULL))
	return (void *)NULL;

    while ((ans = (*compar)(key, (*rootp)->key)) != 0)
    {
	p = *rootp;
	rootp = (ans < 0) ? &(*rootp)->llink : &(*rootp)->rlink;

	if (*rootp == (_t_NODE *)NULL)
	    return (void *)NULL;
    }

    r = (*rootp)->rlink;

    if ((q = (*rootp)->llink) == (_t_NODE *)NULL)
	    q = r;

    else if (r != (_t_NODE *)NULL)
    {
	if (r->llink == (_t_NODE *)NULL)
	{
	    r->llink = q;
	    q = r;
	}

	else
	{
	    for (q = r->llink; q->llink != (_t_NODE *)NULL; q = r->llink)
		r = q;

	    r->llink = q->rlink;
	    q->llink = (*rootp)->llink;
	    q->rlink = (*rootp)->rlink;
	}
    }

    ReleaseMemoryCell ((char *) *rootp);
    *rootp = q;
    return (void *)p;
}

/*
 * Find node with key
 */

void	*tfind (key, rootcp, compar)
void	*key;			/* Key to be located			*/
void	**rootcp;		/* Address of the root of the tree	*/
int	(*compar)(const void *, const void *);
{
    register _t_NODE	**rootp = (_t_NODE **)rootcp;

    if (rootp == (_t_NODE **)NULL)
	return (void *)NULL;

    while (*rootp != (_t_NODE *)NULL)
    {
	int	r = (*compar)(key, (*rootp)->key);

	if (r == 0)
	    return (void *)*rootp;

	rootp = (r < 0) ? &(*rootp)->llink : &(*rootp)->rlink;
    }

    return (void *)NULL;
}

/*
 * Search for a node with key key and add it if appropriate
 */

void	*tsearch (key, rootcp, compar)
void	*key;			/* Key to be located			*/
void	**rootcp;		/* Address of the root of the tree	*/
int	(*compar)(const void *, const void *);
{
    register _t_NODE	**rootp = (_t_NODE **)rootcp;
    register _t_NODE	*q;	/* New node if key not found */

    if (rootp == (_t_NODE **)NULL)
	return (void *)NULL;

    while (*rootp != (_t_NODE *)NULL)
    {
	int	r = (*compar)(key, (*rootp)->key);

	if (r == 0)
	    return (void *)*rootp;

	rootp = (r < 0) ? &(*rootp)->llink : &(*rootp)->rlink;
    }

    q = (_t_NODE *) GetAllocatedSpace (sizeof (_t_NODE));

    if (q != (_t_NODE *)NULL)
    {
        SetMemoryAreaNumber ((void *)q, 0);
	*rootp = q;
	q->key = key;
	q->llink = q->rlink = (_t_NODE *)NULL;
    }

    return (void *)q;
}


/* Walk the nodes of a tree */

void		twalk (root, action)
const void	*root;			/* Root of the tree to be walked */
void		(*action)(const void *, VISIT, int);
{
    if ((root != (char *)NULL) && (action != NULL))
	_twalk (root, action, 0);
}

static void		_twalk (root, action, level)
register _t_NODE	*root;
register void		(*action)(const void *, VISIT, int);
register int		level;
{
    if (root->llink == (_t_NODE *)NULL && root->rlink == NULL)
	(*action)(root, leaf, level);

    else
    {
	(*action)(root, preorder, level);

	if (root->llink != (_t_NODE *)NULL)
	    _twalk (root->llink, action, level + 1);

	(*action)(root, postorder, level);

	if (root->rlink != (_t_NODE *)NULL)
	    _twalk (root->rlink, action, level + 1);

	(*action)(root, endorder, level);
    }
}
#endif
