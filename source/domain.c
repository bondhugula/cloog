
   /**-------------------------------------------------------------------**
    **                               CLooG                               **
    **-------------------------------------------------------------------**
    **                             domain.c                              **
    **-------------------------------------------------------------------**
    **                   First version: october 28th 2001                **
    **-------------------------------------------------------------------**/


/******************************************************************************
 *               CLooG : the Chunky Loop Generator (experimental)             *
 ******************************************************************************
 *                                                                            *
 * Copyright (C) 2001-2005 Cedric Bastoul                                     *
 *                                                                            *
 * This is free software; you can redistribute it and/or modify it under the  *
 * terms of the GNU General Public License as published by the Free Software  *
 * Foundation; either version 2 of the License, or (at your option) any later *
 * version.                                                                   *
 *                                                                            *
 * This software is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with software; if not, write to the Free Software Foundation, Inc.,        *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA                     *
 *                                                                            *
 * CLooG, the Chunky Loop Generator                                           *
 * Written by Cedric Bastoul, Cedric.Bastoul@inria.fr                         *
 *                                                                            *
 ******************************************************************************/
/* CAUTION: the english used for comments is probably the worst you ever read,
 *          please feel free to correct and improve it !
 */


# include <stdlib.h>
# include <stdio.h>
# include <ctype.h>
# include "../include/cloog/cloog.h"


/**
 * The maximal number of rays allowed to be allocated by PolyLib. In fact since
 * version 5.20, PolyLib automatically tune the number of rays by multiplying
 * by 2 this number each time the maximum is reached. For unknown reasons
 * PolyLib makes a segmentation fault if this number is too small. If this
 * number is too small, performances will be reduced, if it is too high, memory
 * will be saturated. Note that the option "-rays X" set this number to X.
 */
int MAX_RAYS = 50 ;


/******************************************************************************
 *                             Memory leaks hunting                           *
 ******************************************************************************/


/**
 * These functions and global variables are devoted to memory leaks hunting: we
 * want to know at each moment how many Polyhedron structures had been allocated
 * (cloog_domain_allocated) and how many had been freed (cloog_domain_freed).
 * Each time a Polyhedron structure is allocated, a call to the function
 * cloog_domain_leak_up() must be carried out, and respectively
 * cloog_domain_leak_down() when a Polyhedron structure is freed. The special
 * variable cloog_domain_max gives the maximal number of Polyhedron structures
 * simultaneously alive (i.e. allocated and non-freed) in memory.
 * - July 3rd->11th 2003: first version (memory leaks hunt and correction).
 */


int cloog_domain_allocated = 0 ;
int cloog_domain_freed = 0 ;
int cloog_domain_max = 0 ;


static void cloog_domain_leak_up()
{ cloog_domain_allocated ++ ;
  if ((cloog_domain_allocated-cloog_domain_freed) > cloog_domain_max)
  cloog_domain_max = cloog_domain_allocated - cloog_domain_freed ;
}


static void cloog_domain_leak_down()
{ cloog_domain_freed ++ ;
}


/******************************************************************************
 *                              PolyLib interface                             *
 ******************************************************************************/


/* CLooG makes an intensive use of polyhedral operations and the PolyLib do
 * the job. Here are the interfaces to all the PolyLib calls (CLooG uses 19
 * PolyLib functions), with or without some adaptations. If another polyhedral
 * library can be used, only these functions have to be changed.
 * - April 16th 2005: Since PolyLib 5.20, compacting is no more useful and have
 *                    been removed. The direct use of the PolyLib's Polyhedron
 *                    data structure is also replaced with the CloogDomain data
 *                    structure that includes the Polyhedron and an additional
 *                    counter on how many pointers point on this structure.
 *                    This allows to save memory (cloog_domain_copy now only
 *                    increment the counter) while memory leaks are avoided (the
 *                    function cloog_domain_free decrements the counter and
 *                    actually frees the data structure only when its value
 *                    is 0).
 */


/**
 * cloog_domain_matrix2domain function:
 * Given a matrix of constraints (matrix), this function constructs and returns
 * the corresponding domain (i.e. the CloogDomain structure including the
 * polyhedron with its double representation: constraint matrix and the set of
 * rays).
 */ 
CloogDomain * cloog_domain_matrix2domain(CloogMatrix * matrix)
{ return (cloog_domain_alloc(Constraints2Polyhedron(matrix,MAX_RAYS))) ;
}


/**
 * cloog_domain_domain2matrix function:
 * Given a polyhedron (in domain), this function returns its corresponding
 * matrix of constraints.
 */
CloogMatrix * cloog_domain_domain2matrix(CloogDomain * domain)
{
  return cloog_matrix_matrix(Polyhedron2Constraints(domain->polyhedron));
}


/**
 * cloog_domain_print function:
 * This function prints the content of a CloogDomain structure (domain) into
 * a file (foo, possibly stdout).
 */
void cloog_domain_print(FILE * foo, CloogDomain * domain)
{ Polyhedron_Print(foo,P_VALUE_FMT,domain->polyhedron) ;
  fprintf(foo,"Number of active references: %d\n",domain->references) ;
}


/**
 * cloog_polyhedron_print function:
 * This function prints the content of a Polyhedron structure (polyhedron) into
 * a file (foo, possibly stdout). Just there as a development facility.
 */
void cloog_polyhedron_print(FILE * foo, Polyhedron * polyhedron)
{ Polyhedron_Print(foo,P_VALUE_FMT,polyhedron) ;
}


/**
 * cloog_domain_free function:
 * This function frees the allocated memory for a CloogDomain structure
 * (domain). It decrements the number of active references to this structure,
 * if there are no more references on the structure, it frees it (with the
 * included list of polyhedra).
 */
void cloog_domain_free(CloogDomain * domain)
{ if (domain != NULL)
  { domain->references -- ;
    
    if (domain->references == 0)
    { if (domain->polyhedron != NULL)
      { cloog_domain_leak_down() ;
        Domain_Free(domain->polyhedron) ;
      }
      free(domain) ;
    }
  }
}


/**
 * cloog_domain_copy function:
 * This function returns a copy of a CloogDomain structure (domain). To save
 * memory this is not a memory copy but we increment a counter of active
 * references inside the structure, then return a pointer to that structure.
 */ 
CloogDomain * cloog_domain_copy(CloogDomain * domain)
{ domain->references ++ ;
  return domain ;
}


/**
 * cloog_domain_image function:
 * This function returns a CloogDomain structure such that the included
 * polyhedral domain is computed from the former one into another
 * domain according to a given affine mapping function (mapping). 
 */
CloogDomain * cloog_domain_image(CloogDomain * domain, CloogMatrix * mapping)
{ return (cloog_domain_alloc(DomainImage(domain->polyhedron,mapping,MAX_RAYS)));
}


/**
 * cloog_domain_preimage function:
 * Given a polyhedral domain (polyhedron) inside a CloogDomain structure and a
 * mapping function (mapping), this function returns a new CloogDomain structure
 * with a polyhedral domain which when transformed by mapping function (mapping)
 * gives (polyhedron).
 */
CloogDomain * cloog_domain_preimage(CloogDomain * domain, CloogMatrix * mapping)
{ return (cloog_domain_alloc(DomainPreimage(domain->polyhedron,
                                            mapping,MAX_RAYS))) ;
}


/**
 * cloog_domain_convex function:
 * Given a polyhedral domain (polyhedron), this function concatenates the lists
 * of rays and lines of the two (or more) polyhedra in the domain into one
 * combined list, and  find the set of constraints which tightly bound all of
 * those objects. It returns the corresponding polyhedron.
 */ 
CloogDomain * cloog_domain_convex(CloogDomain * domain)
{ return (cloog_domain_alloc(DomainConvex(domain->polyhedron,MAX_RAYS)));
}


