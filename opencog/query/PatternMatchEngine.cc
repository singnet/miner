/*
 * PatternMatchEngine.cc
 *
 * Copyright (C) 2008,2009 Linas Vepstas
 *
 * Author: Linas Vepstas <linasvepstas@gmail.com>  February 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "PatternMatchEngine.h"

#include <opencog/util/platform.h>
#include <opencog/atomspace/Foreach.h>
#include <opencog/atomspace/ForeachTwo.h>
#include <opencog/atomspace/Link.h>
#include <opencog/atomspace/Node.h>
#include <opencog/atomspace/TLB.h>

using namespace opencog;

// #define DEBUG 1
#ifdef WIN32
#ifdef DEBUG
	#define dbgprt printf
#else
	// something better?
	#define dbgprt
#endif
#else
#ifdef DEBUG
	#define dbgprt(f, varargs...) printf(f, ##varargs)
#else
	#define dbgprt(f, varargs...) 
#endif
#endif

PatternMatchEngine::PatternMatchEngine(void)
{
	atom_space = NULL;
}

void PatternMatchEngine::set_atomspace(AtomSpace *as)
{
	atom_space = as;

	HandleSeq oset;
	invalid_grounding = as->addLink(LINK, oset);
}

bool PatternMatchEngine::prt(Atom *atom)
{
	if (!atom) return false;
	std::string str = atom->toString();
	printf ("%s\n", str.c_str());
	return false;
}

inline void PatternMatchEngine::prtmsg(const char * msg, Atom *atom)
{
#ifdef DEBUG
	if (!atom)
	{
		printf ("%s (null handle)\n", msg);
		return;
	}
	std::string str = atom->toString();
	printf ("%s %s\n", msg, str.c_str());
#endif
}

inline void PatternMatchEngine::prtmsg(const char * msg, Handle h)
{
#ifdef DEBUG
	prtmsg(msg, TLB::getAtom(h));
#endif
}

#define POPTOP(soln,stack) {         \
   stack.pop();                      \
   if (stack.empty()) soln.clear();  \
   else soln = stack.top();          \
}

/* ======================================================== */

/**
 * tree_compare compares two trees, side-by-side.
 *
 * Compare two incidence trees, side-by-side. It is assumed that the 
 * first of these is a clause in the predicate, and so the comparison
 * is between the clause, and a candidate graph. 
 *
 * The graph/tree refered to here is the incidence graph/tree (aka 
 * Levi graph) of the hypergraph (and not the hypergraph itself).
 * The incidence graph is given by the "outgoing set" of the atom.
 *
 * This routine is recursive, calling itself on each subtree of the 
 * predicate clause, performing comparisions until a match is found 
 * (or not found).
 *
 * Return true if there's a mis-match. The goal here is to walk over
 * the entire tree, without mismatches.  Since a return value of true
 * stops the iteration, true is used to signal a mismatch.
 */
bool PatternMatchEngine::tree_compare(Atom *aa, Atom *ab)
{
	Handle ha = TLB::getHandle(aa);
	Handle hb = TLB::getHandle(ab);

	// Atom aa is from the predicate, and it might be one
	// of the bound variables. If so, then declare a match.
	if (bound_vars.end() != bound_vars.find(ha))
	{
		// But... if atom b happens to also be a bound var,
		// then its a mismatch.
		if (bound_vars.end() != bound_vars.find(hb)) return true;

		// If we already have a grounding for this variable, the new
		// proposed grounding must match the existing one. Such multiple
		// groundings can occur when traversing graphs with loops in them.
		Handle gnd = var_grounding[ha];
		if (TLB::isValidHandle(gnd))
		{
			if (gnd != hb) return true;
			return false;
		}

		// Else, we have a candidate grounding for this variable.
		// Make a record of it.
		dbgprt("Found grounding of variable:\n");
		prtmsg("$$ variable:    ", ha);
		prtmsg("$$ ground term: ", hb);
		var_grounding[ha] = hb;
		return false;
	}

	// If they're the same atom, then clearly they match.
	// ... but only if ab is not a subclause of the current clause.
	if ((ha == hb) && (hb != curr_pred_handle))
	{
		var_grounding[ha] = hb;
		return false;
	}

	// If both are links, compare them as such.
	Link *la = dynamic_cast<Link *>(aa);
	Link *lb = dynamic_cast<Link *>(ab);
	if (la && lb)
	{
		// Let the callback perform basic checking.
		bool mismatch = pmc->link_match(la, lb);
		if (mismatch) return true;

		dbgprt("depth=%d\n", depth);
		prtmsg("> tree_compare", aa);
		prtmsg(">           to", ab);

		// The recursion step: traverse down the tree.
		// Only links can have non-empty outgoing sets.
		depth ++;
		var_solutn_stack.push(var_grounding);
		mismatch = foreach_outgoing_atom_pair(ha, hb,
		              	      &PatternMatchEngine::tree_compare, this);
		depth --;
		dbgprt("tree_comp down link mismatch=%d\n", mismatch);

		if (false == mismatch)
		{
			var_grounding[ha] = hb;
			var_solutn_stack.pop();  // pop entry created, but keep current.
		}
		else
			POPTOP(var_grounding, var_solutn_stack);
		return mismatch;
	}

	// If both are nodes, compare them as such.
	Node *na = dynamic_cast<Node *>(aa);
	Node *nb = dynamic_cast<Node *>(ab);
	if (na && nb)
	{
		// Call the callback to make the final determination.
		bool mismatch = pmc->node_match(na, nb);
		if (false == mismatch)
		{
			dbgprt("Found matching nodes\n");
			prtmsg("# pattern: ", ha);
			prtmsg("# match:   ", hb);
			var_grounding[ha] = hb;
		}
		return mismatch;
	}

	// If we got to here, there is a clear mismatch, probably because
	// one is a node, and the other a link. 
	return true;
}

