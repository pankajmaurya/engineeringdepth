➜  C-is-spartan-language git:(main) ✗ rm f1 f2
➜  C-is-spartan-language git:(main) ✗ python3 tester.py
Writing 2 files with 10k lines differing on line 7668
➜  C-is-spartan-language git:(main) ✗ ./diff f1 f2
Difference found at line 7668
File1: This is line 7668
File2: Changed line 7668

For 1 Million lines
====================
➜  C-is-spartan-language git:(main) ✗ python3 tester.py 
Writing 2 files with 10k lines differing on line 866956
➜  C-is-spartan-language git:(main) ✗ time ./diff f1 f2 
Difference found at line 866956
File1: This is line 866956
File2: Changed line 866956

./diff f1 f2  0.08s user 0.01s system 98% cpu 0.093 total
➜  C-is-spartan-language git:(main) ✗ du -sh f1
 19M	f1
➜  C-is-spartan-language git:(main) ✗ du -sh f2
 19M	f2