/**
 * cloog_domain_simplified_hull:
 * Given a list (union) of polyhedra, this function returns a single
 * polyhedron that contains this union and uses only contraints that
 * appear in one or more of the polyhedra in the list.
 * 
 * We simply iterate over all constraints of all polyhedra and test
 * whether all rays of the other polyhedra satisfy/saturate the constraint.
 */
static CloogDomain *cloog_domain_simplified_hull(CloogDomain * domain)
{
  int dim = cloog_domain_dimension(domain);
  int i, j, k, l;
  int nb_pol = 0, nb_constraints = 0;
  Polyhedron *P;
  CloogMatrix **rays, *matrix;
  Value tmp;
  CloogDomain *bounds;

  value_init(tmp);
  for (P = domain->polyhedron; P; P = P->next) {
    ++nb_pol;
    nb_constraints += P->NbConstraints;
  }
  matrix = cloog_matrix_alloc(nb_constraints, 1 + dim + 1);
  nb_constraints = 0;
  rays = (CloogMatrix **)malloc(nb_pol * sizeof(CloogMatrix*));
  for (P = domain->polyhedron, i = 0; P; P = P->next, ++i)
    rays[i] = Polyhedron2Rays(P);

  for (P = domain->polyhedron, i = 0; P; P = P->next, ++i) {
    CloogMatrix *constraints = Polyhedron2Constraints(P);
    for (j = 0; j < constraints->NbRows; ++j) {
      for (k = 0; k < nb_pol; ++k) {
	if (i == k)
	  continue;
	for (l = 0; l < rays[k]->NbRows; ++l) {
	  Inner_Product(constraints->p[j]+1, rays[k]->p[l]+1, dim+1, &tmp);
	  if (value_neg_p(tmp))
	    break;
	  if ((value_zero_p(constraints->p[j][0]) || 
	       value_zero_p(rays[k]->p[l][0])) && value_pos_p(tmp))
	    break;
	}
	if (l < rays[k]->NbRows)
	  break;
      }
      if (k == nb_pol)
	Vector_Copy(constraints->p[j], matrix->p[nb_constraints++], 1+dim+1);
    }
    Matrix_Free(constraints);
  }

  for (P = domain->polyhedron, i = 0; P; P = P->next, ++i)
    Matrix_Free(rays[i]);
  free(rays);
  value_clear(tmp);

  matrix->NbRows = nb_constraints;
  bounds = cloog_domain_matrix2domain(matrix);
  cloog_matrix_free(matrix);

  return bounds;
}


/**
 * cloog_domain_simple_convex:
 * Given a list (union) of polyhedra, this function returns a "simple"
 * convex hull of this union.  In particular, the constraints of the
 * the returned polyhedron consist of (parametric) lower and upper
 * bounds on individual variables and constraints that appear in the
 * original polyhedra.
 *
 * nb_par is the number of parameters of the domain.
 */
CloogDomain * cloog_domain_simple_convex(CloogDomain * domain, int nb_par)
{
  int i;
  int dim = cloog_domain_dimension(domain) - nb_par;
  CloogDomain *convex = NULL;

  if (cloog_domain_isconvex(domain))
    return cloog_domain_copy(domain);

  for (i = 0; i < dim; ++i) {
    CloogDomain *bounds = cloog_domain_bounds(domain, i, nb_par);

    if (!convex)
      convex = bounds;
    else {
      CloogDomain *temp = cloog_domain_intersection(convex, bounds);
      cloog_domain_free(bounds);
      cloog_domain_free(convex);
      convex = temp;
    }
  }
  if (dim > 1) {
    CloogDomain *temp, *bounds;

    bounds = cloog_domain_simplified_hull(domain);
    temp = cloog_domain_intersection(convex, bounds);
    cloog_domain_free(bounds);
    cloog_domain_free(convex);
    convex = temp;
  }

  return convex;
}


/**
 * cloog_domain_simplify function:
 * Given two polyhedral domains (pol1) and (pol2) inside two CloogDomain
 * structures, this function finds the largest domain set (or the smallest list
 * of non-redundant constraints), that when intersected with polyhedral
 * domain (pol2) equals (Pol1)intersect(Pol2). The output is a new CloogDomain
 * structure with a polyhedral domain with the "redundant" constraints removed.
 * NB: this function do not work as expected with unions of polyhedra...
 */ 
CloogDomain * cloog_domain_simplify(CloogDomain * dom1, CloogDomain * dom2)
{
  CloogMatrix *M, *M2;
  CloogDomain *dom;
  Polyhedron *P = dom1->polyhedron;

  /* DomainSimplify doesn't remove all redundant equalities,
   * so we remove them here first in case both dom1 and dom2
   * are single polyhedra (i.e., not unions of polyhedra).
   */
  if (!dom1->polyhedron->next && !dom2->polyhedron->next &&
      P->NbEq && dom2->polyhedron->NbEq) {
    int i, row;
    int rows = P->NbEq + dom2->polyhedron->NbEq;
    int cols = P->Dimension+2;
    int rank;
    M = cloog_matrix_alloc(rows, cols);
    M2 = cloog_matrix_alloc(P->NbConstraints, cols);
    Vector_Copy(dom2->polyhedron->Constraint[0], M->p[0], 
		dom2->polyhedron->NbEq * cols);
    rank = dom2->polyhedron->NbEq;
    row = 0;
    for (i = 0; i < P->NbEq; ++i) {
      Vector_Copy(P->Constraint[i], M->p[rank], cols);
      if (Gauss(M, rank+1, cols-1) > rank) {
	Vector_Copy(P->Constraint[i], M2->p[row++], cols);
	rank++;
      }
    }
    if (row < P->NbEq) {
      Vector_Copy(P->Constraint[P->NbEq], M2->p[row], 
		  (P->NbConstraints - P->NbEq) * cols);
      P = Constraints2Polyhedron(M2, MAX_RAYS);
    }
    cloog_matrix_free(M2);
    cloog_matrix_free(M);
  }
  dom = cloog_domain_alloc(DomainSimplify(P, dom2->polyhedron,MAX_RAYS));
  if (P != dom1->polyhedron)
    Polyhedron_Free(P);
  return dom;
}


/**
 * cloog_domain_union function:
 * This function returns a new CloogDomain structure including a polyhedral
 * domain which is the union of two polyhedral domains (pol1) U (pol2) inside
 * two CloogDomain structures.
 */
CloogDomain * cloog_domain_union(CloogDomain * dom1, CloogDomain * dom2)
{ return (cloog_domain_alloc(DomainUnion(dom1->polyhedron,
                                         dom2->polyhedron,MAX_RAYS))) ;
}


/**
 * cloog_domain_disjoint function:
 * This function returns a new CloogDomain structure including a polyhedral
 * domain represented using union of *disjoint* polyhedra (no intersection
 * between the different union components).
 */
CloogDomain * cloog_domain_disjoint(CloogDomain * dom)
{ return (cloog_domain_alloc(Disjoint_Domain(dom->polyhedron,0,MAX_RAYS))) ;
}


/**
 * cloog_domain_intersection function:
 * This function returns a new CloogDomain structure including a polyhedral
 * domain which is the intersection of two polyhedral domains (pol1)inter(pol2)
 * inside two CloogDomain structures.
 */ 
CloogDomain * cloog_domain_intersection(CloogDomain * dom1, CloogDomain * dom2)
{ return (cloog_domain_alloc(DomainIntersection(dom1->polyhedron,
                                                dom2->polyhedron,MAX_RAYS))) ;
}


/**
 * cloog_domain_difference function:
 * This function returns a new CloogDomain structure including a polyhedral
 * domain which is the difference of two polyhedral domains domain \ minus
 * inside two CloogDomain structures.
 * - November 8th 2001: first version.
 */ 
