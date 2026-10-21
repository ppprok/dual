# The dualization problem and the joint enumeration problem

English rendering of the Russian research task statement *"Opisanie zadachi dualizatsii i postanovka
zadachi sovmestnogo perechisleniya"* ("Description of the dualization problem and statement of the joint
enumeration problem"). The original LaTeX source (cp1251) and its PDF are kept in
[`legacy/docs/task/`](../legacy/docs/task/). Commented-out parts of the original (a draft section on
generalizing logical classifiers and unused bibliography entries) are omitted. Obvious typos of the
original are fixed silently; notes in *[brackets]* are the translator's.

## 1. Introduction

Let a partial order be defined on a finite set $\mathcal{P}$. We write $x \preccurlyeq y$ to denote that
$y \in \mathcal{P}$ follows $x \in \mathcal{P}$. A partial order is reflexive, transitive and antisymmetric:

1. $x \preccurlyeq x$, $\forall x \in \mathcal{P}$;
2. $(x \preccurlyeq y,\; y \preccurlyeq z) \to (x \preccurlyeq z)$, $\forall x,y,z \in \mathcal{P}$;
3. $(x \preccurlyeq y,\; y \preccurlyeq x) \to (x = y)$, $\forall x, y \in \mathcal{P}$.

When $x \preccurlyeq y$ and $x \ne y$, we write $x \prec y$.

An element $a \in A$, $A \subseteq \mathcal{P}$, is called a *maximal element* of $A$ if the relation
$a \prec x$ holds for no $x \in A$. Denote by $\max(A)$ the set of maximal elements of $A$. A minimal
element of $A$ and the set of minimal elements $\min(A)$ are defined analogously.

For $A \subseteq \mathcal{P}$, the sets $A^+ = \{x \in \mathcal{P} \colon \exists a \in A, a \preccurlyeq x\}$
and $A^- = \{x \in \mathcal{P} \colon \exists a \in A, x \preccurlyeq a\}$ are called the *ideal and the
filter of $A$*, respectively. An element $x \in \mathcal{P} \setminus A^+$ is called *independent* of $A$.
The set $\max(\mathcal{P} \setminus A^+)$ consists of the *maximal elements independent of $A$* and is
denoted by $\mathcal{I}(A)$. The elements of $A$ define a partition
$\mathcal{P} = A^+ \cup \mathcal{I}(A)^-,\; A^+ \cap \mathcal{I}(A)^- = \emptyset$,
so $\mathcal{I}(A)$ is also called *dual to* $A$.

Let $\mathcal{P} = \mathcal{P}_1 \times \ldots \times \mathcal{P}_n$, where $\mathcal{P}_1, \ldots, \mathcal{P}_n$
are partially ordered sets, and let the partial order on $\mathcal{P}$ be defined as follows. For elements
$x=(x_1, \ldots, x_n), y = (y_1, \ldots, y_n) \in \mathcal{P}$, the relation $x \preccurlyeq y$ holds if and
only if $x_1 \preccurlyeq y_1, \ldots, x_n \preccurlyeq y_n$ all hold simultaneously. We consider the problem
of enumerating the elements of $\mathcal{I}(A)$, where $A = \{a_1, \ldots, a_m\}$,
$a_i = (a_{i1}, \ldots, a_{in}) \in \mathcal{P}$, $i \in \{1, \ldots, m\}$. This problem is called
*dualization* over a product of partial orders. It generalizes the well-known problem of enumerating the
maximal independent sets of vertices of a hypergraph, which is equivalent to constructing the irredundant
disjunctive normal form (DNF) of a monotone Boolean function given by a conjunctive normal form (CNF).

Dualization can be used to solve more complex enumeration problems. Let a monotone property
$\pi : \mathcal{P} \to \{0,1\}$ be defined on $\mathcal{P}$, i.e. $x \preccurlyeq y \to \pi(x) \le \pi(y)$.
We need to enumerate the "minimal ones" of the property $\pi$ — the elements
$F_\pi = \min\{x \in \mathcal{P} \colon \pi(x)=1\}$, and the "maximal zeros" of $\pi$ — the elements
$G_\pi = \max\{x \in \mathcal{P} \colon \pi(x)=0\}$. Denote by $O_\pi$ an oracle that checks the property
$\pi$ for elements of $\mathcal{P}$. To enumerate $F_\pi$ and $G_\pi$ incrementally, essentially the
following problem has to be solved at every step.

**Problem $\mathbf{GEN}(\mathcal{P}, O_\pi, A, B)$.** Given an oracle $O_\pi$ of a monotone property $\pi$
and sets $A \subseteq F_\pi$, $B \subseteq G_\pi$. Find an element
$x \in (F_\pi \setminus A) \cup (G_\pi \setminus B)$ or establish that there are no such elements,
i.e. $A = F_\pi$ and $B = G_\pi$.

A survey of applications in which the problem $\mathbf{GEN}(\mathcal{P}, O_\pi, A, B)$ arises can be found
in [1]. These applications belong to areas such as machine learning, integer programming, artificial
intelligence, knowledge discovery, etc. Let us look in more detail at one application in knowledge
discovery: constructing so-called association rules [2, 3, 4]. The most general theory of association
rule mining is formulated in [5].

