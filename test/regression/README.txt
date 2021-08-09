Asset Importer Lib Regression Test Suite
========================================

1) How does it work?
---------------------------------------------------------------------------------
run.py checks all model in the <root>/test/models* folders and compares the result
against a regression database provided with assimp (db.zip). A few failures
are totally fine (see sections 7+). You need to worry if a huge
majority of all files in a particular format (or post-processing configuration)
fails as this might be a sign of a recent regression in assimp's codebase or
gross incompatibility with your system or compiler.

2) What do I need?
---------------------------------------------------------------------------------
 - You need Python installed (2.7+, 3.x). On Windows, run the scripts using "py".
 - You need to build the assimp command line tool (ASSIMP_BUILD_ASSIMP_TOOLS
   CMake build flag). Both run.py and gen_db.py take the full path to the binary
   as first command line parameter.

3) How to add more test files?
---------------------------------------------------------------------------------
Use the following procedure:
 - Verify the correctness of your assimp build - run the regression suite.
   DO NOT continue if more tests fail than usual.
 - Add your additional test files to <root>/test/models/<fileformat>, where
   <fileformat> is the file type (typically the file extension).
 - If you test file does not meet the BSD license requirements, add it to
   <root>/test/models-nonbsd/<fileformat> so people know to be careful with it.
 - Rebuild the regression database:
   "gen_db.py <binary> -ixyz" where .xyz is the file extension of the new file.
 - Run the regression suite again. There should be no new failures and the new
   file should not be among the failures.
 - Include the db.zip file with your Pull Request. Travis CI enforces a passing
   regression suite (with offenders whitelisted as a last resort).

4) I made a change/fix/patch to a loader, how to update the database?
---------------------------------------------------------------------------------
 - Rebuild the regression database using "gen_db.py <binary> -ixyz"
   where .xyz is the file extension for which the loader was patched.
 - Run the regression suite again. There should be no new failures and the new
   file should not be among the failures.
 - Include the db.zip file with your Pull Request. Travis CI enforces a passing
   regression suite (with offenders whitelisted as a last resort).

5) How to add my whole model repository to the database?
---------------------------------------------------------------------------------

Edit the reg_settings.py file and add the path to your repository to
<<model_directories>>. Then, rebuild the database.

6) So what is actually tested?
---------------------------------------------------------------------------------
The regression database includes mini dumps of the aiScene data structure, i.e.
the scene hierarchy plus the sizes of all data arrays MUST match. Floating-point
data buffers, such as vertex positions are handled less strictly: min, max and
average values are stored with low precision. This takes hardware- or
compiler-specific differences in floating-point computations into account.
Generally, almost all significant regressions will be detected while the
number of false positives is relatively low.

7) The test suite fails, what do do?
---------------------------------------------------------------------------------
Get back to <root>/test/results and look at regression_suite_failures.txt.
It contains a list of all files which failed the test. Failing dumps are copied to
<root>/test/results/tmp. Both an EXPECTED and an ACTUAL file is produced per test.
The output of "assimp cmpdump" is written to regressions_suite_output.txt. Grep
for the file name in question and locate the log for the failed comparison. It
contains a full trace of which scene elements have been compared before, which
makes it reasonably easy to locate the offending field.

8) fp:fast vs fp:precise fails the test suite (same for gcc equivalents)
---------------------------------------------------------------------------------
As mentioned above, floating-point inaccuracies between differently optimized
builds are not considered regressions and all float comparisons done by the test
suite involve an epsilon to accommodate. However compiler settings that allow
compilers to perform non-IEEE754 compliant optimizations can cause arbitrary
failures in the test suite. Even if the compiler is configured to be IEE754
comformant, there is lots of code in assimp that leaves the compiler a choice
and different compilers make different choices (for example the precision of
float intermediaries is implementation-specified).