CloogDomain * cloog_domain_difference(CloogDomain * domain, CloogDomain * minus)
{ if (cloog_domain_isempty(minus))
  return(cloog_domain_copy(domain)) ;
  else
  return (cloog_domain_alloc(DomainDifference(domain->polyhedron,
                                              minus->polyhedron,MAX_RAYS))) ;
}


/**
 * cloog_domain_includes function:
 * This function returns 1 if the polyhedral domain inside 'container' includes
 * the polyhedral domain inside 'contents', 0 otherwise.
 * - September 14th 2002: first version. 
 */
int cloog_domain_includes(CloogDomain * container, CloogDomain * contents)
{ int is_in ;
  Polyhedron * p1, * p2 ;
  
  for (p1=container->polyhedron; p1; p1=p1->next)
  { is_in = 0 ;
    
    for (p2=contents->polyhedron; p2; p2=p2->next)
    if (PolyhedronIncludes(p1,p2))
    { is_in = 1 ;
      break ;
    }
    
    if (!is_in)
    return 0 ;
  }
  
  return 1 ;
}


/**
 * cloog_domain_addconstraints function :
 * This function adds source's polyhedron constraints to target polyhedron: for
 * each element of the polyhedron inside 'target' (i.e. element of the union
 * of polyhedra) it adds the constraints of the corresponding element in
 * 'source'.
 * - August 10th 2002: first version.
 * Nota bene for future : it is possible that source and target don't have the
 * same number of elements (try iftest2 without non-shared constraint
 * elimination in cloog_loop_separate !). This function is yet another part
 * of the DomainSimplify patching problem...
 */
CloogDomain * cloog_domain_addconstraints(domain_source, domain_target)
CloogDomain * domain_source, * domain_target ;
{ unsigned nb_constraint ;
  Value * constraints ;
  Polyhedron * source, * target, * new, * next, * last ;

  source = domain_source->polyhedron ;
  target = domain_target->polyhedron ;
  
  constraints = source->p_Init ;
  nb_constraint = source->NbConstraints ;
  source = source->next ;
  new = AddConstraints(constraints,nb_constraint,target,MAX_RAYS) ;
  last = new ;
  next = target->next ;

  while (next != NULL)
  { /* BUG !!! This is actually a bug. I don't know yet how to cleanly avoid
     * the situation where source and target do not have the same number of
     * elements. So this 'if' is an awful trick, waiting for better.
     */
    if (source != NULL)
    { constraints = source->p_Init ;
      nb_constraint = source->NbConstraints ;
      source = source->next ;
    }
    last->next = AddConstraints(constraints,nb_constraint,next,MAX_RAYS) ;
    last = last->next ;
    next = next->next ;
  }

  return (cloog_domain_alloc(new)) ;
}


/**
 * cloog_domain_sort function:
 * This function topologically sorts (nb_pols) polyhedra. Here (pols) is a an
 * array of pointers to polyhedra, (nb_pols) is the number of polyhedra,
 * (level) is the level to consider for partial ordering (nb_par) is the
 * parameter space dimension, (permut) if not NULL, is an array of (nb_pols)
 * integers that contains a permutation specification after call in order to
 * apply the topological sorting. 
 */
void cloog_domain_sort(pols, nb_pols, level, nb_par, permut)
Polyhedron ** pols ;
unsigned nb_pols, level, nb_par ;
int * permut ;
{ int * time ;
  
  /* time is an array of (nb_pols) integers to store logical time values. We
   * do not use it, but it is compulsory for PolyhedronTSort.
   */
  time = (int *)malloc(nb_pols*sizeof(int)) ;

  /* PolyhedronTSort will fill up permut (and time). */
  PolyhedronTSort(pols,nb_pols,level,nb_par,time,permut,MAX_RAYS) ;
    
  free(time) ;
}


/**
 * cloog_domain_empty function:
 * This function allocates the memory space for a CloogDomain structure and
 * sets its polyhedron to an empty polyhedron with 'dimension' dimensions.
 * Then it returns a pointer to the allocated space.
 * - June 10th 2005: first version.
 */
CloogDomain * cloog_domain_empty(int dimension)
{ return (cloog_domain_alloc(Empty_Polyhedron(dimension))) ;
}


/******************************************************************************
 *                          Structure display function                        *
 ******************************************************************************/


/**
 * cloog_domain_print_structure :
 * this function is a more human-friendly way to display the CloogDomain data
 * structure, it only shows the constraint system and includes an indentation
 * level (level) in order to work with others print_structure functions.
 * Written by Olivier Chorier, Luc Marchaud, Pierre Martin and Romain Tartiere.
 * - April 24th 2005: Initial version.
 * - May   26th 2005: Memory leak hunt.
 * - June  16th 2005: (Ced) Integration in domain.c.
 */
void cloog_domain_print_structure(FILE * file, CloogDomain * domain, int level)
{ int i ;
  CloogMatrix * matrix ;

  /* Go to the right level. */
  for(i=0; i<level; i++)
  fprintf(file,"|\t") ;
  
  if (domain != NULL)
  { fprintf(file,"+-- CloogDomain\n") ;
  
    /* Print the matrix. */
    matrix = cloog_domain_domain2matrix(domain) ;
    cloog_matrix_print_structure(file,matrix,level) ;
    cloog_matrix_free(matrix) ;

    /* A blank line. */
    for (i=0; i<level+1; i++)
    fprintf(file,"|\t") ;   
    fprintf(file,"\n") ;    
  }
  else
  fprintf(file,"+-- Null CloogDomain\n") ;

}


/**
 * cloog_domain_list_print function:
 * This function prints the content of a CloogDomainList structure into a
 * file (foo, possibly stdout).
 * - November 6th 2001: first version.
 */
void cloog_domain_list_print(FILE * foo, CloogDomainList * list)
{ while (list != NULL)
  { cloog_domain_print(foo,list->domain) ;
    list = list->next ;
  }
}


/******************************************************************************
 *                         Memory deallocation function                       *
 ******************************************************************************/


/**
 * cloog_domain_list_free function:
 * This function frees the allocated memory for a CloogDomainList structure.
 * - November 6th 2001: first version.
 */
void cloog_domain_list_free(CloogDomainList * list)
{ CloogDomainList * temp ;
  
  while (list != NULL)
  { temp = list->next ;
    cloog_domain_free(list->domain) ;
    free(list) ;
    list = temp ;
  }
}


/******************************************************************************
 *                               Reading function                             *
 ******************************************************************************/


/**
 * cloog_domain_read function:
 * Adaptation from the PolyLib. This function reads a matrix into a file (foo,
 * posibly stdin) and returns a pointer to a polyhedron containing the read
 * information. 
 * - October 18th 2001: first version.
 */
CloogDomain * cloog_domain_read(FILE * foo)
{ CloogMatrix * matrix ;
  CloogDomain * domain ;
  
  matrix = cloog_matrix_read(foo) ;
  domain = cloog_domain_matrix2domain(matrix) ;
  cloog_matrix_free(matrix) ;
  
  return(domain) ;
}


/**
 * cloog_domain_union_read function:
 * This function reads a union of polyhedra into a file (foo, posibly stdin) and
 * returns a pointer to a Polyhedron containing the read information. 
 * - September 9th 2002: first version.
 * - October  29th 2005: (debug) removal of a leak counting "correction" that
 *                       was just false since ages.
 */
