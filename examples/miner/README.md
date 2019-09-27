# Collection of examples of the pattern miner

In order to run an example type the following:

```
guile -l miner-main.scm
```

You need to be in this examples folder in order for the aforementioned command to work. After that just type the example number you want to run.

# Complete Examples List

All the available examples are presented next.

## Simple

Mine simple 1-conjunct (a.k.a. 1-gram) pattern.

### Expected results

Given the KB

```scheme
(Inheritance
  (Concept "A")
  (Concept "B"))
(Inheritance
  (Concept "A")
  (Concept "C"))
```

Mine patterns such as

```scheme
(Lambda
  (Variable "$X")
  (Present
    (Inheritance
      (Concept "A")
      (Variable "$X"))))
```

## 2-conjunct

Mine simple 2-conjunct (a.k.a. 2-gram) pattern starting with an
abstract pattern of 2 conjuncts.

### Expected results

Given the KB

```scheme
(Inheritance
  (Concept "A")
  (Concept "B")))
(Inheritance
  (Concept "B")
  (Concept "C")))
(Inheritance
  (Concept "D")
  (Concept "E")))
(Inheritance
  (Concept "E")
  (Concept "F")))

```

Mine patterns such as

```scheme
(Lambda
  (VariableList
    (Variable "$X")
    (Variable "$Y")
    (Variable "$Z"))
  (Present
    (Inheritance
      (Variable "$X")
      (Variable "$Y"))
    (Inheritance
      (Variable "$Y")
      (Variable "$Z"))))
```

## Ugly Male Soda Drinker

Given a KB of people and their characteristics, such as `man`,
`woman`, `ugly`, etc.

### Expected results

Given the KB

```scheme
(Inheritance
  (Concept "Allen")
  (Concept "man"))
...
(Inheritance
  (Concept "Davion")
  (Concept "ugly"))
...
(Inheritance
  (Concept "Jenica")
  (Concept "soda drinker"))
```

Mine patterns such as

```scheme
(Lambda
  (Variable "$X")
  (Present
    (Inheritance
      (Variable "$X")
      (Concept "man"))
    (Inheritance
      (Variable "$X")
      (Concept "ugly"))
    (Inheritance
      (Variable "$X")
      (Concept "soda drinker"))))
```

## MOZI-AI

Experiments running the pattern miner over the various gene ontologies
and other KBs from MOZI-AI.

## SUMO

TODO::Write explanation