/* ======================================================== */

bool PatternMatchEngine::soln_up(Handle hsoln)
{
	// Let's not look at our own navel
	if (hsoln == curr_root) return false;
	Atom *ap = TLB::getAtom(curr_pred_handle);
	Atom *as = TLB::getAtom(hsoln);
	depth = 1;
	bool no_match = tree_compare(ap, as);

	// If no match, then try the next one.
	if (no_match) return false;

	// If we've navigated to the top of the clause, and its matched, 
	// then it is fully grounded, and we're done with it. 
	// Start work on the next unsovled predicate. But do all of this
	// only if the callback allows it.
	if (curr_pred_handle == curr_root)
	{
		Link *lp = dynamic_cast<Link *>(ap);
		Link *ls = dynamic_cast<Link *>(as);

		// Is this required to match? If so, then let the callback
		// make the final decision; if callback rejects, then it's
		// the same as a mismatch; try the next one.
		if (optionals.count(curr_root))
		{
			clause_accepted = true;
			no_match = pmc->optional_clause_match(lp, ls);
		}
		else
		{
			no_match = pmc->clause_match(lp, ls);
		}
		if (no_match) return false;

		root_handle_stack.push(curr_root);
		pred_handle_stack.push(curr_pred_handle);
		soln_handle_stack.push(curr_soln_handle);
		pred_solutn_stack.push(clause_grounding);
		var_solutn_stack.push(var_grounding);
		issued_stack.push(issued);
		pmc->push();

		curr_soln_handle = TLB::getHandle(as);
		clause_grounding[curr_root] = curr_soln_handle;
		prtmsg("--------------------- \nclause:", curr_root);
		prtmsg("ground:", curr_soln_handle);
		dbgprt("\n");
		
		get_next_untried_clause();

		prtmsg("next clause is", curr_root);
		dbgprt("This clause is %s\n", optionals.count(curr_root)? "optional" : "required");
		prtmsg("joining handle is", curr_pred_handle);

		// If there are no further predicates to solve,
		// we are really done! Report the solution via callback.
		bool found = false;
		if (Handle::UNDEFINED == curr_root)
		{
			dbgprt ("==================== FINITO!\n");
#ifdef DEBUG
			print_solution(var_grounding, clause_grounding);
#endif
			found = pmc->solution(clause_grounding, var_grounding);
		}
		else
		{
			// Else, start solving the next unsolved clause.
			// We continue our search at the atom that "joins" (is shared in common)
			// between the previous (solved) clause, and this clause. If the "join"
			// was a variable, look up its grounding; else the join is a 'real' atom.

			clause_accepted = false;
			curr_soln_handle = var_grounding[curr_pred_handle];
			found = soln_up(curr_soln_handle);

			// If we are here, and found is false, then we've exhausted all
			// of the search possibilities for the current clause. If this 
			// is an optional clause, and no solutions were reported for it,
			// then report the failure of finding a solution now. If this was
			// also the final optional clause, then in fact, we've got a 
			// grounding for the whole thing ... report that!
			if ((false == found) && 
			    (false == clause_accepted) &&
			    (optionals.count(curr_root)))
			{
				Atom *acl = TLB::getAtom(curr_pred_handle);
				Link *lcl = dynamic_cast<Link *>(acl);
				no_match = pmc->optional_clause_match(lcl, NULL);
				dbgprt ("Exhausted search for optional clause, cb=%d\n", no_match);
				if (no_match) return false;

				// XXX Maybe should push n pop here? No, maybe not ... 
				clause_grounding[curr_root] = invalid_grounding;
				get_next_untried_clause();
				prtmsg("Next optional clause is", curr_root);
				if (Handle::UNDEFINED == curr_root)
				{
					dbgprt ("==================== FINITO BANDITO!\n");
#ifdef DEBUG
					print_solution(var_grounding, clause_grounding);
#endif
					found = pmc->solution(clause_grounding, var_grounding);
				}
			}
		}

		// If we failed to find anything at this level, we need to 
		// backtrack, i.e. pop the stack, and begin a search for
		// other possible matches and groundings.
		pmc->pop();
		curr_root = root_handle_stack.top();
		root_handle_stack.pop();

		curr_pred_handle = pred_handle_stack.top();
		pred_handle_stack.pop();

		curr_soln_handle = soln_handle_stack.top();
		soln_handle_stack.pop();

		// The grounding stacks are handled differently.
		POPTOP(clause_grounding, pred_solutn_stack);
		POPTOP(var_grounding, var_solutn_stack);

		issued = issued_stack.top();
		issued_stack.pop();

		prtmsg("pop to joiner", curr_pred_handle);
		prtmsg("pop to clause", curr_root);

		return found;
	}

	// If we are here, then we are somewhere in the middle of a clause,
	// and everything below us matches. So need to move up.
	soln_handle_stack.push(curr_soln_handle);
	curr_soln_handle = hsoln;

	// Move up the predicate, and hunt for a match, again.
	prtmsg("node has grnd, move up:", as);
	bool found = foreach_incoming_handle(curr_pred_handle,
	                &PatternMatchEngine::pred_up, this);
	dbgprt("after moving up the clause, found = %d\n", found);

	curr_soln_handle = soln_handle_stack.top();
	soln_handle_stack.pop();

	return found;
}