CloogDomain * cloog_domain_union_read(FILE * foo)
{ int i, nb_components ;
  char s[MAX_STRING] ;
  CloogDomain * domain, * temp, * old ;

  /* domain reading: nb_components (constraint matrices). */
  while (fgets(s,MAX_STRING,foo) == 0) ;
  while ((*s=='#' || *s=='\n') || (sscanf(s," %d",&nb_components)<1))
  fgets(s,MAX_STRING,foo) ;
  
  if (nb_components > 0)
  { /* 1. first part of the polyhedra union, */
    domain = cloog_domain_read(foo) ;
    /* 2. and the nexts. */
    for (i=1;i<nb_components;i++)
    { /* Leak counting is OK since next allocated domain is freed here. */
      temp = cloog_domain_read(foo) ;
      old = domain ;
      domain = cloog_domain_union(temp,old) ;
      cloog_domain_free(temp) ;
      cloog_domain_free(old) ;
    }    
    return domain ;
  }
  else
  return NULL ;
}


/**
 * cloog_domain_list_read function:
 * This function reads a list of polyhedra into a file (foo, posibly stdin) and
 * returns a pointer to a CloogDomainList containing the read information. 
 * - November 6th 2001: first version.
 */
CloogDomainList * cloog_domain_list_read(FILE * foo)
{ int i, nb_pols ;
  char s[MAX_STRING] ;
  CloogDomainList * list, * now, * next ;
  
  
  /* We read first the number of polyhedra in the list. */
  while (fgets(s,MAX_STRING,foo) == 0) ;
  while ((*s=='#' || *s=='\n') || (sscanf(s," %d",&nb_pols)<1))
  fgets(s,MAX_STRING,foo) ;
  
  /* Then we read the polyhedra. */
  list = NULL ;
  if (nb_pols > 0)
  { list = (CloogDomainList *)malloc(sizeof(CloogDomainList)) ;
    list->domain = cloog_domain_read(foo) ;
    list->next = NULL ;
    now = list ;
    for (i=1;i<nb_pols;i++)
    { next = (CloogDomainList *)malloc(sizeof(CloogDomainList)) ;
      next->domain = cloog_domain_read(foo) ;
      next->next = NULL ;
      now->next = next ;
      now = now->next ;
    }
  }
  return(list) ;
}


/******************************************************************************
 *                            Processing functions                            *
 ******************************************************************************/


/**
 * cloog_domain_malloc function:
 * This function allocates the memory space for a CloogDomain structure and
 * sets its fields with default values. Then it returns a pointer to the
 * allocated space.
 * - November 21th 2005: first version.
 */
CloogDomain * cloog_domain_malloc()
{ CloogDomain * domain ;
  
  domain = (CloogDomain *)malloc(sizeof(CloogDomain)) ;
  if (domain == NULL) 
  { fprintf(stderr, "[CLooG]ERROR: memory overflow.\n") ;
    exit(1) ;
  }
  cloog_domain_leak_up() ;
  
  /* We set the various fields with default values. */
  domain->polyhedron = NULL ;
  domain->references = 1 ;
  
  return domain ;
}


/**
 * cloog_domain_alloc function:
 * This function allocates the memory space for a CloogDomain structure and
 * sets its fields with those given as input. Then it returns a pointer to the
 * allocated space.
 * - April    19th 2005: first version.
 * - November 21th 2005: cloog_domain_malloc use.
 */
CloogDomain * cloog_domain_alloc(Polyhedron * polyhedron)
{ CloogDomain * domain ;
  
  if (polyhedron == NULL)
  return NULL ;
  else
  { domain = cloog_domain_malloc() ;
    domain->polyhedron = polyhedron ;
    
    return domain ;
  }
}


/**
 * cloog_domain_isempty function:
 * This function returns 1 if the polyhedron given as input is empty, 0
 * otherwise.
 * - October 28th 2001: first version.
 */ 
int cloog_domain_isempty(CloogDomain * domain)
{ if (domain->polyhedron == NULL)
  return 1 ;

  if (domain->polyhedron->next)
  return(0) ;
  return((domain->polyhedron->Dimension < domain->polyhedron->NbEq) ? 1 : 0) ;
}


/**
 * cloog_domain_universe function:
 * This function returns 1 if the polyhedron given as input describe the
 * universe of its dimension, 0 otherwise. Nb: the NbBid field of a polyhedron
 * gives the number of bidirectionnal rays.
 * - November 19th 2001: first version.
 */
int cloog_domain_universe(CloogDomain * domain)
{ if (domain->polyhedron->next)
  return(0) ;
  return((domain->polyhedron->Dimension == domain->polyhedron->NbBid) ? 1 : 0) ;
}


/**
 * cloog_domain_project function:
 * From Quillere's LoopGen 0.4. This function returns the projection of
 * (domain) on the (level) first dimensions (i.e. outer loops). It returns a
 * pointer to the projected Polyhedron.
 * - nb_par is the number of parameters.
 **
 * - October 27th 2001: first version.
 * - June 21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                   CLooG 0.12.1).
 */ 
CloogDomain * cloog_domain_project(CloogDomain * domain, int level, int nb_par)
{ int row, column, nb_rows, nb_columns, difference ;
  CloogDomain * projected_domain ;
  CloogMatrix * matrix ;

  nb_rows = level + nb_par + 1 ;
  nb_columns = domain->polyhedron->Dimension + 1 ;
  difference = nb_columns - nb_rows ;
  
  if (difference == 0)
  return(cloog_domain_copy(domain)) ;
  
  matrix = cloog_matrix_alloc(nb_rows,nb_columns) ;
     
  for (row=0;row<level;row++)
  for (column=0;column<nb_columns; column++)
  value_set_si(matrix->p[row][column],(row == column ? 1 : 0)) ;

  for (;row<nb_rows;row++)
  for (column=0;column<nb_columns;column++)
  value_set_si(matrix->p[row][column],(row+difference == column ? 1 : 0)) ;
  
  projected_domain = cloog_domain_image(domain,matrix) ;
  cloog_matrix_free(matrix) ;

  return(projected_domain) ;
}  


/**
 * cloog_domain_bounds:
 * Given a list (union) of polyhedra "domain", this function returns a single
 * polyhedron with constraints that reflect the (parametric) lower and
 * upper bound on dimension "dim".
 *
 * nb_par is the number of parameters of the domain.
 */
CloogDomain * cloog_domain_bounds(CloogDomain * domain, int dim, int nb_par)
{
  int row, nb_rows, nb_columns, difference;
  CloogDomain * projected_domain, *extended_domain, *bounds;
  CloogMatrix * matrix ;

  nb_rows = 1 + nb_par + 1;
  nb_columns = domain->polyhedron->Dimension + 1 ;
  difference = nb_columns - nb_rows ;
  
  if (difference == 0)
    return(cloog_domain_convex(domain));
  
  matrix = cloog_matrix_alloc(nb_rows, nb_columns);
     
  value_set_si(matrix->p[0][dim], 1);
  for (row = 1; row < nb_rows; row++)
    value_set_si(matrix->p[row][row+difference], 1);
  
  projected_domain = cloog_domain_image(domain,matrix) ;
  extended_domain = cloog_domain_preimage(projected_domain, matrix);
  cloog_domain_free(projected_domain);
  cloog_matrix_free(matrix) ;
  bounds = cloog_domain_convex(extended_domain);
  cloog_domain_free(extended_domain);

  return bounds;
}  


/**
 * cloog_domain_extend function:
 * From Quillere's LoopGen 0.4. This function returns the (domain) given as
 * input with (dim)+(nb_par) dimensions. The new dimensions are added before
 * the (nb_par) parameters. This function does not free (domain), and returns
 * a new polyhedron.
 * - nb_par is the number of parameters.
 **
 * - October 27th 2001: first version.
 * - June 21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                 CLooG 0.12.1).
 */ 
