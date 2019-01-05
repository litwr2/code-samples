/*
 * XLife Copyright 1998 Eric S. Raymond <esr@thyrsus.com>
 *
 */

/*
Several modifications were added at 2011 by Vladimir Lidovski <vol.litwr@gmail.com>
$Id: isave.cpp 310 2014-02-11 07:40:09Z litwr $
*/

/* it saves pattern in I-format.
*/

#include <stdlib.h>
#include <ctype.h>
#include "defs.h"
#include "file.h"
#include "tile.h"

#define NO_MATCH -1 /* not 0..16 */

char matchlibfile[64] = "named-patterns";

typedef struct {
    int xtrans, ytrans;	/* translation point */
    int	flip;		/* 1 or -1 */
    int	rotation;	/* 0..3, 90-degree increments clockwise */
} dihedral;

typedef struct blobref_t {
    int			xsize, ysize, number;
    dihedral		where;
    pattern		*pattern_val;
    pattern		*blob;
    struct blobref_t	*ref;
#ifdef COALESCE
    int			refcount;
#endif /* COALESCE */
} blobref;

static void transform(int flip, int rotation,
		     coord_t j, coord_t i,
		     coord_t xsize, coord_t ysize, 
		     coord_t *newx, coord_t *newy) {
    switch (rotation + 4 * flip) {
    case 0:		/* no rotation, no flip */
	*newx = j;
	*newy = i;
	break;

    case 1:		/* one rotation, no flip */
	*newx = i;
	*newy = ysize - j;
	break;

    case 2:		/* two rotations, no flip */
	*newx = xsize - j;
	*newy = ysize - i;
	break;

    case 3:		/* three rotations, no flip */
	*newx = xsize - i;
	*newy = j;
	break;

    case 4:		/* no rotation, one flip */
	*newx = j;
	*newy = ysize - i;
	break;

    case 5:		/* one rotation, one flip */
	*newx = i;
	*newy = j;
	break;

    case 6:		/* two rotations, one flip */
	*newx = xsize - j;
	*newy = i;
	break;

    case 7:		/* three rotations, one flip */
	*newx = xsize - i;
	*newy = ysize - j;
    }
}

static int blob_equal(pattern *within, 
		      const blobref *tp, const blobref *sp,
		      dihedral *where) {
/* check blob tp for equality with blob sp */
    static offsets xoffmagic[] =
	{{0, 0}, {0, 1}, {1, 0}, {0, 0}, {0, 0}, {0, 0}, {1, 0}, {0, 1}}; 
    static offsets yoffmagic[] =
	{{0, 0}, {0, 0}, {0, 1}, {1, 0}, {0, 1}, {0, 0}, {0, 0}, {1, 0}};
    int		i, j, rotation, flip;

    /* check for identical population */
    if (cellcnt(tp->blob) != cellcnt(sp->blob))
	return NO_MATCH;

    /* bounding rectangles must be compatible */
    if (!(tp->xsize == sp->xsize && tp->ysize == sp->ysize
	|| tp->xsize == sp->ysize && tp->ysize == sp->xsize))
	    return NO_MATCH;

    for (rotation = 0; rotation < 4; rotation++)
	for (flip = 0; flip < 2; flip++) {
	    int	trans = rotation + 4*flip;
	    coord_t newx, newy, xbcorner, ybcorner;

#ifdef BLOBDEBUG
	    fprintf(stderr, "Flip %d, rotation %d\n", flip ? -1 : 1, rotation);
#endif /* BLOBDEBUG */

	    for (i = 0; i < sp->ysize; i++)
		for (j = 0; j < sp->xsize; j++) {
		    transform(flip, rotation, j, i, tp->xsize-1, tp->ysize-1,
			      &newx, &newy);
		    if (lookcell(sp->blob, sp->blob->xmin + j, sp->blob->ymin + i)
			!= lookcell(tp->blob, tp->blob->xmin + newx, tp->blob->ymin + newy))
			goto nomatch;
		}

	    /* OK, now we have the corner of the matching blob pinned down */
	    xbcorner = sp->where.xtrans
		+ xoffmagic[trans].xi * (tp->xsize-1)
		+ xoffmagic[trans].yi * (tp->ysize-1);
	    ybcorner = sp->where.ytrans
		+ yoffmagic[trans].xi * (tp->xsize-1)
		+ yoffmagic[trans].yi * (tp->ysize-1);

	    flip = flip ? -1 : 1;

#ifdef BLOBDEBUG
	    /*
	     * OK, the blobs matched.  What if there is a larger composite
	     * pattern associated with tp?  Does it match the rest of the 
	     * context?
	     */
	    if (tp->pattern_val != tp->blob) {
		coord_t	xsize = tp->pattern_val->xmax - tp->pattern_val->xmin;
		coord_t	ysize = tp->pattern_val->ymax - tp->pattern_val->ymin;
		coord_t xinner, yinner, xouter, youter, xloc, yloc;

		printf("# Possible %s with flip %2d rotation %d based on blob at (%d,%d)\n",
		       tp->pattern_val->pattern_name,
		       flip, rotation,
		       sp->where.xtrans, sp->where.ytrans);
		continue;
	    }
#endif

	    if (where) {
		where->flip = flip;
		where->rotation = rotation;
		where->xtrans = xbcorner; 
		where->ytrans = ybcorner;
	    }
	    return trans;
	nomatch:;
	}

    return NO_MATCH;
}

