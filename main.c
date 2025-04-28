#include <ncurses.h>
#include <stdlib.h>

#include "dat.h"

#define ABS(n) (n < 0 ? -n : n)
#define PANE_DISTANCE_X(pn1, pn2) ABS(((pn1->x + pn1->w / 2) - (pn2->x + pn2->w / 2)))
#define PANE_DISTANCE_Y(pn1, pn2) ABS(((pn1->y + pn1->h / 2) - (pn2->y + pn2->h / 2)))
#define PANE_CLOSEST_CHILD(pn, ref) \
	(pn->type == VERTICAL \
		? (PANE_DISTANCE_X(ref, pn->right) < PANE_DISTANCE_X(ref, pn->left) \
			? pn->right \
			: pn->left) \
		: (PANE_DISTANCE_Y(ref, pn->right) < PANE_DISTANCE_Y(ref, pn->left) \
			? pn->right \
			: pn->left) \
	)

Pane *
panecreate(int y, int x, int h, int w, Buffer *buf)
{
	Pane *pn;

	if ((pn = malloc(sizeof(Pane))) == nil)
		return nil;
	pn->y = y;
	pn->x = x;
	pn->h = h;
	pn->w = w;
	pn->type = LEAF;
	pn->left = nil;
	pn->right = nil;
	pn->cursy = pn->cursx = 0;
	pn->buf = nil;
	pn->parent = nil;
	pn->buf = buf;
	return pn;
}

Pane *
viewdeletepane(View *v, Pane *pn)
{
	Pane *keep, *ret;

	if (pn->parent == nil) {
		v->root = nil;
		v->focus = nil;
		free(pn);
		ret = nil;
	} else if (pn->parent != nil && pn->parent->parent == nil) {
		if (pn->parent->left == pn) {
			keep = pn->parent->right;
			ret = pn->parent->left;
		} else {
			keep = pn->parent->left;
			ret = pn->parent->right;
		}
		v->root = keep;
		keep->parent = v->root;
		free(pn->parent);
		free(pn);
	} else {
		if (pn->parent->left == pn) {
			keep = pn->parent->right;
			ret = pn->parent->left;
			if (pn->parent->parent->left == pn->parent)
				pn->parent->parent->left = keep;
			else
				pn->parent->parent->right = keep;
		} else {
			keep = pn->parent->left;
			ret = pn->parent->right;
			if (pn->parent->parent->right == pn->parent)
				pn->parent->parent->right = keep;
			else
				pn->parent->parent->left = keep;
		}
		keep->parent = pn->parent->parent;
		free(pn->parent);
		free(pn);
	}
	return ret;
}

void
viewfocuspane(View *v, Pane *pn)
{
	v->focus = pn;
	move(pn->y + pn->cursy, pn->x + pn->cursx);
}

void
panedrawborders(Pane *pn)
{
	int i;

	switch (pn->type) {
	case LEAF:
		return;
		break;
	case HORIZONTAL:
		for (i = pn->x; i < (pn->x + pn->w); i++)
			mvaddch(pn->left->y + pn->left->h, i, ACS_HLINE);
		break;
	case VERTICAL:
		for (i = pn->y; i < (pn->y + pn->h); i++)
			mvaddch(i, pn->left->x + pn->left->w, ACS_VLINE);
		break;
	}
	panedrawborders(pn->left);
	panedrawborders(pn->right);
}

int
viewmovecurs(View *v, int y, int x)
{
	return viewmovecursto(v, v->focus->cursy + y,  v->focus->cursx + x);
}

int
viewmovecursto(View *v, int y, int x)
{
	if (0 <= y && y < v->focus->h)
		v->focus->cursy = y;
	if (0 <= x && x < v->focus->w)
		v->focus->cursx = x;
	return move(v->focus->y + v->focus->cursy, v->focus->x + v->focus->cursx);
}

Pane *
viewfocusleftpane(View *v, Pane *pn, enum BspNodeType direction)
{
	Pane *current = pn;
	Pane *candidate;

	// BUG: Bug randomly appears where moving up creates a ghost pane with height 1
	while (current->parent != nil) {
		if (current->parent->type == direction && current == current->parent->right) {
			candidate = current->parent->left;
			while (candidate->type != LEAF) {
				candidate = PANE_CLOSEST_CHILD(candidate, pn);
			}
			v->focus = candidate;
			viewfocuspane(v, candidate);
			return candidate;
		}
		current = current->parent;
	}
	return pn;
}

Pane *
viewfocusrightpane(View *v, Pane *pn, enum BspNodeType direction)
{
	Pane *current = pn;
	Pane *candidate;

	// BUG: Bug randomly appears where moving up creates a ghost pane with height 1
	while (current->parent != nil) {
		if (current->parent->type == direction && current == current->parent->left) {
			candidate = current->parent->right;
			while (candidate->type != LEAF) {
				candidate = PANE_CLOSEST_CHILD(candidate, pn);
			}
			v->focus = candidate;
			viewfocuspane(v, candidate);
			return candidate;
		}
		current = current->parent;
	}
	return pn;
}