CloogDomain * cloog_domain_extend(CloogDomain * domain, int dim, int nb_par)
{ int row, column, nb_rows, nb_columns, difference ;
  CloogDomain * extended_domain ;
  CloogMatrix * matrix ;
  
  nb_rows = 1 + domain->polyhedron->Dimension ;
  nb_columns = dim + nb_par + 1 ;
  difference = nb_columns - nb_rows ;
  
  if (difference == 0)
  return(cloog_domain_copy(domain)) ;
  
  matrix = cloog_matrix_alloc(nb_rows,nb_columns) ;
    
  for (row=0;row<domain->polyhedron->Dimension-nb_par;row++)
  for (column=0;column<nb_columns;column++)
  value_set_si(matrix->p[row][column],(row == column ? 1 : 0)) ;
  
  for (;row<=domain->polyhedron->Dimension;row++)
  for (column=0;column<nb_columns;column++)
  value_set_si(matrix->p[row][column],(row+difference == column ? 1 : 0)) ;
  
  extended_domain = cloog_domain_preimage(domain,matrix) ;
  cloog_matrix_free(matrix) ;

  return(extended_domain) ;
}


/**
 * cloog_domain_never_integral function:
 * For us, an equality like 3*i -4 = 0 is always false since 4%3 != 0. This
 * function returns a boolean set to 1 if there is this kind of 'never true'
 * constraint inside a polyhedron, 0 otherwise.
 * - domain is the polyhedron to check,
 **
 * - November 28th 2001: first version. 
 * - June 26th 2003: for iterators, more 'never true' constraints are found
 *                   (compare cholesky2 and vivien with a previous version),
 *                   checking for the parameters created (compare using vivien).
 * - June 28th 2003: Previously in loop.c and called
 *                   cloog_loop_simplify_nevertrue, now here !
 * - June 21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                   CLooG 0.12.1).
 * - October 14th 2005: Complete rewriting, not faster but code quite shorter.
 */
int cloog_domain_never_integral(CloogDomain * domain)
{ int i, dimension ;
  Value gcd, modulo ;
  Polyhedron * polyhedron ;

  if ((domain == NULL) || (domain->polyhedron == NULL))
  return 1 ;
  
  value_init_c(gcd) ;
  value_init_c(modulo) ;
  polyhedron = domain->polyhedron ;
  dimension = polyhedron->Dimension + 2  ;
  
  /* For each constraint... */
  for (i=0; i<polyhedron->NbConstraints; i++)  
  { /* If we have an equality and the scalar part is not zero... */
    if (value_zero_p(polyhedron->Constraint[i][0]) && 
        value_notzero_p(polyhedron->Constraint[i][dimension-1]))
    { /* Then we check whether the scalar can be divided by the gcd of the
       * unknown vector (including iterators and parameters) or not. If not,
       * there is no integer point in the polyhedron and we return 1.
       */
      Vector_Gcd(&(polyhedron->Constraint[i][1]),dimension-2,&gcd) ;
      value_modulus(modulo,polyhedron->Constraint[i][dimension-1],gcd) ;
      
      if (value_notzero_p(modulo))
      { value_clear_c(gcd) ;
        value_clear_c(modulo) ;
        return 1 ;
      }
    }
  }
  
  value_clear_c(gcd) ;
  value_clear_c(modulo) ;
  return(0) ;
}


/**
 * cloog_domain_stride function:
 * This function finds the stride imposed to unknown with the column number
 * 'strided_level' in order to be integral. For instance, if we have a
 * constraint like -i - 2j + 2k = 0, and we consider k, then k can be integral
 * only if (i + 2j)%2 = 0. Then only if i%2 = 0. Then k imposes a stride 2 to
 * the unknown i. The function returns the imposed stride in a parameter field.
 * - domain is the set of constraint we have to consider,
 * - strided_level is the column number of the unknown for which a stride have
 *   to be found,
 * - looking_level is the column number of the unknown that impose a stride to
 *   the first unknown.
 * - stride is the stride that is returned back as a function parameter. 
 * - offset is the value of the constant c if the condition is of the shape
 *   (i + c)%s = 0, s being the stride.
 **
 * - June 28th 2003: first version.
 * - July 14th 2003: can now look for multiple striding constraints and returns
 *                   the GCD of the strides and the common offset.
 * - June 21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                   CLooG 0.12.1).
 */
void cloog_domain_stride(domain, strided_level, nb_par, stride, offset)
CloogDomain * domain ;
int strided_level, nb_par ;
Value  * stride, * offset;
{ int i, dimension;
  Polyhedron * polyhedron ;
  int n_col, n_row, rank;
  CloogMatrix *M;
  Matrix *U;
  Vector *V;

  polyhedron = domain->polyhedron ;
  dimension = polyhedron->Dimension ;

  /* Look at all equalities involving strided_level and the inner
   * iterators.  We can ignore the outer iterators and the parameters
   * here because the lower bound on strided_level is assumed to
   * be a constant.
   */
  n_col = (1+dimension-nb_par) - strided_level;
  for (i=0, n_row=0; i < polyhedron->NbEq; i++)
    if (First_Non_Zero(polyhedron->Constraint[i]+strided_level, n_col) != -1)
      ++n_row;

  M = cloog_matrix_alloc(n_row+1, n_col+1);
  for (i=0, n_row = 0; i < polyhedron->NbEq; i++) {
    if (First_Non_Zero(polyhedron->Constraint[i]+strided_level, n_col) == -1)
      continue;
    Vector_Copy(polyhedron->Constraint[i]+strided_level, M->p[n_row], n_col);
    value_assign(M->p[n_row][n_col], polyhedron->Constraint[i][1+dimension]);
    ++n_row;
  }
  value_set_si(M->p[n_row][n_col], 1);

  /* Then look at the general solution to the above equalities. */
  rank = SolveDiophantine(M, &U, &V);
  cloog_matrix_free(M);

  if (rank == -1) {
    /* There is no solution, so the body of this loop will
     * never execute.  We just leave the constraints alone here so
     * that they will ensure the body will not be executed.
     * We should probably propagate this information up so that
     * the loop can be removed entirely.
     */ 
    value_set_si(*offset, 0);
    value_set_si(*stride, 1);
  } else {
    /* Compute the gcd of the coefficients defining strided_level. */
    Vector_Gcd(U->p[0], U->NbColumns, stride);
    value_oppose(*offset, V->p[0]);
    value_pmodulus(*offset, *offset, *stride);
  }
  Matrix_Free(U);
  Vector_Free(V);

  return ;
}


/**
 * cloog_domain_integral_lowerbound function:
 * This function returns 1 if the lower bound of an iterator (such as its
 * column rank in the constraint set 'domain' is 'level') is integral,
 * 0 otherwise. If the lower bound is actually integral, the function fills
 * the 'lower' field with the lower bound value.
 * - June 29th 2003: first version.
 * - June 21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                   CLooG 0.12.1).
 */
