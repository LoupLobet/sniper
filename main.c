#include <ncurses.h>
#include <stdlib.h>

#include "dat.h"

#define PANE_CENTER(pn, x, y) do { x = pn->x + pn->w/2; y = pn->y + pn->h / 2; } while(0);

Pane *
panecreate(int y, int x, int h, int w)
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
	if ((pn->buf = bufcreate(BUFFER_INIT_SIZE)) == nil) {
		free(pn);
		return nil;
	}
	return pn;
}

void
panefocus(Pane *pn)
{
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
			mvaddch(pn->left->y + pn->left->h, i, '-');
		break;
	case VERTICAL:
		for (i = pn->y; i < (pn->y + pn->h); i++)
			mvaddch(i, pn->left->x + pn->left->w, '|');
		break;
	}
	panedrawborders(pn->left);
	panedrawborders(pn->right);
}

int
panemovecurs(Pane *pn, int y, int x)
{
	return panemovecursto(pn, pn->cursy + y,  pn->cursx + x);
}

int
panemovecursto(Pane *pn, int y, int x)
{
	if (0 <= y && y < pn->h)
		pn->cursy = y;
	if (0 <= x && x < pn->w)
		pn->cursx = x;
	return move(pn->y + pn->cursy, pn->x + pn->cursx);
}

Pane *
panemovefocusdown(Pane *pn)
{
	Pane *current = pn;
	Pane *candidate;

	while (current->parent != nil) {
		if (current->parent->type == HORIZONTAL && current == current->parent->left) {
			candidate = current->parent->right;
			while (candidate->type != LEAF) {
				candidate = (candidate->left != nil ? candidate->left : candidate->right);
			}
			panefocus(candidate);
			return candidate;
		}
		current = current->parent;
	}
	return pn;
}

Pane *
panemovefocusleft(Pane *pn)
{
	Pane *current = pn;
	Pane *candidate;

	while (current->parent != nil) {
		if (current->parent->type == VERTICAL && current == current->parent->right) {
			candidate = current->parent->left;
			while (candidate->type != LEAF) {
				if (candidate->type == VERTICAL)
					candidate = (candidate->right != nil ? candidate->right : candidate->left);
				else
					candidate = (candidate->left != nil ? candidate->left : candidate->right);
			}
			panefocus(candidate);
			return candidate;
		}
		current = current->parent;
	}
	return pn;
}

Pane *
panemovefocusright(Pane *pn)
{
	Pane *current = pn;
	Pane *candidate;

	while (current->parent != nil) {
		if (current->parent->type == VERTICAL && current == current->parent->left) {
			candidate = current->parent->right;
			while (candidate->type != LEAF) {
				candidate = (candidate->left != nil ? candidate->left : candidate->right);
			}
			panefocus(candidate);
			return candidate;
		}
		current = current->parent;
	}
	return pn;
}

Pane *
panemovefocusup(Pane *pn)
{
	Pane *current = pn;
	Pane *candidate;

	while (current->parent != nil) {
		if (current->parent->type == HORIZONTAL && current == current->parent->right) {
			candidate = current->parent->left;
			while (candidate->type != LEAF) {
				if (candidate->type == HORIZONTAL)
					candidate = (candidate->right != nil ? candidate->right : candidate->left);
				else
					candidate = (candidate->left != nil ? candidate->left : candidate->right);
			}
			panefocus(candidate);
			return candidate;
		}
		current = current->parent;
	}
	return pn;
}

Pane *
panehsplit(Pane *pn)
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
	if ((split = panecreate(pn->y, pn->x, pn->h, pn->w)) == nil)
		return nil;
	split->type = HORIZONTAL;
	split->buf = nil;
	if ((split->right = panecreate(pn->y + h1 + 1, pn->x, h2, pn->w)) == nil) {
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
	}
	return split;
}

Pane *
panevsplit(Pane *pn)
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
	if ((split = panecreate(pn->y, pn->x, pn->h, pn->w)) == nil)
		return nil;
	split->type = VERTICAL;
	split->buf = nil;
	if ((split->right = panecreate(pn->y, pn->x + w1 + 1, pn->h, w2)) == nil) {
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
	if ((v->root = panecreate(y, x, h, w)) == nil) {
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
	Pane *focus;
	Pane *split;
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
	//panecnt = 0;
	v->root = panevsplit(v->root);
	panedrawborders(v->root);
	focus = v->root->left;
	panefocus(focus);

	quit = 0;
	refresh();
	while (!quit) {
		c = getch();
		switch (c) {
		case 's':
			if ((split = panehsplit(focus)) == nil)
				sysfatal("cannot split pane horizontally");
			focus = split->right;
			panedrawborders(v->root);
			panefocus(focus);
			break;
		case 'v':
			if ((split = panevsplit(focus)) == nil)
				sysfatal("cannot split pane vertically");
			focus = split->right;
			panedrawborders(v->root);
			panefocus(focus);
			break;
		case 'H':
			focus = panemovefocusleft(focus);
			break;
		case 'J':
			focus = panemovefocusdown(focus);
			break;
		case 'K':
			focus = panemovefocusup(focus);
			break;
		case 'L':
			focus = panemovefocusright(focus);
			break;
		case 'h':
			panemovecurs(focus, 0, -1);
			break;
		case 'l':
			panemovecurs(focus, 0, 1);
			break;
		case 'k':
			panemovecurs(focus, -1, 0);
			break;
		case 'j':
			panemovecurs(focus, 1, 0);
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
