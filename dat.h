#include "util.h"

#define BUFFER_INIT_SIZE 32

typedef struct Buffer Buffer;
typedef struct Pane Pane;
typedef struct View View;

enum BspNodeType {
	LEAF,
	HORIZONTAL,
	VERTICAL,
};

struct Buffer {
	char *bob;
	char *bog;
	char *eob;
	char *eog;
	vlong gapsize;
	vlong size;
	vlong len;
};

void	 bufprint(Buffer *); /* debug only */
Buffer	*bufcreate(unsigned int);
int	 bufdeleteafter(Buffer *, int);
int	 bufdeleteruneafter(Buffer *, int);
int	 bufdeletebefore(Buffer *, int);
int	 bufdeleterunebefore(Buffer *, int);
void	 buffree(Buffer *);
int	 bufinsert(Buffer *, char *, int);
int	 bufmovebackwards(Buffer *, int);
int	 bufmovecolumn(Buffer *, int);
int	 bufmoveforward(Buffer *, int);
int	 bufmoverunebackwards(Buffer *, int);
int	 bufmoveruneforward(Buffer *, int);
int	 bufresize(Buffer *);
char	*buftostr(Buffer *);

struct Pane {
	int y, x;
	int h, w;
	int cursy, cursx;
	Buffer *buf;
	Pane *parent;
	/* 
	 * on v split left and right are respectively left and right, on 
	 * h split they are respectively up and down.
	 */
	Pane *left, *right;
	enum BspNodeType type;
};

Pane	*panecreate(int, int, int, int, Buffer *);
Pane	*panedelete(Pane *);
void	 panedrawborders(Pane *);

struct View {
	int x, y;
	int h, w;
	Pane *root;
	Pane *focus;
};

View	*viewcreate(int, int, int, int);
Pane	*viewdeletepane(View *, Pane *);
Pane	*viewfocusleftpane(View *, Pane *, enum BspNodeType);
Pane	*viewfocusrightpane(View *, Pane *, enum BspNodeType);
void	 viewfocuspane(View *, Pane *);
int	 viewmovecurs(View *, int, int);
int	 viewmovecursto(View *, int, int);
Pane	*viewsplithpane(View *, Pane *);
Pane	*viewsplitvpane(View *, Pane *);
