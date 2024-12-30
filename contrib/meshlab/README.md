# meshlab
Meshlab project cloned in entirety but only using two files: "Parser" and "Scanner" in order to
reformat .wrl/.x3dv files as .xml

## Automatic repo clone
Meshlab repo is automatically cloned.  Users who haven't opted-in to VRML support
won't be burdened with the extra download volume.

To update the git commit hash pulled down, modify `Meshlab_GIT_TAG` in file
`code/CMakeLists.txt`; it is not expected that the sole files of interest "Parser" and "Scanner"
will change frequently, if at all, going forward
