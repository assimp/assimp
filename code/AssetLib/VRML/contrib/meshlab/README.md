# meshlab

## Files used from meshlab project
For reformatting .wrl/.x3dv files as .xml on-the-fly, 2 source/header file pairs from
Meshlab project have been patched to work with pugixml: "Parser" and "Scanner"

Git repo: `https://github.com/cnr-isti-vclab/meshlab`
Git commit: `ad55b47a9b0700e7b427db6db287bb3a39aa31e7`

These files were updated very infrequently and it saves a lot of trouble to just
use the patched versions instead of cloning repo and patching a couple of files
every time

## Patch file
In case it ever proves necessary to update the files from meshlab, patch file
is provided under `contrib/meshlab/patches/meshlab.patch`

It may also be used to confirm the provenance of the Parser/Scanner file set