bool PatternMatchEngine::pred_up(Handle h)
{
	// Is this link even a part of the predicate we are considering?
	// If not, try the next atom. 
	bool valid = ot.is_node_in_tree(curr_root, h);
	if (!valid) return false;

	// Move up the solution outgoing set, looking for a match.
	Handle curr_pred_save = curr_pred_handle;
	curr_pred_handle = h;

	bool found = foreach_incoming_handle(curr_soln_handle,
	                     &PatternMatchEngine::soln_up, this);

	curr_pred_handle = curr_pred_save;
	dbgprt("found upward soln = %d\n", found);
	return found;
}

/**
 * Search for the next untried, (thus ungrounded, unsolved) clause.
 *
 * The "issued" set contains those clauses which are currently in play,
 * i.e. those for which a grounding is currently being explored. Both
 * grounded, and as-yet-ungrounded clauses may be in this set.  The
 * sole reason of this set is to avoid infinite resursion, i.e. of 
 * re-identifying the same clause over and over as unsolved. 
 */
void PatternMatchEngine::get_next_untried_clause(void)
{
	// Search for an as-yet ungrounded clause. Search for required
	// clauses first; then, only if none of those are left, move on
	// to the optional clauses.  We can find ungrounded clauses by
	// looking at the grounded vars, looking up the root, to see if
	// the root is grounded.  If its not, start working on that.
	Handle pursue = Handle::UNDEFINED;
	Handle unsolved_clause = Handle::UNDEFINED;
	RootMap::iterator k;
	for (k=root_map.begin(); k != root_map.end(); k++)
	{
		RootPair vk = *k;
		RootList *rl = vk.second;
		pursue = vk.first;

		bool unsolved = false;
		bool solved = false;

		std::vector<Handle>::iterator i;
		for (i=rl->begin(); i != rl->end(); i++)
		{
			Handle root = *i;
			if(TLB::isValidHandle(clause_grounding[root]))
			{
				solved = true;
			}
			else if ((issued.end() == issued.find(root)) &&
			         (optionals.end() == optionals.find(root)))
			{
				unsolved_clause = root;
				unsolved = true;
			}
		}
		if (solved && unsolved) break;
	}

	// Pursue is a pointer to a node that's shared between
	// several clauses. One of the predicates has been
	// solved, another has not.  We want to now traverse 
	// upwards from this node, to find the top of the 
	// unsolved clause.
	curr_root = unsolved_clause;
	curr_pred_handle = pursue;

	if (Handle::UNDEFINED != unsolved_clause)
	{
		issued.insert(unsolved_clause);
		return;
	}

	// Try again, this time, considering the optional clauses.
	for (k=root_map.begin(); k != root_map.end(); k++)
	{
		RootPair vk = *k;
		RootList *rl = vk.second;
		pursue = vk.first;

		bool unsolved = false;
		bool solved = false;

		std::vector<Handle>::iterator i;
		for (i=rl->begin(); i != rl->end(); i++)
		{
			Handle root = *i;
			if(TLB::isValidHandle(clause_grounding[root]))
			{
				solved = true;
			}
			else if (issued.end() == issued.find(root))
			{
				unsolved_clause = root;
				unsolved = true;
			}
		}
		if (solved && unsolved) break;
	}

	// Pursue is a pointer to a node that's shared between
	// several clauses. One of the predicates has been
	// solved, another has not.  We want to now traverse 
	// upwards from this node, to find the top of the 
	// unsolved clause.
	curr_root = unsolved_clause;
	curr_pred_handle = pursue;

	if (Handle::UNDEFINED != unsolved_clause)
	{
		issued.insert(unsolved_clause);
	}
}

