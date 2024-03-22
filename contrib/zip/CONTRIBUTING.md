# Contribution Rules/Coding Standards
No need to throw away your coding style, just do your best to follow default clang-format style.
Apply `clang-format` to the source files before commit:
```sh
for file in $(git ls-files | \grep -E '\.(c|h)$' | \grep -v -- '#')
do
    clang-format -i $file
done
```
