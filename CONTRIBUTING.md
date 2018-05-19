#How to contribute

If you want to contribute you can follow these setps:
- Fist create your own clone of assimp
- When you want to fix a bug or add a new feature create a branch on your own fork ( just follow https://help.github.com/articles/creating-a-pull-request-from-a-fork/ )
- Push it to the repo and open a pull request
- A pull request will start our CI-service, which checks if the build works for linux and windows. 
  It will check for memory leaks, compiler warnings and memory alignment issues. If any of these tests fails: fix it and the tests will be reastarted automatically
  - At the end we will perform a code review and merge your branch to the master branch.
  
  