int cloog_domain_integral_lowerbound(domain, level, lower)
CloogDomain * domain ;
int level ;
Value * lower ;
{ int i, first_lower=1, dimension, lower_constraint=-1 ;
  Value iterator, constant, tmp;
  Polyhedron * polyhedron ;
 
  polyhedron = domain->polyhedron ;
  dimension = polyhedron->Dimension ;
  
  /* We want one and only one lower bound (e.g. no equality, no maximum
   * calculation...).
   */
  for (i=0; i<polyhedron->NbConstraints; i++)  
  if (value_zero_p(polyhedron->Constraint[i][0]) && 
      value_notzero_p(polyhedron->Constraint[i][level]))
  return 0 ;

  for (i=0; i<polyhedron->NbConstraints; i++)  
  if (value_pos_p(polyhedron->Constraint[i][level]))
  { if (first_lower)
    { first_lower = 0 ;
      lower_constraint = i ;
    }
    else
    return 0 ;
  }
  if (first_lower)
  return 0 ;
  
  /* We want an integral lower bound: no other non-zero entry except the
   * iterator coefficient and the constant.
   */
  for (i=1; i<level; i++)  
  if (value_notzero_p(polyhedron->Constraint[lower_constraint][i]))
  return 0 ;
  for (i=level+1; i<=polyhedron->Dimension; i++)  
  if (value_notzero_p(polyhedron->Constraint[lower_constraint][i]))
  return 0 ;

  value_init_c(iterator) ;
  value_init_c(constant) ;
  value_init_c(tmp) ;

  /* If all is passed, then find the lower bound and return 1. */
  value_assign(iterator, polyhedron->Constraint[lower_constraint][level]) ;
  value_oppose(constant, polyhedron->Constraint[lower_constraint][dimension+1]);

  value_modulus(tmp, constant, iterator) ;
  value_division(*lower, constant, iterator) ;

  if (!(value_zero_p(tmp) || value_neg_p(constant)))
  value_increment(*lower, *lower) ;
  
  value_clear_c(iterator) ;
  value_clear_c(constant) ;
  value_clear_c(tmp) ;

  return 1 ;
}


/**
 * cloog_domain_lowerbound_update function:
 * This function updates the integral lower bound of an iterator (such as its
 * column rank in the constraint set 'domain' is 'level') into  'lower'.
 * - Jun  29th 2003: first version.
 * - June 21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                   CLooG 0.12.1).
 */
void cloog_domain_lowerbound_update(domain, level, lower)
CloogDomain * domain ;
int level ;
Value lower ;
{ int i ;
  Polyhedron * polyhedron ;
 
  polyhedron = domain->polyhedron ;
 
  /* There is only one lower bound, the first one is the good one. */
  for (i=0; i<polyhedron->NbConstraints; i++)  
  if (value_pos_p(polyhedron->Constraint[i][level]))
  { value_set_si(polyhedron->Constraint[i][level], 1) ;
    value_oppose(polyhedron->Constraint[i][polyhedron->Dimension+1], lower) ;
    break ;
  }
}


/**
 * cloog_domain_lazy_equal function:
 * This function returns 1 if the domains given as input are the same, 0 if it
 * is unable to decide. This function makes an entry-to-entry comparison between
 * the constraint systems, if all the entries are the same, the domains are
 * obviously the same and it returns 1, at the first difference, it returns 0.
 * This is a very fast way to verify this property. It has been shown (with the
 * CLooG benchmarks) that operations on equal domains are 17% of all the
 * polyhedral computations. For 75% of the actually identical domains, this
 * function answer that they are the same and allow to give immediately the
 * trivial solution instead of calling the heavy general functions of PolyLib.
 * - August 22th 2003: first version.
 * - June 21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                   CLooG 0.12.1).
 */
int cloog_domain_lazy_equal(CloogDomain * d1, CloogDomain * d2)
{ int i, nb_elements ;
  Polyhedron * p1, * p2 ;
 
  p1 = d1->polyhedron ;
  p2 = d2->polyhedron ;

  while ((p1 != NULL) && (p2 != NULL))
  { if ((p1->NbConstraints != p2->NbConstraints) ||
        (p1->Dimension != p2->Dimension))
    return 0 ;
  
    nb_elements = p1->NbConstraints * (p1->Dimension + 2) ;
  
    for (i=0;i<nb_elements;i++)
    if (value_ne(p1->p_Init[i], p2->p_Init[i]))
    return 0 ;
    
    p1 = p1->next ;
    p2 = p2->next ;
  }
  
  if ((p1 != NULL) || (p2 != NULL))
  return 0 ;
  
  return 1 ;
}


/**
 * cloog_domain_lazy_block function:
 * This function returns 1 if the two domains d1 and d2 given as input are the
 * same (possibly except for a dimension equal to a constant where we accept
 * a difference of 1) AND if we are sure that there are no other domain in
 * the code generation problem that may put integral points between those of
 * d1 and d2 (0 otherwise). In fact this function answers the question "can I
 * safely consider the two domains as only one with two statements (a block) ?".
 * This function is lazy: it asks for very standard scattering representation
 * (only one constraint per dimension which is an equality, and the constraints
 * are ordered per dimension depth: the left hand side of the constraint matrix
 * is the identity) and will answer NO at the very first problem.
 * - d1 and d2 are the two domains to check for blocking,
 * - scattering is the linked list of all domains,
 * - scattdims is the total number of scattering dimentions.
 **
 * - April   30th 2005: beginning
 * - June     9th 2005: first working version.
 * - June    10th 2005: debugging.
 * - June    21rd 2005: Adaptation for GMP.
 * - October 16th 2005: (debug) some false blocks have been removed.
 */