/* ======================================================== */

Handle PatternMatchEngine::find_starter(Handle h)
{
	curr_pred_handle = h;
	Atom *a = TLB::getAtom(h);
	Link *l = dynamic_cast<Link *>(a);
	if (NULL == l)
	{
		Type t = a->getType();
		if (t != VARIABLE_NODE) return h;
		return Handle::UNDEFINED;
	}

	const std::vector<Handle> &vh = l->getOutgoingSet();
	for (size_t i = 0; i < vh.size(); i++) {
		Handle hout = vh[i];
		Handle s = find_starter(hout);
		if (s != Handle::UNDEFINED) return s;
	}

	return Handle::UNDEFINED;
}

/**
 * do_candidate - examine candidates, looking for matches.
 *
 * This routine is invoked on every candidate atom taken from
 * the atom space. That atom is assumed to anchor some part of
 * a graph that hopefully will match the predicate.
 */
bool PatternMatchEngine::do_candidate(Handle ah)
{
	// Cleanup
	var_grounding.clear();
	clause_grounding.clear();
	issued.clear();
	while(!pred_handle_stack.empty()) pred_handle_stack.pop();
	while(!soln_handle_stack.empty()) soln_handle_stack.pop();
	while(!root_handle_stack.empty()) root_handle_stack.pop();
	while(!pred_solutn_stack.empty()) pred_solutn_stack.pop();
	while(!var_solutn_stack.empty()) var_solutn_stack.pop();
	while(!issued_stack.empty()) issued_stack.pop();

	// Match the required clauses.
	curr_root = cnf_clauses[0];
	curr_pred_handle = starter_pred;
	issued.insert(curr_root);
	bool found = soln_up(ah);

	// If found is false, then there's no solution here.
	// Bail out, return false to try again with the next candidate.
	return found;
}

/**
 * Create an associative array that gives a list of all of the
 * predicatees that a given node participates in.
 */
bool PatternMatchEngine::note_root(Handle h)
{
	RootList *rl = root_map[h];
	if (NULL == rl)
	{
		rl = new RootList();
		root_map[h] = rl;
	}
	rl->push_back(curr_root);

	foreach_outgoing_handle(h, &PatternMatchEngine::note_root, this);
	return false;
}

/**
 * Find a grounding for a sequence of clauses in conjunctive normal form.
 *
 * The list of clauses, and the list of negations, are both OpenCog
 * hypergraphs.  Both should also be envisioned as a kind of predicate:
 * i.e. something which may or may not exist (may or may not be "true")
 * in that the subgraph defined by the predicate might not actually
 * be found in the universe of all atoms in the atomspace.
 *
 * The list of "bound vars" are to be solved for ("grounded", or 
 * "evaluated") during pattern matching. That is, if the subgraph
 * defined by the clauses is located, then the vars are given the 
 * corresponding values associated to that match.
 *
 * The negations are a set of clauses whose truth values are to be
 * inverted.  That is, while the clauses define a subgraph that 
 * *must* be found, the negations define a subgraph that probably
 * should not be found, or, if found, should have a truth value of
 * 'false'.  The precise handing of the negated clauses is determined
 * by the callback, although the search engine itself will proclaim
 * a match whether or not it finds negated clauses. Thus, the neg
 * clauses can be understood as "optional" matches: they will be
 * matched, if possible, but are not required to be matched. The idea
 * that these are actually "negated" is governed by the callback.
 *
 * The PatternMatchCallback is consulted to determine whether a 
 * veritable match has been found, or not. The callback is given
 * individual nodes and links to compare for a match.
 */