void savestructured(pattern *context, FILE *ofp) {
/* save with full scene analysis in structured (#I) format */
    pattern	*pp, *parts;
    blobref	*scene, *sp, *tp;
    int		blobcount, i, nonrefcount;
    FILE	*fp;
    LoadReq	*loadqueue;
    static int		matchcount = 0;
    static blobref	*matchlib;

    if ((parts = analyze(context)) == 0) {
	perror("Pattern analysis failed; too many blobs in picture.\n");
	return;
    }
/* if there is only one blob, save in #P format */
    if (!parts[1].tiles) {
	bounding_box(context);
        savesimple(context, context->xmin - STARTX, context->ymin - STARTY, ofp);
	free(parts);
	return;
    }
/*
 * Otherwise it's time to do a full scene analysis.
 */
    blobcount = 0;
    for (pp = parts; pp->tiles; pp++)
	blobcount++;
    scene = (blobref*)calloc(sizeof(blobref), blobcount);

/* assemble a list of blob references */
    for (i = 0; i < blobcount; i++) {
	pattern	*pp = parts + i;
	sp = scene + i;

	sp->xsize = pp->xmax - pp->xmin + 1;
	sp->ysize = pp->ymax - pp->ymin + 1;

	sp->pattern_val = sp->blob = &parts[i];

	sp->ref = 0; 	/* defensive programming */
	sp->where.rotation = 0;		 	/* defensive programming */
	sp->where.flip = 1;
	sp->where.xtrans = sp->pattern_val->xmin;
	sp->where.ytrans = sp->pattern_val->ymin;
#ifdef COALESCE
	sp->refcount = 1;
#endif /* COALESCE */
    }

    /* search for duplicate blobs */
    for (sp = scene; sp < scene + blobcount; sp++)
	for (tp = scene; tp < sp; tp++)	{
	    if (tp->ref)
		continue;
	    if (blob_equal(context, tp, sp, &sp->where) != NO_MATCH) {
		sp->ref = tp;
#ifdef COALESCE
		tp->refcount++;
#endif /* COALESCE */
#ifdef BLOBDEBUG
		fprintf(stderr, "Matched, flip %d and rotation %d\n", 
			sp->where.flip, sp->where.rotation);
#endif /* BLOBDEBUG */
		break;
	    }
	}
    /* initialize the list of named blobs */
    if (!matchcount) {
        if (!strcmp(active_rules, "life") && !find_file(matchlibfile, dirs) && (fp = fopen(rfullpath, "r"))) {
	    char buf[BUFSIZ], name[BUFSIZ], request[BUFSIZ];
	    int gens, gen;
	    cell_t state;
	    matchcount = 0;
	    matchlib = (blobref*)calloc(sizeof(blobref), 1);
	    while (fgets(buf, sizeof(buf) - 1, fp)) {
		char *cp;
		static pattern matcher;	/* must be initially zeroed */
		if (buf[0] == '#')	/* allow comments */
		    continue;
		initcells(&matcher);
		if (cp = strchr(buf, '#'))
		    while (isspace(*cp) || *cp == '#')
			*cp-- = '\0';
		name[0] = request[0] = '\0';
		gens = 0;
		sscanf(buf, "%s %s %d", name, request, &gens);
		if (name[0] == '\0')
		    continue;
		else if (request[0] == '\0')
		    strcpy(request, name);
		buf[strlen(buf) - 1] = '\0';
		loadqueue = 0;
		loadx = xpos; 
		loady = ypos;
		txx = tyy = 1; 
		txy = tyx = 0; 
		add_loadreq(&loadqueue, 0, request, 0,
                    STARTX, STARTY, 1, 0, 0, 1);
                if (!do_loadreq(loadqueue, &matcher)) return;
		gen = 0;
		do {
		    tile *ptr;
		    int dx, dy;
		    if (gen == 0)
			strcpy(request, name);
		    else
			sprintf(request, "%s%d", name, gen + 1);
		    matchlib = (blobref*)realloc(matchlib, sizeof(blobref)*(matchcount + 1));
		    tp = matchlib + matchcount;
		    tp->pattern_val = (pattern*)malloc(sizeof(pattern));
		    initcells(tp->pattern_val);
		    FOR_CELLS(&matcher, ptr, dx, dy)
			if ((state = getcell(&ptr->cells, dx, dy)))
			    chgcell(tp->pattern_val, ptr->x + dx, ptr->y + dy, state);
		    bounding_box(tp->pattern_val);
		    tp->blob = 0;
		    tp->pattern_val->pattern_name = strdup(request);
		    matchcount++;
		    generate(&matcher);
		} while
		      (++gen < gens);
	    }
	    fclose(fp);
	}
	/* now go through looking for composites */
	for (tp = matchlib; tp < matchlib + matchcount; tp++) {
	    pattern *pp, *parts = analyze(tp->pattern_val);
	    if (!parts[1].tiles)
		tp->blob = tp->pattern_val;
	    else
	    {
		int	merit = 0;
		pattern	*best;
		for (pp = parts; pp->tiles; pp++) {
		    int	m = cellcnt(pp)*(pp->xmax - pp->xmin)*(pp->ymax - pp->ymin);
		    if (m > merit) {
			best = pp;
			merit = m;
		    }
		}
		tp->blob = (pattern*)malloc(sizeof(pattern));
		memcpy(tp->blob, best, sizeof(pattern));
		initcells(best);
	    }
	    for (pp = parts; pp->tiles; pp++)
		clear_pattern(pp);
	    free(parts);
	    bounding_box(tp->blob);
	    tp->xsize = (tp->blob->xmax - tp->blob->xmin + 1); 
	    tp->ysize = (tp->blob->ymax - tp->blob->ymin + 1);
	}
    }
    /* check for named blocks in the pattern */
    if (matchcount) {
	int	point;
	for (sp = scene; sp < scene + blobcount; sp++)
	    for (tp = matchlib; tp < matchlib + matchcount; tp++)
		if ((point = blob_equal(context, tp, sp, (dihedral *)NULL)) != NO_MATCH) {
		    char	*cp;

		    if (sp->ref)
			continue;
		    if ((cp = strchr(tp->pattern_val->pattern_name, ':')))
			cp++;
		    else
			cp = tp->pattern_val->pattern_name;
		    if (sp->pattern_val->pattern_name)
			free(sp->pattern_val->pattern_name);
		    sp->pattern_val->pattern_name = strdup(cp);
		}
	fputs("\n", ofp);
    }

#ifdef COALESCE
    /*
     * Coalesce unnamed blobs with bounding rectangles that nearly touch.
     * This way we get an output report with fewer tiny "junk" blobs.
     */
#define NEAR	2
#define LAP(s1, e1, s2, e2)	((((s2)+NEAR <= (e1)-NEAR) && ((e2)+NEAR >= (s1)-NEAR)) \
				 || (((s1)+NEAR <= (e2)-NEAR) && ((e1)+NEAR >= (s2)-NEAR)))
    do {
	i = 0;
	for (sp = scene; sp < scene + blobcount; sp++)
	    if (sp->refcount > 0 && !sp->pattern_val->pattern_name)
		for (tp = scene; tp < sp; tp++)
		    if (tp->refcount > 0 && !tp->pattern_val->pattern_name)
			if (LAP(sp->pattern_val->box.min.x,
				sp->pattern_val->box.max.x, 
				tp->pattern_val->box.min.x,
				tp->pattern_val->box.max.x)
			    && LAP(sp->pattern_val->box.min.y,
				   sp->pattern_val->box.max.y, 
				   tp->pattern_val->box.min.y,
				   tp->pattern_val->box.max.y)) {
			int 	state, dx, dy;
			tile	*ptr;

			FOR_CELLS(sp->pattern_val, ptr, dx, dy)
			    if ((state=getcell(&ptr->cells, dx, dy))) {
				chgcell(tp->pattern_val, ptr->x+dx, ptr->y+dy, state);
				chgcell(sp->pattern_val, ptr->x+dx, ptr->y+dy, 0);
			    }
			sp->refcount--;
			i++;
		    }
    } while
	(i);