int cloog_domain_lazy_block(d1, d2, scattering, scattdims)
CloogDomain * d1, * d2 ;
CloogDomainList * scattering ;
int scattdims ;
{ int i, j, difference=0, different_constraint=0 ;
  Value date1, date2, date3, temp ;
  Polyhedron * p1, * p2, * p3 ;
   
  p1 = d1->polyhedron ;
  p2 = d2->polyhedron ;
  
  /* Some basic checks: we only accept convex domains, with same constraint
   * and dimension numbers.
   */
  if ((p1->next != NULL) || (p2->next != NULL) ||
      (p1->NbConstraints != p2->NbConstraints) ||
      (p1->Dimension != p2->Dimension))
  return 0 ;
    
  /* There should be only one difference between the two domains, it
   * has to be at the constant level and the difference must be of +1,
   * moreover, after the difference all domain coefficient has to be 0.
   * The matrix shape is:
   *
   * |===========|=====|<- 0 line
   * |===========|=====|
   * |===========|====?|<- different_constraint line (found here)
   * |===========|0000=|
   * |===========|0000=|<- pX->NbConstraints line
   *  ^         ^     ^
   *  |         |     |
   *  |         |     (pX->Dimension + 2) column
   *  |         scattdims column
   *  0 column
   */

  value_init_c(temp) ;
  for (i=0;i<p1->NbConstraints;i++)
  { if (difference == 0)
    { /* All elements except scalar must be equal. */
      for (j=0;j<(p1->Dimension + 1);j++)
      if (value_ne(p1->Constraint[i][j],p2->Constraint[i][j]))
      { value_clear_c(temp) ;
        return 0 ;
      }
      /* The scalar may differ from +1 (now j=(p1->Dimension + 1)). */ 
      if (value_ne(p1->Constraint[i][j],p2->Constraint[i][j]))
      { value_increment(temp,p2->Constraint[i][j]) ;
        if (value_ne(p1->Constraint[i][j],temp))
        { value_clear_c(temp) ;
          return 0 ;
        }
        else
        { difference = 1 ;
          different_constraint = i ;
        }
      }
    }
    else
    { /* Scattering coefficients must be equal. */
      for (j=0;j<(scattdims+1);j++)
      if (value_ne(p1->Constraint[i][j],p2->Constraint[i][j]))
      { value_clear_c(temp) ;
        return 0 ;
      }
      
      /* Domain coefficients must be 0. */
      for (;j<(p1->Dimension + 1);j++)
      if (value_notzero_p(p1->Constraint[i][j]) ||
          value_notzero_p(p2->Constraint[i][j]))
      { value_clear_c(temp) ;
        return 0 ;
      }
      
      /* Scalar must be equal. */
      if (value_ne(p1->Constraint[i][j],p2->Constraint[i][j]))
      { value_clear_c(temp) ;
        return 0 ;
      }
    }
  }
  value_clear_c(temp) ;

  /* If the domains are exactly the same, this is a block. */
  if (difference == 0)
  return 1 ;
   
  /* Now a basic check that the constraint with the difference is an
   * equality of a dimension with a constant.
   */
  for (i=0;i<=different_constraint;i++)
  if (value_notzero_p(p1->Constraint[different_constraint][i]))
  return 0 ;
  
  if (value_notone_p(p1->Constraint[different_constraint]
                                   [different_constraint+1]))
  return 0 ;
  
  for (i=different_constraint+2;i<(p1->Dimension + 1);i++)
  if (value_notzero_p(p1->Constraint[different_constraint][i]))
  return 0 ;
  
  /* For the moment, d1 and d2 are a block candidate. There remains to check
   * that there is no other domain that may put an integral point between
   * them. In our lazy test we ensure this property by verifying that the
   * constraint matrices have a very strict shape: let us consider that the
   * dimension with the difference is d. Then the first d dimensions are
   * defined in their depth order using equalities (thus the first column begins
   * with d zeroes, there is a d*d identity matrix and a zero-matrix for
   * the remaining simensions). If a domain can put integral points between the
   * domains of the block candidate, this means that the other entries on the
   * first d constraints are equal to those of d1 or d2. Thus we are looking for
   * such a constraint system, if it exists d1 and d2 is considered to not be
   * a block, it is a bock otherwise.
   *
   *  1. Only equalities (for the first different_constraint+1 lines).
   *  |  2. Must be the identity.
   *  |  |    3. Must be zero.
   *  |  |    |     4. Elements are equal, the last one is either date1 or 2.
   *  |  |    |     |
   *  | /-\ /---\ /---\
   * |0|100|00000|=====|<- 0 line
   * |0|010|00000|=====|
   * |0|001|00000|====?|<- different_constraint line
   * |*|***|*****|*****|
   * |*|***|*****|*****|<- pX->NbConstraints line
   *  ^   ^     ^     ^
   *  |   |     |     |
   *  |   |     |     (pX->Dimension + 2) column
   *  |   |     scattdims column
   *  |   different_constraint+1 column
   *  0 column
   */
  
  /* Step 1 and 2. This is only necessary to check one domain because
   * we checked that they are equal on this part before.
   */
  for (i=0;i<=different_constraint;i++)
  { for (j=0;j<i+1;j++)
    if (value_notzero_p(p1->Constraint[i][j]))
    return 0 ;
    
    if (value_notone_p(p1->Constraint[i][i+1]))
    return 0 ;
    
    for (j=i+2;j<=different_constraint+1;j++)
    if (value_notzero_p(p1->Constraint[i][j]))
    return 0 ;
  }
  
  /* Step 3. */
  for (i=0;i<=different_constraint;i++)
  for (j=different_constraint+2;j<=scattdims;j++)
  if (value_notzero_p(p1->Constraint[i][j]))
  return 0 ;
  
  value_init_c(date1) ;
  value_init_c(date2) ;
  value_init_c(date3) ;

  /* Now we have to check that the two different dates are unique. */
  value_assign(date1, p1->Constraint[different_constraint][p1->Dimension + 1]) ;
  value_assign(date2, p2->Constraint[different_constraint][p2->Dimension + 1]) ;

  /* Step 4. We check all domains except d1 and d2 and we look for at least
   * a difference with d1 or d2 on the first different_constraint+1 dimensions.
   */
  while (scattering != NULL)
  { if ((scattering->domain != d1) && (scattering->domain != d2))
    { p3 = scattering->domain->polyhedron ;
      value_assign(date3,p3->Constraint[different_constraint][p3->Dimension+1]);
      difference = 0 ;

      if (value_ne(date3,date2) && value_ne(date3,date1))
      difference = 1 ;
      
      for (i=0;(i<different_constraint)&&(!difference);i++)
      for (j=0;(j<(p3->Dimension + 2))&&(!difference);j++)
      if (value_ne(p1->Constraint[i][j],p3->Constraint[i][j]))
      difference = 1 ;

      for (j=0;(j<(p3->Dimension + 1))&&(!difference);j++)
      if (value_ne(p1->Constraint[different_constraint][j],
                   p3->Constraint[different_constraint][j]))
      difference = 1 ;
      
      if (!difference)
      { value_clear_c(date1) ;
        value_clear_c(date2) ;
        value_clear_c(date3) ;
        return 0 ;
      }
    }
  
    scattering = scattering->next ;
  }
   
  value_clear_c(date1) ;
  value_clear_c(date2) ;
  value_clear_c(date3) ;
  return 1 ;
}


/**
 * cloog_domain_lazy_disjoint function:
 * This function returns 1 if the domains given as input are disjoint, 0 if it
 * is unable to decide. This function finds the unknown with fixed values in
 * both domains (on a given constraint, their column entry is not zero and
 * only the constant coefficient can be different from zero) and verify that
 * their values are the same. If not, the domains are obviously disjoint and
 * it returns 1, if there is not such case it returns 0.  This is a very fast
 * way to verify this property. It has been shown (with the CLooG benchmarks)
 * that operations on disjoint domains are 36% of all the polyhedral
 * computations. For 94% of the actually identical domains, this
 * function answer that they are disjoint and allow to give immediately the
 * trivial solution instead of calling the heavy general functions of PolyLib.
 * - August 22th 2003: first version.
 * - June   21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                     CLooG 0.12.1).
 */
int cloog_domain_lazy_disjoint(CloogDomain * d1, CloogDomain * d2)
{ int i1, j1, i2, j2, scat_dim ;
  Value scat_val ;
  Polyhedron * p1, * p2 ;
 
  p1 = d1->polyhedron ;
  p2 = d2->polyhedron ;

  if ((p1->next != NULL) || (p2->next != NULL))
  return 0 ;
  
  value_init_c(scat_val) ;
  
  for (i1=0; i1<p1->NbConstraints; i1++)
  { if (value_notzero_p(p1->Constraint[i1][0]))
    continue ;
    
    scat_dim = 1 ;
    while (value_zero_p(p1->Constraint[i1][scat_dim]) &&
           (scat_dim < p1->Dimension))
    scat_dim ++ ;
    
    if (value_notone_p(p1->Constraint[i1][scat_dim]))
    continue ;
    else
    { for (j1=scat_dim+1; j1<=p1->Dimension; j1++)
      if (value_notzero_p(p1->Constraint[i1][j1]))
      break ;
      
      if (j1 != p1->Dimension+1)
      continue ;
      
      value_assign(scat_val,p1->Constraint[i1][p1->Dimension+1]) ;
            
      for (i2=0; i2<p2->NbConstraints; i2++)
      { for (j2=0;j2<scat_dim;j2++)
        if (value_notzero_p(p2->Constraint[i2][j2]))
        break ;
       
        if ((j2 != scat_dim) || value_notone_p(p2->Constraint[i2][scat_dim]))
        continue ;
       
        for (j2=scat_dim+1; j2<=p2->Dimension; j2++)
        if (value_notzero_p(p2->Constraint[i2][j2]))
        break ;
       
        if (j2 != p2->Dimension+1)
        continue ;
       
        if (value_ne(p2->Constraint[i2][p2->Dimension+1],scat_val)) 
	{ value_clear_c(scat_val) ;
	  return 1 ;
	}
      }
    }
  }

  value_clear_c(scat_val) ;
  return 0 ;
} 
 
 
/**
 * cloog_domain_list_lazy_same function:
 * This function returns 1 if two domains in the list are the same, 0 if it
 * is unable to decide.
 * - February 9th 2004: first version.
 */
int cloog_domain_list_lazy_same(CloogDomainList * list)
{ /*int i=1, j=1 ;*/
  CloogDomainList * current, * next ;

  current = list ;
  while (current != NULL)
  { next = current->next ;
    /*j=i+1;*/
    while (next != NULL)
    { if (cloog_domain_lazy_equal(current->domain,next->domain))
      { /*printf("Same domains: %d and %d\n",i,j) ;*/
        return 1 ;
      }
      /*j++ ;*/
      next = next->next ;
    }
    /*i++ ;*/
    current = current->next ;
  }
  
  return 0 ;
}