Pane *
viewsplithpane(View *v, Pane *pn)
{
	int h1, h2;
	Pane *split;
	Pane *parent;
	if (pn->h % 2) {
		h1 = (pn->h - 1) / 2;
		h2 = (pn->h - 1) / 2;
	} else {
		h1 = pn->h / 2;
		h2 = (pn->h - 2) / 2;
	}
	/* 
	 * create a new pane to represent the split, with the same geometry
	 * than the current pane, and move the current pane as the left child
	 * and create the new pane for the right child.
	 */
	parent = pn->parent;
	if ((split = panecreate(pn->y, pn->x, pn->h, pn->w, nil)) == nil)
		return nil;
	split->type = HORIZONTAL;
	if ((split->right = panecreate(pn->y + h1 + 1, pn->x, h2, pn->w, nil)) == nil) {
		free(split);
		return nil;
	}
	split->right->parent = split;
	split->left = pn;
	split->left->h = h1;
	split->left->parent = split;
	/*
	 * reconnect the split pane as a child of the parent pane if it exitst, if the
	 * splited pane was the root pane, MUST reaffect v->root = pane*split(v->root)
	 */
	if (parent != nil) {
		if (parent->right == pn)
			parent->right = split;
		else
			parent->left = split;
		split->parent = parent;
	} else {
		v->root = split;
	}
	return split;
}

Pane *
viewsplitvpane(View *v, Pane *pn)
{
	int w1, w2;
	Pane *split;
	Pane *parent;
	if (pn->w % 2) {
		w1 = (pn->w - 1) / 2;
		w2 = (pn->w - 1) / 2;
	} else {
		w1 = pn->w / 2;
		w2 = (pn->w - 2) / 2;
	}
	/* 
	 * create a new pane to represent the split, with the same geometry
	 * than the current pane, and move the current pane as the left child
	 * and create the new pane for the right child.
	 */
	parent = pn->parent;
	if ((split = panecreate(pn->y, pn->x, pn->h, pn->w, nil)) == nil)
		return nil;
	split->type = VERTICAL;
	if ((split->right = panecreate(pn->y, pn->x + w1 + 1, pn->h, w2, nil)) == nil) {
		free(split);
		return nil;
	}
	split->right->parent = split;
	split->left = pn;
	split->left->w = w1;
	split->left->parent = split;
	/*
	 * reconnect the split pane as a child of the parent pane if it exitst, if the
	 * splited pane was the root pane, MUST reaffect v->root = pane*split(v->root)
	 */
	if (parent != nil) {
		if (parent->right == pn)
			parent->right = split;
		else
			parent->left = split;
		split->parent = parent;
	} else {
		v->root = split;
	}
	return split;
}

View *
viewcreate(int y, int x, int h, int w)
{
	View *v;

	if ((v = malloc(sizeof(View))) == nil)
		return nil;
	v->y = y;
	v->x = x;
	v->h = h;
	v->w = w;
	if ((v->root = panecreate(y, x, h, w, nil)) == nil) {
		free(v);
		return nil;
	}
	v->focus = v->root;
	return v;
}

int
main(int argc, char *argv[])
{
	View *v;
	int c, quit;
	Pane *split, *del;
	int scrh, scrw;

	initscr();
	cbreak();
	noecho();
	curs_set(1);
	keypad(stdscr, TRUE);

    getmaxyx(stdscr, scrh, scrw);
	if ((v = viewcreate(0, 0, scrh, scrw)) == nil)
		sysfatal("cannot create view");
	if ((v->root->buf = bufcreate(BUFFER_INIT_SIZE)) == nil)
		sysfatal("cannot create buffer");
	panedrawborders(v->root);
	viewfocuspane(v, v->focus);

	quit = 0;
	refresh();
	while (!quit) {
		c = getch();
		switch (c) {
		case 's':
			if ((split = viewsplithpane(v, v->focus)) == nil)
				sysfatal("cannot split pane horizontally");
			panedrawborders(v->root);
			viewfocuspane(v, split->right);
			break;
		case 'v':
			if ((split = viewsplitvpane(v, v->focus)) == nil)
				sysfatal("cannot split pane vertically");
			panedrawborders(v->root);
			viewfocuspane(v, split->right);
			break;
		case 'd':
			if (v->focus != v->root) {
				if ((del = viewdeletepane(v, v->focus)) != nil) { 
					viewfocuspane(v, del);
				}
				panedrawborders(v->root);
			}
			break;
		case 'H':
			viewfocusleftpane(v, v->focus, VERTICAL);
			break;
		case 'J':
			viewfocusrightpane(v, v->focus, HORIZONTAL);
			break;
		case 'K':
			viewfocusleftpane(v, v->focus, HORIZONTAL);
			break;
		case 'L':
			viewfocusrightpane(v, v->focus, VERTICAL);
			break;
		case 'h':
			viewmovecurs(v, 0, -1);
			break;
		case 'l':
			viewmovecurs(v, 0, 1);
			break;
		case 'k':
			viewmovecurs(v, -1, 0);
			break;
		case 'j':
			viewmovecurs(v, 1, 0);
			break;
		case 'Q':
			quit = 1;
			break;
		}
		if (c == KEY_BACKSPACE) {
		} else if (0x20 <= c && c <= 0x7e) {
		}
		refresh();
	}
	endwin();
	
	return 0;
}