Let $D=\{d_1, \ldots, d_m\} \subseteq \mathcal{P}$ and $p \in \mathcal{P}$. Denote by $S_D(p)$ the set
$\{d_i \in D \colon p \preceq d_i \}$ of elements that *support* $p$. Note that $D$ is conveniently
interpreted as the set of records of a database, and $S_D(p)$ as the result of a "search query" $p$ to $D$.
A non-negative threshold $t$ is fixed. An element $p$ is called *$t$-frequent* if $|S_D(p)| \ge t$;
otherwise $p$ is called *$t$-infrequent*. It is easy to check that the property
$\pi: \mathcal{P} \to \{0,1\}$ equal to $0$ on $t$-frequent and $1$ on $t$-infrequent elements is monotone.
The $t$-frequent and $t$-infrequent elements are enumerated "jointly", i.e. at every step a new element is
found that is either $t$-frequent or $t$-infrequent.

## 2. Joint enumeration of maximal and minimal independent elements

To adapt asymptotically optimal algorithms to the joint enumeration problem, we propose to change the
formulation of the problem being solved:

**Problem DualInterval(A, B).** Let $A, B \subset \mathcal{P}$. Enumerate the pairs of elements $(x, y)$
with the following properties:

1. $x \preccurlyeq y$;
2. $y \in \max(\mathcal{P} \setminus A^+)$;
3. $x \in \min(\mathcal{P} \setminus B^-)$.

At every step (except the "redundant" ones, in the case of asymptotically optimal algorithms) a new pair is
found, so either the first or the second element of this pair corresponds to an element being enumerated
"jointly".

### 2.1. The Boolean case

Let $\mathcal{P}_i=\{0,1\}$, $0 \prec 1$. The set of elements $A = \{a_1, \ldots, a_m \}$ corresponds to the
Boolean matrix $L_1$ with entries $a_{ij}$. The set of elements $B=\{b_1, \ldots, b_l\}$ corresponds to the
Boolean matrix $L_2$ with entries $\neg b_{ij}$. The condition $x \in \mathcal{I}(A)$ means that the set
$H_1=\{j \mid x_j=0\}$ is an irreducible cover of $L_1$. The condition
$y \in \min(\mathcal{P} \setminus B^-)$ means that the set $H_2=\{j \mid y_j=1\}$ is an irreducible cover of
$L_2$. The condition $y \preccurlyeq x$ is equivalent to $H_1 \subseteq \overline{H_2}$, which is the same as
$H_1 \cap H_2 = \emptyset$. So the following problem arises. *[In this subsection the roles of $x$ and $y$
are swapped relative to DualInterval: here $x$ is the element dual to $A$ and $y$ the one dual to $B$; the
original text is kept as is.]*

**Problem DualIntervalBool($L_1$, $L_2$).** Given two Boolean matrices $L_1$ and $L_2$ with $n$ columns.
Enumerate the pairs of column sets $(H_1, H_2)$ satisfying the following properties:

1. $H_1$ is an irreducible cover of the matrix $L_1$;
2. $H_2$ is an irreducible cover of the matrix $L_2$;
3. $H_1 \cap H_2 = \emptyset$.

The task is to propose an algorithm for this problem. The simplest way is to adapt one of the existing
dualization algorithms, for example RUNC-M. For that it is enough to modify the iteration of the algorithm:

1. Choose one of the rows in one of the matrices $L_1$ or $L_2$ (think about how to make this choice
   optimally).
2. Choose a column that can cover this row and add it either to $H_1$ or to $H_2$.
3. Forbid adding columns "incompatible" with $H_1$, $H_2$ in subsequent iterations (the definition of
   column compatibility may need to be refined).
4. Check that the remaining rows of $L_1$ and $L_2$ can still be covered by non-forbidden columns. If not,
   the step is considered redundant.

Implementation requirements:

1. Programming language: C/C++.
2. Rows and/or columns are represented as bit sets, using the bitwise operations OR, XOR, AND.
3. For a clean implementation of the algorithm, do not use parallelism or GPU. Later it would be good to
   make a GPU or OpenMP implementation.
4. I can give a link to my implementation of the dualization algorithms, so that you can start right away
   with the adaptation stage without spending time on your own implementation of all the stages of loading
   data and enumerating solutions.

Where this algorithm can be used:

1. Suppose we are dualizing a matrix $L_1$ with a very large number of solutions and for some reason we
   have to pause enumeration after finding part of the solutions. To resume enumerating covers from where
   we stopped, we can build the matrix $L_2$ from the covers already found and continue enumerating
   jointly. The results of previously made "redundant" steps can also be added to $L_2$.
2. Enumerating covers that satisfy conditions of the form
   $H: J_1 \not \subseteq H, \ldots, J_k \not \subseteq H$. For example, conditions of this form suffice
   for dualization over partial orders when the original problem is transformed into the Boolean case,
   in particular when enumerating irredundant covers of integer matrices.
3. Joint enumeration of $t$-infrequent/frequent sets when building association rules for a binary
   database.
4. Parallel dualization, when some processors need to be told which covers were found by other
   processors, so that previously found covers are not enumerated again.

## References

1. K. Elbassioni. Algorithms for Dualization over Products of Partially Ordered Sets. SIAM Journal on
   Discrete Mathematics, 2009, 23(1), pp. 487–510.
2. R. Agrawal, T. Imieliński, A. Swami. Mining association rules between sets of items in large databases.
   ACM SIGMOD Record, ACM, 1993, 22(2), pp. 207–216.
3. R. Srikant, R. Agrawal. Mining quantitative association rules in large relational tables.
   ACM SIGMOD Record, ACM, 1996, 25(2), pp. 1–12.
4. J. Han, Y. Fu. Discovery of multiple-level association rules from large databases. VLDB, 1995, vol. 95,
   pp. 420–431.
5. K. M. Elbassioni. On Finding Minimal Infrequent Elements in Multi-dimensional Data Defined over
   Partially Ordered Sets. arXiv preprint arXiv:1411.2275, 2014.

See also [`references.md`](references.md).