/**
 * cloog_domain_grow function:
 * This function extend the polyhedron (domain) onto the dimension (level) by a
 * step of 1, if (lower) is 1 then the lower bound of the dimension is the same
 * minus one, if (lower) is 0 then the upper bound of the dimension is the
 * same plus one. This function frees the Polyhedron structure given as input
 * and returns the extended one.
 * - March 27th 2004: first version.
 * - June  21rd 2005: Adaptation for GMP.
 */
CloogDomain * cloog_domain_grow(CloogDomain * domain, int level, int lower)
{ int i, scalar_dim ;
  CloogMatrix * matrix ;
  CloogDomain * grow ;  
  
  matrix = cloog_domain_domain2matrix(domain) ;
  cloog_domain_free(domain) ;
  scalar_dim = matrix->NbColumns - 1 ;

  for (i=0;i<matrix->NbRows;i++)
  if (value_one_p(matrix->p[i][0]))
  { if (((lower == 1) && value_pos_p(matrix->p[i][level])) ||
        ((lower == 0) && value_neg_p(matrix->p[i][level])))
    value_increment(matrix->p[i][scalar_dim],matrix->p[i][scalar_dim]) ;
  }
  
  grow = cloog_domain_matrix2domain(matrix) ;
  cloog_matrix_free(matrix) ;
  return grow ;
}


/**
 * Those functions are provided for "object encapsulation", to separate as much
 * as possible the inside of the CloogDomain structure from the rest of the
 * program, in order to ease the change of polyhedral library. For efficiency
 * reasons, they are defined and used as macros in domain.h.
 * - April 20th 2005: setting.
 *
Polyhedron * cloog_domain_polyhedron(CloogDomain * domain)
{ return domain->polyhedron ;
}

int cloog_domain_dimension(CloogDomain * domain)
{ return domain->polyhedron->Dimension ;
}

int cloog_domain_nbconstraints(CloogDomain * domain)
{ return domain->polyhedron->NbConstraints ;
}

int cloog_domain_isconvex(CloogDomain * domain)
{ return (domain->polyhedron->next == NULL)? 1 : 0 ;
}
 */


/**
 * cloog_domain_cut_first function:
 * this function returns a CloogDomain structure with everything except the
 * first part of the polyhedra union of the input domain as domain. After a call
 * to this function, there remains in the CloogDomain structure provided as
 * input only the first part of the original polyhedra union.
 * - April 20th 2005: first version, extracted from different part of loop.c.
 */
CloogDomain * cloog_domain_cut_first(CloogDomain * domain)
{ CloogDomain * rest ;
  
  if ((domain != NULL) && (domain->polyhedron != NULL))
  { rest = cloog_domain_alloc(domain->polyhedron->next) ;
    domain->polyhedron->next = NULL ;
  }
  else
  rest = NULL ;
  
  return rest ;
}


/**
 * cloog_domain_lazy_isscalar function:
 * this function returns 1 if the dimension 'dimension' in the domain 'domain'
 * is scalar, this means that the only constraint on this dimension must have
 * the shape "x.dimension + scalar = 0" with x an integral variable. This
 * function is lazy since we only accept x=1 (further calculations are easier
 * in this way).
 * - June 14th 2005: first version.
 * - June 21rd 2005: Adaptation for GMP.
 */
int cloog_domain_lazy_isscalar(CloogDomain * domain, int dimension)
{ int i, j ;
  Polyhedron * polyhedron ;
 
  polyhedron = domain->polyhedron ;
  /* For each constraint... */
  for (i=0;i<polyhedron->NbConstraints;i++)
  { /* ...if it is concerned by the potentially scalar dimension... */
    if (value_notzero_p(polyhedron->Constraint[i][dimension+1]))
    { /* ...check that the constraint has the shape "dimension + scalar = 0". */
      for (j=0;j<=dimension;j++)
      if (value_notzero_p(polyhedron->Constraint[i][j]))
      return 0 ;
  
      if (value_notone_p(polyhedron->Constraint[i][dimension+1]))
      return 0 ;
  
      for (j=dimension+2;j<(polyhedron->Dimension + 1);j++)
      if (value_notzero_p(polyhedron->Constraint[i][j]))
      return 0 ;
    }
  }
  
  return 1 ;
}


/**
 * cloog_domain_scalar function:
 * when we call this function, we know that "dimension" is a scalar dimension,
 * this function finds the scalar value in "domain" and returns it in "value".
 * - June 30th 2005: first version.
 */
void cloog_domain_scalar(CloogDomain * domain, int dimension, Value * value)
{ int i ;
  Polyhedron * polyhedron ;
 
  polyhedron = domain->polyhedron ;
  /* For each constraint... */
  for (i=0;i<polyhedron->NbConstraints;i++)
  { /* ...if it is the equality defining the scalar dimension... */
    if (value_notzero_p(polyhedron->Constraint[i][dimension+1]) &&
        value_zero_p(polyhedron->Constraint[i][0]))
    { /* ...Then send the scalar value. */
      value_assign(*value,polyhedron->Constraint[i][polyhedron->Dimension+1]) ;
      value_oppose(*value,*value) ;
      return ;
    }
  }
  
  /* We should have found a scalar value: if not, there is an error. */
  fprintf(stderr, "[CLooG]ERROR: dimension %d is not scalar as expected.\n",
          dimension) ;
  exit(0) ;
}


/**
 * cloog_domain_erase_dimension function:
 * this function returns a CloogDomain structure builds from 'domain' where
 * we removed the dimension 'dimension' and every constraint considering this
 * dimension. This is not a projection ! Every data concerning the
 * considered dimension is simply erased.
 * - June 14th 2005: first version.
 * - June 21rd 2005: Adaptation for GMP.
 */
CloogDomain * cloog_domain_erase_dimension(CloogDomain * domain, int dimension)
{ int i, j, mi, nb_dim ;
  CloogMatrix * matrix ;
  CloogDomain * erased ;
  Polyhedron * polyhedron ;
 
  polyhedron = domain->polyhedron ;
  nb_dim = polyhedron->Dimension ;
  
  /* The matrix is one column less and at least one constraint less. */
  matrix = cloog_matrix_alloc(polyhedron->NbConstraints-1,nb_dim+1) ;
 
  /* mi is the constraint counter for the matrix. */
  mi = 0 ;
  for (i=0;i<polyhedron->NbConstraints;i++)
  if (value_zero_p(polyhedron->Constraint[i][dimension+1]))
  { for (j=0;j<=dimension;j++)
    value_assign(matrix->p[mi][j],polyhedron->Constraint[i][j]) ;
    
    for (j=dimension+2;j<nb_dim+2;j++)
    value_assign(matrix->p[mi][j-1],polyhedron->Constraint[i][j]) ;

    mi ++ ;
  }
  
  erased = cloog_domain_matrix2domain(matrix) ;
  cloog_matrix_free(matrix) ;

  return erased ;
}


/**
 * To change the order of the part of a polyhedral union, for funny results !
 * - September 10th 2005.
 */
void cloog_domain_reverse(CloogDomain * domain)
{ Polyhedron * polyhedron, * p, * q ,* r ;
  
  polyhedron = domain->polyhedron ;
  
  if ((polyhedron == NULL)||(polyhedron->next == NULL)) 
  return ;
  
  q = polyhedron->next ;
  polyhedron->next = NULL ; 
  r = q->next ;
  q->next = polyhedron ;
  while (r != NULL)
  { p = q ;
    q = r ;
    r = r->next ;
    q->next  = p ;
  }
  domain->polyhedron = q ;
}
