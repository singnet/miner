;; Load the miner module
(use-modules (opencog) ;; for opencog general procedures
             (opencog miner) ;; for opencog miner related procedures
             (opencog bioscience) ;; for bioscience procedures in opencog
)

;; display helper
(define (display-all . string)
    (for-each display string)
)

;; examples
(define (simple)
    (begin
        ;; display what will be executed
        (display-all 
            "\n\n====================================\n\n"
            "This is the simple miner example.\n\n"
            "It is being called with the following parameters:\n\n"
            "\tminsup: 2\n"
            "\tconjunction-expansion: #f\n"
            "\tsurprisingness: 'none\n\n"
        )

        ;; load the KB
        (load "examples-kbs/simple-kb.scm")

        ;; call for the pattern miner
        (cog-mine (cog-atomspace)
                    #:minsup 2
                    #:conjunction-expansion #f
                    #:surprisingness 'none)
    )
)

(define (2-conjunct)
    (begin
        ;; display what will be executed
        (display-all
                "\n\n====================================\n\n"
                "This is the 2-conjunct example.\n\n"
                "It is being called with the following parameters:\n\n"
                "\tminsup: 2\n"
                "\tinitpat: (conjunct-pattern 2)\n"
                "\tconjunction-expansion: #f\n"
                "\tsurprisingness: 'none\n\n"
        )

        ;; load the KB
        (load "examples-kbs/2-conjunct-kb.scm")

        ;; call for the pattern miner
        (cog-mine (cog-atomspace)
                    #:minsup 2
                    #:initpat (conjunct-pattern 2)
                    #:conjunction-expansion #f
                    #:max-variables 4
                    #:surprisingness 'none)
    )
)

(define (ugly-male-soda-drinker)
    (begin
        ;; display what will be executed
        (display-all
                "\n\n====================================\n\n"
                "This is the ugly male soda drinker example.\n\n"
                "It is being called with the following parameters:\n\n"
                "\tminsup: 2\n"
                "\tmaximum-iterations: 100\n"
                "\tmax-conjuncts: 3\n"
                "\tmax-variables: 2\n"
                "\tconjunction-expansion: #t\n"
                "\tsurprisingness: 'nisurp\n\n"
        )

        ;; load the KB
        (load "examples-kbs/ugly-male-soda-drinker-kb.scm")

        ;; call for the pattern miner
        (cog-mine (cog-atomspace)
                    #:minsup 5
                    #:maximum-iterations 100
                    #:conjunction-expansion #t
                    #:max-conjuncts 3
                    #:max-variables 2
                    #:surprisingness 'nisurp)
    )
)

(define (mozi-ai-kb)
    (begin
        ;; display what will be executed
        (display-all 
                "\n\n====================================\n\n"
                "This is the mozi-ai example.\n\n"
                "It is being called with the following parameters:\n\n"
                "\tminfreq: 0.001\n"
                "\tmaximum-iterations: 1000\n"
                "\tmax-conjuncts: 3\n"
                "\tmax-variables: 2\n"
                "\tconjunction-expansion: #t\n"
                "\tsurprisingness: 'nisurp\n\n"
        )

        ;; load the KB
        (load "examples-kbs/mozi-ai-kb.scm")

        (cog-mine (cog-atomspace)
                    #:minfreq 0.001
                    #:maximum-iterations 1000
                    #:conjunction-expansion #t
                    #:max-conjuncts 3
                    #:max-variables 2
                    #:surprisingness 'nisurp)
    )
)

(define (sumo)
    (begin
        ;; display what will be executed
        (display-all 
                "\n\n====================================\n\n"
                "This is the sumo example.\n"
                "It is not implemented yet.\n\n"
        )

        ;; TODO::implement this call
    )
)

;; define an global execution control flag
(define run #t)

;; ask for the user input example number
(define option "NOTHING")

;; define the cal output variable
(define results "NOTHING")

;; main loop
(while run
    (begin
        ;; read user input and call for the proper example
        (display-all 
                "\n\n====================================\n\n"
                "COLLECTION OF EXAMPLES OF THE PATTERN MINER.\n\n"
                "Available example numbers:\n\n"
                "\t1 - simple\n"
                "\t2 - 2-conjunct\n"
                "\t3 - ugly-male-soda-drinker\n"
                "\t4 - mozi-ai\n"
                "\t5 - sumo\n"
                "\t6 - EXITS THE PROGRAM\n\n"
                "Enter the example NUMBER you want to run: "
        )

        ;; ask for the user input example number
        (set! option (read))

        ;; define the cal output variable
        (set! results "NOTHING")

        ;; Call for the pattern miner example
        (if (eq? option 1)
            (set! results (simple))
        )

        (if (eq? option 2)
            (set! results (2-conjunct))
        )

        (if (eq? option 3)
            (set! results (ugly-male-soda-drinker))
        )

        (if (eq? option 4)
            (set! results (mozi-ai))
        )

        (if (eq? option 5)
            (set! results (sumo))
        )

        (if (eq? option 6)
            (begin
                (display "\n\nThanks for testing the pattern-miner ;)\n\n")
                (exit)
            )
        )

        ;; extra line break for the better visualization
        (display "\n\n============== RESULT ==============\n\n")
        (display results)
        (display "\n\n============ END RESULT ============\n\n")
        (display "PLEASE SEE YOUR RESULTS ABOVE !!!\n\n")
    )
)