void PatternMatchEngine::match(PatternMatchCallback *cb,
                         const std::vector<Handle> &vars,
                         const std::vector<Handle> &clauses,
                         const std::vector<Handle> &negations)
{
	if (!atom_space) return;

	// Copy the variables from vector to set; this makes it easier to
	// determine set membership.
	std::vector<Handle>::const_iterator i;
	for (i = vars.begin();
	     i != vars.end(); i++)
	{
		Handle h = *i;
		bound_vars.insert(h);
	}

	cnf_clauses = clauses;

	// Copy the negates into the clause list
	// Copy the negates into a set.
	for (i = negations.begin();
	     i != negations.end(); i++)
	{
		Handle h = *i;
		cnf_clauses.push_back(h);
		optionals.insert(h);
	}

	var_grounding.clear();
	clause_grounding.clear();

	if (cnf_clauses.size() == 0) return;

	// Preparation prior to search.
	// Create a table of the nodes that appear in the clauses, and
	// a list of the clauses that each node participates in.
	root_map.clear();
	for (i = cnf_clauses.begin();
	     i != cnf_clauses.end(); i++)
	{
		Handle h = *i;
		curr_root = h;
		note_root(h);
	}
	pmc = cb;

#ifdef DEBUG
	// Print out the predicate ...
	printf("\nPredicate consists of the following clauses:\n");
	int cl = 0;
	for (i = cnf_clauses.begin();
	     i != cnf_clauses.end(); i++)
	{
		printf("Clause %d: ", cl);
		Handle h = *i;
		prt(TLB::getAtom(h));
		cl++;
	}

	// Print out the bound variables in the predicate.
	std::set<Handle>::const_iterator j;
	for (j=bound_vars.begin(); j != bound_vars.end(); j++)
	{
		Handle h = *j;
		Atom *a = TLB::getAtom(h);
		Node *n = dynamic_cast<Node *>(a);
		if (n)
		{
			printf(" Bound var: "); prt(a);
		}
	}
#endif

	// Ideally, we start our search at some node, any node, that is
	// not a variable, that is in the first clause. If the first
	// clause consists entirely of variable nodes, then we are 
	// screwed, and must search over all links that have the same 
	// type as the first clause.
	Handle h = cnf_clauses[0];
	curr_root = h;
	starter_pred = find_starter(h);
	if (Handle::UNDEFINED != starter_pred)
	{
		do_candidate(starter_pred);
	}
	else
	{
		starter_pred = curr_root;

		// Get type of the first item in the predicate list.
		Atom *a = TLB::getAtom(h);
		Type ptype = a->getType();

		// Plunge into the deep end - start looking at all viable
		// candidates in the AtomSpace.
		atom_space->foreach_handle_of_type(ptype,
		      &PatternMatchEngine::do_candidate, this);
	}

	dbgprt ("==================== Done Matching ==================\n");
#ifdef DEBUG
	fflush(stdout);
#endif
}

void PatternMatchEngine::print_solution(
	const std::map<Handle, Handle> &vars,
	const std::map<Handle, Handle> &clauses)
{
	printf("\nNode groundings:\n");

	// Print out the bindings of solutions to variables.
	std::map<Handle, Handle>::const_iterator j;
	for (j=vars.begin(); j != vars.end(); j++)
	{
		std::pair<Handle, Handle> pv = *j;
		Handle var = pv.first;
		Handle soln = pv.second;
		Atom *av = TLB::getAtom(var);
		Atom *as = TLB::getAtom(soln);
		Node *nv = dynamic_cast<Node *>(av);
		Node *ns = dynamic_cast<Node *>(as);
		if (ns && nv)
		{
			printf("\tNode %s maps to %s\n", 
			       nv->getName().c_str(), ns->getName().c_str());
		}
	}

	// Print out the full binding to all of the clauses.
	printf("\nGrounded clauses:\n");
	std::map<Handle, Handle>::const_iterator m;
	int i = 0;
	for (m = clauses.begin(); m != clauses.end(); m++) 
	{
		Atom *ac = TLB::getAtom(m->second);
		if (NULL == ac) continue;
		std::string str = ac->toString();
		printf ("%d.   %s\n", i, str.c_str());
		i++;
	}
	printf ("\n");
}

/* ===================== END OF FILE ===================== */
