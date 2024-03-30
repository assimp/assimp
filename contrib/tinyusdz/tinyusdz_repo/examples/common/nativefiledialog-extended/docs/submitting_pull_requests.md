# Pull Requests #

I have had to turn away a number of pull requests due to avoidable circumstances.  Please read this file before submitting a pull request. Also look at existing, rejected pull requests which state the reason for rejection.

Here are the rules:

- **Submit pull requests to the devel branch**. The library must be tested on every compiler and OS, so there is no way I am going to just put your change in the master before it has been sync'd and tested on a number of machines. Master branch is depended upon by hundreds of projects.

- **Test your changes on all platforms and compilers that you can.**  Also, state which platforms you have tested your code on.  32-bit or 64-bit.  Clang or GCC.  Visual Studio or Mingw.  I have to test all these to accept pull requests, so I prioritize changes that respect my time.

- **Submit Premake build changes only**.  As of 1.1, SCons is deprecated.  Also, do not submit altered generated projects.  I will re-run Premake to re-generate them to ensure that I can still generate the project prior to admitting your pull request.

- **Do not alter existing behavior to support your desired behavior**. For instance, rewriting file open dialogs to behave differently, while trading off functionality for compatibility, will get you rejected.  Consider creating an additional code path.  Instead of altering `nfd_win.cpp` to support Windows XP, create `nfd_win_legacy.cpp`, which exists alongside the newer file dialog.

- **Do not submit anything I can't verify or maintain**.  If you add support for a compiler, include from-scratch install instructions  that you have tested yourself.  Accepting a pull request means I am now the maintainer of your code, so I must understand what it does and how to test it.

- **Do not change the externally facing API**.  NFD needs to maintain ABI compatibility.

## Submitting Cloud Autobuild systems ##

I have received a few pull requests for Travis and AppVeyor-based autobuilding which I have not accepted.  NativeFileDialog is officially covered by my private BuildBot network which supports all three target OSes, both CPU architectures and four compilers.  I take the view that having a redundant, lesser autobuild system does not improve coverage: it gives a false positive when partial building succeeds.  Please do not invest time into cloud-based building with the hope of a pull request being accepted.

## Contact Me ##

Despite all of the "do nots" above, I am happy to recieve new pull requests!  If you have any questions about style, or what I would need to accept your specific request, please contact me ahead of submitting the pull request by opening an issue on Github with your question.  I will do my best to answer you.