#undef LAP
#endif /* COALESCE */

/* name the unnamed blobs */
    nonrefcount = 0;
    for (sp = scene; sp < scene + blobcount; sp++)
	if (!sp->ref && !sp->pattern_val->pattern_name) {
	    char	namebuf[10];

	    sprintf(namebuf, "part%d", ++nonrefcount);
	    sp->pattern_val->pattern_name = strdup(namebuf);
	}

    /* emit the blob list */
    for (sp = scene; sp < scene + blobcount; sp++) {
#ifdef COALESCE
	if (!sp->refcount)
	   continue;
#endif /* COALESCE */
	if (!sp->ref) {
	    fprintf(ofp, "#B %s\n", sp->pattern_val->pattern_name);
	    savesimple(sp->pattern_val, 0, 0, ofp);
	    fputs("#E\n\n", ofp);
	}
    }
    /* issue the inclusion list */
    fprintf(ofp, "#C %d blobs\n", blobcount);
    for (sp = scene; sp < scene + blobcount; sp++) {
#ifdef COALESCE
	if (!sp->refcount)
	   continue;
#endif /* COALESCE */ 
	if (sp->ref)
	    tp = sp->ref;
	else
	    tp = sp;
	fprintf(ofp, "#I :%s\t%4d %4d %d %2d 0\n",
		tp->pattern_val->pattern_name, 
		sp->where.xtrans - STARTX, 
		sp->where.ytrans - STARTY,
		sp->where.rotation,
		sp->where.flip);
    }
    free(parts);
}
